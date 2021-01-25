import org.gradle.internal.os.OperatingSystem

plugins {
    kotlin("multiplatform") version "1.4.21"
}

repositories {
    jcenter()
}

val host: OperatingSystem = OperatingSystem.current()

kotlin {
    val nativeTarget = when {
        host.isLinux -> linuxX64("native")
        host.isMacOsX -> macosX64("native")
        host.isWindows -> mingwX64("native")
        else -> error("Unknown host")
    }

    nativeTarget.apply {
        binaries {
            executable {
                entryPoint = "kaleidoscope.main"
            }
        }
    }

    sourceSets {
        all {
            languageSettings.apply {
                useExperimentalAnnotation("kotlin.RequiresOptIn")
            }
        }
    }
}
