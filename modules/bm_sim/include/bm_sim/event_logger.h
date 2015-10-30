/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef _BM_EVENT_LOGGER_H_
#define _BM_EVENT_LOGGER_H_

#include <string>
#include <memory>

#include "packet.h"
#include "phv.h"
#include "pipeline.h"
#include "checksums.h"
#include "transport.h"

/* Forward declarations of P4 object classes. This is ugly, but:
   1) I don't have to worry about circular dependencies
   2) If I decide to switch from id to name for msgs, I won't have to modify the
   EventLogger interface */
class Parser;
class Deparser;
class MatchTableAbstract;
class MatchEntry;
class ActionFn;
class ActionData;
class Conditional;
class Checksum;
class ActionFn;

typedef uint64_t entry_handle_t;

class EventLogger {
public:
  EventLogger(std::unique_ptr<TransportIface> transport)
    : transport_instance(std::move(transport)) { }

  // we need the ingress / egress ports, but they are part of the Packet
  void packet_in(const Packet &packet);
  void packet_out(const Packet &packet);

  void parser_start(const Packet &packet, const Parser &parser);
  void parser_done(const Packet &packet, const Parser &parser);
  void parser_extract(const Packet &packet, header_id_t header);

  void deparser_start(const Packet &packet, const Deparser &deparser);
  void deparser_done(const Packet &packet, const Deparser &deparser);
  void deparser_emit(const Packet &packet, header_id_t header);

  void checksum_update(const Packet &packet, const Checksum &checksum);

  void pipeline_start(const Packet &packet, const Pipeline &pipeline);
  void pipeline_done(const Packet &packet, const Pipeline &pipeline);

  void condition_eval(const Packet &packet,
		      const Conditional &cond, bool result);
  void table_hit(const Packet &packet,
		 const MatchTableAbstract &table, entry_handle_t handle);
  void table_miss(const Packet &packet, const MatchTableAbstract &table);

  void action_execute(const Packet &packet,
		      const ActionFn &action_fn, const ActionData &action_data);

private:
  std::unique_ptr<TransportIface> transport_instance;
};

extern EventLogger *event_logger;

#define ELOGGER event_logger

#endif
