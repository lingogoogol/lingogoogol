#include "android/native/activity.hpp"

#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <cstdarg>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace
{
    android_app* application{};
    std::thread::id application_thread;
    std::mutex posted_mutex;
    std::deque<std::function<void()>> posted;

    // Detaches threads that android_jni attached, when they exit.
    struct thread_attachment_t
    {
        JavaVM* vm{};
        ~thread_attachment_t()
        {
            if (vm) vm->DetachCurrentThread();
        }
    };
    thread_local thread_attachment_t attachment;

    auto check(JNIEnv* env, const char* name) -> void
    {
        if (!env->ExceptionCheck()) return;
        env->ExceptionDescribe();
        env->ExceptionClear();
        throw std::runtime_error{std::string{"Android call failed: "} + name};
    }

    auto method(JNIEnv* env, const char* name, const char* signature) -> jmethodID
    {
        const auto activity{application->activity->javaGameActivity};
        const auto type{env->GetObjectClass(activity)};
        const auto id{env->GetMethodID(type, name, signature)};
        env->DeleteLocalRef(type);
        check(env, name);
        return id;
    }
}

auto android_application_set(android_app* app) -> void
{
    application = app;
    application_thread = std::this_thread::get_id();
}
auto android_application_thread() -> std::thread::id { return application_thread; }
auto android_application() -> android_app* { return application; }

auto android_jni() -> JNIEnv*
{
    const auto vm{application->activity->vm};
    JNIEnv* env{};
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) return env;
    if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
        throw std::runtime_error{"Cannot attach the thread to the Java VM."};
    attachment.vm = vm;
    return env;
}

auto android_call_void(const char* name, const char* signature, ...) -> void
{
    const auto env{android_jni()};
    const auto id{method(env, name, signature)};
    va_list arguments;
    va_start(arguments, signature);
    env->CallVoidMethodV(application->activity->javaGameActivity, id, arguments);
    va_end(arguments);
    check(env, name);
}

auto android_call_boolean(const char* name, const char* signature, ...) -> bool
{
    const auto env{android_jni()};
    const auto id{method(env, name, signature)};
    va_list arguments;
    va_start(arguments, signature);
    const auto result{env->CallBooleanMethodV(application->activity->javaGameActivity, id, arguments)};
    va_end(arguments);
    check(env, name);
    return result == JNI_TRUE;
}

auto android_call_string(const char* name, const char* signature, ...) -> std::string
{
    const auto env{android_jni()};
    const auto id{method(env, name, signature)};
    va_list arguments;
    va_start(arguments, signature);
    const auto result{static_cast<jstring>(
        env->CallObjectMethodV(application->activity->javaGameActivity, id, arguments))};
    va_end(arguments);
    check(env, name);
    auto text{android_string(env, result)};
    if (result) env->DeleteLocalRef(result);
    return text;
}

auto android_string(JNIEnv* env, const std::string& text) -> jstring
{
    // NewStringUTF expects modified UTF-8; build from UTF-8 bytes instead, so
    // characters outside the BMP (emoji in file names) survive.
    const auto bytes{env->NewByteArray(static_cast<jsize>(text.size()))};
    env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(text.size()), reinterpret_cast<const jbyte*>(text.data()));
    const auto string_type{env->FindClass("java/lang/String")};
    const auto constructor{env->GetMethodID(string_type, "<init>", "([BLjava/lang/String;)V")};
    const auto charset{env->NewStringUTF("UTF-8")};
    const auto result{static_cast<jstring>(env->NewObject(string_type, constructor, bytes, charset))};
    env->DeleteLocalRef(charset);
    env->DeleteLocalRef(string_type);
    env->DeleteLocalRef(bytes);
    check(env, "String(byte[], String)");
    return result;
}

auto android_string(JNIEnv* env, jstring text) -> std::string
{
    if (!text) return {};
    const auto string_type{env->FindClass("java/lang/String")};
    const auto get_bytes{env->GetMethodID(string_type, "getBytes", "(Ljava/lang/String;)[B")};
    const auto charset{env->NewStringUTF("UTF-8")};
    const auto bytes{static_cast<jbyteArray>(env->CallObjectMethod(text, get_bytes, charset))};
    env->DeleteLocalRef(charset);
    env->DeleteLocalRef(string_type);
    check(env, "String.getBytes");
    std::string result(static_cast<std::size_t>(env->GetArrayLength(bytes)), '\0');
    env->GetByteArrayRegion(bytes, 0, static_cast<jsize>(result.size()), reinterpret_cast<jbyte*>(result.data()));
    env->DeleteLocalRef(bytes);
    return result;
}

auto android_post(std::function<void()> work) -> void
{
    {
        std::lock_guard lock{posted_mutex};
        posted.push_back(std::move(work));
    }
    if (application) ALooper_wake(application->looper);
}

auto android_run_posted() -> void
{
    for (;;)
    {
        std::function<void()> work;
        {
            std::lock_guard lock{posted_mutex};
            if (posted.empty()) return;
            work = std::move(posted.front());
            posted.pop_front();
        }
        work();
    }
}
