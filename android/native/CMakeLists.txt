# Android glue: GameActivity entry point, the GLFW API on top of GameActivity,
# and calls into MainActivity.kt. Only configured for Android builds.

# Provided by the androidx.games:games-activity AAR through Gradle's Prefab.
find_package(game-activity REQUIRED CONFIG)

add_library(android.glfw STATIC glfw.cpp activity.cpp)
target_link_libraries(
    android.glfw
    PUBLIC
        game-activity::game-activity_static
        android
        log
        "${Vulkan_LIBRARY}"
    PRIVATE
        common
)

add_library(android.app STATIC android_main.cpp bridge.cpp)
target_link_libraries(android.app PRIVATE common android.glfw)
