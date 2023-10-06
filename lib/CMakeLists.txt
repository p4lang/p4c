# Copyright 2013-present Barefoot Networks, Inc.
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

set (LIBP4CTOOLKIT_SRCS
    alloc_trace.cpp
    backtrace_exception.cpp
    bitvec.cpp
    compile_context.cpp
    crash.cpp
    cstring.cpp
    error_catalog.cpp
    error_message.cpp
    exename.cpp
    gc.cpp
    big_int_util.cpp
    hash.cpp
    hashvec.cpp
    hex.cpp
    indent.cpp
    json.cpp
    log.cpp
    match.cpp
    nullstream.cpp
    options.cpp
    path.cpp
    source_file.cpp
    stringify.cpp
    timer.cpp
)

set (LIBP4CTOOLKIT_HDRS
    algorithm.h
    alloc_trace.h
    backtrace_exception.h
    bitops.h
    bitrange.h
    bitvec.h
    compile_context.h
    crash.h
    cstring.h
    enumerator.h
    error.h
    error_catalog.h
    error_message.h
    error_helper.h
    error_reporter.h
    exceptions.h
    exename.h
    gc.h
    big_int_util.h
    hash.h
    hashvec.h
    hex.h
    hvec_map.h
    indent.h
    json.h
    log.h
    ltbitmatrix.h
    map.h
    match.h
    n4.h
    null.h
    nullstream.h
    options.h
    ordered_map.h
    ordered_set.h
    path.h
    range.h
    safe_vector.h
    set.h
    source_file.h
    sourceCodeBuilder.h
    stringify.h
    stringref.h
    symbitmatrix.h
    timer.h
)


add_library(p4ctoolkit STATIC ${LIBP4CTOOLKIT_SRCS})

# Disable libcall (realloc, malloc) optimizations which may cause infinite loops.
set_target_properties(p4ctoolkit PROPERTIES COMPILE_FLAGS -fno-builtin)
