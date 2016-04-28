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

#ifndef BM_BM_SIM_PHV_SOURCE_H_
#define BM_BM_SIM_PHV_SOURCE_H_

#include <memory>

#include "phv.h"

namespace bm {

class PHVSourceIface {
 public:
  virtual ~PHVSourceIface() { }

  std::unique_ptr<PHV> get(size_t cxt) {
    return get_(cxt);
  }

  void release(size_t cxt, std::unique_ptr<PHV> phv) {
    release_(cxt, std::move(phv));
  }

  void set_phv_factory(size_t cxt, const PHVFactory *factory) {
    set_phv_factory_(cxt, factory);
  }

  size_t phvs_in_use(size_t cxt) {
    return phvs_in_use_(cxt);
  }

  static std::unique_ptr<PHVSourceIface> make_phv_source(size_t size = 1);

 private:
  virtual std::unique_ptr<PHV> get_(size_t cxt) = 0;

  virtual void release_(size_t cxt, std::unique_ptr<PHV> phv) = 0;

  virtual void set_phv_factory_(size_t cxt, const PHVFactory *factory) = 0;

  virtual size_t phvs_in_use_(size_t cxt) = 0;
};

}  // namespace bm

#endif  // BM_BM_SIM_PHV_SOURCE_H_
