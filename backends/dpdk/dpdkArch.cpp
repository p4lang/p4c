/**
 * DPDK architecture assume the following control block signature
 *
 * control ingress(header h, metadata m);
 * control egress(header h, metadata m);
 *
 * We need to convert psa control blocks to this form.
 */

#include "dpdkArch.h"

namespace DPDK {

const IR::Node *ConvertToDpdkArch::postorder(IR::P4Program *p) {
  // std::cout << p << std::endl;
  return p;
}

const IR::Type_Control *
ConvertToDpdkArch::rewriteControlType(const IR::Type_Control *c, cstring name) {
  auto applyParams = new IR::ParameterList();
  if (name == "Ingress" || name == "Egress") {
    applyParams->push_back(c->applyParams->parameters.at(0));
    applyParams->push_back(c->applyParams->parameters.at(1));
  } else if (name == "IngressDeparser") {
    applyParams->push_back(c->applyParams->parameters.at(0));
    applyParams->push_back(c->applyParams->parameters.at(4));
    applyParams->push_back(c->applyParams->parameters.at(5));
  } else if (name == "EgressDeparser") {
    applyParams->push_back(c->applyParams->parameters.at(0));
    applyParams->push_back(c->applyParams->parameters.at(3));
    applyParams->push_back(c->applyParams->parameters.at(4));
  }
  auto tc = new IR::Type_Control(c->name, c->annotations, c->typeParameters,
                                 applyParams);
  return tc;
}

// translate control block signature in arch.p4
const IR::Node *ConvertToDpdkArch::postorder(IR::Type_Control *c) {
  auto ctxt = findOrigCtxt<IR::P4Control>();
  if (ctxt)
    return c;
  return rewriteControlType(c, c->name);
}

const IR::Type_Parser *
ConvertToDpdkArch::rewriteParserType(const IR::Type_Parser *p, cstring name) {
  auto applyParams = new IR::ParameterList();
  if (name == "IngressParser") {
    applyParams->push_back(p->applyParams->parameters.at(0));
    applyParams->push_back(p->applyParams->parameters.at(1));
    applyParams->push_back(p->applyParams->parameters.at(2));
  } else if (name == "EgressParser") {
    applyParams->push_back(p->applyParams->parameters.at(0));
    applyParams->push_back(p->applyParams->parameters.at(1));
    applyParams->push_back(p->applyParams->parameters.at(2));
  }
  auto tp = new IR::Type_Parser(p->name, p->annotations, p->typeParameters,
                                applyParams);

  return tp;
}

const IR::Node *ConvertToDpdkArch::postorder(IR::Type_Parser *p) {
  auto ctxt = findOrigCtxt<IR::P4Parser>();
  if (ctxt)
    return p;
  return rewriteParserType(p, p->name);
}

const IR::Node *ConvertToDpdkArch::postorder(IR::P4Control *c) {
  auto orig = getOriginal();
  if (block_info->count(orig) != 0) {
    auto bi = block_info->at(orig);
    LOG1("bi " << bi.pipe << " " << bi.gress << " " << bi.block);
    auto tc = rewriteControlType(c->type, bi.pipe);
    auto cont = new IR::P4Control(c->name, tc, c->constructorParams,
                                  c->controlLocals, c->body);
    LOG1(cont);
    return cont;
  }
  return c;
}

const IR::Node *ConvertToDpdkArch::postorder(IR::P4Parser *p) {
  auto orig = getOriginal();
  if (block_info->count(orig) != 0) {
    auto bi = block_info->at(orig);
    LOG1("bi " << bi.pipe << " " << bi.gress << " " << bi.block);
    auto tp = rewriteParserType(p->type, bi.pipe);
    auto prsr = new IR::P4Parser(p->name, tp, p->constructorParams,
                                 p->parserLocals, p->states);
    LOG1(prsr);
    return prsr;
  }
  return p;
}

void ParsePsa::parseIngressPipeline(const IR::PackageBlock *block) {
  auto ingress_parser = block->getParameterValue("ip");
  BlockInfo ip("IngressParser", INGRESS, PARSER);
  BUG_CHECK(ingress_parser->is<IR::ParserBlock>(), "Expected ParserBlock");
  toBlockInfo.emplace(ingress_parser->to<IR::ParserBlock>()->container, ip);

  auto ingress = block->getParameterValue("ig");
  BlockInfo ig("Ingress", INGRESS, PIPELINE);
  BUG_CHECK(ingress->is<IR::ControlBlock>(), "Expected ControlBlock");
  toBlockInfo.emplace(ingress->to<IR::ControlBlock>()->container, ig);

  auto ingress_deparser = block->getParameterValue("id");
  BUG_CHECK(ingress_deparser->is<IR::ControlBlock>(), "Expected ControlBlock");
  BlockInfo id("IngressDeparser", INGRESS, DEPARSER);
  toBlockInfo.emplace(ingress_deparser->to<IR::ControlBlock>()->container, id);
}

void ParsePsa::parseEgressPipeline(const IR::PackageBlock *block) {
  auto egress_parser = block->getParameterValue("ep");
  BUG_CHECK(egress_parser->is<IR::ParserBlock>(), "Expected ParserBlock");
  BlockInfo ep("EgressParser", EGRESS, PARSER);
  toBlockInfo.emplace(egress_parser->to<IR::ParserBlock>()->container, ep);

  auto egress = block->getParameterValue("eg");
  BUG_CHECK(egress->is<IR::ControlBlock>(), "Expected ControlBlock");
  BlockInfo eg("Egress", EGRESS, PIPELINE);
  toBlockInfo.emplace(egress->to<IR::ControlBlock>()->container, eg);

  auto egress_deparser = block->getParameterValue("ed");
  BUG_CHECK(egress_deparser->is<IR::ControlBlock>(), "Expected ControlBlock");
  BlockInfo ed("EgressDeparser", EGRESS, DEPARSER);
  toBlockInfo.emplace(egress_deparser->to<IR::ControlBlock>()->container, ed);
}

bool ParsePsa::preorder(const IR::PackageBlock *block) {
  auto decl = block->node->to<IR::Declaration_Instance>();
  // If no declaration found (anonymous instantiation) get the pipe name from
  // arch definition
  cstring pipe = decl->Name();
  BUG_CHECK(!pipe.isNullOrEmpty(), "Cannot determine pipe name for pipe block");

  auto ingress = block->getParameterValue("ingress");
  if (auto block = ingress->to<IR::PackageBlock>())
    parseIngressPipeline(block);
  auto egress = block->getParameterValue("egress");
  if (auto block = egress->to<IR::PackageBlock>())
    parseEgressPipeline(block);

  // collect user-provided structure
  LOG1(block->instanceType);
  // for (auto n : block->instanceType->constantValue) {
  //     LOG1("n " << n);
  // }
  return false;
}

// bool ParsePsa::preorder(const IR::Type_Struct* s){
//     s->fields
// }

void CollectMetadataHeaderInfo::pushMetadata(const IR::Parameter *p) {
  for (auto m : used_metadata) {
    if (m->to<IR::Type_Name>()->path->name.name ==
        p->type->to<IR::Type_Name>()->path->name.name) {
      return;
    }
  }
  used_metadata.push_back(p->type);
}

bool CollectMetadataHeaderInfo::preorder(const IR::P4Program *) {
  for (auto kv : *toBlockInfo) {
    if (kv.second.pipe == "IngressParser") {
      auto parser = kv.first->to<IR::P4Parser>();
      auto local_metadata = parser->getApplyParameters()->getParameter(2);
      local_metadata_type =
          local_metadata->type->to<IR::Type_Name>()->path->name;
      auto header = parser->getApplyParameters()->getParameter(1);
      header_type = header->type->to<IR::Type_Name>()->path->name;
      pushMetadata(parser->getApplyParameters()->getParameter(2));
      pushMetadata(parser->getApplyParameters()->getParameter(3));
      pushMetadata(parser->getApplyParameters()->getParameter(4));
      pushMetadata(parser->getApplyParameters()->getParameter(5));
    } else if (kv.second.pipe == "Ingress") {
      auto control = kv.first->to<IR::P4Control>();
      pushMetadata(control->getApplyParameters()->getParameter(1));
      pushMetadata(control->getApplyParameters()->getParameter(2));
      pushMetadata(control->getApplyParameters()->getParameter(3));
    } else if (kv.second.pipe == "IngressParser") {
      auto deparser = kv.first->to<IR::P4Control>();
      pushMetadata(deparser->getApplyParameters()->getParameter(1));
      pushMetadata(deparser->getApplyParameters()->getParameter(2));
      pushMetadata(deparser->getApplyParameters()->getParameter(3));
      pushMetadata(deparser->getApplyParameters()->getParameter(5));
      pushMetadata(deparser->getApplyParameters()->getParameter(6));
    } else if (kv.second.pipe == "EgressParser") {
      auto parser = kv.first->to<IR::P4Parser>();
      pushMetadata(parser->getApplyParameters()->getParameter(2));
      pushMetadata(parser->getApplyParameters()->getParameter(3));
      pushMetadata(parser->getApplyParameters()->getParameter(4));
      pushMetadata(parser->getApplyParameters()->getParameter(5));
      pushMetadata(parser->getApplyParameters()->getParameter(6));
    } else if (kv.second.pipe == "Egress") {
      auto control = kv.first->to<IR::P4Control>();
      pushMetadata(control->getApplyParameters()->getParameter(1));
      pushMetadata(control->getApplyParameters()->getParameter(2));
      pushMetadata(control->getApplyParameters()->getParameter(3));
    } else if (kv.second.pipe == "EgressDeparser") {
      auto deparser = kv.first->to<IR::P4Control>();
      pushMetadata(deparser->getApplyParameters()->getParameter(1));
      pushMetadata(deparser->getApplyParameters()->getParameter(2));
      pushMetadata(deparser->getApplyParameters()->getParameter(4));
      pushMetadata(deparser->getApplyParameters()->getParameter(5));
      pushMetadata(deparser->getApplyParameters()->getParameter(6));
    }
  }
  return true;
}

bool CollectMetadataHeaderInfo::preorder(const IR::Type_Struct *s) {
  for (auto m : used_metadata) {
    if (m->to<IR::Type_Name>()->path->name.name == s->name.name) {
      for (auto field : s->fields) {
        fields.push_back(new IR::StructField(
            IR::ID(TypeStruct2Name(s->name.name) + "_" + field->name),
            field->type));
      }
      return true;
    }
  }
  return true;
}

const IR::Node *ReplaceMetadataHeaderName::preorder(IR::Member *m) {
  if (auto p = m->expr->to<IR::PathExpression>()) {
    auto declaration = refMap->getDeclaration(p->path);
    if (auto decl = declaration->to<IR::Parameter>()) {
      if (auto type = decl->type->to<IR::Type_Name>()) {
        if (type->path->name == "psa_ingress_parser_input_metadata_t" ||
            type->path->name == "psa_ingress_input_metadata_t" ||
            type->path->name == "psa_ingress_output_metadata_t" ||
            type->path->name == "psa_egress_parser_input_metadata_t" ||
            type->path->name == "psa_egress_input_metadata_t" ||
            type->path->name == "psa_egress_output_metadata_t" ||
            type->path->name == "psa_egress_deparser_input_metadata_t") {
          return new IR::Member(new IR::PathExpression(IR::ID("m")),
                                IR::ID(TypeStruct2Name(type->path->name.name) +
                                       "_" + m->member.name));
        } else if (type->path->name == info->header_type) {
          return new IR::Member(new IR::PathExpression(IR::ID("h")),
                                IR::ID(m->member.name));
        } else if (type->path->name == info->local_metadata_type) {
          return new IR::Member(new IR::PathExpression(IR::ID("m")),
                                IR::ID("local_metadata_" + m->member.name));
        }
      }
    }
  }
  return m;
}

const IR::Node *ReplaceMetadataHeaderName::preorder(IR::Parameter *p) {
  if (auto type = p->type->to<IR::Type_Name>()) {
    if (type->path->name == info->header_type) {
      return new IR::Parameter(IR::ID("h"), p->direction, p->type);
    } else if (type->path->name == info->local_metadata_type) {
      return new IR::Parameter(IR::ID("m"), p->direction, p->type);
    }
  }
  return p;
}
