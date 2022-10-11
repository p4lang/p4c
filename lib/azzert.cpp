#include "azzert.h"

#include <iostream>

#include <cstdlib>

[[noreturn]] void azzertionHasFailed (const char *pszFile, unsigned iLine, const char *pszFunction, const char *pszExpression, const char *pszMessage)
{
    std::cout.flush ();
    std::cerr << "azzertionHasFailed (" << pszFile << ':' << iLine << " in `" << pszFunction << "`): `" << pszExpression << "`.\n";
    if (pszMessage) std::cerr << pszMessage << "\n";
    std::abort ();
}
