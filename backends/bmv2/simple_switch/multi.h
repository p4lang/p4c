#ifndef BACKENDS_BMV2_SIMPLE_SWITCH_MULTI_H_
#define BACKENDS_BMV2_SIMPLE_SWITCH_MULTI_H_

int parse(const IR::P4Program *program, cstring pipe);

int process_midend(BMV2::BMV2Options options, BMV2::SimpleSwitchMidEnd midEnd, const IR::ToplevelBlock *toplevel, const IR::P4Program *program);

#endif 