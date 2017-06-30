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
	bitvec.cpp
	crash.cpp
	cstring.cpp
	error.cpp
	gc.cpp
	gmputil.cpp
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
)

set (LIBP4CTOOLKIT_HDRS
	algorithm.h
	alloc.h
	bitops.h
	bitrange.h
	bitvec.h
	crash.h
	cstring.h
	enumerator.h
	error.h
	exceptions.h
	gc.h
	gmputil.h
	hex.h
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
	set.h
	source_file.h
	sourceCodeBuilder.h
	stringify.h
	stringref.h
	symbitmatrix.h
)

set (CPPLINT_FILES "${CPPLINT_FILES} ${LIBP4CTOOLKIT_SRCS}")

add_library(p4ctoolkit STATIC ${LIBP4CTOOLKIT_SRCS})
