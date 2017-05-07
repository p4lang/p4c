/*
 * Jeferson Santiago da Silva (eng.jefersonsantiago@gmail.com)
 */

#ifndef _V1_EXTERN_INSTANCES_P4_
#define _V1_EXTERN_INSTANCES_P4_

extern ExternRohcCompressor {
	ExternRohcCompressor(bit<1> verbose);
	void rohc_comp_header();
}

extern ExternRohcDecompressor {
	ExternRohcDecompressor(bit<1> verbose);
	void rohc_decomp_header();
}
#endif
