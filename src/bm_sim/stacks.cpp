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

#include <bm/bm_sim/stacks.h>
#include <bm/bm_sim/headers.h>
#include <bm/bm_sim/header_unions.h>

#include <algorithm>  // std::min
#include <string>

namespace bm {

namespace detail {

// legacy implementation of push_front and pop_front

template <typename T>
StackLegacy<T>::StackLegacy(const std::string &name, p4object_id_t id)
    : Stack<T, StackLegacy<T> >(name, id) { }

template <typename T>
size_t
StackLegacy<T>::pop_front() {
  if (this->next == 0) return 0u;
  this->next--;
  for (size_t i = 0; i < this->next; i++) {
    this->elements[i].get().swap_values(&this->elements[i + 1].get());
  }
  this->elements[this->next].get().mark_invalid();
  return 1u;
}

template <typename T>
size_t
StackLegacy<T>::pop_front(size_t num) {
  if (num == 0) return 0;
  size_t popped = std::min(this->next, num);
  this->next -= popped;
  for (size_t i = 0; i < this->next; i++) {
    this->elements[i].get().swap_values(&this->elements[i + num].get());
  }
  for (size_t i = this->next; i < this->next + popped; i++) {
    this->elements[i].get().mark_invalid();
  }
  return popped;
}

template <typename T>
size_t
StackLegacy<T>::push_front() {
  if (this->next < this->elements.size()) this->next++;
  for (size_t i = this->next - 1; i > 0; i--) {
    this->elements[i].get().swap_values(&this->elements[i - 1].get());
  }
  this->elements[0].get().mark_valid();
  return 1u;
}

template <typename T>
size_t
StackLegacy<T>::push_front(size_t num) {
  if (num == 0) return 0;
  this->next = std::min(this->elements.size(), this->next + num);
  for (size_t i = this->next - 1; i > num - 1; i--) {
    this->elements[i].get().swap_values(&this->elements[i - num].get());
  }
  size_t pushed = std::min(this->elements.size(), num);
  for (size_t i = 0; i < pushed; i++) {
    this->elements[i].get().mark_valid();
  }
  return pushed;
}

// P4_16-conformant implementation of push_front and pop_front

template <typename T>
StackP4_16<T>::StackP4_16(const std::string &name, p4object_id_t id)
    : Stack<T, StackP4_16<T> >(name, id) { }

template <typename T>
size_t
StackP4_16<T>::pop_front() {
  if (this->next > 0) this->next--;
  auto size = this->elements.size();
  for (size_t i = 0; i < size - 1; i++) {
    this->elements[i].get().swap_values(&this->elements[i + 1].get());
  }
  this->elements[size - 1].get().mark_invalid();
  return 1u;
}

template <typename T>
size_t
StackP4_16<T>::pop_front(size_t num) {
  if (num == 0) return 0;
  auto size = this->elements.size();
  this->next -= std::min(this->next, num);
  size_t i = 0;
  for (; i < size - num; i++) {
    this->elements[i].get().swap_values(&this->elements[i + num].get());
  }
  for (; i < size; i++) {
    this->elements[i].get().mark_invalid();
  }
  return std::min(num, size);
}

template <typename T>
size_t
StackP4_16<T>::push_front() {
  auto size = this->elements.size();
  if (this->next < size) this->next++;
  for (size_t i = size - 1; i > 0; i--) {
    this->elements[i].get().swap_values(&this->elements[i - 1].get());
  }
  this->elements[0].get().mark_invalid();
  return 1u;
}

template <typename T>
size_t
StackP4_16<T>::push_front(size_t num) {
  if (num == 0) return 0;
  auto size = this->elements.size();
  this->next = std::min(size, this->next + num);
  for (size_t i = size - 1; i > num - 1; i--) {
    this->elements[i].get().swap_values(&this->elements[i - num].get());
  }
  size_t pushed = std::min(size, num);
  for (size_t i = 0; i < pushed; i++) {
    this->elements[i].get().mark_invalid();
  }
  return pushed;
}

// explicit instantiation
template class StackLegacy<Header>;
template class StackLegacy<HeaderUnion>;
template class StackP4_16<Header>;
template class StackP4_16<HeaderUnion>;

}  // namespace detail

template <typename T, typename ShiftImpl>
Stack<T, ShiftImpl>::Stack(const std::string &name, p4object_id_t id)
    : StackIface(), NamedP4Object(name, id) { }

template <typename T, typename ShiftImpl>
size_t
Stack<T, ShiftImpl>::pop_front() {
  return static_cast<ShiftImpl *>(this)->pop_front();
}

template <typename T, typename ShiftImpl>
size_t
Stack<T, ShiftImpl>::pop_front(size_t num) {
  return static_cast<ShiftImpl *>(this)->pop_front(num);
}

template <typename T, typename ShiftImpl>
size_t
Stack<T, ShiftImpl>::push_front() {
  return static_cast<ShiftImpl *>(this)->push_front();
}

template <typename T, typename ShiftImpl>
size_t
Stack<T, ShiftImpl>::push_front(size_t num) {
  return static_cast<ShiftImpl *>(this)->push_front(num);
}

template <typename T, typename ShiftImpl>
size_t
Stack<T, ShiftImpl>::pop_back() {
  if (next == 0) return 0u;
  next--;
  elements[next].get().mark_invalid();
  return 1u;
}

template <typename T, typename ShiftImpl>
size_t
Stack<T, ShiftImpl>::push_back() {
  if (next == elements.size()) return 0u;
  elements[next].get().mark_valid();
  next++;
  return 1u;
}

template <typename T, typename ShiftImpl>
size_t
Stack<T, ShiftImpl>::get_depth() const {
  return elements.size();
}

template <typename T, typename ShiftImpl>
size_t
Stack<T, ShiftImpl>::get_count() const {
  return next;
}

template <typename T, typename ShiftImpl>
bool
Stack<T, ShiftImpl>::is_full() const {
  return (next >= elements.size());
}

template <typename T, typename ShiftImpl>
void
Stack<T, ShiftImpl>::reset() {
  next = 0;
}

template <typename T, typename ShiftImpl>
T &
Stack<T, ShiftImpl>::get_last() {
  assert(next > 0 && "stack empty");
  return elements[next - 1];
}

template <typename T, typename ShiftImpl>
const T &
Stack<T, ShiftImpl>::get_last() const {
  assert(next > 0 && "stack empty");
  return elements[next - 1];
}

template <typename T, typename ShiftImpl>
T &
Stack<T, ShiftImpl>::get_next() {
  assert(next < elements.size() && "stack full");
  return elements[next];
}

template <typename T, typename ShiftImpl>
const T &
Stack<T, ShiftImpl>::get_next() const {
  assert(next < elements.size() && "stack full");
  return elements[next];
}

template <typename T, typename ShiftImpl>
T &
Stack<T, ShiftImpl>::at(size_t idx) {
  return elements.at(idx);
}

template <typename T, typename ShiftImpl>
const T &
Stack<T, ShiftImpl>::at(size_t idx) const {
  return elements.at(idx);
}

template <typename T, typename ShiftImpl>
void
Stack<T, ShiftImpl>::set_next_element(T &e) {  // NOLINT(runtime/references)
  elements.emplace_back(e);
}

// explicit instantiation
template class Stack<Header, detail::StackLegacy<Header> >;
template class Stack<Header, detail::StackP4_16<Header> >;
template class Stack<HeaderUnion, detail::StackLegacy<HeaderUnion> >;
template class Stack<HeaderUnion, detail::StackP4_16<HeaderUnion> >;

}  // namespace bm
