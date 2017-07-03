/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_BM_SIM__ASSERT_H_
#define BM_BM_SIM__ASSERT_H_

// An assert that cannot be removed with NDEBUG

namespace bm {

[[ noreturn ]] void _assert(const char* expr, const char* file, int line);

}  // namespace bm

#define _BM_ASSERT(expr) \
  ((expr) ? (void)0 : bm::_assert(#expr, __FILE__, __LINE__))

#define _BM_UNREACHABLE(msg) bm::_assert(msg, __FILE__, __LINE__)

#define _BM_UNUSED(x) ((void)x)

#endif  // BM_BM_SIM__ASSERT_H_
