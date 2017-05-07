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

extern ext_type {
   	ext_type(bit<1> ext_attr_a, bit<1> ext_attr_b); 
    void ext_method(in bit<9> p1, in bit<9> p2, out bit<9> p3);
}

#endif
