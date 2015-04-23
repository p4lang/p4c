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

class drop : public ActionPrimitive<> {
  void operator ()() {
    get_field("standard_metadata.egress_port").set(0);
  }
};

REGISTER_PRIMITIVE(drop);

class generate_digest : public ActionPrimitive<const Data &, const Data &> {
  void operator ()(const Data &receiver, const Data &learn_id) {
    // discared receiver for now
    get_field("intrinsic_metadata.learn_id").set(learn_id.get_uint());
  }
};

REGISTER_PRIMITIVE(generate_digest);

#endif
