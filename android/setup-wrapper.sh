#!/usr/bin/env bash
# Download Gradle wrapper for Android builds.
# Usage: ./setup-wrapper.sh [gradle-version]
set -euo pipefail

GRADLE_VERSION="${1:-8.5}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WRAPPER_JAR="$SCRIPT_DIR/gradle/wrapper/gradle-wrapper.jar"
WRAPPER_PROPS="$SCRIPT_DIR/gradle/wrapper/gradle-wrapper.properties"

# Check if wrapper already exists
if [ -f "$WRAPPER_JAR" ] && [ -f "$WRAPPER_PROPS" ]; then
    echo "Gradle wrapper already present."
    exit 0
fi

# Write local.properties if SDK root is set
if [ -n "${ANDROID_SDK_ROOT:-}" ] || [ -n "${ANDROID_HOME:-}" ]; then
    SDK_DIR="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
    echo "sdk.dir=${SDK_DIR}" > "$SCRIPT_DIR/local.properties"
    echo "Wrote local.properties with SDK path."
else
    echo "ANDROID_SDK_ROOT or ANDROID_HOME not set; skipping local.properties."
fi

TEMP_DIR=$(mktemp -d)
ZIP_PATH="$TEMP_DIR/gradle-${GRADLE_VERSION}-bin.zip"
GRADLE_URL="https://services.gradle.org/distributions/gradle-${GRADLE_VERSION}-bin.zip"

echo "Downloading $GRADLE_URL"
curl -fsSL "$GRADLE_URL" -o "$ZIP_PATH"

unzip -q "$ZIP_PATH" -d "$TEMP_DIR"
GRADLE_HOME="$TEMP_DIR/gradle-${GRADLE_VERSION}"
GRADLE_BIN="$GRADLE_HOME/bin/gradle"

if [ ! -f "$GRADLE_BIN" ]; then
    echo "Gradle executable not found: $GRADLE_BIN"
    rm -rf "$TEMP_DIR"
    exit 1
fi

cd "$SCRIPT_DIR"
"$GRADLE_BIN" --no-daemon wrapper --gradle-version "$GRADLE_VERSION"

rm -rf "$TEMP_DIR"
echo "Gradle wrapper generated."
