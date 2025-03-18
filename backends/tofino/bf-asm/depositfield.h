/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DEPOSITFIELD_H_
#define _DEPOSITFIELD_H_

#include <stdint.h>

namespace DepositField {

struct RotateConstant {
    unsigned rotate;
    int32_t value;
};

RotateConstant discoverRotation(int32_t val, int containerSize, int32_t tooLarge, int32_t tooSmall);

}  // namespace DepositField

#endif /* _DEPOSITFIELD_H_ */
