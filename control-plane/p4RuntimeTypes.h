/*
 * SPDX-FileCopyrightText: 2024 Intel Corporation
 * Copyright 2024-present Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONTROL_PLANE_P4RUNTIMETYPES_H_
#define CONTROL_PLANE_P4RUNTIMETYPES_H_

namespace P4 {

/// P4Runtime serialization formats.
enum class P4RuntimeFormat { BINARY, JSON, TEXT, TEXT_PROTOBUF };

}  // namespace P4

#endif /* CONTROL_PLANE_P4RUNTIMETYPES_H_ */
