#include <v1model.p4>

const bool test = static_assert(V1MODEL_VERSION >= 20160101, "V1 model version is not >= 20160101");
