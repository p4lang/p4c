#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "lib/boost_format_helper.h"
#include "lib/cstring.h"
#include "lib/source_file.h"

namespace P4::Test {

// Helper struct with operator<< and toString, but no dbprint
struct SimpleStreamable {
    int id;
    std::string name;

    friend std::ostream &operator<<(std::ostream &os, const SimpleStreamable &s) {
        return os << "SimpleStreamable<" << s.id << ", " << s.name << ">";
    }

    // Add toString for testing fallback in streamArgument if needed
    [[nodiscard]] std::string toString() const {
        return absl::StrCat("SimpleStreamable::toString<", id, ", ", name, ">");
    }
};

// Helper struct with only toString
struct ToStringOnly {
    std::string data = "ToStringOnlyData";
    [[nodiscard]] std::string toString() const { return data; }
};

// Helper struct with only operator<<
struct OperatorStreamOnly {
    std::string data = "OperatorStreamOnlyData";
    friend std::ostream &operator<<(std::ostream &os, const OperatorStreamOnly &o) {
        return os << o.data;
    }
};

// Helper struct with neither stream nor toString (relies on fallback/error)
struct Unstreamable {
    int value = 99;
};

// --- Test Suite ---
class FormatHelperTest : public ::testing::Test {
 protected:
    // Create some common test objects
    cstring testCstring = cstring("Test CString");
    cstring nullCstring = nullptr;
    std::string testStdstring = "Test std::string";
    int testInt = 123;
    double testDouble = 45.67;
    const char *testCStr = "Test C str";
    const char *nullCStr = nullptr;
    void *testPtr = reinterpret_cast<void *>(0xABCD);
    void *nullPtr = nullptr;
    P4::Util::SourceInfo testSourceInfo =
        P4::Util::SourceInfo(new Util::InputSources(), Util::SourcePosition(10, 5));

    SimpleStreamable simpleObj{1, "Obj1"};
    ToStringOnly toStringObj;
    OperatorStreamOnly opStreamObj;
    Unstreamable unstreamable_obj;
};

// --- Literal Style Tests ---

TEST_F(FormatHelperTest, LiteralEmpty) { EXPECT_EQ(P4::createFormattedMessage(""), ""); }

TEST_F(FormatHelperTest, LiteralSimple) {
    EXPECT_EQ(P4::createFormattedMessage("Hello, world!"), "Hello, world!");
}

TEST_F(FormatHelperTest, LiteralWithEscapedPercent) {
    EXPECT_EQ(P4::createFormattedMessage("A rate of 10%%"), "A rate of 10%");
}

TEST_F(FormatHelperTest, LiteralWithBraces) {
    // Braces are ignored by detection, treated as literal
    EXPECT_EQ(P4::createFormattedMessage("Braces {} here"), "Braces {} here");
}

TEST_F(FormatHelperTest, LiteralWithArgs) {
    // Args are ignored if format string is literal
    EXPECT_EQ(P4::createFormattedMessage("Literal", 1, "two"), "Literal");
}

TEST_F(FormatHelperTest, BoostSimple) {
    EXPECT_EQ(P4::createFormattedMessage("%1% and %2%", "one", 2), "one and 2");
}

TEST_F(FormatHelperTest, BoostReordered) {
    EXPECT_EQ(P4::createFormattedMessage("%2% then %1%", 10, "twenty"), "twenty then 10");
}

TEST_F(FormatHelperTest, BoostRepeated) {
    EXPECT_EQ(P4::createFormattedMessage("%1%, %1%, %2%", "A", "B"), "A, A, B");
}

TEST_F(FormatHelperTest, BoostCString) {
    EXPECT_EQ(P4::createFormattedMessage("Val: %1%", testCstring), "Val: Test CString");
}

TEST_F(FormatHelperTest, BoostNullCString) {
    // streamArgument uses operator<< for cstring, which outputs <null>
    EXPECT_EQ(P4::createFormattedMessage("Val: %1%", nullCstring), "Val: <null>");
}

TEST_F(FormatHelperTest, BoostStdString) {
    EXPECT_EQ(P4::createFormattedMessage("Val: %1%", testStdstring), "Val: Test std::string");
}

TEST_F(FormatHelperTest, BoostCStr) {
    EXPECT_EQ(P4::createFormattedMessage("Val: %1%", testCStr), "Val: Test C str");
}

TEST_F(FormatHelperTest, BoostNullCStr) {
    // streamArgument special cases nullptr char*
    EXPECT_EQ(P4::createFormattedMessage("Val: %1%", nullCStr), "Val: <nullptr>");
}

TEST_F(FormatHelperTest, BoostPointer) {
    // streamArgument dereferences non-char pointers
    EXPECT_EQ(P4::createFormattedMessage("Val: %1%", &testInt), "Val: 123");
}

TEST_F(FormatHelperTest, BoostNullPointer) {
    int *p = nullptr;
    EXPECT_EQ(P4::createFormattedMessage("Val: %1%", p), "Val: <nullptr>");
}

TEST_F(FormatHelperTest, BoostSourceInfoFiltered) {
    // SourceInfo should produce no output via streamArgument
    EXPECT_EQ(P4::createFormattedMessage("Start %1% End", testSourceInfo), "Start  End");
    EXPECT_EQ(P4::createFormattedMessage("SI: %1%, Int: %2%", testSourceInfo, 10), "SI: , Int: 10");
    EXPECT_EQ(P4::createFormattedMessage("Int: %1%, SI: %2%", 20, testSourceInfo), "Int: 20, SI: ");
    EXPECT_EQ(P4::createFormattedMessage("%1%", testSourceInfo), "");  // Only SI
}

TEST_F(FormatHelperTest, BoostInvalidIndexZero) {
    // Invalid N treated as literal % by formatBoostStyle
    EXPECT_EQ(P4::createFormattedMessage("Value %0% is bad", 1), "Value %0% is bad");
}

TEST_F(FormatHelperTest, BoostInvalidIndexHigh) {
    // Invalid N treated as literal % by formatBoostStyle
    EXPECT_EQ(P4::createFormattedMessage("Value %2% is bad", 1), "Value %2% is bad");
}

TEST_F(FormatHelperTest, BoostNoArgs) {
    // Invalid N treated as literal % by formatBoostStyle
    EXPECT_EQ(P4::createFormattedMessage("Value %1% is bad"), "Value %1% is bad");
}

TEST_F(FormatHelperTest, BoostIncompleteSpecifier) {
    // %N without closing % treated as literal %
    EXPECT_THROW(P4::createFormattedMessage("Value %1 is bad", 1), std::runtime_error);
}

TEST_F(FormatHelperTest, BoostEscapedPercent) {
    EXPECT_EQ(P4::createFormattedMessage("Rate %% %1%", 10), "Rate % 10");
}

TEST_F(FormatHelperTest, BoostCustomTypes) {
    // streamArgument uses toString before operator<< if available
    EXPECT_EQ(P4::createFormattedMessage("Obj: %1%", simpleObj),
              "Obj: SimpleStreamable::toString<1, Obj1>");
    // streamArgument uses toString if operator<< not available
    EXPECT_EQ(P4::createFormattedMessage("Obj: %1%", toStringObj), "Obj: ToStringOnlyData");
    // streamArgument uses operator<< if toString not available.
    EXPECT_EQ(P4::createFormattedMessage("Obj: %1%", opStreamObj), "Obj: OperatorStreamOnlyData");
}

TEST_F(FormatHelperTest, BoostUnstreamable) {
    // streamArgument throws for unknown types without toString or operator<<
    EXPECT_THROW(P4::createFormattedMessage("Obj: %1%", unstreamable_obj), std::runtime_error);
}

// --- Abseil Style (printf) Tests ---

TEST_F(FormatHelperTest, AbseilSimpleInt) {
    // Integral passed natively
    EXPECT_EQ(P4::createFormattedMessage("Value: %d", 123), "Value: 123");
}

TEST_F(FormatHelperTest, AbseilSimpleString) {
    // String is converted to string, passed to Abseil
    EXPECT_EQ(P4::createFormattedMessage("Name: %s", "test"), "Name: test");
}

TEST_F(FormatHelperTest, AbseilHex) {
    // Integral passed natively, Abseil applies %x
    EXPECT_EQ(P4::createFormattedMessage("Hex: %x", 255), "Hex: ff");
    EXPECT_EQ(P4::createFormattedMessage("Hex: %X", 255), "Hex: FF");
}

TEST_F(FormatHelperTest, AbseilOctal) {
    // Integral passed natively, Abseil applies %o
    EXPECT_EQ(P4::createFormattedMessage("Oct: %o", 63), "Oct: 77");
}

TEST_F(FormatHelperTest, AbseilPointer) {
    // Pointer is converted to string "<address>", passed to Abseil as %s (implicitly)
    // The formatAbslStyle implementation converts non-integrals to string.
    // Let's test %p specifically - it should get the stringified pointer.
    // std::stringstream ss;
    // ss << testPtr;  // Get expected default pointer output
    // std::string expectedPtrStr = ss.str();
    // EXPECT_EQ(P4::createFormattedMessage("Ptr: %p", testPtr), "Ptr: " + expectedPtrStr);
}

TEST_F(FormatHelperTest, AbseilNullPointer) {
    // Null pointer is converted to string "<nullptr>", passed as %s (implicitly)
    // EXPECT_EQ(P4::createFormattedMessage("Ptr: %p", nullPtr), "Ptr: <nullptr>");
}

TEST_F(FormatHelperTest, AbseilCString) {
    // cstring converted to string, passed as %s
    EXPECT_EQ(P4::createFormattedMessage("Val: %s", testCstring), "Val: Test CString");
}

TEST_F(FormatHelperTest, AbseilNullCString) {
    // null cstring converted to string "<null>", passed as %s
    EXPECT_EQ(P4::createFormattedMessage("Val: %s", nullCstring), "Val: <null>");
}

TEST_F(FormatHelperTest, AbseilStdString) {
    // std::string converted to string, passed as %s
    EXPECT_EQ(P4::createFormattedMessage("Val: %s", testStdstring), "Val: Test std::string");
}

TEST_F(FormatHelperTest, AbseilCStr) {
    // C string converted to string, passed as %s
    EXPECT_EQ(P4::createFormattedMessage("Val: %s", testCStr), "Val: Test C str");
}

TEST_F(FormatHelperTest, AbseilNullCStr) {
    // null C string converted to string "<nullptr>", passed as %s
    EXPECT_EQ(P4::createFormattedMessage("Val: %s", nullCStr), "Val: <nullptr>");
}

TEST_F(FormatHelperTest, AbseilFloat) {
    // Float is converted to string via streamArgument, passed as %s (implicitly)
    std::stringstream ss;
    ss << testDouble;
    std::string expectedDoubleStr = ss.str();
    EXPECT_EQ(P4::createFormattedMessage("Val: %f", testDouble), "Val: " + expectedDoubleStr);
    // Check that using %f still works (Abseil gets the string)
    EXPECT_EQ(P4::createFormattedMessage("Val: %f", testDouble), "Val: " + expectedDoubleStr);
}

TEST_F(FormatHelperTest, AbseilCustomTypesStringified) {
    // All these are converted to string via streamArgument before Abseil sees them
    EXPECT_EQ(P4::createFormattedMessage("Obj: %s", simpleObj),
              "Obj: SimpleStreamable::toString<1, Obj1>");
    EXPECT_EQ(P4::createFormattedMessage("Obj: %s", toStringObj), "Obj: ToStringOnlyData");
    EXPECT_EQ(P4::createFormattedMessage("Obj: %s", opStreamObj), "Obj: OperatorStreamOnlyData");
}

TEST_F(FormatHelperTest, AbseilSourceInfoFiltered) {
    // SourceInfo is converted to "" before Abseil sees it
    EXPECT_EQ(P4::createFormattedMessage("Start %s End", testSourceInfo), "Start  End");
    EXPECT_EQ(P4::createFormattedMessage("SI: %s, Int: %d", testSourceInfo, 10), "SI: , Int: 10");
    EXPECT_EQ(P4::createFormattedMessage("Int: %d, SI: %s", 20, testSourceInfo), "Int: 20, SI: ");
    EXPECT_EQ(P4::createFormattedMessage("%s", testSourceInfo), "");  // Only SI
}

TEST_F(FormatHelperTest, AbseilPositionalArgs) {
    // Native ints, stringified custom type
    EXPECT_EQ(P4::createFormattedMessage("%2$d %1$s %3$x", "ignore", 99, 10), "99 ignore a");
    EXPECT_EQ(P4::createFormattedMessage("%2$s %1$d", testInt, opStreamObj),
              "OperatorStreamOnlyData 123");
}

TEST_F(FormatHelperTest, AbseilFlagsWidthPrecision) {
    // Integrals passed natively should respect flags
    EXPECT_EQ(P4::createFormattedMessage("[%04d]", 7), "[0007]");
    EXPECT_EQ(P4::createFormattedMessage("[%-4d]", 7), "[7   ]");
    EXPECT_EQ(P4::createFormattedMessage("[%+d]", 7), "[+7]");
    // Floats are stringified, so precision likely won't work as expected via Abseil
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << 1.2345;
    // How streamArgument would likely format it by default might differ
    std::string expectedFloatStr = "1.2";
    EXPECT_EQ(P4::createFormattedMessage("[%.1f]", 1.2345), "[" + expectedFloatStr + "]");
}

TEST_F(FormatHelperTest, AbseilUnstreamable) {
    // Rendered to exception "<unstreamable:...>"
    EXPECT_THROW(P4::createFormattedMessage("Obj: %s", unstreamable_obj), std::runtime_error);
}

TEST_F(FormatHelperTest, AbseilRuntimeErrorArgMismatch) {
    // Abseil should throw if format specifier count mismatches arg count
    EXPECT_THROW(P4::createFormattedMessage("%d %s", 1), std::runtime_error);      // Too few args
    EXPECT_THROW(P4::createFormattedMessage("%d", 1, "two"), std::runtime_error);  // Too many args
}

TEST_F(FormatHelperTest, AbseilRuntimeErrorTypeMismatch) {
    // Pass integral, format as string (Abseil likely handles this)
    EXPECT_EQ(P4::createFormattedMessage("%s", 123), "123");
    // Pass stringified custom type, format as int (Abseil should fail)
    EXPECT_THROW(P4::createFormattedMessage("%d", opStreamObj), std::runtime_error);
    // Pass "" placeholder from SourceInfo, format as int (Abseil should fail)
    EXPECT_THROW(P4::createFormattedMessage("%d", testSourceInfo), std::runtime_error);
}

TEST_F(FormatHelperTest, AbseilLonePercent) {
    // Detected as Abseil style, Abseil should report error
    EXPECT_THROW(P4::createFormattedMessage("Test %"), std::runtime_error);
}

TEST_F(FormatHelperTest, UnknownMixedBoostAbseil) {
    EXPECT_THROW(P4::createFormattedMessage("%1% %s", 1, "a"), std::runtime_error);
    EXPECT_THROW(P4::createFormattedMessage("%d %1%", 1, "a"), std::runtime_error);
}

// Note: `{}` is not handled by detection, so mixing it won't trigger Unknown.
// It will be treated as literal characters within Boost or Abseil styles.
TEST_F(FormatHelperTest, MixedBraceAndBoost) {
    EXPECT_EQ(P4::createFormattedMessage("Test %1% {}", 1, 2), "Test 1 {}");
}
TEST_F(FormatHelperTest, MixedBraceAndAbseil) {
    EXPECT_EQ(P4::createFormattedMessage("Test %d {}", 1, 2), "Test 1 {}");
}

}  // namespace P4::Test
