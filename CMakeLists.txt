# Copyright (c) 2020-2024 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.7)

project(webos-hfp-service C CXX)

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC7)
webos_component(1 0 0)

# Enable C++11 support (still gcc 4.6 so can't use -std=c++11)
_webos_manipulate_flags(APPEND CXX ALL -std=c++0x)

find_program(GDBUS_CODEGEN_EXECUTABLE NAMES gdbus-codegen DOC "gdbus-codegen executable")
if(NOT GDBUS_CODEGEN_EXECUTABLE)
	message(FATAL_ERROR "Executable gdbus-codegen not found")
endif()

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS})

pkg_check_modules(GIO2 REQUIRED gio-2.0)
include_directories(${GIO2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GIO2_CFLAGS})

pkg_check_modules(GIO-UNIX REQUIRED gio-unix-2.0)
include_directories(${GIO-UNIX_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GIO-UNIX_CFLAGS_OTHER})

pkg_check_modules(LUNASERVICE2 REQUIRED luna-service2)
include_directories(${LUNASERVICE2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LUNASERVICE2_CFLAGS})

pkg_check_modules(PBNJSON_CXX REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CXX_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CXX_CFLAGS_OTHER})

pkg_check_modules(PMLOG REQUIRED PmLogLib)
include_directories(${PMLOG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOG_CFLAGS_OTHER})

set(GDBUS_IF_DIR ${WEBOS_BINARY_CONFIGURED_DIR}/src)
file(MAKE_DIRECTORY ${GDBUS_IF_DIR})

if(MULTI_SESSION_SUPPORT)
    add_definitions(-DMULTI_SESSION_SUPPORT)
endif()

execute_process(COMMAND ${GDBUS_CODEGEN_EXECUTABLE}
        --c-namespace Ofono
        --generate-c-code ${GDBUS_IF_DIR}/ofono-interface
        --interface-prefix org.ofono.
        ${CMAKE_CURRENT_SOURCE_DIR}/files/xml/ofono.xml
        RESULT_VARIABLE codegen_failed)
if (codegen_failed)
    message(FATAL_ERROR "Error in generating code for ofono dbus interface using gdbus-codegen")
endif()


include_directories(src ${GDBUS_IF_DIR})

file(GLOB SOURCES 
     ${GDBUS_IF_DIR}/ofono-interface.c
     src/*.cpp
     src/AG/*.cpp
     src/HF/*.cpp)

webos_configure_source_files(SOURCES src/config.h)

webos_add_linker_options(ALL --no-undefined)

webos_add_compiler_flags(ALL -I${CMAKE_CURRENT_BINARY_DIR}/Configured/src)

add_executable(webos-hfp-service ${SOURCES})
target_link_libraries(webos-hfp-service
    ${GLIB2_LDFLAGS} ${LUNASERVICE2_LDFLAGS} ${PBNJSON_CXX_LDFLAGS}
    ${GIO2_LDFLAGS} ${GIO-UNIX_LDFLAGS} ${PMLOG_LDFLAGS}
    rt pthread luna-service2++)

webos_build_daemon()
webos_build_system_bus_files()
webos_build_configured_file(files/conf/pmlog/webos-hfp-service.conf SYSCONFDIR pmlog.d)
