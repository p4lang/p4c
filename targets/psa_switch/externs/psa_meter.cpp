/* Copyright 2020-present Cornell University
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
 * Yunhe Liu (yunheliu@cs.cornell.edu)
 *
 */

#include "psa_meter.h"
#include <iostream>

namespace bm {

namespace psa {

void
PSA_Meter::init() {
    bm::MeterArray::MeterType meter_type;
    if (type == "bytes") {
        meter_type = bm::MeterArray::MeterType::BYTES;
    } else {
        meter_type = bm::MeterArray::MeterType::PACKETS;
    }
    _meter = std::unique_ptr<MeterArray>(
        new MeterArray(get_name() + ".$impl",
                         spec_id,
                         meter_type,
                         rate_count.get<size_t>(),
                         n_meters.get<size_t>()));
}

void
PSA_Meter::execute(const Data &index, Data &value) {
    auto color_out = _meter->execute_meter(get_packet(), index.get<size_t>(), static_cast<unsigned int>(0));

    // color adjustment for PSA:
    // bmv2 meter implementation assign higher value for busier flow:
    // (bmv2-meter) GREEN = 0, YELLOW = 1, RED = 2.
    // PSA specification order enums differently:
    // (see p4c/p4include/psa.p4)
    // (PSA-specification) RED = 0, GREEN = 1, YELLOW = 2.
    // The following code maps color_out (bmv2-meter) to
    // psa_color_out (PSA-specification).
    bm::Meter::color_t psa_color_out = 1;
    switch(color_out) {
        case 0 :
            psa_color_out = 1;
            break;
        case 1 :
            psa_color_out = 2;
            break;
        case 2 :
            psa_color_out = 0;
            break;
    }

    value.set(psa_color_out);
}

Meter &
PSA_Meter::get_meter(size_t idx) {
  return _meter->get_meter(idx);
}

const Meter &
PSA_Meter::get_meter(size_t idx) const {
  return _meter->get_meter(idx);
}

Meter::MeterErrorCode
PSA_Meter::set_rates(const std::vector<Meter::rate_config_t> &configs) {
    return _meter->set_rates(configs);
}

BM_REGISTER_EXTERN_W_NAME(Meter, PSA_Meter);
BM_REGISTER_EXTERN_W_NAME_METHOD(Meter, PSA_Meter, execute, const Data &, Data &);

}  // namespace bm::psa

}  // namespace bm

int import_meters(){
  return 0;
}
