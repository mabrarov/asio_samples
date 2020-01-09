#!/bin/bash

#
# Copyright (c) 2019 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

set -e

# shellcheck source=vercomp.sh
source "${TRAVIS_BUILD_DIR}/scripts/travis/vercomp.sh"

if [[ "${TRAVIS_OS_NAME}" = "osx" ]]; then
  export PATH="/usr/local/opt/gnu-sed/libexec/gnubin:${PATH}"
fi

detected_cmake_version="$({ cmake --version 2> /dev/null || echo ""; } \
  | sed -r 's/cmake version ([0-9]+\.[0-9]+\.[0-9]+)/\1/;t;d')"
echo "Detected CMake of ${detected_cmake_version} version"

if [[ -n "${CMAKE_VERSION+x}" ]]; then
  echo "CMake of ${CMAKE_VERSION} version is requested"
  if [[ "${CMAKE_VERSION}" != "${detected_cmake_version}" ]]; then
    if [[ "${TRAVIS_OS_NAME}" = "linux" ]]; then
      if [[ "$(vercomp "${CMAKE_VERSION}" "3.1.0")" -ge 0 ]]; then
        cmake_archive_base_name="cmake-${CMAKE_VERSION}-Linux-x86_64"
      else
        # CMake x64 binary is not available for CMake version < 3.1.0
        cmake_archive_base_name="cmake-${CMAKE_VERSION}-Linux-i386"
      fi
      cmake_home="${DEPENDENCIES_HOME}/${cmake_archive_base_name}"
      if [[ ! -d "${cmake_home}" ]]; then
        echo "CMake ${CMAKE_VERSION} not found at ${cmake_home}"
        cmake_archive_name="${cmake_archive_base_name}.tar.gz"
        cmake_archive_file="${DOWNLOADS_HOME}/${cmake_archive_name}"
        if [[ ! -f "${cmake_archive_file}" ]]; then
          cmake_download_url="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/${cmake_archive_name}"
          echo "Downloading CMake ${CMAKE_VERSION} archive from ${cmake_download_url} to ${cmake_archive_file}"
          mkdir -p "${DOWNLOADS_HOME}"
          curl \
            --connect-timeout "${CURL_CONNECT_TIMEOUT}" \
            --max-time "${CURL_MAX_TIME}" \
            --retry "${CURL_RETRY}" \
            --retry-delay "${CURL_RETRY_DELAY}" \
            -jksSL \
            "${cmake_download_url}" \
            --output "${cmake_archive_file}"
          echo "Downloading of CMake archive completed successfully"
        fi
        mkdir -p "${DEPENDENCIES_HOME}"
        echo "Extracting CMake ${CMAKE_VERSION} from ${cmake_archive_file} to ${DEPENDENCIES_HOME}"
        tar -xzf "${cmake_archive_file}" -C "${DEPENDENCIES_HOME}"
        echo "Extracting of CMake completed successfully"
      fi
      echo "CMake ${CMAKE_VERSION} is located at ${cmake_home}"
      export PATH="${cmake_home}/bin:${PATH}"
    else
      echo "Chosen version of CMake: ${CMAKE_VERSION} is not supported for OS: ${TRAVIS_OS_NAME}"
      exit 1
    fi
  fi
fi

system_boost_home=0
if [[ "${TRAVIS_OS_NAME}" = "linux" ]]; then
  if [[ "${TRAVIS_DIST}" = "trusty" ]]; then
    if [[ "${BOOST_VERSION}" = "1.55.0" ]] \
      || [[ "${BOOST_VERSION}" = "1.68.0" ]] \
      || [[ "${BOOST_VERSION}" = "1.70.0" ]]; then
      system_boost_home=1
    fi
  fi
elif [[ "${TRAVIS_OS_NAME}" = "osx" ]]; then
  if [[ "${BOOST_VERSION}" = "1.55.0" ]] \
    || [[ "${BOOST_VERSION}" = "1.57.0" ]] \
    || [[ "${BOOST_VERSION}" = "1.59.0" ]] \
    || [[ "${BOOST_VERSION}" = "1.60.0" ]] \
    || [[ "${BOOST_VERSION}" = "1.69.0" ]] \
    || [[ "${BOOST_VERSION}" = "1.70.0" ]] \
    || [[ "${BOOST_VERSION}" = "1.71.0" ]]; then
    system_boost_home=1
  fi
fi

if [[ "${system_boost_home}" -eq 0 ]]; then
  boost_archive_base_name="boost-${BOOST_VERSION}-x64-gcc4.8"
  export BOOST_HOME="${DEPENDENCIES_HOME}/${boost_archive_base_name}"
  if [[ ! -d "${BOOST_HOME}" ]]; then
    echo "Boost is absent for the chosen Boost version (${BOOST_VERSION}) at ${BOOST_HOME}"
    boost_archive_name="${boost_archive_base_name}.tar.gz"
    boost_archive_file="${DOWNLOADS_HOME}/${boost_archive_name}"
    if [[ ! -f "${boost_archive_file}" ]]; then
      boost_download_url="${PREBUILT_BOOST_DOWNLOAD_URL}/${BOOST_VERSION}/${boost_archive_name}"
      mkdir -p "${DOWNLOADS_HOME}"
      echo "Downloading Boost from ${boost_download_url} to ${boost_archive_file}"
      curl \
        --connect-timeout "${CURL_CONNECT_TIMEOUT}" \
        --max-time "${CURL_MAX_TIME}" \
        --retry "${CURL_RETRY}" \
        --retry-delay "${CURL_RETRY_DELAY}" \
        -jksSL \
        "${boost_download_url}" \
        --output "${boost_archive_file}"
      echo "Downloading of Boost completed successfully"
    fi
    echo "Extracting Boost from ${boost_archive_file} to ${DEPENDENCIES_HOME}"
    mkdir -p "${DEPENDENCIES_HOME}"
    tar -xzf "${boost_archive_file}" -C "${DEPENDENCIES_HOME}"
    echo "Extracting of Boost completed successfully"
  fi
  echo "Boost ${BOOST_VERSION} is located at ${BOOST_HOME}"
  export LD_LIBRARY_PATH="${BOOST_HOME}/lib:${LD_LIBRARY_PATH}"
fi

COVERAGE_BUILD=0
if [[ "${COVERAGE_BUILD_CANDIDATE}" -ne 0 ]]; then
  COVERAGE_BUILD=1
fi
export COVERAGE_BUILD

if [[ "${COVERAGE_BUILD}" -ne 0 ]]; then
  echo "Installing Codecov from pip package"
  travis_retry pip3 install --user --upgrade pip
  pip3 install --user --retries "${PIP_RETRY}" codecov=="${CODECOV_VERSION}"
  echo "Codecov installed"
fi
