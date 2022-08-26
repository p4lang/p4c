#ifndef SMITH_OPTIONS_H_
#define SMITH_OPTIONS_H_

#include "backends/p4tools/common/options.h"

namespace P4Tools {

class SmithOptions : public AbstractP4cToolOptions {
 public:
    static SmithOptions& get();

    const char* getIncludePath() override;

 private:
    SmithOptions();
};

}  // namespace P4Tools

#endif /* SMITH_OPTIONS_H_ */
