#ifndef FRONTENDS_P4_HEADER_METRICS_H_
#define FRONTENDS_P4_HEADER_METRICS_H_

#include "../ir/ir.h"
#include "metrics.h"

namespace P4 {

class HeaderMetricsPass : public Inspector {
public:
    HeaderMetricsPass() { setName("HeaderMetricsPass"); }
    // Collect metrics for each header
    bool preorder(const IR::Type_Header *header) override;
    // Calculate averages at the end of traversal
    void postorder([[maybe_unused]] const IR::P4Program *program) override;
private:
    unsigned totalFieldsNum = 0;
    unsigned totalFieldsSize = 0;
};

}  // namespace P4

#endif /* FRONTENDS_P4_HEADER_METRICS_H_ */
