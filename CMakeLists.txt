# libspnavdev - direct 6dof device handling library
# Copyright (C) 2021 Collabora, Ltd.
#
# SPDX-License-Identifier: GPL-3-or-later
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.1...3.19)
project(
  libspnavdev
  VERSION 0.1
  LANGUAGES C)

find_package(HIDAPI)


add_library(spnavdev
  src/serdev.c
  src/spnavdev.c
  src/usbdev.c
)

if(HIDAPI_FOUND)
  set(HAVE_HIDAPI TRUE)
  target_link_libraries(spnavdev PRIVATE ${HIDAPI_LIBRARY})
  target_include_directories(spnavdev PRIVATE ${HIDAPI_INCLUDE_DIR})
endif()
target_include_directories(spnavdev PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
target_include_directories(spnavdev PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(spnavdev PRIVATE HAVE_CONFIG_H)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  # We are our own project, not a subproject
  if(WIN32)
    add_subdirectory(examples/win32)
  else()
    add_subdirectory(examples/unix)
  endif()
endif()
