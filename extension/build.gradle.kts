import com.android.build.gradle.internal.tasks.factory.dependsOn

plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
}

val pluginName = "GodotMidi"

val pluginPackageName = "org.godotengine.plugin.android.gdextension.godotmidi"

android {
    namespace = pluginPackageName
    compileSdk = 33

    buildFeatures {
        buildConfig = true
    }

    defaultConfig {
        minSdk = 21

        externalNativeBuild {
            cmake {
                cppFlags("")
            }
        }
        ndk {
            abiFilters.add("arm64-v8a")
        }

        manifestPlaceholders["godotPluginName"] = pluginName
        manifestPlaceholders["godotPluginPackageName"] = pluginPackageName
        buildConfigField("String", "GODOT_PLUGIN_NAME", "\"${pluginName}\"")
        setProperty("archivesBaseName", pluginName)
    }
    externalNativeBuild {
        cmake {
            path("CMakeLists.txt")
            version = "3.22.1"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
}

dependencies {
    implementation("org.godotengine:godot:4.3.0.stable")
}

// BUILD TASKS DEFINITION
val cleanAssetsAddons by tasks.registering(Copy::class) {
    delete("src/main/assets/addons")
}

val copyGdExtensionConfigToAssets by tasks.registering(Copy::class) {
    description = "Copies the gdextension config file to the plugin's assets directory"

    dependsOn(cleanAssetsAddons)

    from("../game/addons/godot_midi")
    include("godotmidi.gdextension")
    into("src/main/assets/addons/godot_midi")
}

val copyDebugAARToDemoAddons by tasks.registering(Copy::class) {
    description = "Copies the generated debug AAR binary to the plugin's addons directory"
    from("build/outputs/aar")
    include("$pluginName-debug.aar")
    into("../game/addons/godot_midi/bin")
}

val copyReleaseAARToDemoAddons by tasks.registering(Copy::class) {
    description = "Copies the generated release AAR binary to the plugin's addons directory"
    from("build/outputs/aar")
    include("$pluginName-release.aar")
    into("../game/addons/godot_midi/bin")
}

val copyDebugSharedLibs by tasks.registering(Copy::class) {
    description = "Copies the generated debug .so shared library to the plugin's addons directory"
    from("build/intermediates/cmake/debug/obj")
    into("../game/addons/godot_midi/bin/debug")
}

val copyReleaseSharedLibs by tasks.registering(Copy::class) {
    description = "Copies the generated release .so shared library to the plugin's addons directory"
    from("build/intermediates/cmake/release/obj")
    into("../game/addons/godot_midi/bin/release")
}

val cleanDemoAddons by tasks.registering(Delete::class) {
    delete("../game/addons/godot_midi/$pluginName-*.aar")
}

val copyAddonsToDemo by tasks.registering(Copy::class) {
    description = "Copies the plugin's output artifact to the output directory"

    dependsOn(cleanDemoAddons)
    finalizedBy(copyDebugAARToDemoAddons)
    finalizedBy(copyReleaseAARToDemoAddons)
}

tasks.named("preBuild").dependsOn(copyGdExtensionConfigToAssets)

tasks.named("assemble").configure {
    dependsOn(copyGdExtensionConfigToAssets)
    finalizedBy(copyAddonsToDemo)
}

tasks.named<Delete>("clean").apply {
    dependsOn(cleanDemoAddons)
    dependsOn(cleanAssetsAddons)
}
