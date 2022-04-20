enum bit<1> TableType {TT_ACL = 0, TT_FWD = 1};

const bit<10> b10 = (bit<10>)(bit<1>)TableType.TT_ACL;
const bit<10> mask = 1 << 9;

const bit<10> test1 = b10 << 3;
const bit<10> test2 = mask | b10;
