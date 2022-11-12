#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

const bool test = static_assert(20180101 >= 20200000, "V1 model version is not >= 20200000");
control c() {
    apply {
        static_assert(2 != 1, "Arithmetic is broken");
        static_assert(3 == 1);
    }
}

