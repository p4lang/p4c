/*
 * Jeferson Santiago da Silva (eng.jefersonsantiago@gmail.com)
 */

#ifndef _FRONTENDS_P4_EXTERN_H_
#define _FRONTENDS_P4_EXTERN_H_

#include "lib/cstring.h"
#include "frontends/common/model.h"

namespace P4V1 {

// Template for any extern object
// TODO: methods parameters
struct External_Instance_Model : public ::Model::Extern_Model {
	std::vector<::Model::Elem> Parameters;
    std::vector<::Model::Elem> Methods;
    External_Instance_Model(cstring extern_name,
							std::vector<cstring> param_name,
							std::vector<cstring> method_name) :
							Extern_Model(extern_name) {
		for (auto param : param_name) {
			::Model::Elem p(param);
			Parameters.push_back(p);
		}
		for (auto method : method_name) {
			::Model::Elem m(method);
			Methods.push_back(m);
		}
	}
};

// Fixed structure that holds all extern modules
// TODO: automatize this generation
struct External_Instances_Model {
	// Each index is:
	// External instance name, parameters vector, methods vector
	// TODO: methods parameters
	const std::vector<External_Instance_Model> External_Inst =
	{
		{
			"ExternRohcCompressor", {"verbose"}, {"rohc_comp_header"},
		},
		{
			"ExternRohcDecompressor", {"verbose"}, {"rohc_decomp_header"}
		}
	};
};

} // end namespace
#endif
