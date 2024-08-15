#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_WORDLIST_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_WORDLIST_H_
#include <array>
#include <cstddef>

#define WORDLIST_LENGTH 10000

namespace P4::P4Tools::P4Smith {

/// This class is a wrapper around an underlying array of words, which is currently being
/// used to aid random name generation.
class Wordlist {
 public:
    Wordlist() = default;

    ~Wordlist() = default;

    /// Pops and @returns the top-most(closest to the beginning of the array) non-popped
    /// element from the array.
    static const char *getFromWordlist();

 private:
    /// Stores the address of the next word to be popped of the words array
    static std::size_t counter;

    /// The actual array storing the words.
    static const std::array<const char *, WORDLIST_LENGTH> WORDS;
};

}  // namespace P4::P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_UTIL_WORDLIST_H_ */
