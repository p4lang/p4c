#include "gtest/gtest.h"
#include "test/gtest/helpers.h"
#include "gmock/gmock.h"

#include "frontends/p4/hierarchicalNames.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/optional.hpp>

namespace Test {

namespace {

boost::optional<FrontendTestCase>
createTestCase(const std::string& ingress) {
    auto source = P4_SOURCE(R"(
control CC();
package P(CC cc);
%INGRESS%
P(C()) main;
)");
    boost::replace_first(source, "%INGRESS%", ingress);
    return FrontendTestCase::create(source);
}

class P4C_NameDuplicated : public P4CTest {
    std::ostream* org;
 public:
    std::stringstream err;

    void SetUp() {
        org = BaseCompileContext::get().errorReporter().getOutputStream();
        BaseCompileContext::get().errorReporter().setOutputStream(&err);
    }

    void TearDown() {
        BaseCompileContext::get().errorReporter().setOutputStream(org);
        err.clear();
    }
};

TEST_F(P4C_NameDuplicated, LocalOk) {
    auto test = createTestCase(P4_SOURCE(R"(
control C2() {
    @name("a") action a2() {}
    apply { a2(); }
}
control C() {
    @name("a") action a1() {}
    apply { C2.apply(); a1(); }
})"));

    EXPECT_TRUE(test);
    EXPECT_EQ(0u, ::errorCount());
}

TEST_F(P4C_NameDuplicated, LocalDuplicated) {
    auto test = createTestCase(P4_SOURCE(R"(
control C() {
    @name("a") action a1() {}
    @name("a") action a2() {}
    apply { a1(); a2(); }
})"));

    EXPECT_FALSE(test);
    EXPECT_EQ(::errorCount(), 1u);
    EXPECT_THAT(err.str(), testing::HasSubstr(ERR_STR_DUPLICATED_NAME));
}

TEST_F(P4C_NameDuplicated, GlobalDuplicated) {
    auto test = createTestCase(P4_SOURCE(R"(
control C2() {
    @name(".a") action a2() {}
    apply { a2(); }
}
control C() {
    @name(".a") action a1() {}
    apply { C2.apply(); a1(); }
})"));

    EXPECT_FALSE(test);
    EXPECT_EQ(::errorCount(), 1u);
    EXPECT_THAT(err.str(), testing::HasSubstr(ERR_STR_DUPLICATED_NAME));
}

}

}  // namespace Test
