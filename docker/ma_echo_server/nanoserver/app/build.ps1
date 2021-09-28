#
# Copyright (c) 2019 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

# Stop immediately if any error happens
$ErrorActionPreference = "Stop"

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
