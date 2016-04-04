#ifndef _P4_P4_PARSE_H_
#define _P4_P4_PARSE_H_

#include <memory>

namespace IR { class Global; }

const IR::P4Program *parse_p4v1_2_file(const char *name, FILE *in);
void p4v1_2_error(int lineno, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void p4v1_2_warning(int lineno, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#endif /* _P4_P4_PARSE_H_ */
