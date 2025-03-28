#ifndef FRONTENDS_P4_HEADER_MANIPULATION_METRICS_H_
#define FRONTENDS_P4_HEADER_MANIPULATION_METRICS_H_

#include <algorithm>
#include "../../common/resolveReferences/referenceMap.h"
#include "../ir/ir.h"
#include "../typeChecking/typeChecker.h"
#include "metricsStructure.h"

namespace P4 {

class HeaderManipulationMetricsPass : public Inspector {
 private:
    TypeMap* typeMap;
    Metrics& metrics;

    /// Helper function that calculates the total size (in bits) of a header.
    int getHeaderSize(const IR::Type_Header* header) const;

 public:
    HeaderManipulationMetricsPass(TypeMap* typeMap, Metrics& metrics)
        : typeMap(typeMap), metrics(metrics) { 
            setName("HeaderManipulationMetricsPass");
        }

    bool preorder(const IR::MethodCallStatement* mcs) override;
    void postorder(const IR::P4Program* program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_HEADER_MANIPULATION_METRICS_H_ */
