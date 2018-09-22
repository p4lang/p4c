#ifndef LIB_N4_H_
#define LIB_N4_H_

#include <cstdint>
#include <iomanip>
#include <ostream>

class n4 {
    /* format a value as 4 chars */
    uint64_t val, div;

 public:
    explicit n4(uint64_t v, uint64_t d = 1) : val(v), div(d) {}
    std::ostream &print(std::ostream &os) const {
        uint64_t v;
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
        } else if (v < UINT64_C(999500)) {
            os << std::setw(3) << (v + 500)/1000 << 'K';
        } else if (v < UINT64_C(9950000)) {
            v = (v + UINT64_C(50000))/UINT64_C(100000);
            os << v/10 << '.' << v%10 << 'M';
        } else if (v < UINT64_C(999500000)) {
            os << std::setw(3) << (v + UINT64_C(500000))/UINT64_C(1000000) << 'M';
        } else if (v < UINT64_C(9950000000)) {
            v = (v + UINT64_C(50000000))/UINT64_C(100000000);
            os << v/10 << '.' << v%10 << 'G';
        } else {
            os << std::setw(3) << (v + UINT64_C(500000000))/UINT64_C(1000000000) << 'G';
        }
        return os; }
};

inline std::ostream &operator<<(std::ostream &os, n4 v) {
    return v.print(os); }

#endif /* LIB_N4_H_ */
