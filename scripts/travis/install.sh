#!/bin/bash

set -e

if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  export PATH="/usr/local/opt/gnu-sed/libexec/gnubin:${PATH}"
fi

export DETECTED_CMAKE_VERSION="$(cmake --version \
  | sed -r 's/cmake version ([0-9]+\.[0-9]+\.[0-9]+)/\1/;t;d')"
echo "Detected CMake of ${DETECTED_CMAKE_VERSION} version"

if [[ ! -z "${CMAKE_VERSION+x}" ]]; then
  echo "CMake of ${CMAKE_VERSION} version is requested"
  if [[ "${CMAKE_VERSION}" != "${DETECTED_CMAKE_VERSION}" ]]; then
    if [[ "${TRAVIS_OS_NAME}" == "linux" ]]; then
      cmake_archive_base_name="cmake-${CMAKE_VERSION}-Linux-x86_64"
      cmake_home="${DEPENDENCIES_HOME}/${cmake_archive_base_name}"
      if [[ ! -d "${cmake_home}" ]]; then
        echo "CMake ${CMAKE_VERSION} not found at ${cmake_home}"
        cmake_archive_name="${cmake_archive_base_name}.tar.gz"
        cmake_archive_file="${DOWNLOADS_HOME}/${cmake_archive_name}"
        if [[ ! -f "${cmake_archive_file}" ]]; then
          cmake_download_url="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/${cmake_archive_name}"
          echo "Going to download CMake ${CMAKE_VERSION} archive from ${cmake_download_url} to ${cmake_archive_file}"
          mkdir -p "${DOWNLOADS_HOME}"
          curl \
            --connect-timeout "${CURL_CONNECT_TIMEOUT}" \
            --max-time "${CURL_MAX_TIME}" \
            --retry "${CURL_RETRY}" \
            --retry-delay "${CURL_RETRY_DELAY}" \
            -jksSL
            "${cmake_download_url}" \
            --output "${cmake_archive_file}"
          echo "Downloading of CMake ${CMAKE_VERSION} archive from ${cmake_download_url} to ${cmake_archive_file} completed successfully"
        fi
        mkdir -p "${DEPENDENCIES_HOME}"
        echo "Extracting CMake ${CMAKE_VERSION} from ${cmake_archive_file} to ${DEPENDENCIES_HOME}"
        tar -xzf "${cmake_archive_file}" -C "${DEPENDENCIES_HOME}"
        echo "Extracting of CMake ${CMAKE_VERSION} from ${cmake_archive_file} to ${DEPENDENCIES_HOME} completed successfully"
      fi
      echo "CMake ${CMAKE_VERSION} is located at ${cmake_home}"
      export PATH="${cmake_home}/bin:${PATH}"
    else
      echo "Custom version of CMake - ${CMAKE_VERSION} - is not supported for ${TRAVIS_OS_NAME} OS"
      exit 1
    fi
  fi
fi

export BOOST_FROM_PACKAGE=0
if [[ "${TRAVIS_OS_NAME}" == "linux" ]]; then
  if [[ "${TRAVIS_DIST}" == "trusty" ]]; then
    if [[ "${BOOST_VERSION}" == "1.55.0" ]] \
      || [[ "${BOOST_VERSION}" == "1.68.0" ]] \
      || [[ "${BOOST_VERSION}" == "1.70.0" ]]; then
      export BOOST_FROM_PACKAGE=1
    fi
  fi
elif [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  if [[ "${BOOST_VERSION}" == "1.55.0" ]] \
    || [[ "${BOOST_VERSION}" == "1.57.0" ]] \
    || [[ "${BOOST_VERSION}" == "1.59.0" ]] \
    || [[ "${BOOST_VERSION}" == "1.60.0" ]] \
    || [[ "${BOOST_VERSION}" == "1.69.0" ]] \
    || [[ "${BOOST_VERSION}" == "1.70.0" ]]; then
    export BOOST_FROM_PACKAGE=1
  fi
fi

if [[ "${BOOST_FROM_PACKAGE}" != 0 ]]; then
  echo "Boost ${BOOST_VERSION} is installed from package"
else
  boost_archive_base_name="boost-${BOOST_VERSION}-x64-gcc4.8"
  export PREBUILT_BOOST_HOME="${DEPENDENCIES_HOME}/${boost_archive_base_name}"
  if [[ ! -d "${PREBUILT_BOOST_HOME}" ]]; then
    echo "Boost libraries are absent for the selected Boost version (${BOOST_VERSION}) at ${PREBUILT_BOOST_HOME}"
    boost_archive_name="${boost_archive_base_name}.tar.gz"
    boost_archive_file="${DOWNLOADS_HOME}/${boost_archive_name}"
    if [[ ! -f "${boost_archive_file}" ]]; then
      boost_download_url="${PREBUILT_BOOST_DOWNLOAD_URL}/${BOOST_VERSION}/${boost_archive_name}"
      mkdir -p "${DOWNLOADS_HOME}"
      echo "Going to download Boost libraries from ${boost_download_url} to ${boost_archive_file}"
      curl \
        --connect-timeout "${CURL_CONNECT_TIMEOUT}" \
        --max-time "${CURL_MAX_TIME}" \
        --retry "${CURL_RETRY}" \
        --retry-delay "${CURL_RETRY_DELAY}" \
        -jksSL \
        "${boost_download_url}" \
        --output "${boost_archive_file}"
      echo "Downloading of Boost libraries from ${boost_download_url} to ${boost_archive_file} completed successfully"
    fi
    echo "Extracting Boost libraries from ${boost_archive_file} to ${DEPENDENCIES_HOME}"
    mkdir -p "${DEPENDENCIES_HOME}"
    tar -xzf "${boost_archive_file}" -C "${DEPENDENCIES_HOME}"
    echo "Extracting of Boost libraries from ${boost_archive_file} to ${DEPENDENCIES_HOME} completed successfully"
  fi
  echo "Boost ${BOOST_VERSION} is located at ${PREBUILT_BOOST_HOME}"
  export LD_LIBRARY_PATH="${PREBUILT_BOOST_HOME}/lib:${LD_LIBRARY_PATH}"
fi
