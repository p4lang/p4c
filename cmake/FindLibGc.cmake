# Copyright 2017 Xilinx, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

find_path(LibGc_INCLUDE_DIR NAMES gmp.h)
find_library(LibGc_LIBRARY NAMES gmp libgmp)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(LibGc
    REQUIRED_VARS LibGc_INCLUDE_DIR LibGc_LIBRARY
    )

set(LibGc_FOUND ${LIBGC_FOUND})
unset(LIBGC_FOUND)

if(LibGc_FOUND)
    set(LibGc_INCLUDE_DIRS ${LibGc_INCLUDE_DIR})
    set(LibGc_LIBRARIES    ${LibGc_LIBRARY})
endif()
