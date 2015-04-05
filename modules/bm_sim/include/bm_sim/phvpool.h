#ifndef _BM_PHVPOOL_H_
#define _BM_PHVPOOL_H_

#include <vector>
#include <memory>

#include "bm_sim/phv.h"

class PHVPool {
public:
  PHVPool(size_t size);

  std::unique_ptr<PHV> get();

  void add(std::unique_ptr<PHV> phv);

private:
  size_t size;
  // std::mutex q_mutex;
  std::vector<std::unique_ptr<PHV> > phvs;
};

#endif
