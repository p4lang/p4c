#ifndef _MIDEND_MOVECONSTRUCTORS_H_
#define _MIDEND_MOVECONSTRUCTORS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

class MoveConstructors : public PassManager {
 public:
    explicit MoveConstructors(bool isv1);
};

}  // namespace P4

#endif /* _MIDEND_MOVECONSTRUCTORS_H_ */
