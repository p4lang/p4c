#include "bm_sim/phvpool.h"

std::unique_ptr<PHV>
PHVPool::get() {
  std::unique_ptr<PHV> phv = std::move(phvs.back());
  phvs.pop_back();
  return phv;
}

void
PHVPool::add(std::unique_ptr<PHV> phv) {
  phvs.push_back(std::move(phv));
}

PHVPool::PHVPool(size_t size)
  : size(size) {
  phvs.reserve(size);
}
