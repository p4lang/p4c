#include "backends/p4tools/testgen/lib/tf.h"

#include <boost/none.hpp>

namespace P4Tools {

namespace P4Testgen {

TF::TF(cstring testName, boost::optional<unsigned int> seed = boost::none)
    : testName(testName), seed(seed) {}

}  // namespace P4Testgen

}  // namespace P4Tools
