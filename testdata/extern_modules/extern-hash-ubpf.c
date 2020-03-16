#include <stdint.h>
/*
 * This file implements sample C extern function. It contains definition of the following C extern function:
 *
 * extern bit<16> compute_hash(in <bit<32> srcAddr, in bit<32> dstAddr)
 *
 */
struct data {
	uint32_t ipSrcAddr;
	uint32_t ipDstAddr;
};

/**
 * This is really simple (not verified) hash function generating 16-bit hash.
 * @param a 1st field
 * @param b 2nd field
 * @return 16-bit wide hashed value.
 */
uint16_t compute_hash(const uint32_t a, const uint32_t b)
{
	struct data s = {
			.ipSrcAddr = a,
			.ipDstAddr = b
	};
	unsigned char *to_hash = (unsigned char *) &s;

	int length = sizeof(s);
	unsigned char x;
	uint16_t crc = 0xFFFF;
	for (int i = length-1; i > 0; i--) {
		x = crc >> 8 ^ *to_hash++;
		x ^= x>>4;
		crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
	}
	return crc;
}

