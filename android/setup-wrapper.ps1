param(
    [string]$GradleVersion = "8.5"
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$wrapperJar = Join-Path $scriptDir "gradle\wrapper\gradle-wrapper.jar"
$wrapperProps = Join-Path $scriptDir "gradle\wrapper\gradle-wrapper.properties"

# Check if wrapper already exists (both jar and properties file must exist)
if ((Test-Path $wrapperJar) -and (Test-Path $wrapperProps)) {
    Write-Host "Gradle wrapper already present."
    exit 0
}

$sdkRoot = $env:ANDROID_SDK_ROOT
if (-not $sdkRoot) {
    $sdkRoot = $env:ANDROID_HOME
}

if ($sdkRoot) {
    $sdkDir = $sdkRoot -replace "\\", "/"
    $localProps = Join-Path $scriptDir "local.properties"
    $lines = @("sdk.dir=$sdkDir")
    # ndk.dir is deprecated; use android.ndkVersion in build.gradle instead
    Set-Content -Path $localProps -Value $lines -Encoding ASCII
    Write-Host "Wrote local.properties with SDK path."
} else {
    Write-Host "ANDROID_SDK_ROOT or ANDROID_HOME not set; skipping local.properties."
}

$tempDir = Join-Path $env:TEMP ("gradle-wrapper-" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempDir | Out-Null

$zipPath = Join-Path $tempDir ("gradle-" + $GradleVersion + "-bin.zip")
$gradleUrl = "https://services.gradle.org/distributions/gradle-$GradleVersion-bin.zip"

Write-Host "Downloading $gradleUrl"
Invoke-WebRequest -Uri $gradleUrl -OutFile $zipPath

Expand-Archive -Path $zipPath -DestinationPath $tempDir
$gradleHome = Join-Path $tempDir ("gradle-" + $GradleVersion)
$gradleBat = Join-Path $gradleHome "bin\gradle.bat"

if (-not (Test-Path $gradleBat)) {
    throw "Gradle executable not found: $gradleBat"
}

Push-Location $scriptDir
try {
    & $gradleBat --no-daemon wrapper --gradle-version $GradleVersion
} finally {
    Pop-Location
}

try {
    Remove-Item -Recurse -Force $tempDir
} catch {
    Write-Host "Warning: failed to remove temp dir $tempDir (in use)."
}
Write-Host "Gradle wrapper generated."
