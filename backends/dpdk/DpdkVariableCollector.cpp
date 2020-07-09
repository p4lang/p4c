#include "DpdkVariableCollector.h"

namespace DPDK{
// cstring DpdkVariableCollector::get_next_tmp(IR::Expression* type){
cstring DpdkVariableCollector::get_next_tmp(){
    std::ostringstream out;
    out << "backend_tmp" << next_tmp_id;
    cstring tmp = out.str();
    // variables.push_back(new std::pair<IR::Expression*, cstring>(type, tmp));
    next_tmp_id++;
    return tmp;
}

void DpdkVariableCollector::push_variable(const IR::DpdkDeclaration * d){
    variables.push_back(d);
}
}
