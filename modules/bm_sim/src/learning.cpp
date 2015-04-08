#include <cassert>

#include "bm_sim/learning.h"

template <typename Transport>
LearnWriterImpl<Transport>::LearnWriterImpl(const std::string &transport_name)
  : transport_instance(std::unique_ptr<Transport>(new Transport()))
{
  // should not be doing this in the constructor ?
  transport_instance->open(transport_name);
}

template <typename Transport>
int LearnWriterImpl<Transport>::send(const char *buffer, size_t len) const 
{
  return transport_instance->send(buffer, len);
}

void LearnEngine::LearnSampleBuilder::push_back_constant(
    const ByteContainer &constant
)
{
  unsigned int offset = constants.size();
  constants.push_back(constant);
  LearnSampleEntry entry;
  entry.tag = LearnSampleEntry::CONSTANT;
  entry.constant.offset = offset;
  entries.push_back(entry);
}

void LearnEngine::LearnSampleBuilder::push_back_field(
    header_id_t header_id, int field_offset
)
{
  LearnSampleEntry entry;
  entry.tag = LearnSampleEntry::FIELD;
  entry.field.header = header_id;
  entry.field.offset = field_offset;
  entries.push_back(entry);
}

void LearnEngine::LearnSampleBuilder::operator()(
    const PHV &phv, ByteContainer *sample
) const
{
  for(const LearnSampleEntry &entry : entries) {
    const ByteContainer *bytes;
    switch(entry.tag) {
    case LearnSampleEntry::FIELD:
      bytes = &phv.get_field(entry.field.header, entry.field.offset).get_bytes();
      break;
    case LearnSampleEntry::CONSTANT:
      bytes = &constants[entry.constant.offset];
      break;
    }
    sample->append(*bytes);
    // buffer.insert(buffer.end(), bytes->begin(), bytes->end());
  }
}

LearnEngine::LearnList::LearnList(size_t max_samples, unsigned int timeout)
  : max_samples(max_samples), last_sent(clock::now()), timeout(timeout) { }

void LearnEngine::LearnList::init()
{
  transmit_thread = std::thread(&LearnList::buffer_transmit_loop, this);
  transmit_thread.detach();
}

void LearnEngine::LearnList::set_learn_writer(
    std::shared_ptr<LearnWriter> learn_writer
)
{
  writer = learn_writer;
}

void LearnEngine::LearnList::push_back_field(
    header_id_t header_id, int field_offset
)
{
  builder.push_back_field(header_id, field_offset);
}

void LearnEngine::LearnList::push_back_constant(const std::string &hexstring)
{
  builder.push_back_constant(ByteContainer(hexstring));
}

void LearnEngine::LearnList::add_sample(const PHV &phv)
{
  static thread_local ByteContainer sample;
  sample.clear();
  builder(phv, &sample);

  std::unique_lock<std::mutex> lock(mutex);

  const auto it = filter.find(sample);
  if(it != filter.end()) return;

  buffer.insert(buffer.end(), sample.begin(), sample.end());
  num_samples++;

  filter.insert(filter.end(), std::move(sample));

  clock::time_point now = clock::now();
  using std::chrono::duration_cast;
  milliseconds elapsed = duration_cast<milliseconds>(now - last_sent);
  if(num_samples >= max_samples || elapsed >= timeout) {
    while(buffer_tmp.size() != 0) {
      b_can_swap.wait(lock);
    }
    buffer_tmp.swap(buffer);
    num_samples = 0;
    last_sent = now;

    lock.unlock();
    b_can_send.notify_one();
  }
}

void LearnEngine::LearnList::buffer_transmit_loop()
{
  std::unique_lock<std::mutex> lock(mutex);
  while(buffer_tmp.size() == 0) {
    b_can_send.wait(lock);
  }

  writer->send(buffer_tmp.data(), buffer_tmp.size());
  buffer_tmp.clear();

  lock.unlock();
  b_can_swap.notify_one();
}

void LearnEngine::list_create(
    list_id_t list_id, std::shared_ptr<LearnWriter> learn_writer,
    size_t max_samples, unsigned int timeout_ms
)
{
  assert(learn_lists.find(list_id) == learn_lists.end());
  learn_lists[list_id] =
    std::unique_ptr<LearnList>(new LearnList(max_samples, timeout_ms));
  learn_lists[list_id]->set_learn_writer(learn_writer);
}

void LearnEngine::list_push_back_field(
    list_id_t list_id, header_id_t header_id, int field_offset
) {
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->push_back_field(header_id, field_offset);
}

void LearnEngine::list_push_back_constant(
    list_id_t list_id, const std::string &hexstring
) {
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->push_back_constant(hexstring);
}

void LearnEngine::list_init(list_id_t list_id)
{
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->init();
}

void LearnEngine::learn(list_id_t list_id, const Packet &pkt) {
  // TODO : event logging
  auto it = learn_lists.find(list_id);
  assert(it != learn_lists.end());
  LearnList *list = it->second.get();
  list->add_sample(*pkt.get_phv());
}
