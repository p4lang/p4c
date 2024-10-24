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

#include "compute_multi_write_containers.h"

namespace Parde::Lowered {

PHV::Container ComputeMultiWriteContainers::offset_container(const PHV::Container &container) {
    if (stack_offset == 0) return container;

    unsigned index = Device::phvSpec().physicalAddress(container, PhvSpec::PARSER);
    auto curr = Device::phvSpec().physicalAddressToContainer(index + stack_offset, PhvSpec::PARSER);
    // If we have an 8b container on JBay, point to the correct
    // half in the 16b parser container
    if ((Device::currentDevice() == Device::JBAY) &&  // NOLINT(whitespace/parens)
        container.type().size() == PHV::Size::b8 && container.index() % 2)
        curr = PHV::Container(curr->type(), curr->index() + 1);

    return *curr;
}

bool ComputeMultiWriteContainers::preorder(IR::BFN::LoweredParserMatch *match) {
    auto orig = getOriginal<IR::BFN::LoweredParserMatch>();

    auto match_offset = std::make_pair(orig, stack_offset);
    if (visited_matches.count(match_offset)) return false;

    for (auto e : match->extracts) {
        if (auto extract = e->to<IR::BFN::LoweredExtractPhv>()) {
            PHV::Container container = extract->dest->container;
            if (extract->source->is<IR::BFN::LoweredInputBufferRVal>())
                container = offset_container(extract->dest->container);
            if (extract->write_mode == IR::BFN::ParserWriteMode::CLEAR_ON_WRITE) {
                clear_on_write[container].insert(orig);
            } else {
                // Two extraction in the same transition, container should be bitwise or
                if (bitwise_or.count(container) && bitwise_or[container].count(orig)) {
                    bitwise_or_containers.insert(container);
                }
                bitwise_or[container].insert(orig);
            }
        }
    }

    for (auto csum : match->checksums) {
        PHV::Container container;
        if (csum->csum_err) {
            container = csum->csum_err->container->container;
        } else if (csum->phv_dest) {
            container = offset_container(csum->phv_dest->container);
        }
        if (container) {
            if (csum->write_mode == IR::BFN::ParserWriteMode::CLEAR_ON_WRITE) {
                clear_on_write[container].insert(orig);
            } else if (csum->write_mode == IR::BFN::ParserWriteMode::BITWISE_OR) {
                bitwise_or[container].insert(orig);
            }
        }
    }

    visited_matches.emplace(match_offset);
    if (match->offsetInc) stack_offset += *match->offsetInc;

    return true;
}

void ComputeMultiWriteContainers::postorder(IR::BFN::LoweredParserMatch *match) {
    if (match->offsetInc) stack_offset -= *match->offsetInc;
}

bool ComputeMultiWriteContainers::preorder(IR::BFN::LoweredParser *) {
    bitwise_or = clear_on_write = {};
    clear_on_write_containers = bitwise_or_containers = {};
    return true;
}

bool ComputeMultiWriteContainers::has_non_mutex_writes(
    const IR::BFN::LoweredParser *parser,
    const std::set<const IR::BFN::LoweredParserMatch *> &matches) {
    for (auto i : matches) {
        for (auto j : matches) {
            if (i == j) continue;

            bool mutex = parser_info.graph(parser).is_mutex(i, j);
            if (!mutex) return true;
        }
    }

    return false;
}

void ComputeMultiWriteContainers::detect_multi_writes(
    const IR::BFN::LoweredParser *parser,
    const std::map<PHV::Container, std::set<const IR::BFN::LoweredParserMatch *>> &writes,
    std::set<PHV::Container> &write_containers, const char *which) {
    for (const auto &w : writes) {
        if (has_non_mutex_writes(parser, w.second)) {
            write_containers.insert(w.first);
            LOG4("mark " << w.first << " as " << which);
        } else if (Device::currentDevice() != Device::TOFINO) {
            // In Jbay, even and odd pair of 8-bit containers share extractor in the parser.
            // So if both are used, we need to mark the extract as a multi write.
            if (w.first.is(PHV::Size::b8)) {
                PHV::Container other(w.first.type(), w.first.index() ^ 1);
                if (writes.count(other)) {
                    bool has_even_odd_pair = false;

                    for (const auto &x : writes.at(other)) {
                        for (const auto &y : w.second) {
                            if (x == y || !parser_info.graph(parser).is_mutex(x, y)) {
                                has_even_odd_pair = true;
                                break;
                            }
                        }
                    }

                    if (has_even_odd_pair) {
                        write_containers.insert(w.first);
                        write_containers.insert(other);
                        LOG4("mark " << w.first << " and " << other << " as " << which
                                     << " (even-and-odd pair)");
                    }
                }
            }
        }
    }
}

void ComputeMultiWriteContainers::postorder(IR::BFN::LoweredParser *parser) {
    auto orig = getOriginal<IR::BFN::LoweredParser>();

    detect_multi_writes(orig, bitwise_or, bitwise_or_containers, "bitwise-or");
    detect_multi_writes(orig, clear_on_write, clear_on_write_containers, "clear-on-write");

    for (const auto &f : phv) {
        if (f.gress != parser->gress) continue;

        if (f.name.endsWith("$stkvalid")) {
            auto ctxt = PHV::AllocContext::PARSER;
            f.foreach_alloc(ctxt, nullptr, [&](const PHV::AllocSlice &alloc) {
                bitwise_or_containers.insert(alloc.container());
            });
        }
    }

    // validate
    for (const auto &c : bitwise_or_containers) {
        if (clear_on_write_containers.count(c))
            BUG("Container cannot be both clear-on-write and bitwise-or: %1%", c);
    }

    for (const auto &c : bitwise_or_containers)
        parser->bitwiseOrContainers.push_back(new IR::BFN::ContainerRef(c));

    for (const auto &c : clear_on_write_containers)
        parser->clearOnWriteContainers.push_back(new IR::BFN::ContainerRef(c));
}

}  // namespace Parde::Lowered
