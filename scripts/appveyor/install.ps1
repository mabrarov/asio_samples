#
# Copyright (c) 2019 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

$env:OS_VERSION = (Get-WmiObject win32_operatingsystem).version

$vswhere_home = "${env:DEPENDENCIES_FOLDER}\vswhere-${env:VSWHERE_VERSION}"
$vswhere_executable = "${vswhere_home}\${env:VSWHERE_DIST_NAME}"
if (!(Test-Path -Path "${vswhere_executable}")) {
  Write-Host "Visual Studio Locator ${env:VSWHERE_VERSION} not found at ${vswhere_home}"
  $vswhere_archive_file = "${env:DOWNLOADS_FOLDER}\${env:VSWHERE_DIST_NAME}"
  $vswhere_download_url = "${env:VSWHERE_URL}/${env:VSWHERE_VERSION}/${env:VSWHERE_DIST_NAME}"
  if (!(Test-Path -Path "${vswhere_archive_file}")) {
    Write-Host "Downloading Visual Studio Locator ${env:VSWHERE_VERSION} from ${vswhere_download_url} to ${vswhere_archive_file}"
    if (!(Test-Path -Path "${env:DOWNLOADS_FOLDER}")) {
      New-Item -Path "${env:DOWNLOADS_FOLDER}" -ItemType "directory" | out-null
    }
    curl.exe `
      --connect-timeout "${env:CURL_CONNECT_TIMEOUT}" `
      --max-time "${env:CURL_MAX_TIME}" `
      --retry "${env:CURL_RETRY}" `
      --retry-delay "${env:CURL_RETRY_DELAY}" `
      --show-error --silent --location `
      --output "${vswhere_archive_file}" `
      "${vswhere_download_url}"
    if (${LastExitCode} -ne 0) {
      throw "Downloading of Visual Studio Locator failed with exit code ${LastExitCode}"
    }
    Write-Host "Downloading of Visual Studio Locator completed successfully"
  }
  if (!(Test-Path -Path "${vswhere_home}")) {
    New-Item -Path "${vswhere_home}" -ItemType "directory" | out-null
  }
  Write-Host "Copying Visual Studio Locator ${env:VSWHERE_VERSION} from ${vswhere_archive_file} to ${vswhere_home}"
  Copy-Item -Path "${vswhere_archive_file}" -Destination "${vswhere_home}" -Force
  Write-Host "Copying of Visual Studio Locator completed successfully"
}

$toolchain_id = ""
switch (${env:TOOLCHAIN}) {
  "msvc" {
    $env:TOOLCHAIN_ID = "${env:TOOLCHAIN}-${env:MSVC_VERSION}"
    $env:WINDOWS_SDK_ENV_BATCH_FILE = ""
    $env:WINDOWS_SDK_ENV_PARAMETERS = ""
    $env:MSVS_HOME = "${env:ProgramFiles(x86)}\Microsoft Visual Studio ${env:MSVC_VERSION}"
    $env:MSVC_VARS_BATCH_FILE = "${env:MSVS_HOME}\VC\vcvarsall.bat"
    $env:MSVS_PATCH_FOLDER = ""
    $env:MSVS_PATCH_BATCH_FILE = ""
    $msvs_install_dir = ""
    switch (${env:MSVC_VERSION}) {
      "14.2" {
        $msvs_install_dir = &"${vswhere_executable}" --% -latest -products Microsoft.VisualStudio.Product.Community -version [16.0,17.0) -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath
      }
      "14.1" {
        $msvs_install_dir = &"${vswhere_executable}" --% -latest -products Microsoft.VisualStudio.Product.Community -version [15.0,16.0) -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath
      }
    }
    switch (${env:PLATFORM}) {
      "Win32" {
        switch (${env:MSVC_VERSION}) {
          "14.2" {
            $env:MSVC_VARS_BATCH_FILE = "${msvs_install_dir}\VC\Auxiliary\Build\vcvars32.bat"
            $env:MSVC_VARS_PLATFORM = ""
          }
          "14.1" {
            $env:MSVC_VARS_BATCH_FILE = "${msvs_install_dir}\VC\Auxiliary\Build\vcvars32.bat"
            $env:MSVC_VARS_PLATFORM = ""
          }
          default {
            $env:MSVC_VARS_PLATFORM = "x86"
          }
        }
      }
      "x64" {
        switch (${env:MSVC_VERSION}) {
          "14.2" {
            $env:MSVC_VARS_BATCH_FILE = "${msvs_install_dir}\VC\Auxiliary\Build\vcvars64.bat"
            $env:MSVC_VARS_PLATFORM = ""
          }
          "14.1" {
            $env:MSVC_VARS_BATCH_FILE = "${msvs_install_dir}\VC\Auxiliary\Build\vcvars64.bat"
            $env:MSVC_VARS_PLATFORM = ""
          }
          "14.0" {
            $env:MSVC_VARS_PLATFORM = "amd64"
          }
          "12.0" {
            $env:MSVC_VARS_PLATFORM = "amd64"
          }
          "11.0" {
            $env:MSVC_VARS_PLATFORM = "x86_amd64"
          }
          "10.0" {
            $env:MSVC_VARS_BATCH_FILE = ""
            $env:WINDOWS_SDK_ENV_BATCH_FILE = "${env:ProgramFiles}\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd"
            $env:WINDOWS_SDK_ENV_PARAMETERS = "/x64 /${env:CONFIGURATION}"
          }
          "9.0" {
            Write-Host "Chosen MSVS version (${env:MSVC_VERSION}) and platform (${env:PLATFORM}) require patching"
            $env:MSVS_PATCH_FOLDER = "${env:DEPENDENCIES_FOLDER}\vs2008_patch"
            if (!(Test-Path -Path "${env:MSVS_PATCH_FOLDER}")) {
              $msvs_patch_download_folder = "${env:DOWNLOADS_FOLDER}\condaci"
              $msvs_patch_download_file = "${msvs_patch_download_folder}\vs2008_patch.zip"
              if (!(Test-Path -Path "${msvs_patch_download_file}")) {
                $msvs2008_patch_url = "https://github.com/menpo/condaci.git"
                $msvs2008_patch_revision = "98d76b2089d494433ac2e3abd920123088a95a6d"
                $msvs_patch_download_folder_nix = "${msvs_patch_download_folder}" -replace "\\", "/"
                Write-Host "Downloading MSVS patch from ${msvs2008_patch_url} to ${msvs_patch_download_folder_nix}"
                git clone --quiet "${msvs2008_patch_url}" "${msvs_patch_download_folder_nix}"
                git -C "${msvs_patch_download_folder_nix}" checkout --quiet "${msvs2008_patch_revision}"
                if (${LastExitCode} -ne 0) {
                  throw "Downloading of MSVS patch failed with exit code ${LastExitCode}"
                }
                Write-Host "Downloading of MSVS patch completed successfully"
              }
              Write-Host "Extracting MSVS patch from ${msvs_patch_download_file} to ${env:MSVS_PATCH_FOLDER}"
              New-Item -Path "${env:MSVS_PATCH_FOLDER}" -ItemType "directory" | out-null
              7z.exe x "${msvs_patch_download_file}" -o"${env:MSVS_PATCH_FOLDER}" -aoa -y | out-null
              if (${LastExitCode} -ne 0) {
                throw "Extracting MSVS patch failed with exit code ${LastExitCode}"
              }
              Write-Host "Extracting of MSVS patch completed successfully"
            }
            Write-Host "MSVS patch is located at ${env:MSVS_PATCH_FOLDER}"
            $env:MSVS_PATCH_BATCH_FILE = "${env:MSVS_PATCH_FOLDER}\setup_x64.bat"
            $env:MSVC_VARS_PLATFORM = "x86_amd64"
          }
          default {
            throw "Unsupported ${env:TOOLCHAIN} version: ${env:MSVC_VERSION}"
          }
        }
      }
      default {
        throw "Unsupported platform for ${env:TOOLCHAIN} toolchain: ${env:PLATFORM}"
      }
    }
    $env:ARTIFACT_PATH_SUFFIX = "\${env:CONFIGURATION}\"
  }
  "mingw" {
    $env:TOOLCHAIN_ID = "${env:TOOLCHAIN}-${env:MINGW_VERSION}"
    $mingw_platform_suffix = ""
    $mingw_thread_model_suffix = ""
    switch (${env:PLATFORM}) {
      "Win32" {
        $mingw_platform_suffix = "i686-"
        $mingw_exception_suffix = "-dwarf"
        $mingw_platform_postfix = "mingw32"
      }
      "x64" {
        $mingw_platform_suffix = "x86_64-"
        $mingw_exception_suffix = "-seh"
        $mingw_platform_postfix = "mingw64"
      }
      default {
        throw "Unsupported platform for ${env:TOOLCHAIN} toolchain: ${env:PLATFORM}"
      }
    }
    $env:MINGW_HOME = "C:\mingw-w64\${mingw_platform_suffix}${env:MINGW_VERSION}-posix${mingw_exception_suffix}-rt_v${env:MINGW_RT_VERSION}-rev${env:MINGW_REVISION}\${mingw_platform_postfix}"
    $env:ARTIFACT_PATH_SUFFIX = "\"
  }
  default {
    throw "Unsupported toolchain: ${env:TOOLCHAIN}"
  }
}
try {
  $detected_cmake_version = (cmake --version 2> $null `
    | Where-Object {$_ -match 'cmake version ([0-9]+\.[0-9]+\.[0-9]+)'}) `
    -replace "cmake version ([0-9]+\.[0-9]+\.[0-9]+)", '$1'
} catch {
  $detected_cmake_version = ""
}
Write-Host "Detected CMake of ${detected_cmake_version} version"
if (Test-Path env:CMAKE_VERSION) {
  Write-Host "CMake of ${env:CMAKE_VERSION} version is requested"
  if ([System.Version] "${env:CMAKE_VERSION}" -ne [System.Version] ${detected_cmake_version}) {
    if ([System.Version] "${env:CMAKE_VERSION}" -ge [System.Version] "3.20.0") {
      $cmake_archive_base_name = "cmake-${env:CMAKE_VERSION}-windows-x86_64"
    } else {
      if ([System.Version] "${env:CMAKE_VERSION}" -ge [System.Version] "3.6.0") {
        $cmake_archive_base_name = "cmake-${env:CMAKE_VERSION}-win64-x64"
      } else {
        # CMake x64 binary is not available for CMake version < 3.6.0
        $cmake_archive_base_name = "cmake-${env:CMAKE_VERSION}-win32-x86"
      }
    }
    $cmake_home = "${env:DEPENDENCIES_FOLDER}\${cmake_archive_base_name}"
    if (!(Test-Path -Path "${cmake_home}")) {
      Write-Host "CMake ${env:CMAKE_VERSION} not found at ${cmake_home}"
      $cmake_archive_name = "${cmake_archive_base_name}.zip"
      $cmake_archive_file = "${env:DOWNLOADS_FOLDER}\${cmake_archive_name}"
      $cmake_download_url = "https://github.com/Kitware/CMake/releases/download/v${env:CMAKE_VERSION}/${cmake_archive_name}"
      if (!(Test-Path -Path "${cmake_archive_file}")) {
        Write-Host "Downloading CMake ${env:CMAKE_VERSION} archive from ${cmake_download_url} to ${cmake_archive_file}"
        if (!(Test-Path -Path "${env:DOWNLOADS_FOLDER}")) {
          New-Item -Path "${env:DOWNLOADS_FOLDER}" -ItemType "directory" | out-null
        }
        curl.exe `
          --connect-timeout "${env:CURL_CONNECT_TIMEOUT}" `
          --max-time "${env:CURL_MAX_TIME}" `
          --retry "${env:CURL_RETRY}" `
          --retry-delay "${env:CURL_RETRY_DELAY}" `
          --show-error --silent --location `
          --output "${cmake_archive_file}" `
          "${cmake_download_url}"
        if (${LastExitCode} -ne 0) {
          throw "Downloading of CMake failed with exit code ${LastExitCode}"
        }
        Write-Host "Downloading of CMake completed successfully"
      }
      if (!(Test-Path -Path "${env:DEPENDENCIES_FOLDER}")) {
        New-Item -Path "${env:DEPENDENCIES_FOLDER}" -ItemType "directory" | out-null
      }
      Write-Host "Extracting CMake ${env:CMAKE_VERSION} from ${cmake_archive_file} to ${env:DEPENDENCIES_FOLDER}"
      7z.exe x "${cmake_archive_file}" -o"${env:DEPENDENCIES_FOLDER}" -aoa -y | out-null
      if (${LastExitCode} -ne 0) {
        throw "Extracting CMake failed with exit code ${LastExitCode}"
      }
      Write-Host "Extracting of CMake completed successfully"
    }
    Write-Host "CMake ${env:CMAKE_VERSION} is located at ${cmake_home}"
    $env:PATH = "${cmake_home}\bin;${env:PATH}"
  }
}
$pre_installed_icu = $false
if (Test-Path env:ICU_VERSION) {
  if (${pre_installed_icu}) {
    throw "Installed ICU is missing"
  } else {
    $icu_platform_suffix = ""
    switch (${env:PLATFORM}) {
      "Win32" {
        $icu_platform_suffix = "-x86"
      }
      "x64" {
        $icu_platform_suffix = "-x64"
      }
      default {
        throw "Unsupported platform for ICU: ${env:PLATFORM}"
      }
    }
    $icu_version_suffix = "-${env:ICU_VERSION}"
    $icu_toolchain_suffix = ""
    switch (${env:TOOLCHAIN}) {
      "msvc" {
        switch (${env:MSVC_VERSION}) {
          "14.2" {
            $icu_toolchain_suffix = "-vs2019"
          }
          "14.1" {
            $icu_toolchain_suffix = "-vs2017"
          }
          "14.0" {
            $icu_toolchain_suffix = "-vs2015"
          }
          "12.0" {
            $icu_toolchain_suffix = "-vs2013"
          }
          default {
            throw "Unsupported ${env:TOOLCHAIN} version for ICU: ${env:MSVC_VERSION}"
          }
        }
      }
      "mingw" {
        $icu_toolchain_suffix = "-mingw${env:MINGW_VERSION}" -replace "([\d]+)\.([\d]+)(\.([\d]+))*", '$1$2'
      }
      default {
        throw "Unsupported toolchain for ICU: ${env:TOOLCHAIN}"
      }
    }
    $icu_linkage_suffix = ""
    switch (${env:ICU_LINKAGE}) {
      "static" {
        $icu_linkage_suffix = "-static"
      }
      default {
        $icu_linkage_suffix = "-shared"
      }
    }
    $icu_install_folder = "${env:DEPENDENCIES_FOLDER}\icu4c${icu_version_suffix}${icu_platform_suffix}${icu_toolchain_suffix}${icu_linkage_suffix}"
    if (!(Test-Path -Path "${icu_install_folder}")) {
      Write-Host "ICU is absent for the chosen toolchain (${env:TOOLCHAIN_ID}) and ICU version (${env:ICU_VERSION})"
      $icu_archive_name = "icu4c${icu_version_suffix}${icu_platform_suffix}${icu_toolchain_suffix}${icu_linkage_suffix}.7z"
      $icu_archive_file = "${env:DOWNLOADS_FOLDER}\${icu_archive_name}"
      if (!(Test-Path -Path "${icu_archive_file}")) {
        $icu_download_url = "https://dl.bintray.com/mabrarov/generic/icu/${env:ICU_VERSION}/${icu_archive_name}"
        if (!(Test-Path -Path "${env:DOWNLOADS_FOLDER}")) {
          New-Item -Path "${env:DOWNLOADS_FOLDER}" -ItemType "directory" | out-null
        }
        Write-Host "Downloading ICU from ${icu_download_url} to ${icu_archive_file}"
        curl.exe `
          --connect-timeout "${env:CURL_CONNECT_TIMEOUT}" `
          --max-time "${env:CURL_MAX_TIME}" `
          --retry "${env:CURL_RETRY}" `
          --retry-delay "${env:CURL_RETRY_DELAY}" `
          --show-error --silent --location `
          --output "${icu_archive_file}" `
          "${icu_download_url}"
        if (${LastExitCode} -ne 0) {
          throw "Downloading of ICU failed with exit code ${LastExitCode}"
        }
        Write-Host "Downloading of ICU completed successfully"
      }
      Write-Host "Extracting ICU from ${icu_archive_file} to ${env:DEPENDENCIES_FOLDER}"
      if (!(Test-Path -Path "${env:DEPENDENCIES_FOLDER}")) {
        New-Item -Path "${env:DEPENDENCIES_FOLDER}" -ItemType "directory" | out-null
      }
      7z.exe x "${icu_archive_file}" -o"${env:DEPENDENCIES_FOLDER}" -aoa -y | out-null
      if (${LastExitCode} -ne 0) {
        throw "Extracting ICU failed with exit code ${LastExitCode}"
      }
      Write-Host "Extracting of ICU completed successfully"
    }
    Write-Host "ICU ${env:ICU_VERSION} is located at ${icu_install_folder}"
    $env:ICU_ROOT = "${icu_install_folder}"
    $env:ICU_LIBRARY_FOLDER = "${icu_install_folder}\lib"
  }
  if ((${env:RUNTIME_LINKAGE} -eq "static") -and (${env:ICU_LINKAGE} -ne "static")) {
    throw "Incompatible type of linkage of ICU: ${env:ICU_LINKAGE} for the specified type of linkage of C/C++ runtime: ${env:RUNTIME_LINKAGE}"
  }
}
if (Test-Path env:BOOST_VERSION) {
  $pre_installed_boost = $false
  switch (${env:TOOLCHAIN}) {
    "msvc" {
      switch (${env:MSVC_VERSION}) {
        "14.2" {
          $pre_installed_boost = (${env:BOOST_VERSION} -eq "1.71.0") `
            -or (${env:BOOST_VERSION} -eq "1.73.0") `
        }
        "14.1" {
          $pre_installed_boost = (${env:BOOST_VERSION} -eq "1.69.0") `
            -or (${env:BOOST_VERSION} -eq "1.67.0") `
            -or (${env:BOOST_VERSION} -eq "1.66.0") `
            -or (${env:BOOST_VERSION} -eq "1.65.1") `
            -or (${env:BOOST_VERSION} -eq "1.64.0")
        }
        "14.0" {
          $pre_installed_boost = (${env:BOOST_VERSION} -eq "1.69.0") `
            -or (${env:BOOST_VERSION} -eq "1.67.0") `
            -or (${env:BOOST_VERSION} -eq "1.66.0") `
            -or (${env:BOOST_VERSION} -eq "1.65.1") `
            -or (${env:BOOST_VERSION} -eq "1.63.0") `
            -or (${env:BOOST_VERSION} -eq "1.62.0") `
            -or (${env:BOOST_VERSION} -eq "1.60.0")
        }
        "12.0" {
          $pre_installed_boost = (${env:BOOST_VERSION} -eq "1.58.0")
        }
      }
    }
  }
  if (${pre_installed_boost}) {
    $boost_home = "C:\Libraries\boost"
    if (!(${env:BOOST_VERSION} -eq "1.56.0")) {
      $boost_home_version_suffix = "_${env:BOOST_VERSION}" -replace "\.", '_'
      $boost_home = "${boost_home}${boost_home_version_suffix}"
    }
    $boost_folder_platform_suffix = ""
    switch (${env:PLATFORM}) {
      "Win32" {
        $boost_folder_platform_suffix = "lib32"
      }
      "x64" {
        $boost_folder_platform_suffix = "lib64"
      }
      default {
        throw "Unsupported platform for Boost: ${env:PLATFORM}"
      }
    }
    $boost_folder_toolchain_suffix = "-msvc-${env:MSVC_VERSION}"
    $env:BOOST_INCLUDE_FOLDER = "${boost_home}"
    $env:BOOST_LIBRARY_FOLDER = "${boost_home}\${boost_folder_platform_suffix}${boost_folder_toolchain_suffix}"
  } else {
    switch (${env:PLATFORM}) {
      "Win32" {
        $env:BOOST_PLATFORM_SUFFIX = "-x86"
        $env:BOOST_ARCHITECTURE = "-x32"
      }
      "x64" {
        $env:BOOST_PLATFORM_SUFFIX = "-x64"
        $env:BOOST_ARCHITECTURE = "-x64"
      }
      default {
        throw "Unsupported platform for Boost: ${env:PLATFORM}"
      }
    }
    $boost_version_suffix = "-${env:BOOST_VERSION}"
    $boost_toolchain_suffix = ""
    switch (${env:TOOLCHAIN}) {
      "msvc" {
        switch (${env:MSVC_VERSION}) {
          "14.2" {
            $boost_toolchain_suffix = "-vs2019"
          }
          "14.1" {
            $boost_toolchain_suffix = "-vs2017"
          }
          "14.0" {
            $boost_toolchain_suffix = "-vs2015"
          }
          "12.0" {
            $boost_toolchain_suffix = "-vs2013"
          }
          "11.0" {
            $boost_toolchain_suffix = "-vs2012"
          }
          "10.0" {
            $boost_toolchain_suffix = "-vs2010"
          }
          "9.0" {
            $boost_toolchain_suffix = "-vs2008"
          }
          default {
            throw "Unsupported ${env:TOOLCHAIN} version for Boost: ${env:MSVC_VERSION}"
          }
        }
      }
      "mingw" {
        $boost_toolchain_suffix = "-mingw${env:MINGW_VERSION}" -replace "([\d]+)\.([\d]+)\.([\d]+)", '$1$2'
      }
      default {
        throw "Unsupported toolchain for Boost: ${env:TOOLCHAIN}"
      }
    }
    $boost_install_folder = "${env:DEPENDENCIES_FOLDER}\boost${boost_version_suffix}${env:BOOST_PLATFORM_SUFFIX}${boost_toolchain_suffix}"
    if (!(Test-Path -Path "${boost_install_folder}")) {
      Write-Host "Boost is absent for the chosen toolchain (${env:TOOLCHAIN_ID}) and Boost version (${env:BOOST_VERSION}) at ${boost_install_folder}"
      $boost_archive_name = "boost${boost_version_suffix}${env:BOOST_PLATFORM_SUFFIX}${boost_toolchain_suffix}.7z"
      $boost_archive_file = "${env:DOWNLOADS_FOLDER}\${boost_archive_name}"
      if (!(Test-Path -Path "${boost_archive_file}")) {
        $boost_download_url = "https://dl.bintray.com/mabrarov/generic/boost/${env:BOOST_VERSION}/${boost_archive_name}"
        if (!(Test-Path -Path "${env:DOWNLOADS_FOLDER}")) {
          New-Item -Path "${env:DOWNLOADS_FOLDER}" -ItemType "directory" | out-null
        }
        Write-Host "Downloading Boost from ${boost_download_url} to ${boost_archive_file}"
        curl.exe `
          --connect-timeout "${env:CURL_CONNECT_TIMEOUT}" `
          --max-time "${env:CURL_MAX_TIME}" `
          --retry "${env:CURL_RETRY}" `
          --retry-delay "${env:CURL_RETRY_DELAY}" `
          --show-error --silent --location `
          --output "${boost_archive_file}" `
          "${boost_download_url}"
        if (${LastExitCode} -ne 0) {
          throw "Downloading of Boost failed with exit code ${LastExitCode}"
        }
        Write-Host "Downloading of Boost completed successfully"
      }
      Write-Host "Extracting Boost from ${boost_archive_file} to ${env:DEPENDENCIES_FOLDER}"
      if (!(Test-Path -Path "${env:DEPENDENCIES_FOLDER}")) {
        New-Item -Path "${env:DEPENDENCIES_FOLDER}" -ItemType "directory" | out-null
      }
      7z.exe x "${boost_archive_file}" -o"${env:DEPENDENCIES_FOLDER}" -aoa -y | out-null
      if (${LastExitCode} -ne 0) {
        throw "Extracting of Boost failed with exit code ${LastExitCode}"
      }
      Write-Host "Extracting of Boost completed successfully"
    }
    Write-Host "Boost ${env:BOOST_VERSION} is located at ${boost_install_folder}"
    $boost_include_folder_version_suffix = "-${env:BOOST_VERSION}" -replace "([\d]+)\.([\d]+)(\.[\d]+)*", '$1_$2'
    $env:BOOST_INCLUDE_FOLDER = "${boost_install_folder}\include\boost${boost_include_folder_version_suffix}"
    $env:BOOST_LIBRARY_FOLDER = "${boost_install_folder}\lib"
  }
  if ((${env:RUNTIME_LINKAGE} -eq "static") -and (${env:BOOST_LINKAGE} -ne "static")) {
    throw "Incompatible type of linkage of Boost: ${env:BOOST_LINKAGE} for the specified type of linkage of C/C++ runtime: ${env:RUNTIME_LINKAGE}"
  }
  switch (${env:BOOST_LINKAGE}) {
    "static" {
      $env:BOOST_USE_STATIC_LIBS = "ON"
    }
    default {
      $env:BOOST_USE_STATIC_LIBS = "OFF"
    }
  }
}
if (Test-Path env:QT_VERSION) {
  $pre_installed_qt = $false
  if (!(${env:QT_LINKAGE} -eq "static")) {
    switch (${env:TOOLCHAIN}) {
      "msvc" {
        switch (${env:MSVC_VERSION}) {
          "14.2" {
            $pre_installed_qt = (${env:QT_VERSION} -eq "5.15.2") `
              -or ((${env:QT_VERSION} -eq "6.0.1") -and (${env:PLATFORM} -eq "x64"))
          }
          "14.1" {
            $pre_installed_qt = (${env:QT_VERSION} -eq "5.13.2") `
              -or (${env:QT_VERSION} -eq "5.12.6") `
              -or ((${env:QT_VERSION} -eq "5.10.1") -and (${env:PLATFORM} -eq "x64"))
          }
          "14.0" {
            $pre_installed_qt = ((${env:QT_VERSION} -eq "5.13.2") -and (${env:PLATFORM} -eq "x64")) `
              -or ((${env:QT_VERSION} -eq "5.12.6") -and (${env:PLATFORM} -eq "x64")) `
              -or (${env:QT_VERSION} -eq "5.11.3") `
              -or (${env:QT_VERSION} -eq "5.10.1") `
              -or (${env:QT_VERSION} -eq "5.9.9") `
              -or ((${env:QT_VERSION} -eq "5.7.0") -and (${env:PLATFORM} -eq "x86")) `
              -or (${env:QT_VERSION} -eq "5.6.3")
          }
          "12.0" {
            $pre_installed_qt = ((${env:QT_VERSION} -eq "5.10.1") -and (${env:PLATFORM} -eq "x64")) `
              -or ((${env:QT_VERSION} -eq "5.9.5") -and (${env:PLATFORM} -eq "x64")) `
              -or (${env:QT_VERSION} -eq "5.6.3")
          }
        }
      }
      "mingw" {
        switch (${env:MINGW_VERSION}) {
          "8.1.0" {
            $pre_installed_qt = (${env:QT_VERSION} -eq "5.15.2") `
              -or ((${env:QT_VERSION} -eq "6.0.1") -and (${env:PLATFORM} -eq "x64"))
          }
          "7.3.0" {
            $pre_installed_qt = (${env:QT_VERSION} -eq "5.13.2") `
              -or (${env:QT_VERSION} -eq "5.12.6")
          }
          "5.3.0" {
            $pre_installed_qt = ((${env:QT_VERSION} -eq "5.11.3") -and (${env:PLATFORM} -eq "Win32")) `
              -or ((${env:QT_VERSION} -eq "5.10.1") -and (${env:PLATFORM} -eq "Win32")) `
              -or ((${env:QT_VERSION} -eq "5.9.9") -and (${env:PLATFORM} -eq "Win32")) `
              -or ((${env:QT_VERSION} -eq "5.7.0") -and (${env:PLATFORM} -eq "Win32"))
          }
        }
      }
    }
  }
  if (${pre_installed_qt}) {
    $qt_folder_toolchain_suffix = ""
    switch (${env:TOOLCHAIN}) {
      "msvc" {
        switch (${env:MSVC_VERSION}) {
          "14.1" {
            $qt_folder_toolchain_suffix = "msvc2017"
          }
          "14.0" {
            $qt_folder_toolchain_suffix = "msvc2015"
          }
          "12.0" {
            $qt_folder_toolchain_suffix = "msvc2013"
          }
          "11.0" {
            $qt_folder_toolchain_suffix = "msvc2012"
          }
          "10.0" {
            $qt_folder_toolchain_suffix = "msvc2010"
          }
          "9.0" {
            $qt_folder_toolchain_suffix = "msvc2008"
          }
          default {
            throw "Unsupported ${env:TOOLCHAIN} version for Qt: ${env:MSVC_VERSION}"
          }
        }
      }
      "mingw" {
        $qt_folder_toolchain_suffix = "mingw${env:MINGW_VERSION}" -replace "([\d]+)\.([\d]+)(\.([\d]+))*", '$1$2'
      }
      default {
        throw "Unsupported toolchain for Qt: ${env:TOOLCHAIN}"
      }
    }
    $qt_folder_platform_suffix = ""
    switch (${env:PLATFORM}) {
      "Win32" {
        $qt_folder_platform_suffix = ""
        if (${env:TOOLCHAIN} -eq "mingw") {
          $qt_folder_platform_suffix = "_32"
        }
      }
      "x64" {
        $qt_folder_platform_suffix = "_64"
      }
      default {
        throw "Unsupported platform for Qt: ${env:PLATFORM}"
      }
    }
    $qt_opengl_suffix = ""
    switch (${env:QT_VERSION}) {
      "5.4" {
        $qt_opengl_suffix = "_opengl"
      }
      "5.3" {
        $qt_opengl_suffix = "_opengl"
      }
    }
    $env:QT_HOME = "C:\Qt\${env:QT_VERSION}\${qt_folder_toolchain_suffix}${qt_folder_platform_suffix}${qt_opengl_suffix}"
    $env:QT_BIN_FOLDER = "${env:QT_HOME}\bin"
    $env:QT_QMAKE_EXECUTABLE = "${env:QT_BIN_FOLDER}\qmake.exe"
  } else {
    $qt_platform_suffix = ""
    switch (${env:PLATFORM}) {
      "Win32" {
        $qt_platform_suffix = "-x86"
      }
      "x64" {
        $qt_platform_suffix = "-x64"
      }
      default {
        throw "Unsupported platform for Qt: ${env:PLATFORM}"
      }
    }
    $qt_version_suffix = ""
    if (!(${env:QT_VERSION} -match "[\d]+\.[\d]+\.[\d]+")) {
      $qt_version_suffix = "-${env:QT_VERSION}.0"
    } else {
      $qt_version_suffix = "-${env:QT_VERSION}"
    }
    $qt_toolchain_suffix = ""
    switch (${env:TOOLCHAIN}) {
      "msvc" {
        switch (${env:MSVC_VERSION}) {
          "14.2" {
            $qt_toolchain_suffix = "-vs2019"
          }
          "14.1" {
            $qt_toolchain_suffix = "-vs2017"
          }
          "14.0" {
            $qt_toolchain_suffix = "-vs2015"
          }
          "12.0" {
            $qt_toolchain_suffix = "-vs2013"
          }
          "11.0" {
            $qt_toolchain_suffix = "-vs2012"
          }
          "10.0" {
            $qt_toolchain_suffix = "-vs2010"
          }
          "9.0" {
            $qt_toolchain_suffix = "-vs2008"
          }
          default {
            throw "Unsupported ${env:TOOLCHAIN} version for Qt: ${env:MSVC_VERSION}"
          }
        }
      }
      "mingw" {
        $qt_toolchain_suffix = "-mingw${env:MINGW_VERSION}" -replace "([\d]+)\.([\d]+)\.([\d]+)", '$1$2'
      }
      default {
        throw "Unsupported toolchain for Qt: ${env:TOOLCHAIN}"
      }
    }
    $qt_linkage_suffix = ""
    switch (${env:QT_LINKAGE}) {
      "static" {
        $qt_linkage_suffix = "-static"
      }
      default {
        $qt_linkage_suffix = "-shared"
      }
    }
    $qt_install_folder = "${env:DEPENDENCIES_FOLDER}\qt${qt_version_suffix}${qt_platform_suffix}${qt_toolchain_suffix}${qt_linkage_suffix}"
    if (!(Test-Path -Path "${qt_install_folder}")) {
      Write-Host "Qt is absent for the chosen toolchain (${env:TOOLCHAIN_ID}) and Qt version (${env:QT_VERSION}) and Qt linkage (${env:QT_LINKAGE}) at ${qt_install_folder}"
      $qt_archive_name = "qt${qt_version_suffix}${qt_platform_suffix}${qt_toolchain_suffix}${qt_linkage_suffix}.7z"
      $qt_archive_file = "${env:DOWNLOADS_FOLDER}\${qt_archive_name}"
      if (!(Test-Path -Path "${qt_archive_file}")) {
        $qt_version_url_suffix = ""
        if (!(${env:QT_VERSION} -match "[\d]+\.[\d]+\.[\d]+")) {
          $qt_version_url_suffix = "/${env:QT_VERSION}.0"
        } else {
          $qt_version_url_suffix = "/${env:QT_VERSION}"
        }
        $qt_download_url = "https://dl.bintray.com/mabrarov/generic/qt${qt_version_url_suffix}/${qt_archive_name}"
        if (!(Test-Path -Path "${env:DOWNLOADS_FOLDER}")) {
          New-Item -Path "${env:DOWNLOADS_FOLDER}" -ItemType "directory" | out-null
        }
        Write-Host "Downloading Qt from ${qt_download_url} to ${qt_archive_file}"
        curl.exe `
          --connect-timeout "${env:CURL_CONNECT_TIMEOUT}" `
          --max-time "${env:CURL_MAX_TIME}" `
          --retry "${env:CURL_RETRY}" `
          --retry-delay "${env:CURL_RETRY_DELAY}" `
          --show-error --silent --location `
          --output "${qt_archive_file}" `
          "${qt_download_url}"
        if (${LastExitCode} -ne 0) {
          throw "Downloading of Qt failed with exit code ${LastExitCode}"
        }
        Write-Host "Downloading of Qt completed successfully"
      }
      Write-Host "Extracting Qt from ${qt_archive_file} to ${env:DEPENDENCIES_FOLDER}"
      if (!(Test-Path -Path "${env:DEPENDENCIES_FOLDER}")) {
        New-Item -Path "${env:DEPENDENCIES_FOLDER}" -ItemType "directory" | out-null
      }
      7z.exe x "${qt_archive_file}" -o"${env:DEPENDENCIES_FOLDER}" -aoa -y | out-null
      if (${LastExitCode} -ne 0) {
        throw "Extracting of Qt failed with exit code ${LastExitCode}"
      }
      Write-Host "Extracting of Qt completed successfully"
    }
    Write-Host "Qt ${env:QT_VERSION} is located at ${qt_install_folder}"
    $env:QT_HOME = "${qt_install_folder}"
    $env:QT_BIN_FOLDER = "${qt_install_folder}\bin"
    $env:QT_QMAKE_EXECUTABLE = "${env:QT_BIN_FOLDER}\qmake.exe"
  }
}
switch (${env:CONFIGURATION}) {
  "Debug" {
    $env:CMAKE_BUILD_CONFIG = "DEBUG"
  }
  "Release" {
    $env:CMAKE_BUILD_CONFIG = "RELEASE"
  }
  default {
    throw "Unsupported build configuration: ${env:CONFIGURATION}"
  }
}
$env:CMAKE_GENERATOR_PLATFORM = ""
switch (${env:TOOLCHAIN}) {
  "msvc" {
    $cmake_generator_msvc_version_suffix = " ${env:MSVC_VERSION}" -replace "([\d]+)\.([\d]+)", '$1'
    switch (${env:MSVC_VERSION}) {
      "14.2" {
        $cmake_generator_msvc_version_suffix = " 16 2019"
      }
      "14.1" {
        $cmake_generator_msvc_version_suffix = " 15 2017"
      }
      "14.0" {
        $cmake_generator_msvc_version_suffix = "${cmake_generator_msvc_version_suffix} 2015"
      }
      "12.0" {
        $cmake_generator_msvc_version_suffix = "${cmake_generator_msvc_version_suffix} 2013"
      }
      "11.0" {
        $cmake_generator_msvc_version_suffix = "${cmake_generator_msvc_version_suffix} 2012"
      }
      "10.0" {
        $cmake_generator_msvc_version_suffix = "${cmake_generator_msvc_version_suffix} 2010"
      }
      "9.0" {
        $cmake_generator_msvc_version_suffix = "${cmake_generator_msvc_version_suffix} 2008"
      }
      default {
        throw "Unsupported ${env:TOOLCHAIN} version for CMake generator: ${env:MSVC_VERSION}"
      }
    }
    switch (${env:PLATFORM}) {
      "Win32" {
        $env:CMAKE_GENERATOR_PLATFORM = "Win32"
      }
      "x64" {
        $env:CMAKE_GENERATOR_PLATFORM = "x64"
      }
      default {
        throw "Unsupported platform for CMake generator: ${env:PLATFORM}"
      }
    }
    $env:CMAKE_GENERATOR = "Visual Studio${cmake_generator_msvc_version_suffix}"
  }
  "mingw" {
    $env:CMAKE_GENERATOR = "MinGW Makefiles"
  }
  "default" {
    throw "Unsupported toolchain for CMake generator: ${env:TOOLCHAIN}"
  }
}
$env:COVERITY_SCAN_BUILD = (${env:COVERITY_SCAN_CANDIDATE} -eq "1") `
  -and (${env:APPVEYOR_REPO_BRANCH} -eq "coverity_scan") `
  -and (${env:CONFIGURATION} -eq "Release") `
  -and (${env:PLATFORM} -eq "x64")
$env:COVERAGE_BUILD = (${env:COVERAGE_BUILD_CANDIDATE} -eq "1") `
  -and (${env:CONFIGURATION} -eq "Debug") `
  -and (${env:PLATFORM} -eq "x64")
if ((${env:COVERAGE_BUILD} -eq "True") -and `
 -not (Test-Path -Path env:CODECOV_TOKEN) -or (${env:CODECOV_TOKEN} -eq "")) {
  throw "CODECOV_TOKEN environment variable is not defined but is required to upload coverage report to Codecov"
}
$env:CODECOV_FLAG = "windows_${env:OS_VERSION}__${env:PLATFORM}__${env:TOOLCHAIN_ID}"
if (Test-Path env:BOOST_VERSION) {
  $env:CODECOV_FLAG = "${env:CODECOV_FLAG}__boost_${env:BOOST_VERSION}"
}
if (Test-Path env:QT_VERSION) {
  $env:CODECOV_FLAG = "${env:CODECOV_FLAG}__qt_${env:QT_VERSION}"
}
$env:CODECOV_FLAG = "${env:CODECOV_FLAG}" -replace "[\.-]", "_"
Write-Host "PLATFORM                  : ${env:PLATFORM}"
Write-Host "CONFIGURATION             : ${env:CONFIGURATION}"
Write-Host "TOOLCHAIN_ID              : ${env:TOOLCHAIN_ID}"
switch (${env:TOOLCHAIN}) {
  "msvc" {
    Write-Host "MSVS_PATCH_FOLDER         : ${env:MSVS_PATCH_FOLDER}"
    Write-Host "MSVS_PATCH_BATCH_FILE     : ${env:MSVS_PATCH_BATCH_FILE}"
    Write-Host "WINDOWS_SDK_ENV_BATCH_FILE: ${env:WINDOWS_SDK_ENV_BATCH_FILE}"
    Write-Host "WINDOWS_SDK_ENV_PARAMETERS: ${env:WINDOWS_SDK_ENV_PARAMETERS}"
    Write-Host "MSVC_VARS_BATCH_FILE      : ${env:MSVC_VARS_BATCH_FILE}"
    Write-Host "MSVC_VARS_PLATFORM        : ${env:MSVC_VARS_PLATFORM}"
  }
  "mingw" {
    Write-Host "MINGW_HOME                : ${env:MINGW_HOME}"
  }
}
Write-Host "APPVEYOR_BUILD_FOLDER     : ${env:APPVEYOR_BUILD_FOLDER}"
if (Test-Path env:ICU_ROOT) {
  Write-Host "ICU_ROOT                  : ${env:ICU_ROOT}"
  Write-Host "ICU_LIBRARY_FOLDER        : ${env:ICU_LIBRARY_FOLDER}"
}
Write-Host "ARTIFACT_PATH_SUFFIX      : ${env:ARTIFACT_PATH_SUFFIX}"
Write-Host "BOOST_INCLUDE_FOLDER      : ${env:BOOST_INCLUDE_FOLDER}"
Write-Host "BOOST_LIBRARY_FOLDER      : ${env:BOOST_LIBRARY_FOLDER}"
Write-Host "BOOST_USE_STATIC_LIBS     : ${env:BOOST_USE_STATIC_LIBS}"
Write-Host "QT_HOME                   : ${env:QT_HOME}"
Write-Host "QT_BIN_FOLDER             : ${env:QT_BIN_FOLDER}"
Write-Host "QT_QMAKE_EXECUTABLE       : ${env:QT_QMAKE_EXECUTABLE}"
Write-Host "CMAKE_GENERATOR           : ${env:CMAKE_GENERATOR}"
Write-Host "CMAKE_GENERATOR_PLATFORM  : ${env:CMAKE_GENERATOR_PLATFORM}"
Write-Host "COVERITY_SCAN_BUILD       : ${env:COVERITY_SCAN_BUILD}"
Write-Host "COVERAGE_BUILD            : ${env:COVERAGE_BUILD}"
Write-Host "CODECOV_FLAG              : ${env:CODECOV_FLAG}"
if (${env:COVERAGE_BUILD} -eq "True") {
  Write-Host "COBERTURA_COVERAGE_FILE   : ${env:COBERTURA_COVERAGE_FILE}"
}

if (${env:COVERAGE_BUILD} -eq "True") {
  Write-Host "Installing OpenCppCoverage from Chocolatey package"
  appveyor-retry choco install -y --no-progress opencppcoverage
  if (${LastExitCode} -ne 0) {
    throw "Installation of OpenCppCoverage Chocolatey package failed with ${LastExitCode} exit code"
  }
  Write-Host "OpenCppCoverage installed"

  Write-Host "Installing Codecov from pip package"
  pip install --disable-pip-version-check --retries "${env:PIP_RETRY}" codecov=="${env:CODECOV_VERSION}"
  if (${LastExitCode} -ne 0) {
    throw "Installation of Codecov pip package failed with exit code ${LastExitCode}"
  }
  Write-Host "Codecov installed"
}

if (Test-Path env:MSVS_PATCH_FOLDER) {
  Set-Location -Path "${env:MSVS_PATCH_FOLDER}"
}
if (Test-Path env:MSVS_PATCH_BATCH_FILE) {
  Write-Host "Patching MSVS with ${env:MSVS_PATCH_BATCH_FILE}"
  & "${env:MSVS_PATCH_BATCH_FILE}"
  if (${LastExitCode} -ne 0) {
    throw "Patch of MSVC failed with exit code ${LastExitCode}"
  }
  Write-Host "Patching of MSVS with completed successfully"
}

Set-Location -Path "${env:APPVEYOR_BUILD_FOLDER}"
