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

#include "packet.h"
#include "phv.h"
#include "bytecontainer.h"
#include "transport.h"

class LearnWriter {
public:
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
  virtual int send_msgs(
      const std::initializer_list<TransportIface::MsgBuf> &msgs
  ) const override;

private:
  std::unique_ptr<Transport> transport_instance;
};

class LearnEngine {
public:
  typedef int list_id_t;

  typedef struct {
    int switch_id;
    int list_id;
    unsigned long long buffer_id;
    unsigned int num_samples;
  } __attribute__((packed)) msg_hdr_t;

  void list_create(list_id_t list_id, std::shared_ptr<LearnWriter> learn_writer,
		   size_t max_samples = 1, unsigned int timeout_ms = 1000);

  void list_push_back_field(list_id_t list_id,
			    header_id_t header_id, int field_offset);
  void list_push_back_constant(list_id_t list_id,
			       const std::string &hexstring);

  void list_init(list_id_t list_id);

  void learn(list_id_t list_id, const Packet &pkt);

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
    std::vector<LearnSampleEntry> entries;
    std::vector<ByteContainer> constants;
  };

  typedef std::chrono::high_resolution_clock clock;
  typedef std::chrono::milliseconds milliseconds;
  
  typedef std::unordered_set<ByteContainer, ByteContainerKeyHash> LearnFilter;

  typedef unsigned long long buffer_id_t;

  class LearnList {
  public:
    LearnList(list_id_t list_id, size_t max_samples, unsigned int timeout);
    void init();

    void set_learn_writer(std::shared_ptr<LearnWriter> learn_writer);

    void push_back_field(header_id_t header_id, int field_offset);
    void push_back_constant(const std::string &hexstring);

    void add_sample(const PHV &phv);

  private:
    void swap_buffers();
    void buffer_transmit_loop();
    
  private:
    mutable std::mutex mutex;

    list_id_t list_id;

    LearnSampleBuilder builder;
    std::vector<char> buffer;
    buffer_id_t buffer_id{0};
    // size_t sample_size{0};
    size_t num_samples{0};
    const size_t max_samples;
    clock::time_point last_sent;
    const milliseconds timeout;
    const bool with_timeout;

    LearnFilter filter;

    std::vector<char> buffer_tmp;
    mutable std::condition_variable b_can_swap;
    mutable std::condition_variable b_can_send;
    std::thread transmit_thread;

    std::shared_ptr<LearnWriter> writer{nullptr};
  };

private:
  // LearnList is not movable because of the mutex, I am using pointers
  std::unordered_map<list_id_t, std::unique_ptr<LearnList> > learn_lists;
};

#endif
