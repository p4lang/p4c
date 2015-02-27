#include "behavioral_sim/phv.h"

void PHV::reset() {
  for(std::vector<Header>::iterator it = headers.begin();
      it != headers.end();
      it++) {
    (*it).mark_invalid();
  }
}
