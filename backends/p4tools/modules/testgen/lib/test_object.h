#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_OBJECT_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_OBJECT_H_
#include <map>

#include "backends/p4tools/common/lib/model.h"
#include "lib/castable.h"
#include "lib/cstring.h"

namespace P4Tools::P4Testgen {

/* =========================================================================================
 *  Abstract Test Object Class
 * ========================================================================================= */

class TestObject : public ICastable {
 public:
    TestObject() = default;
    ~TestObject() override = default;
    TestObject(const TestObject &) = default;
    TestObject(TestObject &&) = default;
    TestObject &operator=(const TestObject &) = default;
    TestObject &operator=(TestObject &&) = default;

    /// @returns the string name of this particular test object.
    [[nodiscard]] virtual cstring getObjectName() const = 0;

    /// @returns a version of the test object where all expressions are resolved and symbolic
    /// variables are substituted according to the mapping present in the @param model.
    [[nodiscard]] virtual const TestObject *evaluate(const Model &model, bool doComplete) const = 0;

    DECLARE_TYPEINFO(TestObject);
};

/// A map of test objects.
using TestObjectMap = ordered_map<cstring, const TestObject *>;

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_OBJECT_H_ */
