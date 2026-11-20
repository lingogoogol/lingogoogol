// The subset of the GLFW 3 API used by lgo.GUI, implemented on top of
// GameActivity so the desktop engine and GUI code run unchanged on Android.
//
// Coordinates: one GLFW screen coordinate ("logical pixel") is one Android dp,
// so the desktop's 16 px text is 16 dp on a phone. The window is the part of the
// surface not covered by system bars, display cutouts or the soft keyboard; the
// framebuffer is the whole surface (see glfwGetWindowContentAreaANDROID).
//
// Touch: a tap is a left click; dragging scrolls like a wheel (32 logical
// pixels per wheel step, lgo::scroll_t's wheel_step); a long press hovers
// without clicking, which shows the control's tooltip until the next touch.
#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include "android/native/glfw_android.h"

#include "android/native/activity.hpp"

#include <android/configuration.h>
#include <android/keycodes.h>
#include <android/log.h>
#include <android/native_window.h>
#include <dlfcn.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

struct GLFWwindow
{
    void* user_pointer{};
    GLFWwindowrefreshfun refresh{};
    GLFWcursorenterfun cursor_enter{};
    GLFWcursorposfun cursor_pos{};
    GLFWmousebuttonfun mouse_button{};
    GLFWscrollfun scroll{};
    GLFWkeyfun key{};
    GLFWcharfun character{};
    GLFWsurfacefunANDROID surface{};
    GLFWtouchdragfunANDROID touch_drag{};
    bool should_close{};
};

namespace
{
    using clock_t = std::chrono::steady_clock;

    constexpr float wheel_step{32};       // lgo::scroll_t::wheel_step, in logical pixels.
    constexpr float touch_slop{8};        // Logical pixels before a touch becomes a drag.
    constexpr auto long_press{std::chrono::milliseconds{500}};

    GLFWwindow* the_window{};
    ANativeWindow* native_window{};       // Our reference; see surface_changed.
    std::array<int, GLFW_KEY_LAST + 1> keys{};
    int left_button{GLFW_RELEASE};
    double cursor_x{}, cursor_y{};
    bool hovered{};
    float scale{1};
    ARect content{};                      // Framebuffer pixels.
    int error_code{GLFW_NO_ERROR};
    const char* error_text{};
    std::string clipboard;
    bool text_input{};
    std::u32string ime_text;

    // pending: may become a tap, a long press (hold) or a scrolling drag.
    // mouse: began on a target that takes mouse drags (a scrollbar).
    enum class touch_t { none, pending, drag, hold, mouse };
    struct
    {
        touch_t mode{touch_t::none};
        int32_t pointer{-1};
        float start_x{}, start_y{}, last_x{}, last_y{};
        clock_t::time_point down{};
        double wheel_x{}, wheel_y{};
    } touch;

    auto fail(int code, const char* text) -> void
    {
        error_code = code;
        error_text = text;
        __android_log_print(ANDROID_LOG_ERROR, "lingogoogol", "GLFW: %s", text);
    }

    auto app() -> android_app* { return android_application(); }

    auto update_scale() -> void
    {
        const auto density{app()->config ? AConfiguration_getDensity(app()->config) : 0};
        scale = density > 0 && density != ACONFIGURATION_DENSITY_NONE ? static_cast<float>(density) / 160.f : 1.f;
    }

    // System bars, display cutouts and the soft keyboard reduce the usable area.
    auto update_content() -> void
    {
        if (!native_window)
        {
            content = {};
            return;
        }
        const int width{ANativeWindow_getWidth(native_window)}, height{ANativeWindow_getHeight(native_window)};
        ARect inset{};
        for (const auto type : {GAMECOMMON_INSETS_TYPE_SYSTEM_BARS, GAMECOMMON_INSETS_TYPE_DISPLAY_CUTOUT,
                                GAMECOMMON_INSETS_TYPE_IME})
        {
            ARect part{};
            GameActivity_getWindowInsets(app()->activity, type, &part);
            inset.left = std::max(inset.left, part.left);
            inset.top = std::max(inset.top, part.top);
            inset.right = std::max(inset.right, part.right);
            inset.bottom = std::max(inset.bottom, part.bottom);
        }
        content = {inset.left, inset.top, std::max(inset.left, width - inset.right),
                   std::max(inset.top, height - inset.bottom)};
    }

    auto logical(float physical, int origin) -> double { return (physical - static_cast<float>(origin)) / scale; }

    auto set_cursor(float x, float y) -> void
    {
        cursor_x = logical(x, content.left);
        cursor_y = logical(y, content.top);
        if (!hovered)
        {
            hovered = true;
            if (the_window && the_window->cursor_enter) the_window->cursor_enter(the_window, GLFW_TRUE);
        }
        if (the_window && the_window->cursor_pos) the_window->cursor_pos(the_window, cursor_x, cursor_y);
    }

    auto leave() -> void
    {
        if (!hovered) return;
        hovered = false;
        if (the_window && the_window->cursor_enter) the_window->cursor_enter(the_window, GLFW_FALSE);
    }

    auto button(int action) -> void
    {
        if (!the_window || !the_window->mouse_button) return;
        left_button = action;
        the_window->mouse_button(the_window, GLFW_MOUSE_BUTTON_LEFT, action, 0);
    }

    auto click() -> void
    {
        button(GLFW_PRESS);
        button(GLFW_RELEASE);
    }

    auto press_key(int key, int mods = 0) -> void
    {
        if (!the_window || !the_window->key) return;
        the_window->key(the_window, key, 0, GLFW_PRESS, mods);
        the_window->key(the_window, key, 0, GLFW_RELEASE, mods);
    }

    auto touch_event(const GameActivityMotionEvent& event) -> void
    {
        const auto action{event.action & AMOTION_EVENT_ACTION_MASK};
        const auto index{static_cast<std::uint32_t>(
            (event.action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT)};
        auto position = [&](std::uint32_t i, float& x, float& y)
        {
            x = GameActivityPointerAxes_getX(&event.pointers[i]);
            y = GameActivityPointerAxes_getY(&event.pointers[i]);
        };
        switch (action)
        {
        case AMOTION_EVENT_ACTION_DOWN:
        {
            float x{}, y{};
            position(0, x, y);
            touch = {touch_t::pending, event.pointers[0].id, x, y, x, y, clock_t::now(), 0, 0};
            set_cursor(x, y);
            if (the_window && the_window->touch_drag && the_window->touch_drag(the_window, cursor_x, cursor_y))
            {
                touch.mode = touch_t::mouse;
                button(GLFW_PRESS);
            }
            break;
        }
        case AMOTION_EVENT_ACTION_MOVE:
        {
            if (touch.mode == touch_t::none) break;
            for (std::uint32_t i{}; i < event.pointerCount; ++i)
            {
                if (event.pointers[i].id != touch.pointer) continue;
                float x{}, y{};
                position(i, x, y);
                if (touch.mode == touch_t::mouse)
                {
                    set_cursor(x, y);
                    continue;
                }
                if (touch.mode == touch_t::pending &&
                    std::hypot(x - touch.start_x, y - touch.start_y) / scale > touch_slop)
                    touch.mode = touch_t::drag;
                if (touch.mode == touch_t::drag && the_window && the_window->scroll)
                {
                    // The cursor stays where the drag began, so the wheel goes to
                    // the scroll area under the finger. The engine passes whole
                    // wheel steps only; keep the remainder for the next move.
                    touch.wheel_x += (x - touch.last_x) / scale / wheel_step;
                    touch.wheel_y += (y - touch.last_y) / scale / wheel_step;
                    const double dx{std::trunc(touch.wheel_x)}, dy{std::trunc(touch.wheel_y)};
                    touch.wheel_x -= dx;
                    touch.wheel_y -= dy;
                    if (dx != 0 || dy != 0) the_window->scroll(the_window, dx, dy);
                }
                touch.last_x = x;
                touch.last_y = y;
            }
            break;
        }
        case AMOTION_EVENT_ACTION_UP:
            if (touch.mode == touch_t::pending) click();
            if (touch.mode == touch_t::mouse) button(GLFW_RELEASE);
            // A long press keeps hovering so its tooltip stays readable.
            if (touch.mode != touch_t::hold) leave();
            touch.mode = touch_t::none;
            break;
        case AMOTION_EVENT_ACTION_POINTER_UP:
            if (event.pointers[index].id == touch.pointer)
            {
                if (touch.mode == touch_t::mouse) button(GLFW_RELEASE);
                touch.mode = touch_t::none;
                leave();
            }
            break;
        case AMOTION_EVENT_ACTION_CANCEL:
            if (touch.mode == touch_t::mouse) button(GLFW_RELEASE);
            touch.mode = touch_t::none;
            leave();
            break;
        default:
            break;
        }
    }

    auto key_code(int32_t code) -> int
    {
        if (code >= AKEYCODE_A && code <= AKEYCODE_Z) return GLFW_KEY_A + (code - AKEYCODE_A);
        if (code >= AKEYCODE_0 && code <= AKEYCODE_9) return GLFW_KEY_0 + (code - AKEYCODE_0);
        if (code >= AKEYCODE_F1 && code <= AKEYCODE_F12) return GLFW_KEY_F1 + (code - AKEYCODE_F1);
        switch (code)
        {
        case AKEYCODE_BACK:
        case AKEYCODE_ESCAPE: return GLFW_KEY_ESCAPE;
        case AKEYCODE_ENTER: return GLFW_KEY_ENTER;
        case AKEYCODE_NUMPAD_ENTER: return GLFW_KEY_KP_ENTER;
        case AKEYCODE_DEL: return GLFW_KEY_BACKSPACE;
        case AKEYCODE_FORWARD_DEL: return GLFW_KEY_DELETE;
        case AKEYCODE_TAB: return GLFW_KEY_TAB;
        case AKEYCODE_SPACE: return GLFW_KEY_SPACE;
        case AKEYCODE_DPAD_LEFT: return GLFW_KEY_LEFT;
        case AKEYCODE_DPAD_RIGHT: return GLFW_KEY_RIGHT;
        case AKEYCODE_DPAD_UP: return GLFW_KEY_UP;
        case AKEYCODE_DPAD_DOWN: return GLFW_KEY_DOWN;
        case AKEYCODE_MOVE_HOME: return GLFW_KEY_HOME;
        case AKEYCODE_MOVE_END: return GLFW_KEY_END;
        case AKEYCODE_PAGE_UP: return GLFW_KEY_PAGE_UP;
        case AKEYCODE_PAGE_DOWN: return GLFW_KEY_PAGE_DOWN;
        case AKEYCODE_SHIFT_LEFT: return GLFW_KEY_LEFT_SHIFT;
        case AKEYCODE_SHIFT_RIGHT: return GLFW_KEY_RIGHT_SHIFT;
        case AKEYCODE_CTRL_LEFT: return GLFW_KEY_LEFT_CONTROL;
        case AKEYCODE_CTRL_RIGHT: return GLFW_KEY_RIGHT_CONTROL;
        case AKEYCODE_ALT_LEFT: return GLFW_KEY_LEFT_ALT;
        case AKEYCODE_ALT_RIGHT: return GLFW_KEY_RIGHT_ALT;
        default: return GLFW_KEY_UNKNOWN;
        }
    }

    auto key_event(const GameActivityKeyEvent& event) -> void
    {
        const auto key{key_code(event.keyCode)};
        int mods{};
        if (event.metaState & AMETA_SHIFT_ON) mods |= GLFW_MOD_SHIFT;
        if (event.metaState & AMETA_CTRL_ON) mods |= GLFW_MOD_CONTROL;
        if (event.metaState & AMETA_ALT_ON) mods |= GLFW_MOD_ALT;
        const bool down{event.action == AKEY_EVENT_ACTION_DOWN};
        if (key != GLFW_KEY_UNKNOWN)
        {
            keys[static_cast<std::size_t>(key)] = down ? GLFW_PRESS : GLFW_RELEASE;
            if (the_window && the_window->key)
                the_window->key(the_window, key, event.scanCode,
                                down ? (event.repeatCount ? GLFW_REPEAT : GLFW_PRESS) : GLFW_RELEASE, mods);
        }
        // Hardware keyboards type through key events; soft keyboards through
        // GameTextInput (text_changed). Control characters are keys, not text.
        if (down && event.unicodeChar >= 0x20 && !(mods & (GLFW_MOD_CONTROL | GLFW_MOD_ALT)) && the_window &&
            the_window->character)
            the_window->character(the_window, static_cast<unsigned int>(event.unicodeChar));
    }

    auto decode(const char* text, int32_t length) -> std::u32string
    {
        std::u32string out;
        for (int32_t i{}; i < length;)
        {
            const auto lead{static_cast<unsigned char>(text[i])};
            const int count{lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2 : (lead >> 4) == 0xE ? 3 : 4};
            char32_t code{count == 1 ? lead : static_cast<char32_t>(lead & (0x7F >> count))};
            for (int j{1}; j < count && i + j < length; ++j)
                code = (code << 6) | (static_cast<unsigned char>(text[i + j]) & 0x3F);
            out.push_back(code);
            i += count;
        }
        return out;
    }

    // The soft keyboard edits its own copy of the text. Replay each change as
    // Backspace presses and typed characters, which lgo text fields understand.
    // Composing text (e.g. Zhuyin or Pinyin before conversion) is typed too and
    // then replaced once the input method commits the converted characters.
    auto text_changed() -> void
    {
        GameActivity_getTextInputState(app()->activity, [](void*, const GameTextInputState* state)
        {
            const auto text{decode(state->text_UTF8, state->text_length)};
            std::size_t common{};
            while (common < text.size() && common < ime_text.size() && text[common] == ime_text[common]) ++common;
            for (auto i{ime_text.size()}; i > common; --i) press_key(GLFW_KEY_BACKSPACE);
            if (the_window && the_window->character)
                for (auto i{common}; i < text.size(); ++i) the_window->character(the_window, text[i]);
            ime_text = text;
        }, nullptr);
    }

    auto reset_text_input() -> void
    {
        ime_text.clear();
        const GameTextInputState empty{"", 0, {0, 0}, {-1, -1}};
        GameActivity_setTextInputState(app()->activity, &empty);
    }

    auto surface_changed(ANativeWindow* window) -> void
    {
        if (native_window)
        {
            // The engine must release the swapchain and surface first.
            if (the_window && the_window->surface) the_window->surface(the_window, GLFW_FALSE);
            ANativeWindow_release(native_window);
            native_window = nullptr;
        }
        if (window)
        {
            ANativeWindow_acquire(window);
            native_window = window;
            update_content();
            if (the_window && the_window->surface) the_window->surface(the_window, GLFW_TRUE);
        }
        else
            update_content();
    }

    auto command(android_app* application, int32_t cmd) -> void
    {
        switch (cmd)
        {
        case APP_CMD_INIT_WINDOW:
        case APP_CMD_WINDOW_RESIZED:
            if (application->window != native_window) surface_changed(application->window);
            else update_content();
            if (the_window && the_window->refresh) the_window->refresh(the_window);
            break;
        case APP_CMD_TERM_WINDOW:
            surface_changed(nullptr);
            break;
        case APP_CMD_CONFIG_CHANGED:
            update_scale();
            update_content();
            break;
        case APP_CMD_WINDOW_INSETS_CHANGED:
        case APP_CMD_CONTENT_RECT_CHANGED:
        case APP_CMD_SOFTWARE_KB_VIS_CHANGED:
            update_content();
            break;
        case APP_CMD_EDITOR_ACTION:
            press_key(GLFW_KEY_ENTER);
            break;
        case APP_CMD_LOST_FOCUS:
            touch.mode = touch_t::none;
            leave();
            break;
        case APP_CMD_DESTROY:
            if (the_window) the_window->should_close = true;
            break;
        default:
            break;
        }
    }

    auto process(int timeout_ms) -> void
    {
        for (;;)
        {
            int events{};
            android_poll_source* source{};
            const auto result{ALooper_pollOnce(timeout_ms, nullptr, &events, reinterpret_cast<void**>(&source))};
            timeout_ms = 0;
            if (result == ALOOPER_POLL_ERROR) break;
            if (source) source->process(app(), source);
            android_run_posted();
            if (result == ALOOPER_POLL_TIMEOUT) break;
        }
        if (auto* input{android_app_swap_input_buffers(app())})
        {
            for (std::uint64_t i{}; i < input->motionEventsCount; ++i) touch_event(input->motionEvents[i]);
            android_app_clear_motion_events(input);
            for (std::uint64_t i{}; i < input->keyEventsCount; ++i) key_event(input->keyEvents[i]);
            android_app_clear_key_events(input);
        }
        if (app()->textInputState)
        {
            app()->textInputState = 0;
            text_changed();
        }
        if (touch.mode == touch_t::pending && clock_t::now() - touch.down >= long_press) touch.mode = touch_t::hold;
        if (app()->destroyRequested && the_window) the_window->should_close = true;
    }
}

extern "C"
{
GLFWAPI int glfwInit(void)
{
    if (!app()) return GLFW_FALSE;
    app()->onAppCmd = command;
    android_app_set_motion_event_filter(app(), nullptr);
    update_scale();
    return GLFW_TRUE;
}

GLFWAPI void glfwTerminate(void)
{
    if (native_window) ANativeWindow_release(native_window);
    native_window = nullptr;
}

GLFWAPI int glfwGetError(const char** description)
{
    const auto code{error_code};
    if (description) *description = error_text;
    error_code = GLFW_NO_ERROR;
    error_text = nullptr;
    return code;
}

GLFWAPI int glfwVulkanSupported(void)
{
    static const bool loaded{dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL) != nullptr};
    return loaded ? GLFW_TRUE : GLFW_FALSE;
}

GLFWAPI const char** glfwGetRequiredInstanceExtensions(uint32_t* count)
{
    static const char* extensions[]{VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
    *count = 2;
    return extensions;
}

GLFWAPI void glfwWindowHint(int, int) {}

GLFWAPI GLFWwindow* glfwCreateWindow(int, int, const char*, GLFWmonitor*, GLFWwindow*)
{
    if (the_window)
    {
        fail(GLFW_PLATFORM_ERROR, "Android has a single window.");
        return nullptr;
    }
    // The activity's surface arrives asynchronously after start-up.
    while (!native_window && !app()->destroyRequested) process(100);
    if (!native_window)
    {
        fail(GLFW_PLATFORM_UNAVAILABLE, "The activity closed before its window appeared.");
        return nullptr;
    }
    the_window = new GLFWwindow{};
    return the_window;
}

GLFWAPI void glfwDestroyWindow(GLFWwindow* window)
{
    if (window != the_window) return;
    delete the_window;
    the_window = nullptr;
}

GLFWAPI int glfwWindowShouldClose(GLFWwindow* window) { return window && window->should_close; }
GLFWAPI void glfwSetWindowUserPointer(GLFWwindow* window, void* pointer) { window->user_pointer = pointer; }
GLFWAPI void* glfwGetWindowUserPointer(GLFWwindow* window) { return window->user_pointer; }

GLFWAPI VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow*, const VkAllocationCallbacks* allocator,
                                         VkSurfaceKHR* surface)
{
    if (!native_window) return VK_ERROR_NATIVE_WINDOW_IN_USE_KHR;
    const VkAndroidSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, nullptr, 0,
                                             native_window};
    const auto create{reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
        vkGetInstanceProcAddr(instance, "vkCreateAndroidSurfaceKHR"))};
    return create ? create(instance, &info, allocator, surface) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

GLFWAPI void glfwGetFramebufferSize(GLFWwindow*, int* width, int* height)
{
    *width = native_window ? ANativeWindow_getWidth(native_window) : 0;
    *height = native_window ? ANativeWindow_getHeight(native_window) : 0;
}

GLFWAPI void glfwGetWindowSize(GLFWwindow*, int* width, int* height)
{
    // Keep the last layout size while the surface is gone; the engine ignores 0.
    static int last_width{}, last_height{};
    if (native_window && content.right > content.left)
    {
        last_width = static_cast<int>(std::lround((content.right - content.left) / scale));
        last_height = static_cast<int>(std::lround((content.bottom - content.top) / scale));
    }
    *width = last_width;
    *height = last_height;
}

GLFWAPI void glfwGetWindowContentScale(GLFWwindow*, float* xscale, float* yscale)
{
    // Also valid before glfwInit: fonts are created before the window.
    update_scale();
    if (xscale) *xscale = scale;
    if (yscale) *yscale = scale;
}

GLFWAPI void glfwGetWindowPos(GLFWwindow*, int* x, int* y)
{
    if (x) *x = 0;
    if (y) *y = 0;
}

GLFWAPI int glfwGetWindowAttrib(GLFWwindow*, int attrib)
{
    switch (attrib)
    {
    case GLFW_MAXIMIZED:
    case GLFW_VISIBLE:
        return GLFW_TRUE;
    // A finger is not a pointer: no hover highlight follows it.
    case GLFW_HOVERED:
    default:
        return GLFW_FALSE;
    }
}

// The activity owns its window geometry.
GLFWAPI void glfwSetWindowSize(GLFWwindow*, int, int) {}
GLFWAPI void glfwSetWindowSizeLimits(GLFWwindow*, int, int, int, int) {}
GLFWAPI void glfwFocusWindow(GLFWwindow*) {}
GLFWAPI void glfwHideWindow(GLFWwindow*) {}
GLFWAPI void glfwRestoreWindow(GLFWwindow*) {}

GLFWAPI void glfwPollEvents(void) { process(0); }
GLFWAPI void glfwWaitEventsTimeout(double timeout)
{
    process(static_cast<int>(std::clamp(timeout * 1000., 0., 1000.)));
}

GLFWAPI int glfwGetKey(GLFWwindow*, int key)
{
    return key >= 0 && key <= GLFW_KEY_LAST ? keys[static_cast<std::size_t>(key)] : GLFW_RELEASE;
}

GLFWAPI int glfwGetMouseButton(GLFWwindow*, int button)
{
    return button == GLFW_MOUSE_BUTTON_LEFT ? left_button : GLFW_RELEASE;
}

GLFWAPI void glfwGetCursorPos(GLFWwindow*, double* x, double* y)
{
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

GLFWAPI GLFWwindowrefreshfun glfwSetWindowRefreshCallback(GLFWwindow* window, GLFWwindowrefreshfun callback)
{
    return std::exchange(window->refresh, callback);
}
GLFWAPI GLFWcursorenterfun glfwSetCursorEnterCallback(GLFWwindow* window, GLFWcursorenterfun callback)
{
    return std::exchange(window->cursor_enter, callback);
}
GLFWAPI GLFWcursorposfun glfwSetCursorPosCallback(GLFWwindow* window, GLFWcursorposfun callback)
{
    return std::exchange(window->cursor_pos, callback);
}
GLFWAPI GLFWmousebuttonfun glfwSetMouseButtonCallback(GLFWwindow* window, GLFWmousebuttonfun callback)
{
    return std::exchange(window->mouse_button, callback);
}
GLFWAPI GLFWscrollfun glfwSetScrollCallback(GLFWwindow* window, GLFWscrollfun callback)
{
    return std::exchange(window->scroll, callback);
}
GLFWAPI GLFWkeyfun glfwSetKeyCallback(GLFWwindow* window, GLFWkeyfun callback)
{
    return std::exchange(window->key, callback);
}
GLFWAPI GLFWcharfun glfwSetCharCallback(GLFWwindow* window, GLFWcharfun callback)
{
    return std::exchange(window->character, callback);
}

GLFWAPI const char* glfwGetClipboardString(GLFWwindow*)
{
    try
    {
        clipboard = android_call_string("clipboardText", "()Ljava/lang/String;");
        return clipboard.c_str();
    }
    catch (const std::exception&)
    {
        fail(GLFW_PLATFORM_ERROR, "Cannot read the clipboard.");
        return nullptr;
    }
}

GLFWAPI void glfwSetClipboardString(GLFWwindow*, const char* string)
{
    try
    {
        const auto env{android_jni()};
        const auto text{android_string(env, std::string{string ? string : ""})};
        android_call_void("setClipboardText", "(Ljava/lang/String;)V", text);
        env->DeleteLocalRef(text);
    }
    catch (const std::exception&)
    {
        fail(GLFW_PLATFORM_ERROR, "Cannot write the clipboard.");
    }
}

GLFWAPI GLFWsurfacefunANDROID glfwSetWindowSurfaceCallbackANDROID(GLFWwindow* window, GLFWsurfacefunANDROID callback)
{
    return std::exchange(window->surface, callback);
}

GLFWAPI GLFWtouchdragfunANDROID glfwSetTouchDragCallbackANDROID(GLFWwindow* window, GLFWtouchdragfunANDROID callback)
{
    return std::exchange(window->touch_drag, callback);
}

GLFWAPI void glfwGetWindowContentAreaANDROID(GLFWwindow*, int* x, int* y, int* width, int* height)
{
    *x = content.left;
    *y = content.top;
    *width = content.right - content.left;
    *height = content.bottom - content.top;
}

GLFWAPI void glfwPressKeyANDROID(int key) { press_key(key); }

GLFWAPI void glfwSetTextInputActiveANDROID(GLFWwindow*, int active)
{
    if (static_cast<bool>(active) == text_input) return;
    text_input = active;
    if (active)
    {
        GameActivity_setImeEditorInfo(app()->activity, TYPE_CLASS_TEXT, IME_ACTION_DONE,
                                      static_cast<GameTextInputImeOptions>(IME_FLAG_NO_FULLSCREEN | IME_FLAG_NO_EXTRACT_UI));
        reset_text_input();
        GameActivity_showSoftInput(app()->activity, GAMEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT);
    }
    else
        GameActivity_hideSoftInput(app()->activity, 0);
}
}
