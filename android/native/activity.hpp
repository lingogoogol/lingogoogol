#pragma once
// Access to the running GameActivity from the app thread and from JNI callbacks.
#include <jni.h>

#include <functional>
#include <string>
#include <thread>

struct android_app;

// Set once by android_main before the shared code runs.
auto android_application_set(android_app* app) -> void;
auto android_application() -> android_app*;
// The thread running android_main, which must keep processing activity events.
auto android_application_thread() -> std::thread::id;

// JNIEnv for the calling thread; attaches it to the VM on first use.
auto android_jni() -> JNIEnv*;

// Calls a method of the activity object (MainActivity.kt). Java exceptions are
// cleared and reported as std::runtime_error.
auto android_call_void(const char* name, const char* signature, ...) -> void;
auto android_call_boolean(const char* name, const char* signature, ...) -> bool;
auto android_call_string(const char* name, const char* signature, ...) -> std::string;
auto android_string(JNIEnv* env, const std::string& text) -> jstring;
auto android_string(JNIEnv* env, jstring text) -> std::string;

// Runs work on the app thread during its next event poll. Used by JNI callbacks,
// which arrive on the Java UI thread.
auto android_post(std::function<void()> work) -> void;
auto android_run_posted() -> void;
