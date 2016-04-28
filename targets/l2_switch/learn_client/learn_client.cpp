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

#include <bm/bm_apps/learn.h>
#include <bm/Standard.h>
#include <bm/SimplePre.h>

#include <iostream>
#include <string>
#include <vector>

#include <cassert>

namespace {

bm_apps::LearnListener *listener;

typedef struct {
  char src_addr[6];
  uint16_t ingress_port;
} __attribute__((packed)) sample_t;

}  // namespace

namespace runtime = bm_runtime::standard;

namespace {

void learn_cb(const bm_apps::LearnListener::MsgInfo &msg_info,
              const char *data, void *cookie) {
  (void) cookie;
  std::cout << "CB with " << msg_info.num_samples << " samples\n";

  boost::shared_ptr<runtime::StandardClient> client = listener->get_client();
  assert(client);

  for (unsigned int i = 0; i < msg_info.num_samples; i++) {
    const sample_t *sample = ((const sample_t *) data) + i;

    std::cout << "ingress port is " << ntohs(sample->ingress_port)
              << std::endl;

    runtime::BmMatchParam match_param;
    match_param.type = runtime::BmMatchParamType::type::EXACT;
    runtime::BmMatchParamExact match_param_exact;
    match_param_exact.key =
      std::string(sample->src_addr, sizeof(sample->src_addr));
    match_param.__set_exact(match_param_exact);

    runtime::BmAddEntryOptions options;

    client->bm_mt_add_entry(0, "smac", {match_param},
                            "_nop", std::vector<std::string>(),
                            options);

    std::vector<std::string> action_data =
      {std::string(reinterpret_cast<const char *>(&sample->ingress_port), 2)};

    client->bm_mt_add_entry(0, "dmac", {match_param},
                            "forward", std::move(action_data),
                            options);
  }

  client->bm_learning_ack_buffer(0, msg_info.list_id, msg_info.buffer_id);
}

}  // namespace

int main() {
  listener = new bm_apps::LearnListener("ipc:///tmp/bmv2-0-notifications.ipc");
  listener->register_cb(learn_cb, nullptr);
  listener->start();

  while (true) std::this_thread::sleep_for(std::chrono::seconds(100));

  return 0;
}
