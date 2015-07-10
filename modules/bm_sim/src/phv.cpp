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

#include "bm_sim/phv.h"

void PHV::reset() {
  for(auto &h : headers)
    h.mark_invalid();
}

void PHV::reset_header_stacks() {
  for(auto &hs : header_stacks)
    hs.reset();
}

// so slow I want to die, but optional for a target...
// I need to find a better way of doing this
void PHV::reset_metadata() {
  for(auto &h : headers) {
    if(h.is_metadata())
      h.reset();
  }
}
