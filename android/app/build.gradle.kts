import com.android.build.api.artifact.SingleArtifact
import com.android.build.api.variant.BuiltArtifactsLoader
import java.nio.file.Files
import java.nio.file.StandardCopyOption
import java.util.zip.ZipEntry
import java.util.zip.ZipFile
import java.util.zip.ZipOutputStream

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

// One APK per ABI in ../../install/<target triple>_<build type>/storage-android.apk
// (e.g. aarch64-linux-android29_Debug), named like the Windows build's
// install/x64-win32-pe-mingw_Release. Derived from the universal
// debug APK, which stays unchanged: keep only one ABI's native libraries, align
// (16 KB pages) and re-sign with the same fixed key. AGP ignores
// android.injected.build.abi outside Android Studio deployments, and ABI splits
// would rename the universal app-debug.apk.
abstract class AbiApksTask : DefaultTask() {
    @get:InputDirectory abstract val apkFolder: DirectoryProperty
    @get:Internal abstract val loader: Property<BuiltArtifactsLoader>
    @get:Input abstract val folders: MapProperty<String, String> // ABI -> <triple>_<build type>
    @get:Internal abstract val installDir: DirectoryProperty
    @get:InputDirectory abstract val buildTools: DirectoryProperty
    @get:InputFile abstract val keystore: RegularFileProperty
    @get:Input abstract val keyAlias: Property<String>
    @get:Internal abstract val storePassword: Property<String>
    @get:Internal abstract val keyPassword: Property<String>
    @get:Inject abstract val exec: ExecOperations

    @TaskAction
    fun generate() {
        val universal = File(loader.get().load(apkFolder.get())!!.elements.single().outputFile)
        val windows = System.getProperty("os.name").startsWith("Windows")
        val tools = buildTools.get().asFile
        val zipalign = tools.resolve(if (windows) "zipalign.exe" else "zipalign").path
        val apksigner = tools.resolve(if (windows) "apksigner.bat" else "apksigner").path
        val work = temporaryDir
        for ((abi, folder) in folders.get()) {
            val stripped = work.resolve("$abi.apk")
            ZipFile(universal).use { zip ->
                ZipOutputStream(stripped.outputStream()).use { out ->
                    for (entry in zip.entries()) {
                        if (entry.name.startsWith("lib/") && !entry.name.startsWith("lib/$abi/")) continue
                        val copy = ZipEntry(entry.name).apply {
                            method = entry.method
                            time = entry.time
                            // Stored (uncompressed) native libraries need their size and CRC.
                            if (entry.method == ZipEntry.STORED) {
                                size = entry.size
                                compressedSize = entry.compressedSize
                                crc = entry.crc
                            }
                        }
                        out.putNextEntry(copy)
                        zip.getInputStream(entry).use { it.copyTo(out) }
                        out.closeEntry()
                    }
                }
            }
            val aligned = work.resolve("$abi-aligned.apk")
            exec.exec { commandLine(zipalign, "-P", "16", "-f", "4", stripped.path, aligned.path) }
            val target = installDir.get().asFile.resolve(folder)
            target.mkdirs()
            val signed = target.resolve("storage-android.apk.tmp")
            // Passwords go through the environment, never the command line.
            exec.exec {
                environment("LGO_STORE_PASSWORD", storePassword.get())
                environment("LGO_KEY_PASSWORD", keyPassword.get())
                // No v4 (.idsig) signature file: installation does not need it.
                commandLine(apksigner, "sign", "--v4-signing-enabled", "false", "--ks", keystore.get().asFile.path,
                    "--ks-pass", "env:LGO_STORE_PASSWORD", "--key-pass", "env:LGO_KEY_PASSWORD",
                    "--ks-key-alias", keyAlias.get(), "--out", signed.path, aligned.path)
            }
            // Replace the published APK only once it is complete.
            Files.move(signed.toPath(), target.resolve("storage-android.apk").toPath(),
                StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE)
        }
    }
}

androidComponents {
    onVariants { variant ->
        variant.sources.assets?.addGeneratedSourceDirectory(appAssets, AppAssetsTask::outputDir)
        if (variant.buildType == "debug") {
            val signing = android.signingConfigs.getByName("debug")
            val installRoot = workspace.resolve("install")
            val minSdk = android.defaultConfig.minSdk!!
            val buildType = variant.buildType!!.replaceFirstChar { it.uppercase() }
            val abis = mapOf("arm64-v8a" to "aarch64-linux-android${minSdk}_$buildType",
                             "x86_64" to "x86_64-linux-android${minSdk}_$buildType")
            tasks.register<AbiApksTask>("installAbiApks") {
                apkFolder.set(variant.artifacts.get(SingleArtifact.APK))
                loader.set(variant.artifacts.getBuiltArtifactsLoader())
                folders.set(abis)
                installDir.set(installRoot)
                buildTools.set(androidComponents.sdkComponents.sdkDirectory.map { it.dir("build-tools/${android.buildToolsVersion}") })
                keystore.set(signing.storeFile!!)
                keyAlias.set(signing.keyAlias!!)
                storePassword.set(signing.storePassword!!)
                keyPassword.set(signing.keyPassword!!)
                outputs.files(abis.values.map { installRoot.resolve("$it/storage-android.apk") })
            }
        }
    }
}

tasks.matching { it.name == "assembleDebug" }.configureEach { finalizedBy("installAbiApks") }

dependencies {
    implementation("androidx.games:games-activity:4.4.2")
    implementation("androidx.appcompat:appcompat:1.8.0")
    implementation("androidx.core:core-ktx:1.19.1")
    // Google sign-in for the Android OAuth client (AuthorizationClient).
    implementation("com.google.android.gms:play-services-auth:22.0.0")
}
