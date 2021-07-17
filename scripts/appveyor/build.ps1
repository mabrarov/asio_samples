#
# Copyright (c) 2019 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

New-Item -Path "${env:BUILD_HOME}" -ItemType "directory" | out-null
Set-Location -Path "${env:BUILD_HOME}"

$generate_cmd = "cmake"
if (${env:TOOLCHAIN} -eq "mingw") {
  $generate_cmd = "${generate_cmd} -D CMAKE_SH=""CMAKE_SH-NOTFOUND"""
}
switch (${env:RUNTIME_LINKAGE}) {
  "static" {
    $generate_cmd = "${generate_cmd} -D CMAKE_USER_MAKE_RULES_OVERRIDE=""${env:APPVEYOR_BUILD_FOLDER}\cmake\static_c_runtime_overrides.cmake"" -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=""${env:APPVEYOR_BUILD_FOLDER}\cmake\static_cxx_runtime_overrides.cmake"""
    if ((Test-Path env:QT_VERSION) -and (${env:QT_VERSION} -match "5\.*") -and (${env:QT_LINKAGE} -eq "static")) {
      if (!(Test-Path env:ICU_ROOT)) {
        throw "ICU location is unspecified. Static Qt 5.x requires static ICU"
      }
      $generate_cmd = "${generate_cmd} -D ICU_ROOT=""${env:ICU_ROOT}"""
    }
  }
  default {
    $generate_cmd = "${generate_cmd} -D gtest_force_shared_crt=ON"
  }
}
$generate_cmd = "${generate_cmd} -D BOOST_INCLUDEDIR=""${env:BOOST_INCLUDE_FOLDER}"" -D BOOST_LIBRARYDIR=""${env:BOOST_LIBRARY_FOLDER}"" -D Boost_USE_STATIC_LIBS=${env:BOOST_USE_STATIC_LIBS} -D Boost_NO_SYSTEM_PATHS=ON"
if ((${env:TOOLCHAIN} -eq "mingw") -and (Test-Path env:BOOST_ARCHITECTURE)) {
  $generate_cmd = "${generate_cmd} -D Boost_ARCHITECTURE=""${env:BOOST_ARCHITECTURE}"""
}
if (!(Test-Path env:QT_VERSION)) {
  $generate_cmd = "${generate_cmd} -D MA_QT=OFF"
} elseif (${env:QT_VERSION} -match "5\.*") {
  $generate_cmd = "${generate_cmd} -D Qt5Core_DIR=""${env:QT_HOME}\lib\cmake\Qt5Core"" -D Qt5Gui_DIR=""${env:QT_HOME}\lib\cmake\Qt5Gui"" -D Qt5Widgets_DIR=""${env:QT_HOME}\lib\cmake\Qt5Widgets"" -D MA_QT_MAJOR_VERSION=5"
} elseif (${env:QT_VERSION} -match "4\.*") {
  $generate_cmd = "${generate_cmd} -D QT_QMAKE_EXECUTABLE=""${env:QT_QMAKE_EXECUTABLE}"" -D MA_QT_MAJOR_VERSION=4"
} else {
  throw "Unsupported version of Qt: ${env:QT_VERSION}"
}
if (${env:TOOLCHAIN} -eq "mingw") {
  $generate_cmd = "${generate_cmd} -D CMAKE_BUILD_TYPE=""${env:CONFIGURATION}"""
}
$generate_cmd = "${generate_cmd} -G ""${env:CMAKE_GENERATOR}"""
if (${env:CMAKE_GENERATOR_PLATFORM}) {
  $generate_cmd = "${generate_cmd} -A ""${env:CMAKE_GENERATOR_PLATFORM}"""
}
$generate_cmd = "${generate_cmd} ""${env:APPVEYOR_BUILD_FOLDER}"""
Write-Host "CMake project generation command: ${generate_cmd}"

Invoke-Expression "${generate_cmd}"
if (${LastExitCode} -ne 0) {
  throw "Generation of project failed with exit code ${LastExitCode}"
}

$build_cmd = "cmake --build . --config ""${env:CONFIGURATION}"""
if ((${env:TOOLCHAIN} -eq "msvc") -and (${env:MSVC_VERSION} -ne "9.0")) {
  $build_cmd = "${build_cmd} -- /maxcpucount /verbosity:normal /logger:""C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"""
}
if (${env:COVERITY_SCAN_BUILD} -eq "True") {
  $build_cmd = "cov-build.exe --dir cov-int ${build_cmd}"
}
Write-Host "CMake project build command: ${build_cmd}"

Invoke-Expression "${build_cmd}"
if (${LastExitCode} -ne 0) {
  throw "Build failed with exit code ${LastExitCode}"
}

if (${env:COVERITY_SCAN_BUILD} -eq "True") {
  # Compress results.
  Write-Host "Compressing Coverity Scan results..."
  7z.exe a -tzip "${env:BUILD_HOME}\${env:APPVEYOR_PROJECT_NAME}.zip" "${env:BUILD_HOME}\cov-int" -aoa -y | out-null
  if (${LastExitCode} -ne 0) {
    throw "Failed to zip Coverity Scan results with exit code ${LastExitCode}"
  }
  # Upload results to Coverity server.
  $coverity_build_version = ${env:APPVEYOR_REPO_COMMIT}.Substring(0, 7)
  Write-Host "Uploading Coverity Scan results (version: ${coverity_build_version})..."
  curl.exe `
    --connect-timeout "${env:CURL_CONNECT_TIMEOUT}" `
    --max-time "${env:CURL_MAX_TIME}" `
    --retry "${env:CURL_RETRY}" `
    --retry-delay "${env:CURL_RETRY_DELAY}" `
    --show-error --silent --location `
    --form token="${env:COVERITY_SCAN_TOKEN}" `
    --form email="${env:COVERITY_SCAN_NOTIFICATION_EMAIL}" `
    --form file=@"${env:BUILD_HOME}\${env:APPVEYOR_PROJECT_NAME}.zip" `
    --form version="${coverity_build_version}" `
    --form description="Build submitted via AppVeyor CI" `
    "https://scan.coverity.com/builds?project=${env:APPVEYOR_REPO_NAME}"
  if (${LastExitCode} -ne 0) {
    throw "Failed to upload Coverity Scan results with exit code ${LastExitCode}"
  }
}

Set-Location -Path "${env:APPVEYOR_BUILD_FOLDER}"
