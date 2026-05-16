# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0

set(LIBP4CTOOLKIT_SRCS
    alloc_trace.cpp
    backtrace_exception.cpp
    bitrange.cpp
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
    nethash.cpp
    nullstream.cpp
    options.cpp
    source_file.cpp
    stringify.cpp
    timer.cpp
)

set(LIBP4CTOOLKIT_HDRS
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
    nethash.h
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
target_link_libraries(p4ctoolkit
  PRIVATE absl::stacktrace
  PRIVATE absl::symbolize
  # These libraries are exposed by a header.
  PUBLIC absl::bits
  PUBLIC absl::strings
  PUBLIC ${LIBGC_LIBRARIES}
)
