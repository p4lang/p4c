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

#ifndef PSA_SWITCH_PSA_METER_H_
#define PSA_SWITCH_PSA_METER_H_

#include <bm/bm_sim/extern.h>
#include <bm/bm_sim/meters.h>

namespace bm {

namespace psa {

class PSA_Meter : public bm::ExternType {
 public:
  static constexpr p4object_id_t spec_id = 0xfffffffe;

  BM_EXTERN_ATTRIBUTES {
    BM_EXTERN_ATTRIBUTE_ADD(n_meters);
    BM_EXTERN_ATTRIBUTE_ADD(type);
    BM_EXTERN_ATTRIBUTE_ADD(is_direct);
    BM_EXTERN_ATTRIBUTE_ADD(rate_count);
  }

  void init() override;

  void execute(const Data &index, Data &value);

  Meter &get_meter(size_t idx);

  const Meter &get_meter(size_t idx) const;

  Meter::MeterErrorCode set_rates(const std::vector<Meter::rate_config_t> &configs);

  size_t size() const { return _meter->size(); };

 private:
  Data n_meters;
  std::string type;
  Data is_direct;
  Data rate_count;
  Data color;
  std::unique_ptr<MeterArray> _meter;
};

}  // namespace bm::psa

}  // namespace bm
#endif
