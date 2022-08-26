#ifndef MUTATE_OPTIONS_H_
#define MUTATE_OPTIONS_H_

#include "backends/p4tools/common/options.h"

namespace P4Tools {

class MutateOptions : public AbstractP4cToolOptions {
 public:
    static MutateOptions& get();

    const char* getIncludePath() override;

 private:
    MutateOptions();
};

}  // namespace P4Tools

#endif /* MUTATE_OPTIONS_H_ */
