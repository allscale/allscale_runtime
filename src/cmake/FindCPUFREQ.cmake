# Copyright (c) 2017 Arne Hendricks
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

find_package(PkgConfig QUIET)
pkg_check_modules(PC_CPUFREQ QUIET cpufreq)

find_path(CPUFREQ_INCLUDE_DIR cpufreq.h
  HINTS
    ${CPUFREQ_ROOT} ENV CPUFREQ_ROOT
    ${PC_CPUFREQ_INCLUDEDIR}
    ${PC_CPUFREQ_INCLUDE_DIRS}
  PATH_SUFFIXES include)

find_library(CPUFREQ_LIBRARY NAMES cpufreq libcpufreq
  HINTS
    ${CPUFREQ_ROOT} ENV CPUFREQ_ROOT
    ${PC_CPUFREQ_LIBDIR}
    ${PC_CPUFREQ_LIBRARY_DIRS}
  PATH_SUFFIXES lib lib64)

set(CPUFREQ_LIBRARIES ${CPUFREQ_LIBRARY})
set(CPUFREQ_INCLUDE_DIRS ${CPUFREQ_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CPUFREQ
  FOUND_VAR CPUFREQ_FOUND
  REQUIRED_VARS
    CPUFREQ_LIBRARY
    CPUFREQ_INCLUDE_DIR
  VERSION_VAR CPUFREQ_VERSION
)

if(CPUFREQ_FOUND)
  set(CPUFREQ_LIBRARIES ${CPUFREQ_LIBRARY})
  set(CPUFREQ_INCLUDE_DIRS ${CPUFREQ_INCLUDE_DIR})
  set(CPUFREQ_DEFINITIONS ${PC_CPUFREQ_CFLAGS_OTHER})
endif()
mark_as_advanced(CPUFREQ_ROOT CPUFREQ_LIBRARY CPUFREQ_INCLUDE_DIR)
