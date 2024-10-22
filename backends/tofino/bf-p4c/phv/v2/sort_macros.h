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

#ifndef BF_P4C_PHV_V2_SORT_MACROS_H_
#define BF_P4C_PHV_V2_SORT_MACROS_H_

/// convenience macros.
#define IF_NEQ_RETURN_IS_LESS(a, b) \
    if ((a) != (b)) return (a) < (b);
#define IF_NEQ_RETURN_IS_GREATER(a, b) \
    if ((a) != (b)) return (a) > (b);

#endif /* BF_P4C_PHV_V2_SORT_MACROS_H_ */
