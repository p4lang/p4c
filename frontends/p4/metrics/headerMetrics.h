#ifndef FRONTENDS_P4_HEADER_METRICS_H_
#define FRONTENDS_P4_HEADER_METRICS_H_

#include "../ir/ir.h"
#include "metricsStructure.h"
#include "../typeMap.h"

namespace P4 {

class HeaderMetricsPass : public Inspector {
 private:
   TypeMap* typeMap;  
   Metrics &metrics;
   unsigned totalFieldsNum = 0;
   unsigned totalFieldsSize = 0;
 public:
    explicit HeaderMetricsPass(TypeMap* typeMap, Metrics &metricsRef)
      : typeMap(typeMap), metrics(metricsRef) { setName("HeaderMetricsPass"); }
    /// Collect metrics for each header.
    bool preorder(const IR::Type_Header *header) override;
    /// Calculate averages at the end of traversal.
    void postorder(const IR::P4Program* /*program*/) override;

};

}  // namespace P4

#endif /* FRONTENDS_P4_HEADER_METRICS_H_ */
