#ifndef FRONTENDS_P4_NESTING_DEPTH_H_
#define FRONTENDS_P4_NESTING_DEPTH_H_

#include "../ir/ir.h"
#include "metricsStructure.h"

namespace P4 {

class NestingDepthMetricPass : public Inspector {
 private:
    Metrics &metrics;
    unsigned currentDepth;
    unsigned currentMax;

 public:
    explicit NestingDepthMetricPass(Metrics &metricsRef)
        : metrics(metricsRef), currentDepth(0), currentMax(0) {
        setName("NestingDepthPass");
    }
    bool increment();

    bool preorder(const IR::P4Parser* /*parser*/) override;
    void postorder(const IR::P4Parser* parser) override;
    bool preorder(const IR::P4Control* /*control*/) override;
    void postorder(const IR::P4Control* control) override;
    bool preorder(const IR::Function* /*function*/) override;
    void postorder(const IR::Function* function) override;
    bool preorder(const IR::ParserState* /*state*/) override;
    void postorder(const IR::ParserState* /*state*/) override;
    bool preorder(const IR::SelectExpression* /*stmt*/) override;
    void postorder(const IR::SelectExpression* /*stmt*/) override;
    bool preorder(const IR::BlockStatement* /*stmt*/) override;
    void postorder(const IR::BlockStatement* /*stmt*/) override;

    /// Final calculation.
    void postorder(const IR::P4Program* /*program*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_NESTING_DEPTH_H_ */