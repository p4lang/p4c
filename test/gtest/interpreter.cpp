// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "midend/interpreter.h"

#include <gtest/gtest.h>

#include <climits>
#include <cstdint>

#include "ir/ir.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class SymbolicArrayTest : public P4CTest {};

TEST_F(SymbolicArrayTest, Shift) {
    TypeMap typeMap;
    const auto *bits = IR::Type_Bits::get(8);
    auto *header = new IR::Type_Header("H", {new IR::StructField("field", bits)});
    typeMap.setType(bits, new IR::Type_Type(bits));
    typeMap.setType(header, new IR::Type_Type(header));
    SymbolicValueFactory factory(&typeMap);

    for (int size : {3, 1, 0}) {
        auto *type = new IR::Type_Array(header, new IR::Constant(size));
        // Exercise every initial validity pattern, including invalid moved headers.
        for (int mask = 0; mask < (1 << size); mask++) {
            for (int amount : {-4, -3, -2, -1, 0, 1, 2, 3, 4, INT_MIN, INT_MAX}) {
                SCOPED_TRACE(::testing::Message()
                             << "size=" << size << " mask=" << mask << " amount=" << amount);
                SymbolicArray array(type, false, &factory);
                for (int i = 0; i < size; i++) {
                    auto *value = array.get(nullptr, i)->checkedTo<SymbolicHeader>();
                    value->setValid((mask & (1 << i)) != 0);
                    if (value->valid->value)
                        value->set("field"_cs, new SymbolicInteger(new IR::Constant(bits, i + 1)));
                }

                array.shift(amount);

                for (int i = 0; i < size; i++) {
                    // Use a wider signed type for an independent source-index oracle.
                    const auto source = static_cast<int64_t>(i) - amount;
                    const bool valid = source >= 0 && source < size && (mask & (1 << source));
                    auto *value = array.get(nullptr, i)->checkedTo<SymbolicHeader>();
                    ASSERT_TRUE(value->valid->isKnown());
                    ASSERT_EQ(value->valid->value, valid) << "index=" << i;
                    if (valid) {
                        auto *field = value->get(nullptr, "field"_cs)->checkedTo<SymbolicInteger>();
                        ASSERT_TRUE(field->isKnown());
                        EXPECT_EQ(field->constant->asInt(), source + 1) << "index=" << i;
                    }
                }
            }
        }
    }
}

}  // namespace P4::Test
