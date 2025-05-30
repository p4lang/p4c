#include <absl/base/log_severity.h>
#include <absl/log/internal/flags.h>

#include <iostream>
#include <ostream>  // For dummy operator<<
#include <string>

#include "absl/flags/flag.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "boost_format_helper.h"

// Dummy Custom P4 Type with operator<<
struct MyP4ObjectWithStream {
    std::string id = "ObjStream";

    friend std::ostream &operator<<(std::ostream &os, const MyP4ObjectWithStream &o) {
        return os << o.id;
    }
};

// --- Include Boost Multiprecision if testing it ---
#include <boost/multiprecision/cpp_int.hpp>

namespace P4::Util {
struct NiceFormat {
    int a, b, c;

    cstring toString() const {
        return "("_cs + Util::toString(this->a) + ","_cs + Util::toString(this->b) + ","_cs +
               Util::toString(this->c) + ")"_cs;
    }
};

}  // namespace P4::Util

// Handle const char* (this will also catch string literals)
void AbslStringify(absl::FormatSink& sink, const char* c_str) {
    if (c_str == nullptr) {
        // Decide how to represent nullptr. "(null)" is common.
        // Or sink.Append(absl::string_view{}); for an empty string.
        sink.Append("(null)");
    } else {
        // FormatSink::Append can efficiently handle const char*
        // (likely by creating a string_view internally).
        sink.Append(c_str);
    }
}

// Handle non-const char*
// This overload simply delegates to the const char* version.
// C++ allows the implicit conversion from char* to const char*.
// Technically, the const char* overload alone might be sufficient due
// to implicit conversion, but having both can sometimes help overload
// resolution clarity or catch specific non-const use cases if needed later.
// In most simple cases, the const char* overload is enough.
void AbslStringify(absl::FormatSink& sink, char* c_str) {
    // Just call the const version
    AbslStringify(sink, static_cast<const char*>(c_str));
}


int main() {
    // absl::SetFlag(&FLAGS_stderrthreshold, 0);
    absl::InitializeLog();

    int i = 123;
    double d = 456.789;
    const char *c_str = "C String";
    std::string std_str = "Std String";
    P4::cstring p4_str("P4 cstring");
    int *i_ptr = &i;
    void *v_ptr = &d;
    const char *null_c_str = nullptr;

    boost::multiprecision::cpp_int big_num = 12345678901234567890;
    std::cout << "--- Testing createFormattedMessage ---\n";

    std::cout << "HELLLO" << absl::StrFormat("Const C-String Var: %v", c_str) << std::endl;

    // 1. Basic types (handled by Abseil natively)
    std::cout << "1. Basic: " << P4::createFormattedMessage("Int: %1%, Double: %2%", i, d)
              << std::endl;

    // // 2. String types (const char*, std::string - native; P4::cstring - custom)
    std::cout << "2. Strings: " << P4::createFormattedMessage("Std: %1%, P4: %2%", std_str, p4_str)
              << std::endl;
    std::cout << "2. Strings: "
              << P4::createFormattedMessage("C: %1%, Std: %2%, P4: %3%", c_str, std_str, p4_str)
              << std::endl;

    // // 3. SourceInfo / SourcePosition (should use their specific AbslStringify)
    // //    NOTE: Using %v forces AbslStringify. Abseil doesn't know SourceInfo natively.
    P4::Util::SourcePosition sp(1, 1);
    P4::Util::SourceInfo si(P4::cstring("FILE"), 1, 1, P4::cstring("Func"));
    std::cout << "3. SourceInfo/Pos: " << P4::createFormattedMessage("SI: %1%", si) << std::endl;

    // P4::Util::NiceFormat nf{};
    // std::cout << "3. Nice: " << P4::createFormattedMessage("Nice: %1%", nf) << std::endl;

    // 4. Pointers (basic handled natively, custom via streamArgument deref)
    // std::cout << "4. Pointers: "
    //           << P4::createFormattedMessage("i*: %1%, obj*: %2%, void*: %3%", i_ptr, obj_ptr,
    //           v_ptr)
    //           << std::endl;

    // 5. Null pointers
    // std::cout << "5. Null Ptrs: " << P4::createFormattedMessage("C_NULL: %1%,", (void *)nullptr)
    //           << std::endl;

    // 6. Custom P4 Objects (toString and operator<<)
    // std::cout << "6. Custom Objs: "
    //           << P4::createFormattedMessage("ToString: %1%, Stream: %2%", obj_tostring,
    //           obj_stream)
    //           << std::endl;

    // 7. Boost Multiprecision (uses boost::multiprecision::AbslStringify)
    std::cout << "7. BigNum: " << P4::createFormattedMessage("Big: %1%", big_num) << std::endl;

    // // 8. Placeholder order and literals
    // std::cout << "8. Order/Literal: "
    //           << P4::createFormattedMessage("Second: %2%, First: %1%, Literal %% for %1%", i, d)
    //           << std::endl;

    // // 9. No args
    // std::cout << "9. No Args: " << P4::createFormattedMessage("Plain text.") << std::endl;

    // // 10. Malformed (test robustness of translator) - Expect literal %
    // std::cout << "10. Malformed: " << P4::createFormattedMessage("Malformed %1 %2%3 %abc%", 1, 2)
    //           << std::endl;

    // std::cout << "--- Test Complete ---\n";

    return 0;
}
