#
# Copyright (c) 2010-2014 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

# Boost C++ Libraries headers
win32 {
  BOOST_INCLUDE = $$(USERPROFILE)/Boost
} else {
  BOOST_INCLUDE = /home/marat/Документы/boost-1_56_0_b1
}

# Boost C++ Libraries binaries
# Base path to binaries of Boost C++ Libraries
win32 {
  BOOST_LIB = $${BOOST_INCLUDE}/lib
} else {
  BOOST_LIB = $${BOOST_INCLUDE}/lib
}

# Target specific path to binaries of Boost C++ Libraries
win32 {
  contains(QMAKE_TARGET.arch, x86) {
    BOOST_LIB = $${BOOST_LIB}/x86
  }
  contains(QMAKE_TARGET.arch, x86_64) {
    BOOST_LIB = $${BOOST_LIB}/amd64
  }
} else {
  linux-g++-64 {
    BOOST_LIB = $${BOOST_LIB}/x86_64-linux-gnu
  }
  linux-llvm | macx-llvm | linux-clang {
    BOOST_LIB = $${BOOST_LIB}/clang
  }
}

# Additional C/C++ preprocessor definitions
win32:DEFINES += WIN32_LEAN_AND_MEAN \
                 _UNICODE \
                 UNICODE

# Compiler options
linux-g++ | linux-g++-32 | linux-g++-64 | win32-g++-4.6 \
  | linux-icc | linux-icc-32 | linux-icc-64 \
  | linux-llvm | macx-llvm | linux-clang {
  QMAKE_CXXFLAGS += -std=c++0x \
                    -Wstrict-aliasing
}

win32-icc {
  QMAKE_CXXFLAGS += /Qstd=c++0x
}

# Fix for Qt 5 to work with Boost C++ Libraries
#   ensure QMAKE_MOC contains the moc executable path
load(moc)
#   for each Boost header we include...
QMAKE_MOC += -DBOOST_ERROR_CODE_HPP \
             -DBOOST_THREAD_LOCKS_HPP \
             -DBOOST_THREAD_MUTEX_HPP \
             -DBOOST_THREAD_RECURSIVE_MUTEX_HPP \
             -DBOOST_TUPLE_HPP \
             -DBOOST_OPTIONAL_FLC_19NOV2002_HPP \
             -DPOSIX_TIME_TYPES_HPP___ \
             -DBOOST_ASIO_HPP \
             -DBOOST_BIND_BIND_HPP_INCLUDED \
             -DBOOST_NONCOPYABLE_HPP_INCLUDED \
             -DBOOST_SHARED_PTR_HPP_INCLUDED \
             -DBOOST_ENABLE_SHARED_FROM_THIS_HPP_INCLUDED
