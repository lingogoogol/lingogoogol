plugins {
    id("com.android.application")
}

// Workspace resources outside the repository, beside the Windows build's ../lib
// and ../font. Override with -Plgo.font=..., -Plgo.keystore=..., -Plgo.slangc=...
val workspace = rootDir.resolve("../..")
val fontFile = file(providers.gradleProperty("lgo.font").getOrElse(
    workspace.resolve("font/NotoSansTC-VariableFont_wght.ttf").path))
val keystoreFile = file(providers.gradleProperty("lgo.keystore").getOrElse(
    workspace.resolve("android-keystore/debug.keystore").path))
val slangc = providers.gradleProperty("lgo.slangc").getOrElse("slangc")
val shaderSources = rootDir.resolve("../lgo/GUI/primitive/engine")

android {
    namespace = "io.github.lingogoogol"
    compileSdk = 37
    ndkVersion = "30.0.16248370"

    defaultConfig {
        applicationId = "io.github.lingogoogol"
        minSdk = 29
        targetSdk = 37
        versionCode = 1
        versionName = "0.0.5.0"
        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }
        externalNativeBuild {
            cmake {
                arguments += listOf("-DANDROID_STL=c++_shared")
                targets += "storage"
            }
        }
    }

    signingConfigs {
        getByName("debug") {
            // A fixed key keeps the signing SHA-1 registered for Google sign-in.
            storeFile = keystoreFile
            storePassword = "android"
            keyAlias = "androiddebugkey"
            keyPassword = "android"
        }
    }

    buildTypes {
        // The shared C++ code is slow without optimization; debug builds still
        // keep Java/Kotlin debuggable.
        getByName("debug") {
            externalNativeBuild { cmake { arguments += "-DCMAKE_BUILD_TYPE=Release" } }
        }
    }

    externalNativeBuild {
        cmake {
            // The same CMake tree as the Windows build. Found on PATH: the SDK's
            // CMake packages predate CMAKE_CXX_STDLIB_MODULES_JSON (CMake 4.2).
            path = file("../../CMakeLists.txt")
            version = "4.2.3"
        }
    }

    buildFeatures {
        prefab = true
    }
}

// Font and SPIR-V shaders, which the app copies into its data directory at start.
abstract class AppAssetsTask : DefaultTask() {
    @get:InputFile abstract val font: RegularFileProperty
    @get:InputDirectory abstract val shaders: DirectoryProperty
    @get:Input abstract val compiler: Property<String>
    @get:OutputDirectory abstract val outputDir: DirectoryProperty
    @get:Inject abstract val exec: ExecOperations

    @TaskAction
    fun generate() {
        val out = outputDir.get().asFile
        out.deleteRecursively()
        font.get().asFile.copyTo(out.resolve("font/${font.get().asFile.name}"))
        out.resolve("shader").mkdirs()
        // Same options as add_slang_target in lgo/GUI/primitive/engine/CMakeLists.txt.
        for (name in listOf("solid", "texture")) {
            exec.exec {
                workingDir = shaders.get().asFile
                commandLine(compiler.get(), "$name.slang", "-target", "spirv", "-profile", "spirv_1_4",
                    "-emit-spirv-directly", "-fvk-use-entrypoint-name",
                    "-entry", "vertex_main", "-entry", "fragment_main",
                    "-o", out.resolve("shader/$name.spv").path)
            }
        }
    }
}

val appAssets = tasks.register<AppAssetsTask>("appAssets") {
    font.set(fontFile)
    shaders.set(shaderSources)
    compiler.set(slangc)
}

androidComponents {
    onVariants { variant ->
        variant.sources.assets?.addGeneratedSourceDirectory(appAssets, AppAssetsTask::outputDir)
    }
}

dependencies {
    implementation("androidx.games:games-activity:4.4.2")
    implementation("androidx.appcompat:appcompat:1.8.0")
    implementation("androidx.core:core-ktx:1.19.1")
    // Google sign-in for the Android OAuth client (AuthorizationClient).
    implementation("com.google.android.gms:play-services-auth:22.0.0")
}
