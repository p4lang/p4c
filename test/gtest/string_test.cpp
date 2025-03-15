#include <gtest/gtest.h>

#include <string>

#include "lib/stringify.h"

namespace P4::Test {

TEST(StringCaseConversionTests, MixedCase) {
    std::string input = "HeLLo WoRLd!";

    std::string lowerInput = input;
    Util::lowerInPlace(lowerInput);
    EXPECT_EQ(lowerInput, "hello world!");

    EXPECT_EQ(Util::toLower(input), "hello world!");

    std::string upperInput = input;
    Util::upperInPlace(upperInput);
    EXPECT_EQ(upperInput, "HELLO WORLD!");

    EXPECT_EQ(Util::toUpper(input), "HELLO WORLD!");
}

TEST(StringCaseConversionTests, DigitsAndSpecialChars) {
    std::string input = "123ABC$%^";

    std::string lowerInput = input;
    Util::lowerInPlace(lowerInput);
    EXPECT_EQ(lowerInput, "123abc$%^");

    EXPECT_EQ(Util::toLower(input), "123abc$%^");

    std::string upperInput = input;
    Util::upperInPlace(upperInput);
    EXPECT_EQ(upperInput, "123ABC$%^");

    EXPECT_EQ(Util::toUpper(input), "123ABC$%^");
}

TEST(StringCaseConversionTests, AlreadyLowercase) {
    std::string input = "already lowercase";

    std::string lowerInput = input;
    Util::lowerInPlace(lowerInput);
    EXPECT_EQ(lowerInput, "already lowercase");

    EXPECT_EQ(Util::toLower(input), "already lowercase");

    Util::upperInPlace(input);
    EXPECT_EQ(input, "ALREADY LOWERCASE");

    EXPECT_EQ(Util::toUpper("already lowercase"), "ALREADY LOWERCASE");
}

TEST(StringCaseConversionTests, AlreadyUppercase) {
    std::string input = "ALREADY UPPERCASE";

    std::string lowerInput = input;
    Util::lowerInPlace(lowerInput);
    EXPECT_EQ(lowerInput, "already uppercase");

    EXPECT_EQ(Util::toLower(input), "already uppercase");

    Util::upperInPlace(input);
    EXPECT_EQ(input, "ALREADY UPPERCASE");

    EXPECT_EQ(Util::toUpper(input), "ALREADY UPPERCASE");
}

TEST(StringCaseConversionTests, EmptyString) {
    std::string input;

    Util::lowerInPlace(input);
    EXPECT_EQ(input, "");

    EXPECT_EQ(Util::toLower(""), "");

    Util::upperInPlace(input);
    EXPECT_EQ(input, "");

    EXPECT_EQ(Util::toUpper(""), "");
}

TEST(StringCaseConversionTests, SingleCharacter) {
    std::string input = "A";

    std::string lowerInput = input;
    Util::lowerInPlace(lowerInput);
    EXPECT_EQ(lowerInput, "a");

    EXPECT_EQ(Util::toLower(input), "a");

    std::string upperInput = "b";
    Util::upperInPlace(upperInput);
    EXPECT_EQ(upperInput, "B");

    EXPECT_EQ(Util::toUpper("b"), "B");
}

TEST(StringCaseConversionTests, WhitespaceString) {
    std::string input = "   HeLlO wOrLd   ";

    std::string lowerInput = input;
    Util::lowerInPlace(lowerInput);
    EXPECT_EQ(lowerInput, "   hello world   ");

    EXPECT_EQ(Util::toLower(input), "   hello world   ");

    std::string upperInput = input;
    Util::upperInPlace(upperInput);
    EXPECT_EQ(upperInput, "   HELLO WORLD   ");

    EXPECT_EQ(Util::toUpper(input), "   HELLO WORLD   ");
}

TEST(StringCaseConversionTests, UnicodeCharacters) {
    std::string input = "áÉíÓú";

    std::string lowerInput = input;
    Util::lowerInPlace(lowerInput);
    EXPECT_EQ(lowerInput, "áéíóú");

    EXPECT_EQ(Util::toLower(input), "áéíóú");

    std::string upperInput = input;
    Util::upperInPlace(upperInput);
    EXPECT_EQ(upperInput, "ÁÉÍÓÚ");

    EXPECT_EQ(Util::toUpper(input), "ÁÉÍÓÚ");
}

TEST(StringCaseConversionTests, SymbolsOnly) {
    std::string input = "@#$%^&*";

    std::string lowerInput = input;
    Util::lowerInPlace(lowerInput);
    EXPECT_EQ(lowerInput, "@#$%^&*");

    EXPECT_EQ(Util::toLower(input), "@#$%^&*");

    std::string upperInput = input;
    Util::upperInPlace(upperInput);
    EXPECT_EQ(upperInput, "@#$%^&*");

    EXPECT_EQ(Util::toUpper(input), "@#$%^&*");
}

TEST(StringCaseConversionTests, LongString) {
    std::string input = "ThisIsAVeryLongStringToTestTheEfficiencyOfTheFunctions";

    std::string lowerInput = input;
    Util::lowerInPlace(lowerInput);
    EXPECT_EQ(lowerInput, "thisisaverylongstringtotesttheefficiencyofthefunctions");

    EXPECT_EQ(Util::toLower(input), "thisisaverylongstringtotesttheefficiencyofthefunctions");

    std::string upperInput = input;
    Util::upperInPlace(upperInput);
    EXPECT_EQ(upperInput, "THISISAVERYLONGSTRINGTOTESTTHEEFFICIENCYOFTHEFUNCTIONS");

    EXPECT_EQ(Util::toUpper(input), "THISISAVERYLONGSTRINGTOTESTTHEEFFICIENCYOFTHEFUNCTIONS");
}

}  // namespace P4::Test
