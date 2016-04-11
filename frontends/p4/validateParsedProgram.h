#ifndef _P4_VALIDATEPARSEDPROGRAM_H_
#define _P4_VALIDATEPARSEDPROGRAM_H_

#include "ir/ir.h"

namespace P4 {

/* Run immediately after parsing.
   There is no type information. */
class ValidateParsedProgram final : public Inspector {
    using Inspector::postorder;

    void postorder(const IR::Constant* c) override;
    void postorder(const IR::Method* t) override;
    void postorder(const IR::StructField* f) override;
    void postorder(const IR::ParserState* s) override;
    void postorder(const IR::P4Table* t) override;
    void postorder(const IR::Type_Union* type) override;
    void postorder(const IR::Type_Bits* type) override;
};

}  // namespace P4

#endif /* _P4_VALIDATEPARSEDPROGRAM_H_ */
