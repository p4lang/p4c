/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_BIG_INT_H_
#define LIB_BIG_INT_H_

#include <boost/multiprecision/cpp_int.hpp>

namespace P4 {

using big_int = boost::multiprecision::cpp_int;

}  // namespace P4

#endif /* LIB_BIG_INT_H_ */
