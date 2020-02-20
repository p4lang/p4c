/*
Copyright 2019 Orange

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef ___constant_swab16
#define ___constant_swab16(x) ((uint16_t)(             \
    (((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |          \
    (((uint16_t)(x) & (uint16_t)0xff00U) >> 8)))
#endif

#ifndef ___constant_swab32
#define ___constant_swab32(x) ((uint32_t)(             \
    (((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |        \
    (((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |        \
    (((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |        \
    (((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))
#endif

#ifndef ___constant_swab64
#define ___constant_swab64(x) ((uint64_t)(             \
    (((uint64_t)(x) & (uint64_t)0x00000000000000ffULL) << 56) |   \
    (((uint64_t)(x) & (uint64_t)0x000000000000ff00ULL) << 40) |   \
    (((uint64_t)(x) & (uint64_t)0x0000000000ff0000ULL) << 24) |   \
    (((uint64_t)(x) & (uint64_t)0x00000000ff000000ULL) <<  8) |   \
    (((uint64_t)(x) & (uint64_t)0x000000ff00000000ULL) >>  8) |   \
    (((uint64_t)(x) & (uint64_t)0x0000ff0000000000ULL) >> 24) |   \
    (((uint64_t)(x) & (uint64_t)0x00ff000000000000ULL) >> 40) |   \
    (((uint64_t)(x) & (uint64_t)0xff00000000000000ULL) >> 56)))
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#ifndef __constant_htonll
#define __constant_htonll(x) (___constant_swab64((x)))
#endif

#ifndef __constant_ntohll
#define __constant_ntohll(x) (___constant_swab64((x)))
#endif

#define __constant_htonl(x) (___constant_swab32((x)))
#define __constant_ntohl(x) (___constant_swab32(x))
#define __constant_htons(x) (___constant_swab16((x)))
#define __constant_ntohs(x) ___constant_swab16((x))

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# warning "I never tested BIG_ENDIAN machine!"
#define __constant_htonll(x) (x)
#define __constant_ntohll(X) (x)
#define __constant_htonl(x) (x)
#define __constant_ntohl(x) (x)
#define __constant_htons(x) (x)
#define __constant_ntohs(x) (x)

#else
# error "Fix your compiler's __BYTE_ORDER__?!"
#endif

#undef htonl
#define htonl(d) __constant_htonl(d)
#undef htons
#define htons(d) __constant_htons(d)
#undef htonll
#define htonll(d) __constant_htonll(d)
#undef ntohl
#define ntohl(d) __constant_ntohl(d)
#undef ntohs
#define ntohs(d) __constant_ntohs(d)
#undef ntohll
#define ntohll(d) __constant_ntohll(d)

#define load_byte(data, b) (*(((uint8_t*)(data)) + (b)))
#define load_half(data, b) __constant_ntohs(*(uint16_t *)((uint8_t*)(data) + (b)))
#define load_word(data, b) __constant_ntohl(*(uint32_t *)((uint8_t*)(data) + (b)))
#define load_dword(data, b) __constant_ntohll(*(uint64_t *)((uint8_t*)(data) + (b)))