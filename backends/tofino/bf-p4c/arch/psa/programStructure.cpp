/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "programStructure.h"

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "midend/validateProperties.h"
#include "psa_converters.h"

namespace BFN {

namespace PSA {

void ProgramStructure::createParsers() {
    IngressParserConverter cvt_i(this);
    auto ingressParser = parsers.at(getBlockName(ProgramStructure::INGRESS_PARSER));
    ingressParser = cvt_i.convert(ingressParser);
    declarations.push_back(ingressParser);

    EgressParserConverter cvt_e(this);
    auto egressParser = parsers.at(getBlockName(ProgramStructure::EGRESS_PARSER));
    egressParser = cvt_e.convert(egressParser);
    declarations.push_back(egressParser);
}

void ProgramStructure::createControls() {
    IngressControlConverter cvt_i(this);
    auto ingress = controls.at(getBlockName(ProgramStructure::INGRESS));
    ingress = cvt_i.convert(ingress);
    declarations.push_back(ingress);

    IngressDeparserConverter cvt_igDep(this);
    auto igDeparser = controls.at(getBlockName(ProgramStructure::INGRESS_DEPARSER));
    igDeparser = cvt_igDep.convert(igDeparser);
    declarations.push_back(igDeparser);

    EgressControlConverter cvt_e(this);
    auto egress = controls.at(getBlockName(ProgramStructure::EGRESS));
    egress = cvt_e.convert(egress);
    declarations.push_back(egress);

    EgressDeparserConverter cvt_egDep(this);
    auto egDeparser = controls.at(getBlockName(ProgramStructure::EGRESS_DEPARSER));
    egDeparser = cvt_egDep.convert(egDeparser);
    declarations.push_back(egDeparser);
}

void ProgramStructure::createPipeline() {
    auto name = IR::ID("pipe");
    auto typepath = new IR::Path("Pipeline");
    auto type = new IR::Type_Name(typepath);
    auto typeArgs = new IR::Vector<IR::Type>();
    typeArgs->push_back(new IR::Type_Name(type_params[TYPE_IH]));
    typeArgs->push_back(new IR::Type_Name(type_params[TYPE_IM]));
    typeArgs->push_back(new IR::Type_Name(type_params[TYPE_EH]));
    typeArgs->push_back(new IR::Type_Name(type_params[TYPE_EM]));
    typeArgs->push_back(new IR::Type_Name("compiler_generated_metadata_t"));
    auto typeSpecialized = new IR::Type_Specialized(type, typeArgs);

    auto args = new IR::Vector<IR::Argument>();
    auto emptyArgs = new IR::Vector<IR::Argument>();
    auto iParserPath = new IR::Path("ingressParserImpl");
    auto iParserType = new IR::Type_Name(iParserPath);
    auto iParserConstruct = new IR::ConstructorCallExpression(iParserType, emptyArgs);
    args->push_back(new IR::Argument(iParserConstruct));

    auto ingressPath = new IR::Path("ingress");
    auto ingressType = new IR::Type_Name(ingressPath);
    auto ingressConstruct = new IR::ConstructorCallExpression(ingressType, emptyArgs);
    args->push_back(new IR::Argument(ingressConstruct));

    auto iDeparserPath = new IR::Path("ingressDeparserImpl");
    auto iDeparserType = new IR::Type_Name(iDeparserPath);
    auto iDeparserConstruct = new IR::ConstructorCallExpression(iDeparserType, emptyArgs);
    args->push_back(new IR::Argument(iDeparserConstruct));

    auto eParserPath = new IR::Path("egressParserImpl");
    auto eParserType = new IR::Type_Name(eParserPath);
    auto eParserConstruct = new IR::ConstructorCallExpression(eParserType, emptyArgs);
    args->push_back(new IR::Argument(eParserConstruct));

    auto egressPath = new IR::Path("egress");
    auto egressType = new IR::Type_Name(egressPath);
    auto egressConstruct = new IR::ConstructorCallExpression(egressType, emptyArgs);
    args->push_back(new IR::Argument(egressConstruct));

    auto eDeparserPath = new IR::Path("egressDeparserImpl");
    auto eDeparserType = new IR::Type_Name(eDeparserPath);
    auto eDeparserConstruct = new IR::ConstructorCallExpression(eDeparserType, emptyArgs);
    args->push_back(new IR::Argument(eDeparserConstruct));

    auto result = new IR::Declaration_Instance(name, typeSpecialized, args, nullptr);
    declarations.push_back(result);
}

void ProgramStructure::createMain() {
    auto name = IR::ID(IR::P4Program::main);
    auto typepath = new IR::Path("Switch");
    auto type = new IR::Type_Name(typepath);
    auto typeArgs = new IR::Vector<IR::Type>();
    typeArgs->push_back(new IR::Type_Name(type_params[TYPE_IH]));
    typeArgs->push_back(new IR::Type_Name(type_params[TYPE_IM]));
    typeArgs->push_back(new IR::Type_Name(type_params[TYPE_EH]));
    typeArgs->push_back(new IR::Type_Name(type_params[TYPE_EM]));
    typeArgs->push_back(new IR::Type_Name("compiler_generated_metadata_t"));
    // PSA only uses one pipeline, hence the other type arguments are dont_care.
    for (auto i = 0; i < 15; i++) typeArgs->push_back(IR::Type_Dontcare::get());
    auto typeSpecialized = new IR::Type_Specialized(type, typeArgs);

    auto args = new IR::Vector<IR::Argument>();
    auto pipe0 = new IR::Path("pipe");
    auto expr = new IR::PathExpression(pipe0);
    args->push_back(new IR::Argument(expr));

    auto *annotations = new IR::Annotations();
    annotations->annotations.push_back(
        new IR::Annotation(IR::ID(PragmaAutoInitMetadata::name), {}));

    auto result = new IR::Declaration_Instance(name, annotations, typeSpecialized, args, nullptr);
    declarations.push_back(result);
}

const IR::P4Program *ProgramStructure::create(const IR::P4Program *program) {
    createErrors();
    createTofinoArch();
    createTypes();
    createActions();
    createParsers();
    createControls();
    createPipeline();
    createMain();

    return new IR::P4Program(program->srcInfo, declarations);
}

}  // namespace PSA

}  // namespace BFN
