plugins {
    id("com.android.application") version "9.5.0" apply false
    // AGP compiles Kotlin itself; this only selects the Kotlin compiler version.
    id("org.jetbrains.kotlin.android") version "2.4.21" apply false
}
