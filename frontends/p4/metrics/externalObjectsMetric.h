#ifndef FRONTENDS_P4_EXTERNAL_OBJECTS_H_
#define FRONTENDS_P4_EXTERNAL_OBJECTS_H_

#include "../ir/ir.h"
#include "metrics.h"

namespace P4 {

class ExternalObjectsMetricPass : public Inspector {
 public:
    ExternalObjectsMetricPass() { setName("ExternalObjectsMetricPass"); }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_EXTERNAL_OBJECTS_H_ */
