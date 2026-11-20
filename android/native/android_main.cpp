// Entry point of the Android app (libstorage.so, loaded by MainActivity).
#include "android/native/activity.hpp"
#include "android/native/glfw_android.h"

#include <android/asset_manager.h>
#include <android/log.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

// storage/storage.cpp: the same application code as the Windows wWinMain.
auto storage_main() -> int;

namespace
{
    // The shared code reads its font and shaders relative to the working
    // directory, as beside bin/ on Windows. The APK packages them as assets;
    // copy any that are missing or changed by an app update.
    auto extract_assets(AAssetManager* assets, const char* directory) -> void
    {
        auto* listing{AAssetManager_openDir(assets, directory)};
        if (!listing) return;
        std::filesystem::create_directories(directory);
        while (const char* name{AAssetDir_getNextFileName(listing)})
        {
            const auto path{std::string{directory} + "/" + name};
            auto* asset{AAssetManager_open(assets, path.c_str(), AASSET_MODE_STREAMING)};
            if (!asset) continue;
            std::string content(static_cast<std::size_t>(AAsset_getLength64(asset)), '\0');
            const auto read{AAsset_read(asset, content.data(), content.size())};
            AAsset_close(asset);
            if (read < 0 || static_cast<std::size_t>(read) != content.size()) continue;
            std::ifstream existing{path, std::ios::binary};
            if (existing && std::string{std::istreambuf_iterator<char>{existing}, {}} == content) continue;
            existing.close();
            const auto temporary{path + ".tmp"};
            std::ofstream{temporary, std::ios::binary | std::ios::trunc}.write(content.data(),
                                                                               static_cast<std::streamsize>(content.size()));
            std::filesystem::rename(temporary, path);
        }
        AAssetDir_close(listing);
    }

    // The system trust store, including roots updated through the Conscrypt
    // module (Android 14+), as one PEM bundle for curl's CURLOPT_CAINFO.
    auto write_certificates() -> void
    {
        std::string bundle;
        for (const auto* directory : {"/apex/com.android.conscrypt/cacerts", "/system/etc/security/cacerts"})
        {
            std::error_code error;
            for (const auto& entry : std::filesystem::directory_iterator{directory, error})
            {
                std::ifstream file{entry.path(), std::ios::binary};
                bundle.append(std::istreambuf_iterator<char>{file}, {});
                bundle.push_back('\n');
            }
            if (!bundle.empty()) break;
        }
        if (bundle.empty())
        {
            __android_log_print(ANDROID_LOG_ERROR, "lingogoogol", "No system CA certificates were found.");
            return;
        }
        std::ofstream{"cacert.pem.tmp", std::ios::binary | std::ios::trunc} << bundle;
        std::filesystem::rename("cacert.pem.tmp", "cacert.pem");
    }

    auto JNICALL native_back(JNIEnv*, jobject) -> void
    {
        android_post([] { glfwPressKeyANDROID(GLFW_KEY_ESCAPE); });
    }
}

// Defined in bridge.cpp, registered here with the other native methods.
auto JNICALL android_native_documents_picked(JNIEnv* env, jobject, jobjectArray uris) -> void;
auto JNICALL android_native_error_dismissed(JNIEnv*, jobject) -> void;
auto JNICALL android_native_authorized(JNIEnv* env, jobject, jlong request, jstring token, jstring error) -> void;

extern "C" void android_main(android_app* app)
{
    android_application_set(app);
    const auto env{android_jni()};
    const auto type{env->GetObjectClass(app->activity->javaGameActivity)};
    // Registered by name so the Java package name is not part of the C++ code.
    const JNINativeMethod methods[]{
        {"nativeBack", "()V", reinterpret_cast<void*>(native_back)},
        {"nativeDocumentsPicked", "([Ljava/lang/String;)V", reinterpret_cast<void*>(android_native_documents_picked)},
        {"nativeErrorDismissed", "()V", reinterpret_cast<void*>(android_native_error_dismissed)},
        {"nativeAuthorized", "(JLjava/lang/String;Ljava/lang/String;)V", reinterpret_cast<void*>(android_native_authorized)},
    };
    env->RegisterNatives(type, methods, std::size(methods));
    env->DeleteLocalRef(type);

    try
    {
        std::filesystem::current_path(app->activity->internalDataPath);
        extract_assets(app->activity->assetManager, "font");
        extract_assets(app->activity->assetManager, "shader");
        write_certificates();
    }
    catch (const std::exception& error)
    {
        __android_log_print(ANDROID_LOG_ERROR, "lingogoogol", "Cannot prepare app data: %s", error.what());
    }

    storage_main();

    // The window closed or the app failed: finish the activity, then let the
    // glue process its remaining lifecycle commands until it is destroyed.
    GameActivity_finish(app->activity);
    while (!app->destroyRequested)
    {
        int events{};
        android_poll_source* source{};
        if (ALooper_pollOnce(-1, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0 && source)
            source->process(app, source);
        android_run_posted();
    }
}
