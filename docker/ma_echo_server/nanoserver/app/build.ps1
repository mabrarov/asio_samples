#
# Copyright (c) 2019 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

# Stop immediately if any error happens
$ErrorActionPreference = "Stop"

# Enable all versions of TLS
[System.Net.ServicePointManager]::SecurityProtocol = @("Tls12","Tls11","Tls","Ssl3")

$cmake_dist_base_name = "cmake-${env:CMAKE_VERSION}-win64-x64"
$cmake_dist_name = "${cmake_dist_base_name}.zip"
$cmake_dist = "${env:TMP}\${cmake_dist_name}"
$cmake_url = "${env:CMAKE_URL}/v${env:CMAKE_VERSION}/${cmake_dist_name}"
Write-Host "Downloading CMake from ${cmake_url} into ${cmake_dist}"
(New-Object System.Net.WebClient).DownloadFile("${cmake_url}", "${cmake_dist}")
$cmake_temp_dir = "${env:TMP}\${cmake_dist_base_name}"
Write-Host "Extracting CMake from ${cmake_dist} into ${cmake_temp_dir}"
& "${env:SEVEN_ZIP_HOME}\7z.exe" x "${cmake_dist}" -o"${env:TMP}" -aoa -y -bd | out-null
if (Test-Path -Path "${env:CMAKE_HOME}") {
  Remove-Item -Path "${env:CMAKE_HOME}" -Recurse -Force
}
Write-Host "Moving CMake from ${cmake_temp_dir} into ${env:CMAKE_HOME}"
[System.IO.Directory]::Move("${cmake_temp_dir}", "${env:CMAKE_HOME}")
Write-Host "CMake ${env:CMAKE_VERSION} installed into ${env:CMAKE_HOME}"

$boost_version_suffix = "-${env:BOOST_VERSION}"
$boost_platform_suffix = "-x64"
$boost_toolchain_suffix = ""
switch (${env:MSVS_VERSION}) {
  "16" {
    $boost_toolchain_suffix = "-vs2019"
  }
  "15" {
    $boost_toolchain_suffix = "-vs2017"
  }
  "14" {
    $boost_toolchain_suffix = "-vs2015"
  }
  default {
    throw "Unsupported MSVC version for Boost: ${env:MSVS_VERSION}"
  }
}
$boost_install_folder = "${env:DEPENDENCIES_DIR}\boost${boost_version_suffix}${boost_platform_suffix}${boost_toolchain_suffix}"
if (!(Test-Path -Path "${boost_install_folder}")) {
  $boost_archive_name = "boost${boost_version_suffix}${boost_platform_suffix}${boost_toolchain_suffix}.7z"
  $boost_archive_file = "${env:DOWNLOADS_DIR}\${boost_archive_name}"
  if (!(Test-Path -Path "${boost_archive_file}")) {
    $boost_download_url = "${env:BOOST_URL}/${env:BOOST_VERSION}/${boost_archive_name}"
    if (!(Test-Path -Path "${env:DOWNLOADS_DIR}")) {
      New-Item -Path "${env:DOWNLOADS_DIR}" -ItemType "directory" | out-null
    }
    Write-Host "Downloading Boost from ${boost_download_url} to ${boost_archive_file}"
    (New-Object System.Net.WebClient).DownloadFile("${boost_download_url}", "${boost_archive_file}")
    Write-Host "Downloading of Boost completed successfully"
  }
  Write-Host "Extracting Boost from ${boost_archive_file} to ${env:DEPENDENCIES_DIR}"
  if (!(Test-Path -Path "${env:DEPENDENCIES_DIR}")) {
    New-Item -Path "${env:DEPENDENCIES_DIR}" -ItemType "directory" | out-null
  }
  & "${env:SEVEN_ZIP_HOME}\7z.exe" x "${boost_archive_file}" -o"${env:DEPENDENCIES_DIR}" -aoa -y -bd | out-null
  if (${LastExitCode} -ne 0) {
    throw "Extracting of Boost failed with exit code ${LastExitCode}"
  }
  Write-Host "Extracting of Boost completed successfully"
}
Write-Host "Boost ${env:BOOST_VERSION} is located at ${boost_install_folder}"
$boost_include_folder_version_suffix = "-${env:BOOST_VERSION}" -replace "([\d]+)\.([\d]+)(\.[\d]+)*", '$1_$2'
$env:BOOST_INCLUDE_DIR = "${boost_install_folder}\include\boost${boost_include_folder_version_suffix}"
$env:BOOST_LIBRARY_DIR = "${boost_install_folder}\lib"

switch (${env:MSVS_VERSION}) {
  "16" {
    $env:MSVS_INSTALL_DIR = &vswhere --% -latest -products Microsoft.VisualStudio.Product.Community -version [16.0,17.0) -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath
    $env:MSVC_AUXILARY_DIR = "${env:MSVS_INSTALL_DIR}\VC\Auxiliary"
    $env:MSVC_BUILD_DIR = "${env:MSVC_AUXILARY_DIR}\Build"
    $env:MSVC_CMD_BOOTSTRAP = "vcvars64.bat"
    $env:MSVC_CMD_BOOTSTRAP_OPTIONS = ""
    $env:CMAKE_GENERATOR = "Visual Studio 16 2019"
  }
  "15" {
    $env:MSVS_INSTALL_DIR = &vswhere --% -latest -products Microsoft.VisualStudio.Product.Community -version [15.0,16.0) -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath
    $env:MSVC_AUXILARY_DIR = "${env:MSVS_INSTALL_DIR}\VC\Auxiliary"
    $env:MSVC_BUILD_DIR = "${env:MSVC_AUXILARY_DIR}\Build"
    $env:MSVC_CMD_BOOTSTRAP = "vcvars64.bat"
    $env:MSVC_CMD_BOOTSTRAP_OPTIONS = ""
    $env:CMAKE_GENERATOR = "Visual Studio 15 2017"
  }
  "14" {
    $env:MSVS_INSTALL_DIR = &vswhere --% -legacy -latest -version [14.0,15.0) -property installationPath
    $env:MSVC_BUILD_DIR = "${env:MSVS_INSTALL_DIR}VC"
    $env:MSVC_CMD_BOOTSTRAP = "vcvarsall.bat"
    $env:MSVC_CMD_BOOTSTRAP_OPTIONS = " amd64"
    $env:CMAKE_GENERATOR = "Visual Studio 14 2015"
  }
  default {
    throw "Unsupported MSVC version for Boost: ${env:MSVS_VERSION}"
  }
}

$env:CMAKE_GENERATOR_PLATFORM = "x64"

if (!(Test-Path -Path "${env:SOURCE_DIR}")) {
  New-Item -Path "${env:SOURCE_DIR}" -ItemType "directory" | out-null
}
git clone https://github.com/mabrarov/asio_samples.git "${env:SOURCE_DIR}"
Set-Location -Path "${env:SOURCE_DIR}"
git checkout "${env:MA_REVISION}"

if (!(Test-Path -Path "${env:BUILD_DIR}")) {
  New-Item -Path "${env:BUILD_DIR}" -ItemType "directory" | out-null
}
Set-Location -Path "${env:BUILD_DIR}"

& "${PSScriptRoot}\build.bat"
if (${LastExitCode} -ne 0) {
  throw "Failed to build"
}
