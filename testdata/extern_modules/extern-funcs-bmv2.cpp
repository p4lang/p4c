/*
 * Create a simple extern function and register it. The test will load a P4
 * program that calls this extern. It will verify the extern loads correctly,
 * and that the described action is performed.
 */
#include <bm/bm_sim/data.h>
#include <bm/bm_sim/extern.h>
#include <bm/bm_sim/fields.h>

/**
 * Extern test funcion. Set f to d (f <- d).
 */
void extern_func(bm::Field & f, const bm::Data & d) {
	f.set(d);
}
BM_REGISTER_EXTERN_FUNCTION(extern_func, bm::Field &, const bm::Data &);
