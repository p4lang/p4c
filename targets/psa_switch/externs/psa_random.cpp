/* Copyright 2020-present Cornell University
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
 * Yunhe Liu (yunheliu@cs.cornell.edu)
 *
 */

#include "psa_random.h"
#include <bm/bm_sim/logger.h>

#include <random>
#include <thread>
#include <iostream>

namespace bm {

namespace psa {

void
PSA_Random::init() {
  min_val = min.get_uint64();
  max_val = max.get_uint64();
  _BM_ASSERT((max_val > min_val) && "[Error] Random number range must be positive.");

  /* Note: Even though PSA spec mentioned range should be a power of 2 for
   * max portability, bmv2 does not impose this restriction.
   */
}

void
PSA_Random::read(Data &value) {
  using engine = std::default_random_engine;
  using hash = std::hash<std::thread::id>;
  static thread_local engine generator(hash()(std::this_thread::get_id()));
  using distrib64 = std::uniform_int_distribution<uint64_t>;
  distrib64 distribution(min_val, max_val);
  value.set(distribution(generator));
}

BM_REGISTER_EXTERN_W_NAME(Random, PSA_Random);
BM_REGISTER_EXTERN_W_NAME_METHOD(Random, PSA_Random, read, Data &);

}  // namespace bm::psa

}  // namespace bm

int import_random(){
  return 0;
}
