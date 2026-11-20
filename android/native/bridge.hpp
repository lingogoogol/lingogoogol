#pragma once
// Calls from the shared C++ code into the Android app (Kotlin, through JNI).
// Declared outside any module so module implementation units of the shared
// code can include this header in their global module fragment.
#include <string>
#include <vector>

// Shows the Storage Access Framework picker and waits for it, processing
// activity events meanwhile. Returns the selected document URIs (several files,
// or one folder tree), or nothing when the user cancels.
auto android_pick_documents(bool folder) -> std::vector<std::string>;

// Screen pixels per dp, the GLFW layer's logical pixel (glfw.cpp).
auto android_display_scale() -> float;

// An access token for `scopes` from Google Play services (AuthorizationClient)
// for the app's Android OAuth client. `email` selects a saved account (empty: the
// user picks one). Returns "" when the grant needs the user and `interactive` is
// false; throws when the user cancels or Google Play services fails.
auto android_authorize(const std::string& email, const std::string& scopes, bool interactive) -> std::string;

// Starts or stops the foreground service that keeps transfers running while the
// app is in the background. Cheap to call every frame; acts on changes only.
auto android_set_transfers_active(bool active) -> void;

// Opens a URL in the user's browser; false when no app can handle it.
auto android_open_url(const std::string& url) -> bool;

// Shows a fatal error to the user before the activity finishes.
auto android_report_error(const std::string& message) -> void;
