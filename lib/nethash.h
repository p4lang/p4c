#ifndef LIB_NETHASH_H_
#define LIB_NETHASH_H_

#include <cstddef>
#include <cstdint>
#include <vector>

/// A collection of hashing functions commonly used in network protocols.
namespace NetHash {

/// CRC-16/CRC-16-IBM/CRC-16-ANSI (parameters: bit-reflection, polynomial 0x8005, init = 0 and
/// xor_out = 0).
uint16_t crc16(const uint8_t *buf, size_t len);

/// CRC16-CCITT (parameters: NO bit-reflection, polynomial 0x1021, init = ~0 and xor_out = 0).
uint16_t crcCCITT(const uint8_t *buf, size_t len);

/// CRC-32 (parameters: bit-reflection, polynomial 0x04C11DB7, init = ~0 and xor_out = ~0).
uint32_t crc32(const uint8_t *buf, size_t len);

/// 16-bit ones' complement checksum (used in IP, TCP, UDP, ...).
uint16_t csum16(const uint8_t *buf, size_t len);

uint16_t xor16(const uint8_t *buf, size_t len);
uint64_t identity(const uint8_t *buf, size_t len);

}  // namespace NetHash

#endif /* LIB_NETHASH_H_ */
