// Calls from the shared code into MainActivity.kt (see bridge.hpp).
#include "android/native/bridge.hpp"

#include "android/native/activity.hpp"
#include "android/native/glfw_android.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>

namespace
{
    std::optional<std::vector<std::string>> picked;
    bool error_dismissed{};

    // Authorization requests may come from transfer and Cloud worker threads.
    struct authorization_t
    {
        bool done{};
        std::string token, error;
    };
    std::mutex authorization_mutex;
    std::map<std::int64_t, authorization_t> authorizations;
    std::atomic_int64_t next_authorization{};

    // The system UI runs in front of the activity and reports back through a
    // native callback. Keep processing activity events meanwhile: the glue
    // blocks the Java UI thread until the app thread handles lifecycle commands.
    template <typename t_done> auto wait(t_done done) -> void
    {
        while (!done() && !android_application()->destroyRequested)
            glfwWaitEventsTimeout(0.1);
    }

    // The app thread keeps processing events; other threads only wait.
    template <typename t_done> auto wait_any_thread(t_done done) -> void
    {
        if (std::this_thread::get_id() == android_application_thread()) wait(done);
        else
            while (!done()) std::this_thread::sleep_for(std::chrono::milliseconds{20});
    }
}

auto JNICALL android_native_documents_picked(JNIEnv* env, jobject, jobjectArray uris) -> void
{
    std::vector<std::string> result;
    const auto count{uris ? env->GetArrayLength(uris) : 0};
    for (jsize i{}; i < count; ++i)
    {
        const auto uri{static_cast<jstring>(env->GetObjectArrayElement(uris, i))};
        result.push_back(android_string(env, uri));
        env->DeleteLocalRef(uri);
    }
    android_post([result = std::move(result)] { picked = result; });
}

auto JNICALL android_native_error_dismissed(JNIEnv*, jobject) -> void
{
    android_post([] { error_dismissed = true; });
}

auto JNICALL android_native_authorized(JNIEnv* env, jobject, jlong request, jstring token, jstring error) -> void
{
    std::lock_guard lock{authorization_mutex};
    auto& result{authorizations[request]};
    result.token = android_string(env, token);
    result.error = android_string(env, error);
    result.done = true;
}

auto android_authorize(const std::string& email, const std::string& scopes, bool interactive) -> std::string
{
    const auto request{++next_authorization};
    {
        std::lock_guard lock{authorization_mutex};
        authorizations[request] = {};
    }
    const auto env{android_jni()};
    const auto email_text{android_string(env, email)};
    const auto scopes_text{android_string(env, scopes)};
    android_call_void("authorize", "(JLjava/lang/String;Ljava/lang/String;Z)V", static_cast<jlong>(request), email_text,
                      scopes_text, static_cast<jboolean>(interactive));
    env->DeleteLocalRef(scopes_text);
    env->DeleteLocalRef(email_text);
    wait_any_thread([request]
    {
        std::lock_guard lock{authorization_mutex};
        return authorizations[request].done;
    });
    std::lock_guard lock{authorization_mutex};
    const auto result{authorizations[request]};
    authorizations.erase(request);
    if (!result.error.empty()) throw std::runtime_error{"Google authorization failed: " + result.error};
    return result.token;
}

auto android_set_transfers_active(bool active) -> void
{
    static bool current{};
    if (active == current) return;
    current = active;
    android_call_void("setTransfersActive", "(Z)V", static_cast<jboolean>(active));
}

auto android_display_scale() -> float
{
    float scale{1};
    glfwGetWindowContentScale(nullptr, &scale, nullptr);
    return scale;
}

auto android_pick_documents(bool folder) -> std::vector<std::string>
{
    picked.reset();
    android_call_void("pickDocuments", "(Z)V", static_cast<jboolean>(folder));
    wait([] { return picked.has_value(); });
    auto result{picked.value_or(std::vector<std::string>{})};
    picked.reset();
    return result;
}

auto android_open_url(const std::string& url) -> bool
{
    const auto env{android_jni()};
    const auto text{android_string(env, url)};
    const auto opened{android_call_boolean("openUrl", "(Ljava/lang/String;)Z", text)};
    env->DeleteLocalRef(text);
    return opened;
}

auto android_report_error(const std::string& message) -> void
{
    error_dismissed = false;
    const auto env{android_jni()};
    const auto text{android_string(env, message)};
    android_call_void("reportError", "(Ljava/lang/String;)V", text);
    env->DeleteLocalRef(text);
    wait([] { return error_dismissed; });
}
