#ifndef P4C_LIB_LOG_H_
#define P4C_LIB_LOG_H_

#include <iostream>
#include <vector>

#ifndef __GNUC__
#define __attribute__(X)
#endif

static int _file_log_level __attribute__((unused)) = -1;

extern int get_file_log_level(const char *file, int *level);
extern void add_debug_spec(const char *spec);
extern int verbose;
class output_log_prefix {
    const char *fn;
    int level;
    friend std::ostream &operator<<(std::ostream &, const output_log_prefix &);
 public:
    output_log_prefix(const char *f, int l) : fn(f), level(l) {}
};

#ifdef __BASE_FILE__
#define LOGGING(L)                                                                      \
    ((L) <= (_file_log_level < 0 ? get_file_log_level(__BASE_FILE__, &_file_log_level)  \
                                 : _file_log_level))
#define LOG_PFX(L)      output_log_prefix(__BASE_FILE__, (L))
#else
#define LOGGING(L)                                                                      \
    ((L) <= (_file_log_level < 0 ? get_file_log_level(__FILE__, &_file_log_level)       \
                                     : _file_log_level))
#define LOG_PFX(L)      output_log_prefix(__FILE__, (L))
#endif

#define LOGN(N, X) (LOGGING(N) ? (std::clog << LOG_PFX(N) << X << std::endl) : std::clog)
#define LOG1(X) LOGN(1, X)
#define LOG2(X) LOGN(2, X)
#define LOG3(X) LOGN(3, X)
#define LOG4(X) LOGN(4, X)
#define LOG5(X) LOGN(5, X)

#define ERROR(X) (std::clog << "ERROR: " << X << std::endl)
#define WARNING(X) (verbose > 0 ? (std::clog << "WARNING: " << X << std::endl) : std::clog)
#define ERRWARN(C, X) ((C) ? ERROR(X) : WARNING(X))

template<class T> inline auto operator<<(std::ostream &out, const T &obj) ->
        decltype((void)obj.dbprint(out), out)
{ obj.dbprint(out); return out; }

template<class T> inline auto operator<<(std::ostream &out, const T *obj) ->
        decltype((void)obj->dbprint(out), out) {
    if (obj)
        obj->dbprint(out);
    else
        out << "<null>";
    return out; }

template<class T> std::ostream &operator<<(std::ostream &out, const std::vector<T> &vec) {
    const char *sep = " ";
    out << '[';
    for (auto &el : vec) {
        out << sep << el;
        sep = ", "; }
    out << (sep+1) << ']';
    return out; }

#endif /* P4C_LIB_LOG_H_ */
