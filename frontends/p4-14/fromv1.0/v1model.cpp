#include "frontends/p4-14/fromv1.0/v1model.h"

/* These must be in the same compilation unit to ensure that P4CoreLibrary::instance
 * is initialized before V1Model::instance */
namespace P4::P4V1 {

V1Model V1Model::instance;
const char *V1Model::versionInitial = "20180101";
const char *V1Model::versionCurrent = "20200408";

}  // namespace P4::P4V1
