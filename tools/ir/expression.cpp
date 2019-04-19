/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include "ir.h"
#include "dbprint.h"
#include "lib/gmputil.h"

const IR::Expression *IR::Slice::make(const IR::Expression *e, unsigned lo, unsigned hi) {
    if (auto k = e->to<IR::Constant>()) {
        auto rv = ((*k >> lo) & IR::Constant((1U << (hi-lo+1)) - 1)).clone();
        rv->type = IR::Type::Bits::get(hi-lo+1);
        return rv; }
    if (auto src_width = (unsigned)e->type->width_bits()) {
        if (lo >= src_width)
            return new IR::Constant(IR::Type::Bits::get(hi-lo+1), 0);
        if (hi >= src_width)
            hi = src_width - 1;
        if (lo == 0 && hi == src_width - 1)
            return e; }
    if (auto sl = e->to<IR::Slice>()) {
        lo += sl->getL();
        hi += sl->getL();
        BUG_CHECK(lo >= sl->getL() && hi <= sl->getH(), "MakeSlice slice on slice type mismatch");
        e = sl->e0; }
    return new IR::Slice(e, hi, lo);
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
  while ((*field_iter)->name != member) {
      rv += (*field_iter)->type->width_bits();
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
    mpz_class mask = Util::mask(width);

    if (tb->isSigned) {
        mpz_class max = (one << (width - 1)) - 1;
        mpz_class min = -(one << (width - 1));
        if (value < min || value > max) {
            if (!noWarning)
                ::warning(ErrorType::WARN_OVERFLOW,
                          "%1%: signed value does not fit in %2% bits", this, width);
            LOG2("value=" << value << ", min=" << min <<
                 ", max=" << max << ", masked=" << (value & mask) <<
                 ", adj=" << ((value & mask) - (one << width)));
            value = value & mask;
            if (value > max)
                value -= (one << width);
        }
    } else {
        if (sgn(value) < 0) {
            if (!noWarning)
                ::warning(ErrorType::WARN_MISMATCH,
                          "%1%: negative value with unsigned type", this);
        } else if ((value & mask) != value) {
            if (!noWarning)
                ::warning(ErrorType::WARN_MISMATCH,
                          "%1%: value does not fit in %2% bits", this, width);
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
IR::Constant::operator|(const IR::Constant &c) const {
  mpz_class v;
  mpz_ior(v.get_mpz_t(), value.get_mpz_t(), c.value.get_mpz_t());
  return IR::Constant(v);
}

IR::Constant
IR::Constant::operator^(const IR::Constant &c) const {
  mpz_class v;
  mpz_xor(v.get_mpz_t(), value.get_mpz_t(), c.value.get_mpz_t());
  return IR::Constant(v);
}

IR::Constant
IR::Constant::operator-(const IR::Constant &c) const {
  mpz_class v;
  mpz_sub(v.get_mpz_t(), value.get_mpz_t(), c.value.get_mpz_t());
  return IR::Constant(v);
}

IR::Constant
IR::Constant::operator-() const {
  mpz_class v;
  mpz_neg(v.get_mpz_t(), value.get_mpz_t());
  return IR::Constant(v);
}

IR::Constant
IR::Constant::GetMask(unsigned width) {
  return (IR::Constant(1) << width) - IR::Constant(1);
}
