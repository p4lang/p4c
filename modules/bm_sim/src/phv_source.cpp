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

#include <vector>
#include <mutex>

#include "bm_sim/phv_source.h"

namespace bm {

class PHVSourceContextPools : public PHVSourceIface {
 public:
  explicit PHVSourceContextPools(size_t size)
      : phv_pools(size) { }

 private:
  class PHVPool {
   public:
    void set_phv_factory(const PHVFactory *factory) {
      std::unique_lock<std::mutex> lock(mutex);
      phv_factory = factory;
    }

    std::unique_ptr<PHV> get() {
      std::unique_lock<std::mutex> lock(mutex);
      if (phvs.size() == 0) {
        lock.unlock();
        return phv_factory->create();
      }
      std::unique_ptr<PHV> phv = std::move(phvs.back());
      phvs.pop_back();
      return phv;
    }

    void release(std::unique_ptr<PHV> phv) {
      std::unique_lock<std::mutex> lock(mutex);
      phvs.push_back(std::move(phv));
    }

   private:
    mutable std::mutex mutex{};
    std::vector<std::unique_ptr<PHV> > phvs{};
    const PHVFactory *phv_factory{nullptr};
  };

  std::unique_ptr<PHV> get_(size_t cxt) override {
    return phv_pools.at(cxt).get();
  }

  void release_(size_t cxt, std::unique_ptr<PHV> phv) override {
    return phv_pools.at(cxt).release(std::move(phv));
  }

  void set_phv_factory_(size_t cxt, const PHVFactory *factory) override {
    phv_pools.at(cxt).set_phv_factory(factory);
  }

  std::vector<PHVPool> phv_pools;
};

std::unique_ptr<PHVSourceIface>
PHVSourceIface::make_phv_source(size_t size) {
  return std::unique_ptr<PHVSourceContextPools>(
      new PHVSourceContextPools(size));
}

}  // namespace bm
