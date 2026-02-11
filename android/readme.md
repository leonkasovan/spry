# Android Build

This directory contains the Gradle project for building Spry as an Android app.

## Prerequisites

- Android SDK (API 33+), with NDK installed (e.g. 25.x or 26.x)
- Android SDK Build-Tools
- CMake 3.22.1+ (install via SDK Manager → SDK Tools → CMake)
- Java 17 (for Gradle 8.x)

You can install these through Android Studio's SDK Manager, or via
`sdkmanager` on the command line.

## Configuration

| Setting   | Value           |
|-----------|-----------------|
| minSdk    | 30 (Android 11) |
| targetSdk | 33              |
| ABIs      | arm64-v8a, armeabi-v7a, x86_64 |

Network and Nuklear UI modules are disabled for the Android build.

## Building

### Windows wrapper setup

If you do not have Gradle installed, run the helper script to download Gradle
and generate the wrapper files:

```powershell
cd android
./setup-wrapper.ps1
```

### From the command line

```bash
cd android
./gradlew assembleDebug     # debug APK
./gradlew assembleRelease   # release APK
```

The APK will be at `android/app/build/outputs/apk/debug/app-debug.apk`.

### From Android Studio

1. Open the `android/` directory as a project in Android Studio.
2. Let Gradle sync finish.
3. Click **Run** to build and deploy to a connected device or emulator.

## Game Assets

Place your Lua scripts and game assets (images, sounds, etc.) into:

```
android/app/src/main/assets/
```

At minimum you need a `main.lua` entry point. For example, copy one of the
existing examples:

```bash
cp -r ../examples/basic/* android/app/src/main/assets/
```

The Android app loads assets from the APK's asset bundle via the Android
AAssetManager, which sokol's filesystem abstraction handles automatically.

## How It Works

- The existing `CMakeLists.txt` detects `ANDROID` (set by the NDK toolchain)
  and builds `libspry.so` as a shared library instead of an executable.
- The `AndroidManifest.xml` uses `NativeActivity` which is sokol_app's
  entry point on Android.
- sokol_app handles the Android lifecycle, EGL context, input, etc.
- Miniaudio uses the AAudio backend on Android.

## Notes

- Signing: the debug build uses the default debug keystore. For release,
  configure signing in `app/build.gradle`.
- The `vfs_mount` path handling may need adjustment to load from Android
  assets instead of the filesystem. sokol_app provides
  `sapp_android_get_native_activity()` to access the `AAssetManager`.
