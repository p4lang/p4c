/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/mau/mau_spec.h"

#include "input_xbar.h"

int TofinoIXBarSpec::getExactOrdBase(int group) const {
    return group * Tofino::IXBar::EXACT_BYTES_PER_GROUP;
}

int TofinoIXBarSpec::getTernaryOrdBase(int group) const {
    return Tofino::IXBar::EXACT_GROUPS * Tofino::IXBar::EXACT_BYTES_PER_GROUP +
           (group / 2) * Tofino::IXBar::TERNARY_BYTES_PER_BIG_GROUP +
           (group % 2) * (Tofino::IXBar::TERNARY_BYTES_PER_GROUP + 1 /* mid byte */);
}
