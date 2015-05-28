#include "bm_sim/match_units.h"
#include "bm_sim/match_tables.h"

template<typename V>
MatchErrorCode
MatchUnitAbstract<V>::get_and_set_handle(entry_handle_t *handle)
{
  if(num_entries >= size) { // table is full
    return MatchErrorCode::TABLE_FULL;
  }
  
  if(handles.get_handle(handle)) return MatchErrorCode::ERROR;
  
  num_entries++;
  return MatchErrorCode::SUCCESS;
}

template<typename V>
MatchErrorCode
MatchUnitAbstract<V>::unset_handle(entry_handle_t handle)
{
  if(handles.release_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  
  num_entries--;
  return MatchErrorCode::SUCCESS;
}

template<typename V>
bool
MatchUnitAbstract<V>::valid_handle(entry_handle_t handle) const
{
  return handles.valid_handle(handle);
}

template<typename V>
typename MatchUnitAbstract<V>::MatchUnitLookup
MatchUnitAbstract<V>::lookup(const Packet &pkt) const
{
  static thread_local ByteContainer key;
  key.clear();
  build_key(*pkt.get_phv(), key);

  return lookup_key(key);
}

template<typename V>
typename MatchUnitExact<V>::MatchUnitLookup
MatchUnitExact<V>::lookup_key(const ByteContainer &key) const
{
  boost::shared_lock<boost::shared_mutex> lock(this->t_mutex);

  const auto entry_it = entries_map.find(key);
  if(entry_it == entries_map.end()) return MatchUnitLookup::empty_entry();
  return MatchUnitLookup(entry_it->second, &entries[entry_it->second].value);
}

template<typename V>
MatchErrorCode
MatchUnitExact<V>::add_entry(
  const std::vector<MatchKeyParam> &match_key, V value,
  entry_handle_t *handle, int priority
)
{
  (void) priority;

  ByteContainer new_key;
  new_key.reserve(this->nbytes_key);

  // take care of valid first

  for(const MatchKeyParam &param : match_key) {
    if(param.type == MatchKeyParam::Type::VALID)
      new_key.append(param.key);
  }

  for(const MatchKeyParam &param : match_key) {
    switch(param.type) {
    case MatchKeyParam::Type::EXACT:
      new_key.append(param.key);
      break;
    case MatchKeyParam::Type::VALID: // already done
      break;
    default:
      assert(0 && "invalid param type in match_key");
      break;
    }
  }

  boost::unique_lock<boost::shared_mutex> lock(this->t_mutex);

  MatchErrorCode status = this->get_and_set_handle(handle);
  if(status != MatchErrorCode::SUCCESS) return status;
  
  entries_map[new_key] = *handle; // key is copied, which is not great
  entries[*handle] = Entry(std::move(new_key), std::move(value));
  
  return MatchErrorCode::SUCCESS;
}

template<typename V>
MatchErrorCode
MatchUnitExact<V>::delete_entry(entry_handle_t handle)
{
  boost::unique_lock<boost::shared_mutex> lock(this->t_mutex);
  
  if(!this->valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  const ByteContainer &key = entries[handle].key;
  entries_map.erase(key);

  return this->unset_handle(handle);
}

template<typename V>
MatchErrorCode
MatchUnitExact<V>::modify_entry(entry_handle_t handle, V value)
{
  boost::unique_lock<boost::shared_mutex> lock(this->t_mutex);

  if(!this->valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  Entry &entry = entries[handle];
  entry.value = std::move(value);

  return MatchErrorCode::SUCCESS;
}


template<typename V>
typename MatchUnitLPM<V>::MatchUnitLookup
MatchUnitLPM<V>::lookup_key(const ByteContainer &key) const
{
  boost::shared_lock<boost::shared_mutex> lock(this->t_mutex);

  entry_handle_t handle;
  if(entries_trie.lookup(key, &handle)) {
    return MatchUnitLookup(handle, &entries[handle].value);
  }
  return MatchUnitLookup::empty_entry();
}

template<typename V>
MatchErrorCode
MatchUnitLPM<V>::add_entry(
  const std::vector<MatchKeyParam> &match_key, V value,
  entry_handle_t *handle, int priority
)
{
  (void) priority;

  ByteContainer new_key;
  new_key.reserve(this->nbytes_key);
  int prefix_length = 0;
  const MatchKeyParam *lpm_param = nullptr;

  for(const MatchKeyParam &param : match_key) {
    if(param.type == MatchKeyParam::Type::VALID)
      new_key.append(param.key);
  }

  for(const MatchKeyParam &param : match_key) {
    switch(param.type) {
    case MatchKeyParam::Type::EXACT:
      new_key.append(param.key);
      prefix_length += param.key.size();
      break;
    case MatchKeyParam::Type::LPM:
      assert(!lpm_param && "more than one lpm param in match key");
      lpm_param = &param;
      break;
    case MatchKeyParam::Type::VALID: // already done
      break;
    default:
      assert(0 && "invalid param type in match_key");
      break;
    }
  }

  assert(lpm_param && "no lpm param in match key");
  new_key.append(lpm_param->key);
  prefix_length += lpm_param->prefix_length;

  boost::unique_lock<boost::shared_mutex> lock(this->t_mutex);

  MatchErrorCode status = this->get_and_set_handle(handle);
  if(status != MatchErrorCode::SUCCESS) return status;
  
  // key is copied, which is not great
  entries_trie.insert_prefix(new_key, prefix_length, *handle);
  entries[*handle] = Entry(std::move(new_key), prefix_length, std::move(value));
  
  return MatchErrorCode::SUCCESS;
}

template<typename V>
MatchErrorCode
MatchUnitLPM<V>::delete_entry(entry_handle_t handle)
{
  boost::unique_lock<boost::shared_mutex> lock(this->t_mutex);
  
  if(!this->valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  const Entry &entry = entries[handle];
  assert(entries_trie.delete_prefix(entry.key, entry.prefix_length));

  return this->unset_handle(handle);
}

template<typename V>
MatchErrorCode
MatchUnitLPM<V>::modify_entry(entry_handle_t handle, V value)
{
  boost::unique_lock<boost::shared_mutex> lock(this->t_mutex);

  if(!this->valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  Entry &entry = entries[handle];
  entry.value = std::move(value);

  return MatchErrorCode::SUCCESS;
}


template<typename V>
typename MatchUnitTernary<V>::MatchUnitLookup
MatchUnitTernary<V>::lookup_key(const ByteContainer &key) const
{
  boost::shared_lock<boost::shared_mutex> lock(this->t_mutex);

  int max_priority = 0;
  bool match;

  const Entry *entry;
  const Entry *max_entry = nullptr;
  entry_handle_t max_handle = 0;

  for(auto it = this->handles.begin(); it != this->handles.end(); ++it) {
    entry = &entries[*it];

    if(entry->priority <= max_priority) continue;
    
    match = true;
    for(size_t byte_index = 0; byte_index < this->nbytes_key; byte_index++) {
      if(entry->key[byte_index] != (key[byte_index] & entry->mask[byte_index])) {
	match = false;
	break;
      }
    }

    if(match) {
      max_priority = entry->priority;
      max_entry = entry;
      max_handle = *it;
    }
  }

  if(max_entry) {
    return MatchUnitLookup(max_handle, &max_entry->value);
  }

  return MatchUnitLookup::empty_entry();
}

static std::string create_mask_from_pref_len(int prefix_length, int size) {
  std::string mask(size, '\x00');
  std::fill(mask.begin(), mask.begin() + (prefix_length / 8), '\xff');
  if(prefix_length % 8 != 0) {
    mask[prefix_length / 8] = (char) 0xFF << (8 - (prefix_length % 8));
  }
  return mask;
}

template<typename V>
MatchErrorCode
MatchUnitTernary<V>::add_entry(
  const std::vector<MatchKeyParam> &match_key, V value,
  entry_handle_t *handle, int priority
)
{
  ByteContainer new_key;
  ByteContainer new_mask;
  new_key.reserve(this->nbytes_key);
  new_mask.reserve(this->nbytes_key);

  for(const MatchKeyParam &param : match_key) {
    if(param.type == MatchKeyParam::Type::VALID) {
      new_key.append(param.key);
      new_mask.append("\xff");
    }
  }

  for(const MatchKeyParam &param : match_key) {
    switch(param.type) {
    case MatchKeyParam::Type::EXACT:
      new_key.append(param.key);
      new_mask.append(std::string(param.key.size(), '\xff'));
      break;
    case MatchKeyParam::Type::LPM:
      new_key.append(param.key);
      new_mask.append(create_mask_from_pref_len(param.prefix_length,
						param.key.size()));
      break;
    case MatchKeyParam::Type::TERNARY:
      new_key.append(param.key);
      new_mask.append(param.mask);
      break;
    case MatchKeyParam::Type::VALID: // already done
      break;
    default:
      assert(0 && "invalid param type in match_key");
      break;
    }
  }

  boost::unique_lock<boost::shared_mutex> lock(this->t_mutex);

  MatchErrorCode status = this->get_and_set_handle(handle);
  if(status != MatchErrorCode::SUCCESS) return status;
  
  entries[*handle] = Entry(std::move(new_key), std::move(new_mask), priority,
			   std::move(value));
  
  return MatchErrorCode::SUCCESS;
}

template<typename V>
MatchErrorCode
MatchUnitTernary<V>::delete_entry(entry_handle_t handle)
{
  boost::unique_lock<boost::shared_mutex> lock(this->t_mutex);
  
  if(!this->valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;

  return this->unset_handle(handle);
}

template<typename V>
MatchErrorCode
MatchUnitTernary<V>::modify_entry(entry_handle_t handle, V value)
{
  boost::unique_lock<boost::shared_mutex> lock(this->t_mutex);

  if(!this->valid_handle(handle)) return MatchErrorCode::INVALID_HANDLE;
  Entry &entry = entries[handle];
  entry.value = std::move(value);

  return MatchErrorCode::SUCCESS;
}


// explicit template instantiation

// I did not think I had to explicitly instantiate MatchUnitAbstract, because it
// is a base class for the others, but I get an linker error if I don't
template class MatchUnitAbstract<MatchTableAbstract::ActionEntry>;
template class MatchUnitAbstract<int>;

template class MatchUnitExact<MatchTableAbstract::ActionEntry>;
template class MatchUnitExact<int>;

template class MatchUnitLPM<MatchTableAbstract::ActionEntry>;
template class MatchUnitLPM<int>;

template class MatchUnitTernary<MatchTableAbstract::ActionEntry>;
template class MatchUnitTernary<int>;
