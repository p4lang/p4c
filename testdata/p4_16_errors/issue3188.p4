#include <v1model.p4>

const bool test = static_assert(V1MODEL_VERSION >= 20200000, "V1 model version is not >= 20200000");

control c() {
    apply {
        static_assert(2 != 1, "Arithmetic is broken");
        static_assert(3 == 1);
    }
}
