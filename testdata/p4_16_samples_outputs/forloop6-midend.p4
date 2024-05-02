#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> x;
    bit<32> y;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    @name("c.result") bit<32> result_0;
    bool breakFlag;
    bool continueFlag;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.a0") action a0(@name("m") bit<8> m_1) {
        result_0 = 32w0;
        breakFlag = false;
        continueFlag = false;
        if (m_1 == 8w0) {
            breakFlag = true;
        }
        if (breakFlag) {
            ;
        } else if (m_1[0:0] == 1w0) {
            continueFlag = true;
        }
        if (!breakFlag && !continueFlag) {
            result_0 = hdrs.t1.y & 32w0xf;
        }
        if (breakFlag) {
            ;
        } else {
            continueFlag = false;
            if (m_1 >> 1 == 8w0) {
                breakFlag = true;
            }
            if (breakFlag) {
                ;
            } else if (m_1[1:1] == 1w0) {
                continueFlag = true;
            }
            if (!breakFlag && !continueFlag) {
                result_0 = result_0 + (hdrs.t1.y >> 8w4 & 32w0xf);
            }
            if (breakFlag) {
                ;
            } else {
                continueFlag = false;
                if (m_1 >> 2 == 8w0) {
                    breakFlag = true;
                }
                if (breakFlag) {
                    ;
                } else if (m_1[2:2] == 1w0) {
                    continueFlag = true;
                }
                if (!breakFlag && !continueFlag) {
                    result_0 = result_0 + (hdrs.t1.y >> 8w8 & 32w0xf);
                }
                if (breakFlag) {
                    ;
                } else {
                    continueFlag = false;
                    if (m_1 >> 3 == 8w0) {
                        breakFlag = true;
                    }
                    if (breakFlag) {
                        ;
                    } else if (m_1[3:3] == 1w0) {
                        continueFlag = true;
                    }
                    if (!breakFlag && !continueFlag) {
                        result_0 = result_0 + (hdrs.t1.y >> 8w12 & 32w0xf);
                    }
                    if (breakFlag) {
                        ;
                    } else {
                        continueFlag = false;
                        if (m_1 >> 4 == 8w0) {
                            breakFlag = true;
                        }
                        if (breakFlag) {
                            ;
                        } else if (m_1[4:4] == 1w0) {
                            continueFlag = true;
                        }
                        if (!breakFlag && !continueFlag) {
                            result_0 = result_0 + (hdrs.t1.y >> 8w16 & 32w0xf);
                        }
                        if (breakFlag) {
                            ;
                        } else {
                            continueFlag = false;
                            if (m_1 >> 5 == 8w0) {
                                breakFlag = true;
                            }
                            if (breakFlag) {
                                ;
                            } else if (m_1[5:5] == 1w0) {
                                continueFlag = true;
                            }
                            if (!breakFlag && !continueFlag) {
                                result_0 = result_0 + (hdrs.t1.y >> 8w20 & 32w0xf);
                            }
                            if (breakFlag) {
                                ;
                            } else {
                                continueFlag = false;
                                if (m_1 >> 6 == 8w0) {
                                    breakFlag = true;
                                }
                                if (breakFlag) {
                                    ;
                                } else if (m_1[6:6] == 1w0) {
                                    continueFlag = true;
                                }
                                if (!breakFlag && !continueFlag) {
                                    result_0 = result_0 + (hdrs.t1.y >> 8w24 & 32w0xf);
                                }
                                if (breakFlag) {
                                    ;
                                } else {
                                    continueFlag = false;
                                    if (m_1 >> 7 == 8w0) {
                                        breakFlag = true;
                                    }
                                    if (breakFlag) {
                                        ;
                                    } else if (m_1[7:7] == 1w0) {
                                        continueFlag = true;
                                    }
                                    if (!breakFlag && !continueFlag) {
                                        result_0 = result_0 + (hdrs.t1.y >> 8w28 & 32w0xf);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        hdrs.t1.y = result_0;
    }
    @name("c.test") table test_0 {
        key = {
            hdrs.t1.x: exact @name("hdrs.t1.x");
        }
        actions = {
            a0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        test_0.apply();
    }
}

top<headers_t>(c()) main;
