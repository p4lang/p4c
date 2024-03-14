#ifndef LIB_NETHASH_H_
#define LIB_NETHASH_H_

#include <cstddef>
#include <cstdint>
#include <vector>

/// A collection of hashing functions commonly used in network protocols.
namespace NetHash {

uint16_t crc16(const uint8_t *buf, size_t len);
uint32_t crc32(const uint8_t *buf, size_t len);
uint16_t crcCCITT(const uint8_t *buf, size_t len);
uint16_t csum16(const uint8_t *buf, size_t len);
uint16_t xor16(const uint8_t *buf, size_t len);
uint64_t identity(const uint8_t *buf, size_t len);

}  // namespace NetHash

#endif /* LIB_NETHASH_H_ */
