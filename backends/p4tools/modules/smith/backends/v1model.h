#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_BACKENDS_V1MODEL_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_BACKENDS_V1MODEL_H_

#include "ir/ir.h"

namespace P4Tools {

namespace P4Smith {

class V1Model {
 public:
    V1Model() {}
    ~V1Model() {}

    static void generate_includes(std::ostream *ostream);
    static IR::P4Program *gen();

 private:
    static void set_probabilities();
    static IR::P4Parser *gen_p();
    static IR::P4Control *gen_egress();
    static IR::P4Control *gen_update();
    static IR::P4Control *gen_vrfy();
    static IR::P4Control *gen_ingress();
    static IR::P4Control *gen_deparser();
    static IR::Type_Struct *gen_meta();
    static IR::Type_Struct *gen_standard_metadata_t();
    static IR::Declaration_Instance *gen_main();
    static IR::MethodCallStatement *gen_deparser_emit_call();
};
}  // namespace P4Smith

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_BACKENDS_V1MODEL_H_ */
