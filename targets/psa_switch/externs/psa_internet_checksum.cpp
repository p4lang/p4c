/* Copyright 2021 SYRMIA LLC
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
 * Dusan Krdzic (dusan.krdzic@syrmia.com)
 *
 */


#include "psa_internet_checksum.h"
#include <bm/bm_sim/_assert.h>


namespace {

void
concatenate_fields(const std::vector<bm::Field> &fields, bm::Data &input) {
    uint16_t n_bits = 0;
    bm::Data field_shl;

    // Concatenate fields in one single data
    for (int i = fields.size() - 1; i >= 0; i--) {
        field_shl.shift_left(fields.at(i), n_bits);
        input.add(input, field_shl);
        n_bits += fields.at(i).get_nbits();
    }

    _BM_ASSERT(n_bits % 16 == 0);
}

uint16_t
ones_complement_sum(uint16_t x, uint16_t y) {
    uint32_t ret = (uint32_t) x + (uint32_t) y;
    if (ret >= 0x10000)
        ret++;

    return ret;
}

}  // namespace

namespace bm {

namespace psa {

void
PSA_InternetChecksum::init() {
    clear();
}

void
PSA_InternetChecksum::get(Field &dst) const {
    dst.set(static_cast<uint16_t>(~sum));
}

void
PSA_InternetChecksum::get_state(Field &dst) const {
    dst.set(sum);
}

void
PSA_InternetChecksum::get_verify(Field &dst, Field &equOp) const {
    dst.set(equOp.get<uint16_t>() == static_cast<uint16_t>(~sum));
}

void
PSA_InternetChecksum::set_state(const Data &src) {
    sum = src.get<uint16_t>();
}

void
PSA_InternetChecksum::clear() {
    sum = 0;
}

void
PSA_InternetChecksum::add(const std::vector<Field> &fields) {
    Data input(0);
    Data chunk;
    static const Data mask(0xffff);

    concatenate_fields(fields, input);

    while (!input.test_eq(0)) {
        chunk.bit_and(input, mask);
        uint16_t d = chunk.get<uint16_t>();
        sum = ones_complement_sum(sum, d);
        input.shift_right(input, 16);
    }
}

void
PSA_InternetChecksum::subtract(const std::vector<Field> &fields) {
    Data input(0);
    Data chunk;
    static const Data mask(0xffff);

    concatenate_fields(fields, input);

    while (!input.test_eq(0)) {
        chunk.bit_and(input, mask);
        uint16_t d = chunk.get<uint16_t>();
        sum = ones_complement_sum(sum, ~d);
        input.shift_right(input, 16);
    }
}


BM_REGISTER_EXTERN_W_NAME(InternetChecksum, PSA_InternetChecksum);
BM_REGISTER_EXTERN_W_NAME_METHOD(InternetChecksum, PSA_InternetChecksum, add, const std::vector<Field>);
BM_REGISTER_EXTERN_W_NAME_METHOD(InternetChecksum, PSA_InternetChecksum, subtract, const std::vector<Field>);
BM_REGISTER_EXTERN_W_NAME_METHOD(InternetChecksum, PSA_InternetChecksum, get_state, Field &);
BM_REGISTER_EXTERN_W_NAME_METHOD(InternetChecksum, PSA_InternetChecksum, set_state,const Data &);
BM_REGISTER_EXTERN_W_NAME_METHOD(InternetChecksum, PSA_InternetChecksum, get, Field &);
BM_REGISTER_EXTERN_W_NAME_METHOD(InternetChecksum, PSA_InternetChecksum, get_verify, Field &, Field &);
BM_REGISTER_EXTERN_W_NAME_METHOD(InternetChecksum, PSA_InternetChecksum, clear);

}  // namespace bm::psa

}  // namespace bm

int import_internet_checksum() {
    return 0;
}
