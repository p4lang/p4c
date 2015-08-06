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

#ifndef _BM_SIMPLE_SWITCH_PRIMITIVES_H_
#define _BM_SIMPLE_SWITCH_PRIMITIVES_H_

class modify_field : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.set(d);
  }
};

REGISTER_PRIMITIVE(modify_field);

class add_to_field : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.add(f, d);
  }
};

REGISTER_PRIMITIVE(add_to_field);

class subtract_from_field : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.sub(f, d);
  }
};

REGISTER_PRIMITIVE(subtract_from_field);

class add : public ActionPrimitive<Field &, const Data &, const Data &> {
  void operator ()(Field &f, const Data &d1, const Data &d2) {
    f.add(d1, d2);
  }
};

REGISTER_PRIMITIVE(add);

class bit_xor : public ActionPrimitive<Field &, const Data &, const Data &> {
  void operator ()(Field &f, const Data &d1, const Data &d2) {
    f.bit_xor(d1, d2);
  }
};

REGISTER_PRIMITIVE(bit_xor);

class bit_or : public ActionPrimitive<Field &, const Data &, const Data &> {
  void operator ()(Field &f, const Data &d1, const Data &d2) {
    f.bit_or(d1, d2);
  }
};

REGISTER_PRIMITIVE(bit_or);

class bit_and : public ActionPrimitive<Field &, const Data &, const Data &> {
  void operator ()(Field &f, const Data &d1, const Data &d2) {
    f.bit_and(d1, d2);
  }
};

REGISTER_PRIMITIVE(bit_and);

class drop : public ActionPrimitive<> {
  void operator ()() {
    get_field("standard_metadata.egress_spec").set(511);
  }
};

REGISTER_PRIMITIVE(drop);

class generate_digest : public ActionPrimitive<const Data &, const Data &> {
  void operator ()(const Data &receiver, const Data &learn_id) {
    // discared receiver for now
    get_field("intrinsic_metadata.lf_field_list").set(learn_id);
  }
};

REGISTER_PRIMITIVE(generate_digest);

class add_header : public ActionPrimitive<Header &> {
  void operator ()(Header &hdr) {
    // TODO: reset header to 0?
    hdr.mark_valid();
  }
};

REGISTER_PRIMITIVE(add_header);

class remove_header : public ActionPrimitive<Header &> {
  void operator ()(Header &hdr) {
    hdr.mark_invalid();
  }
};

REGISTER_PRIMITIVE(remove_header);

class copy_header : public ActionPrimitive<Header &, const Header &> {
  void operator ()(Header &dst, const Header &src) {
    if(!src.is_valid()) return;
    dst.mark_valid();
    assert(dst.get_header_type_id() == src.get_header_type_id());
    for(unsigned int i = 0; i < dst.size(); i++) {
      dst[i].set(src[i]);
    }
  }
};

REGISTER_PRIMITIVE(copy_header);

/* TODO: according to the P4 spec, this primitive can take a additional optional
   argument (field list), not supported for now */
class clone_ingress_pkt_to_egress : public ActionPrimitive<const Data &> {
  void operator ()(const Data &clone_spec) {
    get_field("standard_metadata.clone_spec").set(clone_spec);
  }
};

REGISTER_PRIMITIVE(clone_ingress_pkt_to_egress);

class modify_field_with_hash_based_offset
  : public ActionPrimitive<Field &, const Data &,
			   const NamedCalculation &, const Data &> {
  void operator ()(Field &dst, const Data &base,
		   const NamedCalculation &hash, const Data &size) {
    uint64_t v =
      (hash.output(get_packet()) + base.get<uint64_t>()) % size.get<uint64_t>();
    dst.set(v);
  }
};

REGISTER_PRIMITIVE(modify_field_with_hash_based_offset);

class no_op : public ActionPrimitive<> {
  void operator ()() {

  }
};

REGISTER_PRIMITIVE(no_op);

class execute_meter : public ActionPrimitive<Field &, MeterArray &, const Data &> {
  void operator ()(Field &dst, MeterArray &meter_array, const Data &idx) {
    dst.set(meter_array.execute_meter(get_packet(), idx.get_uint()));
  }
};

REGISTER_PRIMITIVE(execute_meter);

class push : public ActionPrimitive<HeaderStack &, const Data &> {
  void operator ()(HeaderStack &stack, const Data &num) {
    stack.push_front(num.get_uint());
  }
};

REGISTER_PRIMITIVE(push);

class pop : public ActionPrimitive<HeaderStack &, const Data &> {
  void operator ()(HeaderStack &stack, const Data &num) {
    stack.pop_front(num.get_uint());
  }
};

REGISTER_PRIMITIVE(pop);

#endif
