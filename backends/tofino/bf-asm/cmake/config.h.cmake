#ifndef __BFASM_CONFIG_H__
#define __BFASM_CONFIG_H__

/* Define to 1 if you have the execinfo.h header */
#cmakedefine HAVE_EXECINFO_H @HAVE_EXECINFO_H@

/* Define to 1 if you have the ucontext.h header */
#cmakedefine HAVE_UCONTEXT_H @HAVE_UCONTEXT_H@

/* Define to 1 if we include JBay */
#cmakedefine HAVE_JBAY @HAVE_JBAY@

/* Define to 1 if we include Tofino */
#cmakedefine HAVE_TOFINO @HAVE_TOFINO@

/* Schema version */
#cmakedefine CONTEXT_SCHEMA_VERSION "@CONTEXT_SCHEMA_VERSION@"

/* define the version */
#cmakedefine TFAS_VERSION "@BFN_P4C_VERSION@"


#endif // __BFASM_CONFIG_H__
