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

find_path(LibGmp_INCLUDE_DIR NAMES gmp.h)
find_library(LibGmp_LIBRARY NAMES gmp libgmp)
find_library(LibGmpxx_LIBRARY NAMES gmpxx libgmpxx)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(LibGmp
    REQUIRED_VARS LibGmp_INCLUDE_DIR LibGmp_LIBRARY LibGmpxx_LIBRARY
    )

set(LibGmp_FOUND ${LIBGMP_FOUND})
unset(LIBGMP_FOUND)

if(LibGmp_FOUND)
    set(LibGmp_INCLUDE_DIRS ${LibGmp_INCLUDE_DIR})
    set(LibGmp_LIBRARIES    ${LibGmp_LIBRARY};${LibGmpxx_LIBRARY})
endif()
