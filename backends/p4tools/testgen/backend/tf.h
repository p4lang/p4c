#ifndef TESTGEN_BACKEND_TF_H_
#define TESTGEN_BACKEND_TF_H_

#include <cstddef>

#include <boost/optional/optional.hpp>

#include "lib/cstring.h"

#include "backends/p4tools/testgen/lib/test_spec.h"

namespace P4Tools {

namespace P4Testgen {

/// Extracts information from the @testSpec to emit a TF test case.
class TF {
 protected:
    /// The @testName to be used in test case generation.
    const cstring testName;

    /// The seed used by the testgen.
    boost::optional<unsigned int> seed;

 public:
    /// Creates a generic test framework.
    explicit TF(cstring, boost::optional<unsigned int>);

    /// The method used to output the test case to be implemented by
    /// all the test frameworks (eg. STF, PTF, etc.).
    /// @param spec the testcase specification to be outputted
    /// @param selectedBranches string describing branches selected for this testcase
    /// @param testIdx testcase unique number identifier
    /// @param currentCoverage current coverage ratio (between 0.0 and 1.0)
    // attaches arbitrary string data to the test preamble.
    virtual void outputTest(const TestSpec* spec, cstring selectedBranches, size_t testIdx,
                            float currentCoverage) = 0;
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_BACKEND_TF_H_ */
