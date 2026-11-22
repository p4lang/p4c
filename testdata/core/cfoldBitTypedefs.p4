
typedef bit<8> t1_t;
const bit<8> v1 = 15;
const t1_t v2 = 16;
typedef bit<v1> t2_t;
typedef bit<v2> t3_t;
typedef t1_t t4_t;
const t4_t v3 = 17;
typedef bit<v3> t5_t;
const t5_t v4 = 18;
typedef bit<v4[7:0]-v1> t6_t;

