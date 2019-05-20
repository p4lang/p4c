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

#ifndef SIMPLE_SWITCH_GRPC_TESTS_UTILS_H_
#define SIMPLE_SWITCH_GRPC_TESTS_UTILS_H_

#include <p4/config/v1/p4info.grpc.pb.h>

#include <condition_variable>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace sswitch_grpc {

namespace testing {

int get_table_id(const p4::config::v1::P4Info &p4info,
                 const std::string &t_name);

int get_action_id(const p4::config::v1::P4Info &p4info,
                  const std::string &a_name);

int get_mf_id(const p4::config::v1::P4Info &p4info,
              const std::string &t_name, const std::string &mf_name);

int get_param_id(const p4::config::v1::P4Info &p4info,
                 const std::string &a_name, const std::string &param_name);

int get_digest_id(const p4::config::v1::P4Info &p4info,
                  const std::string &digest_name);

int get_act_prof_id(const p4::config::v1::P4Info &p4info,
                    const std::string &act_prof_name);

p4::config::v1::P4Info parse_p4info(const char *path);

template <typename StreamType, typename MessageType>
class StreamReceiver {
 public:
  using Clock = std::chrono::steady_clock;

  explicit StreamReceiver(StreamType *stream)
      : stream(stream) {
    read_thread = std::thread(
        &StreamReceiver<StreamType, MessageType>::receive, this);
  }

  ~StreamReceiver() {
    read_thread.join();
  }

  void receive() {
    MessageType msg;
    while (stream->Read(&msg)) {
      Lock lock(mutex);
      messages.push(
          std::unique_ptr<MessageType>(new MessageType(std::move(msg))));
      cvar.notify_one();
    }
  }

  template<typename Predicate, typename Rep, typename Period>
  std::unique_ptr<MessageType> get(
      Predicate predicate,
      const std::chrono::duration<Rep, Period> &timeout) {
    Lock lock(mutex);
    if (cvar.wait_until(
            lock, Clock::now() + timeout,
            [this, predicate] {
              return !messages.empty() && predicate(*messages.front()); })) {
      auto msg = std::move(messages.front());
      messages.pop();
      return msg;
    }
    return nullptr;
  }

 private:
  using Lock = std::unique_lock<std::mutex>;

  StreamType *stream;
  mutable std::mutex mutex{};
  mutable std::condition_variable cvar{};
  std::thread read_thread;
  std::queue<std::unique_ptr<MessageType> > messages;
};

}  // namespace testing

}  // namespace sswitch_grpc

#endif  // SIMPLE_SWITCH_GRPC_TESTS_UTILS_H_
