#include <core.p4>

header h1_t {
    bit<8> f1;
}

header h2_t {
    bit<8> f2;
}

header_union hu1_t {
    h1_t h1;
    h2_t h2;
}

struct s1_t {
    bit<8> f2;
}

enum enum1_t {
    A,
    B
}

enum bit<7> serializable_enum1_t {
    C = 3,
    D = 8
}

typedef int intD_t;
typedef bit<8> bD_t;
typedef int<8> iD_t;
typedef varbit<8> vD_t;
typedef error errorD_t;
typedef bool boolD_t;
typedef enum1_t enum1D_t;
typedef serializable_enum1_t serializable_enum1D_t;
typedef h1_t H1D_t;
typedef header h2a_t {
    bit<16> f2;
    bit<8>  f3;
}
H2aD_t;
typedef h1_t[3] h1StackD_t;
typedef hu1_t hu1D_t;
typedef s1_t s1D_t;
typedef struct PointA_t {
    int<32> x;
    int<32> y;
}
PointAD_t;
typedef tuple<bit<8>, bit<17>> Tuple1D_t;
typedef tuple<bit<8>, varbit<17>> Tuple2D_t;
type int intT_t;
type bit<8> bT_t;
type int<8> iT_t;
type varbit<8> vT_t;
type error errorT_t;
type bool boolT_t;
type enum1_t enum1T_t;
type serializable_enum1_t serializable_enum1T_t;
type h1_t H1T_t;
type header h2b_t {
    bit<16> f2;
    bit<8>  f3;
}
H2bT_t;
type h1_t[3] h1StackT_t;
type hu1_t hu1T_t;
type s1_t s1T_t;
type struct PointB_t {
    int<32> x;
    int<32> y;
}
PointBD_t;
type tuple<bit<8>, bit<17>> Tuple1T_t;
type tuple<bit<8>, varbit<17>> Tuple2T_t;
typedef bD_t bDD_t;
typedef bT_t bTD_t;
type bD_t bDT_t;
type bT_t bTT_t;
type iD_t iDT_t;
type vD_t vDT_t;
type errorD_t errorDT_t;
type boolD_t boolDT_t;
type enum1D_t enum1DT_t;
type serializable_enum1D_t serializable_enum1DT_t;
type H1D_t H1DT_t;
type H2aD_t H2aDT_t;
type h1StackD_t h1StackDT_t;
type hu1D_t hu1DT_t;
type s1D_t s1DT_t;
type PointAD_t PointADT_t;
type Tuple1D_t Tuple1DT_t;
type Tuple2D_t Tuple2DT_t;
