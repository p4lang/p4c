#ifndef _LIB_AZZERT_H_
#define _LIB_AZZERT_H_

[[noreturn]] void azzertionHasFailed (const char *pszFile, unsigned iLine, const char *pszFunction, const char *pszExpression, const char *pszMessage = nullptr);

#ifndef azzert
    #define azzert(condition) do { if (! (condition)) azzertionHasFailed (__FILE__, __LINE__, __func__, #condition); } while (0)
#endif

#ifndef azzert_msg
    #define azzert_msg(condition, msg) do { if (! (condition)) azzertionHasFailed (__FILE__, __LINE__, __func__, #condition, msg); } while (0)
#endif

#endif /* _LIB_ERROR_H_ */
