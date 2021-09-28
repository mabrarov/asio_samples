#
# Copyright (c) 2021 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

# Stop immediately if any error happens
$ErrorActionPreference = "Stop"

# Enable all versions of TLS
[System.Net.ServicePointManager]::SecurityProtocol = @("Tls12","Tls11","Tls","Ssl3")

$boost_version_suffix = "${env:BOOST_VERSION}" -replace "\.", '_'
$boost_platform_suffix = "-64"
$boost_toolchain_suffix = "-msvc"
switch (${env:MSVS_VERSION}) {
  "16" {
    $boost_toolchain_suffix = "${boost_toolchain_suffix}-14.2"
  }
  "15" {
    $boost_toolchain_suffix = "${boost_toolchain_suffix}-14.1"
  }
  "14" {
    $boost_toolchain_suffix = "${boost_toolchain_suffix}-14.0"
  }
  default {
    throw "Unsupported MSVC version for Boost: ${env:MSVS_VERSION}"
  }
}
$boost_installer_file_name = "boost_${boost_version_suffix}${boost_toolchain_suffix}${boost_platform_suffix}.exe"
$boost_installer_file = "${env:DOWNLOADS_DIR}\${boost_installer_file_name}"

Add-Type -AssemblyName System.Web
$boost_download_url = "${env:BOOST_URL}" + [System.Web.HttpUtility]::UrlEncode([System.Web.HttpUtility]::UrlEncode("release/${env:BOOST_VERSION}/binaries/${boost_installer_file_name}"))
if (-not (Test-Path -Path "${env:DOWNLOADS_DIR}")) {
  New-Item -Path "${env:DOWNLOADS_DIR}" -ItemType "directory" | out-null
}
Write-Host "Downloading Boost from ${boost_download_url} to ${boost_installer_file}"
(New-Object System.Net.WebClient).DownloadFile("${boost_download_url}", "${boost_installer_file}")

$boost_parent_dir = Split-Path "${env:BOOST_DIR}" -Parent
if (-not (Test-Path -Path "${boost_parent_dir}")) {
  Write-Host "Creating ${boost_parent_dir} directory"
  New-Item -Path "${boost_parent_dir}" -ItemType "directory" | out-null
}
Write-Host "Installing Boost from ${boost_installer_file} to ${env:BOOST_DIR}"
$p = Start-Process -FilePath "${boost_installer_file}" `
  -ArgumentList ("/SP-", "/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/NOICONS", "/ALLUSERS", "/DIR=""${env:BOOST_DIR}""") `
  -Wait -PassThru
if (${p}.ExitCode -ne 0) {
  throw "Failed to install Boost"
}

$env:BOOST_INCLUDE_DIR = "${env:BOOST_DIR}"
$env:BOOST_LIBRARY_DIR = "${env:BOOST_DIR}\lib64${boost_toolchain_suffix}"

[Environment]::SetEnvironmentVariable("BOOST_INCLUDE_DIR", "${env:BOOST_INCLUDE_DIR}", [System.EnvironmentVariableTarget]::Machine)
[Environment]::SetEnvironmentVariable("BOOST_LIBRARY_DIR", "${env:BOOST_LIBRARY_DIR}", [System.EnvironmentVariableTarget]::Machine)
