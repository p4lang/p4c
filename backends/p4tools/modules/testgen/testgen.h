#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TESTGEN_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TESTGEN_H_

#include "backends/p4tools/common/p4ctool.h"

#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

/// This is main implementation of the P4Testgen tool.
class Testgen : public AbstractP4cTool<TestgenOptions> {
 protected:
    void registerTarget() override;

    int mainImpl(const CompilerResult &compilerResult) override;

 public:
    /// Invokes P4Testgen and returns a list of abstract tests which are generated based on the
    /// input TestgenOptions. The abstract tests can be further specialized depending on the select
    /// test back end. CompilerOptions is required to invoke the correct preprocessor and P4
    /// compiler. It is assumed that `.file` in the compiler options is set.
    static std::optional<AbstractTestList> generateTests(const CompilerOptions &options,
                                                         const TestgenOptions &testgenOptions);
    /// Invokes P4Testgen and returns a list of abstract tests which are generated based on the
    /// input TestgenOptions. The abstract tests can be further specialized depending on the select
    /// test back end. CompilerOptions is required to invoke the correct P4 compiler. This function
    /// assumes that @param program is already preprocessed. P4Testgen will directly parse the input
    /// program.
    static std::optional<AbstractTestList> generateTests(std::string_view program,
                                                         const CompilerOptions &options,
                                                         const TestgenOptions &testgenOptions);

    /// Invokes P4Testgen and writes a list of abstract tests to a specified output directory which
    /// are generated based on the input TestgenOptions. The abstract tests can be further
    /// specialized depending on the select test back end. CompilerOptions is required to invoke the
    /// correct preprocessor and P4 compiler. It is assumed that `.file` in the compiler options is
    /// set.
    static int writeTests(const CompilerOptions &options, const TestgenOptions &testgenOptions);

    /// Invokes P4Testgen and writes a list of abstract tests to a specified output directory which
    /// are generated based on the input TestgenOptions. CompilerOptions is required to invoke the
    /// correct P4 compiler. This function assumes that @param program is already preprocessed.
    /// P4Testgen will directly parse the input program.
    static int writeTests(std::string_view program, const CompilerOptions &options,
                          const TestgenOptions &testgenOptions);

    virtual ~Testgen() = default;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TESTGEN_H_ */
