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

#ifndef _BM_LEARNING_H_
#define _BM_LEARNING_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>

#include "packet.h"
#include "phv.h"
#include "bytecontainer.h"
#include "transport.h"

class LearnWriter {
public:
  virtual ~LearnWriter() { }

  virtual int send(const char *buffer, size_t len) const = 0;
  virtual int send_msgs(
      const std::initializer_list<TransportIface::MsgBuf> &msgs
  ) const = 0;
};

template <typename Transport>
class LearnWriterImpl : public LearnWriter {
public:
  LearnWriterImpl(const std::string &transport_name);

  int send(const char *buffer, size_t len) const override;
  int send_msgs(
      const std::initializer_list<TransportIface::MsgBuf> &msgs
  ) const override;

private:
  std::unique_ptr<Transport> transport_instance;
};

class LearnEngine {
public:
  typedef int list_id_t;
  typedef unsigned long long buffer_id_t;

  typedef struct {
    int switch_id;
    int list_id;
    unsigned long long buffer_id;
    unsigned int num_samples;
  } __attribute__((packed)) msg_hdr_t;

  typedef std::function<void(const msg_hdr_t &, size_t, std::unique_ptr<char []>, void *)> LearnCb;

  void list_create(list_id_t list_id,
		   size_t max_samples = 1, unsigned int timeout_ms = 1000);

  void list_set_learn_writer(list_id_t list_id,
			     std::shared_ptr<LearnWriter> learn_writer);
  void list_set_learn_cb(list_id_t list_id,
			 const LearnCb &learn_cb, void * cookie);

  void list_push_back_field(list_id_t list_id,
			    header_id_t header_id, int field_offset);
  void list_push_back_constant(list_id_t list_id,
			       const std::string &hexstring);

  void list_init(list_id_t list_id);

  void learn(list_id_t list_id, const Packet &pkt);

  void ack(list_id_t list_id, buffer_id_t buffer_id, int sample_id);
  void ack(list_id_t list_id, buffer_id_t buffer_id,
	   const std::vector<int> &sample_ids);
  void ack_buffer(list_id_t list_id, buffer_id_t buffer_id);

  void reset_state();

private:
  class LearnSampleBuilder {
  public:
    void push_back_constant(const ByteContainer &constant);
    void push_back_field(header_id_t header_id, int field_offset);

    void operator()(const PHV &phv, ByteContainer *sample) const;

  private:
    struct LearnSampleEntry {
      enum {FIELD, CONSTANT} tag;
      
      union {
	struct {
	  header_id_t header;
	  int offset;
	} field;
	
	struct {
	  unsigned int offset;
	} constant;
      };
    };

  private:
    std::vector<LearnSampleEntry> entries{};
    std::vector<ByteContainer> constants{};
  };

  typedef std::chrono::high_resolution_clock clock;
  typedef std::chrono::milliseconds milliseconds;
  
  typedef std::unordered_set<ByteContainer, ByteContainerKeyHash> LearnFilter;

  private:
  struct FilterPtrs {
    size_t unacked_count{0};
    std::vector<LearnFilter::iterator> buffer{0};
  };

  class LearnList {
  public:
    enum class LearnMode {NONE, WRITER, CB};

  public:
    LearnList(list_id_t list_id, size_t max_samples, unsigned int timeout);
    void init();

    ~LearnList();

    void set_learn_writer(std::shared_ptr<LearnWriter> learn_writer);
    void set_learn_cb(const LearnCb &learn_cb, void *cookie);

    void push_back_field(header_id_t header_id, int field_offset);
    void push_back_constant(const std::string &hexstring);

    void add_sample(const PHV &phv);

    void ack(buffer_id_t buffer_id, const std::vector<int> &sample_ids);
    void ack_buffer(buffer_id_t buffer_id);

    void reset_state();

    LearnList(const LearnList &other) = delete;
    LearnList &operator=(const LearnList &other) = delete;
    LearnList(LearnList &&other) = delete;
    LearnList &operator=(LearnList &&other) = delete;

  private:
    void swap_buffers();
    void buffer_transmit_loop();
    void buffer_transmit();

  private:
    mutable std::mutex mutex{};

    list_id_t list_id;

    LearnSampleBuilder builder{};
    std::vector<char> buffer{};
    buffer_id_t buffer_id{0};
    // size_t sample_size{0};
    size_t num_samples{0};
    const size_t max_samples;
    clock::time_point buffer_started{};
    clock::time_point last_sent{};
    const milliseconds timeout;
    const bool with_timeout;

    LearnFilter filter{};
    std::unordered_map<buffer_id_t, FilterPtrs> old_buffers{};

    std::vector<char> buffer_tmp{};
    mutable std::condition_variable b_can_swap{};
    mutable std::condition_variable b_can_send{};
    std::thread transmit_thread{};
    bool stop_transmit_thread{false};

    LearnMode learn_mode{LearnMode::NONE};

    // should I use a union here? or is it not worth the trouble?
    std::shared_ptr<LearnWriter> writer{nullptr};
    LearnCb cb_fn{};
    void *cb_cookie{nullptr};

    bool writer_busy{false};
    mutable std::condition_variable can_change_writer{};
  };

private:
  // LearnList is not movable because of the mutex, I am using pointers
  std::unordered_map<list_id_t, std::unique_ptr<LearnList> > learn_lists{};
};

#endif
