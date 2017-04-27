/* Copyright 2013-present Barefoot Networks, Inc.
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
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <bm/bm_sim/actions.h>
#include <bm/bm_sim/counters.h>
#include <bm/bm_sim/extern.h>
#include <bm/bm_sim/meters.h>
#include <bm/bm_sim/packet.h>

#include <string>
#include <thread>

using namespace bm;

class modify_field : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.set(d);
  }
};

REGISTER_PRIMITIVE(modify_field);

class drop : public ActionPrimitive<> {
  void operator ()() {
  }
};

REGISTER_PRIMITIVE(drop);

class add_to_field : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.add(f, d);
  }
};

REGISTER_PRIMITIVE(add_to_field);

class generate_digest : public ActionPrimitive<const Data &, const Data &> {
  void operator ()(const Data &receiver, const Data &learn_id) {
    // stub only
    (void)receiver;
    (void)learn_id;
  }
};

REGISTER_PRIMITIVE(generate_digest);

class execute_meter
  : public ActionPrimitive<MeterArray &, const Data &, Field &> {
  void operator ()(MeterArray &meter_array, const Data &idx, Field &dst) {
    dst.set(meter_array.execute_meter(get_packet(), idx.get_uint()));
  }
};

REGISTER_PRIMITIVE(execute_meter);

class count : public ActionPrimitive<CounterArray &, const Data &> {
  void operator ()(CounterArray &counter_array, const Data &idx) {
    counter_array.get_counter(idx.get_uint()).increment_counter(get_packet());
  }
};

REGISTER_PRIMITIVE(count);

class register_write
  : public ActionPrimitive<RegisterArray &, const Data &, const Data &> {
  void operator ()(RegisterArray &dst, const Data &idx, const Data &src) {
    dst[idx.get_uint()].set(src);
  }
};

REGISTER_PRIMITIVE(register_write);

class ignore_string : public ActionPrimitive<const std::string &> {
  void operator ()(const std::string &s) {
    (void)s;
  }
};

REGISTER_PRIMITIVE(ignore_string);

class RegisterSpin : public ActionPrimitive<RegisterArray &, const Data &> {
  void operator ()(RegisterArray &register_array, const Data &ts) {
    register_array.at(0).set(ts);
    std::this_thread::sleep_for(std::chrono::milliseconds(ts.get_uint()));
  }
};

REGISTER_PRIMITIVE(RegisterSpin);

// one dummy extern

class DummyExtern : public ExternType {
 public:
  BM_EXTERN_ATTRIBUTES { }
};
BM_REGISTER_EXTERN(DummyExtern);
