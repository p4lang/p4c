#include "bm_sim/calculations.h"

#include "xxhash.h"

namespace hash {

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
T xxh64(const char *buf, size_t len) {
  return static_cast<T>(XXH64(buf, len, 0));
}

template unsigned int xxh64<unsigned int>(const char *, size_t);

}


