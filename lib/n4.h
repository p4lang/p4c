#ifndef LIB_N4_H_
#define LIB_N4_H_

#include <iomanip>
#include <ostream>

class n4 {
    /* format a value as 4 chars */
    unsigned long val, div;

 public:
    explicit n4(unsigned long v, unsigned long d = 1) : val(v), div(d) {}
    std::ostream &print(std::ostream &os) const {
        unsigned long v;
        if (val % div && val < div * 100) {
            if ((v = val * 1000 + div/2) < div*1000) {
                char ofill = os.fill('0');
                os << '.' << std::setw(3) << v / div;
                os.fill(ofill);
                return os; }
            if ((v = val * 100 + div/2) < div*1000) {
                char ofill = os.fill('0');
                os << val/div << '.' << std::setw(2) << (v / div) % 100;
                os.fill(ofill);
                return os; }
            if ((v = val * 10 + div/2) < div*1000) {
                os << val/div << '.' << (v / div) % 10;
                return os; } }
        v = (val + div/2) / div;
        if (v < 10000) {
            os << std::setw(4) << v;
        } else if (v < 999500L) {
            os << std::setw(3) << (v + 500)/1000 << 'K';
        } else if (v < 9950000L) {
            v = (v + 50000L)/100000L;
            os << v/10 << '.' << v%10 << 'M';
        } else if (v < 999500000L) {
            os << std::setw(3) << (v + 500000L)/1000000L << 'M';
        } else if (v < 9950000000L) {
            v = (v + 50000000L)/100000000L;
            os << v/10 << '.' << v%10 << 'G';
        } else {
            os << std::setw(3) << (v + 500000000L)/1000000000L << 'G';
        }
        return os; }
};

inline std::ostream &operator<<(std::ostream &os, n4 v) {
    return v.print(os); }

#endif /* LIB_N4_H_ */
