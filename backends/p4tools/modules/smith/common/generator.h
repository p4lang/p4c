#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_GENERATOR_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_GENERATOR_H_

#include <functional>

namespace P4Tools::P4Smith {

class SmithTarget;

class Generator {
    std::reference_wrapper<const SmithTarget> _target;

 public:
    explicit Generator(const SmithTarget &target) : _target(target) {}

    const SmithTarget &target() { return _target; }
};
}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_COMMON_GENERATOR_H_ */
