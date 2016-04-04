#include "ir.h"
#include "dbprint.h"

cstring IR::NamedCond::unique_name() {
    static int unique_counter = 0;
    char buf[16];
    snprintf(buf, sizeof(buf), "cond-%d", unique_counter++);
    return buf;
}

int IR::Member::offset_bits() const {
  // This assumes little-endian number for bits.
  return lsb();
}

int IR::Member::lsb() const {
  int rv = 0;
  auto header_type = dynamic_cast<const IR::Type_StructLike *>(expr->type);
  auto field_iter = header_type->fields.rbegin();
  // This assumes little-endian number for bits.
  while (field_iter->second->name != member) {
    rv += field_iter->second->type->width_bits();
    if (++field_iter == header_type->fields.rend())
        BUG("No field %s in %s", member, expr->type); }
  return rv;
}

int IR::Member::msb() const {
  // This assumes little-endian number for bits.
  return lsb() + type->width_bits() - 1;
}

void
IR::Constant::handleOverflow(bool noWarning) {
    if (type == nullptr)
        BUG("%1%: Null type in typed constant", this);
    if (type->is<IR::Type_InfInt>())
        return;
    auto tb = type->to<IR::Type_Bits>();
    if (tb == nullptr) {
        BUG("%1%: Unexpected type for constant %2%", this, type);
        return;
    }

    int width = tb->size;
    mpz_class one = 1;
    mpz_class mask = (one << width) - 1;

    if (tb->isSigned) {
        mpz_class max = (one << (width - 1)) - 1;
        mpz_class min = -(one << (width - 1));
        if (value < min || value > max) {
            if (!noWarning)
                ::warning("%1%: signed value does not fit in %2% bits", this, width);
            LOG1("value=" << value << ", min=" << min <<
                 ", max=" << max << ", masked=" << (value & mask) <<
                 ", adj=" << ((value & mask)- (one << width)));
            value = value & mask;
            if (value > max)
                value -= (one << width);
        }
    } else {
        if (sgn(value) < 0) {
            if (!noWarning)
                ::warning("%1%: negative value with unsigned type", this);
        } else if ((value & mask) != value) {
            if (!noWarning)
                ::warning("%1%: value does not fit in %2% bits", this, width);
        }

        value = value & mask;
        if (sgn(value) < 0)
            BUG("Negative value after masking %1%", value);
    }
}

IR::Constant
IR::Constant::operator<<(const unsigned &shift) const {
  mpz_class v;
  mpz_mul_2exp(v.get_mpz_t(), value.get_mpz_t(), shift);
  return IR::Constant(v);
}

IR::Constant
IR::Constant::operator>>(const unsigned &shift) const {
  mpz_class v;
  mpz_div_2exp(v.get_mpz_t(), value.get_mpz_t(), shift);
  return IR::Constant(v);
}

IR::Constant
IR::Constant::operator&(const IR::Constant &c) const {
  mpz_class v;
  mpz_and(v.get_mpz_t(), value.get_mpz_t(), c.value.get_mpz_t());
  return IR::Constant(v);
}

IR::Constant
IR::Constant::operator-(const IR::Constant &c) const {
  mpz_class v;
  mpz_sub(v.get_mpz_t(), value.get_mpz_t(), c.value.get_mpz_t());
  return IR::Constant(v);
}

IR::Constant
IR::Constant::GetMask(unsigned width) {
  return (IR::Constant(1) << width) - IR::Constant(1);
}

struct primitive_info_t {
    unsigned    min_operands, max_operands;
    unsigned    out_operands;  // bitset -- 1 bit per operand
    unsigned    type_match_operands;    // bitset -- 1 bit per operand
};

static const std::map<cstring, primitive_info_t> prim_info = {
    { "add",                    { 3, 3, 0x1, 0x7 } },
    { "add_header",             { 1, 1, 0x1, 0x0 } },
    { "add_to_field",           { 2, 2, 0x1, 0x3 } },
    { "bit_and",                { 3, 3, 0x1, 0x7 } },
    { "bit_andca",              { 3, 3, 0x1, 0x7 } },
    { "bit_andcb",              { 3, 3, 0x1, 0x7 } },
    { "bit_nand",               { 3, 3, 0x1, 0x7 } },
    { "bit_nor",                { 3, 3, 0x1, 0x7 } },
    { "bit_not",                { 2, 2, 0x1, 0x3 } },
    { "bit_or",                 { 3, 3, 0x1, 0x7 } },
    { "bit_orca",               { 3, 3, 0x1, 0x7 } },
    { "bit_orcb",               { 3, 3, 0x1, 0x7 } },
    { "bit_xnor",               { 3, 3, 0x1, 0x7 } },
    { "bit_xor",                { 3, 3, 0x1, 0x7 } },
    { "clone_egress_pkt_to_egress",     { 1, 2, 0x0, 0x0 } },
    { "clone_ingress_pkt_to_egress",    { 1, 2, 0x0, 0x0 } },
    { "copy_header",            { 2, 2, 0x1, 0x3 } },
    { "copy_to_cpu",            { 1, 1, 0x0, 0x0 } },
    { "count",                  { 2, 2, 0x1, 0x0 } },
    { "drop",                   { 0, 0, 0x0, 0x0 } },
    { "execute_meter",          { 3, 4, 0x1, 0x0 } },
    { "execute_stateful_alu",   { 1, 1, 0x0, 0x0 } },
    { "generate_digest",        { 2, 2, 0x0, 0x0 } },
    { "max",                    { 3, 3, 0x1, 0x7 } },
    { "min",                    { 3, 3, 0x1, 0x7 } },
    { "modify_field",           { 2, 3, 0x1, 0x7 } },
    { "modify_field_from_rng",  { 2, 3, 0x1, 0x5 } },
    { "modify_field_with_hash_based_offset",    { 4, 4, 0x1, 0x0 } },
    { "no_op",                  { 0, 0, 0x0, 0x0 } },
    { "pop",                    { 2, 2, 0x1, 0x0 } },
    { "push",                   { 2, 2, 0x1, 0x0 } },
    { "recirculate",            { 1, 1, 0x0, 0x0 } },
    { "register_read",          { 3, 3, 0x1, 0x0 } },
    { "register_write",         { 3, 3, 0x0, 0x0 } },
    { "remove_header",          { 1, 1, 0x1, 0x0 } },
    { "resubmit",               { 1, 1, 0x0, 0x0 } },
    { "shift_left",             { 3, 3, 0x1, 0x3 } },
    { "shift_right",            { 3, 3, 0x1, 0x3 } },
    { "subtract",               { 3, 3, 0x1, 0x7 } },
    { "subtract_from_field",    { 2, 2, 0x1, 0x3 } },
    { "valid",                  { 1, 1, 0x0, 0x0 } },
};

void IR::Primitive::typecheck() const {
    if (prim_info.count(name)) {
        auto &info = prim_info.at(name);
        if (operands.size() < info.min_operands)
            error("%s: not enough operands for primitive %s", srcInfo, name);
        if (operands.size() > info.max_operands)
            error("%s: too many operands for primitive %s", srcInfo, name);
    } else {
        error("%s: unknown primitive %s", srcInfo, name); }
}

bool IR::Primitive::isOutput(int operand_index) const {
    if (prim_info.count(name))
        return (prim_info.at(name).out_operands >> (operand_index-1)) & 1;
    return false;
}

unsigned IR::Primitive::inferOperandTypes() const {
    if (prim_info.count(name))
        return prim_info.at(name).type_match_operands;
    return 0;
}
