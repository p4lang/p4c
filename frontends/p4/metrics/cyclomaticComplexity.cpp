#include "cyclomaticComplexity.h"
#include <iostream>
namespace P4 {

bool CyclomaticComplexityPass::preorder(const IR::P4Program *program) {
    std::cout<<"CC pass"<<std::endl;
    cyclomaticComplexity["function Name"] = 10;
    return false;
}

}  // namespace P4
