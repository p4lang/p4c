#ifndef _P4_VALIDATEPARSEDPROGRAM_H_
#define _P4_VALIDATEPARSEDPROGRAM_H_

#include "ir/ir.h"

namespace P4 {

/* Run immediately after parsing.
   There is no type information. */
class ValidateParsedProgram final : public Inspector {
    bool p4v1;
    using Inspector::postorder;

 public:
    ValidateParsedProgram(bool p4v1) : p4v1(p4v1) {}
    void postorder(const IR::Constant* c) override;
    void postorder(const IR::Method* t) override;
    void postorder(const IR::StructField* f) override;
    void postorder(const IR::ParserState* s) override;
    void postorder(const IR::P4Table* t) override;
    void postorder(const IR::Type_Union* type) override;
    void postorder(const IR::Type_Bits* type) override;
    void postorder(const IR::ConstructorCallExpression* expression) override;
};

}  // namespace P4

#endif /* _P4_VALIDATEPARSEDPROGRAM_H_ */
