#include <cstdio>
#include <iostream>
#include <string>

#include <cstdio>
#include <iostream>
#include <string>

#include "backends/bmv2/common/JsonObjects.h"
#include "backends/bmv2/simple_switch/midend.h"
#include "backends/bmv2/simple_switch/options.h"
#include "backends/bmv2/simple_switch/simpleSwitch.h"
#include "backends/bmv2/simple_switch/multi.h"
#include "backends/bmv2/simple_switch/version.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/frontend.h"
#include "fstream"
#include "ir/ir.h"
#include "ir/json_generator.h"
#include "ir/json_loader.h"
#include "lib/algorithm.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/log.h"
#include "lib/nullstream.h"

int parse(const IR::P4Program *program, cstring pipe) {
    int succes = -1;
    IR::P4Program *program_ptr_cpy = const_cast<IR::P4Program*>(program);
    IR::Vector<IR::Node> &nodes = program_ptr_cpy->objects;

    for(unsigned int i = 0; i<nodes.size(); i++) {
        if (auto *declaration_instance = nodes.at(i)->to<IR::Declaration_Instance>()) {
            const IR::Annotations *annotations = declaration_instance->getAnnotations();
            if(annotations->size() == 1) {
                const IR::Vector<IR::Annotation> annotationsVector = annotations->annotations;
                const IR::Annotation *annotation = annotationsVector.at(0);
                const IR::Vector<IR::AnnotationToken> tokenVector = annotation->body;
                if(tokenVector.size() == 1) {
                    const IR::AnnotationToken *token = tokenVector.at(0);
                    const cstring text = token->text;
                    if(text == pipe) {
                        auto *D = nodes.at(i)->to<IR::Declaration>();
                        IR::ID *id = new IR::ID("main");
                        IR::ID *name = const_cast<IR::ID*>(&(D->name));
                        *name = *id;
                        succes = 0;
                    }
                }
                
            }
        }
    }
    
    return succes;
}

