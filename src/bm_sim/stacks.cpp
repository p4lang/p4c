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

template <typename T>
Stack<T>::Stack(const std::string &name, p4object_id_t id)
    : StackIface(), NamedP4Object(name, id) { }

template <typename T>
size_t
Stack<T>::pop_front() {
  if (next == 0) return 0u;
  next--;
  for (size_t i = 0; i < next; i++) {
    elements[i].get().swap_values(&elements[i + 1].get());
  }
  elements[next].get().mark_invalid();
  return 1u;
}

template <typename T>
size_t
Stack<T>::pop_front(size_t num) {
  if (num == 0) return 0;
  size_t popped = std::min(next, num);
  next -= popped;
  for (size_t i = 0; i < next; i++) {
    elements[i].get().swap_values(&elements[i + num].get());
  }
  for (size_t i = next; i < next + popped; i++) {
    elements[i].get().mark_invalid();
  }
  return popped;
}

template <typename T>
size_t
Stack<T>::push_front() {
  if (next < elements.size()) next++;
  for (size_t i = next - 1; i > 0; i--) {
    elements[i].get().swap_values(&elements[i - 1].get());
  }
  // TODO(antonin): do I want to reset the element as well?
  // this may be complicated given the design
  elements[0].get().mark_valid();
  return 1u;
}

template <typename T>
size_t
Stack<T>::push_front(size_t num) {
  if (num == 0) return 0;
  next = std::min(elements.size(), next + num);
  for (size_t i = next - 1; i > num - 1; i--) {
    elements[i].get().swap_values(&elements[i - num].get());
  }
  size_t pushed = std::min(elements.size(), num);
  for (size_t i = 0; i < pushed; i++) {
    elements[i].get().mark_valid();
  }
  return pushed;
}

template <typename T>
size_t
Stack<T>::pop_back() {
  if (next == 0) return 0u;
  next--;
  elements[next].get().mark_invalid();
  return 1u;
}

template <typename T>
size_t
Stack<T>::push_back() {
  if (next == elements.size()) return 0u;
  elements[next].get().mark_valid();
  next++;
  return 1u;
}

template <typename T>
size_t
Stack<T>::get_depth() const {
  return elements.size();
}

template <typename T>
size_t
Stack<T>::get_count() const {
  return next;
}

template <typename T>
bool
Stack<T>::is_full() const {
  return (next >= elements.size());
}

template <typename T>
void
Stack<T>::reset() {
  next = 0;
}

template <typename T>
T &
Stack<T>::get_last() {
  assert(next > 0 && "stack empty");
  return elements[next - 1];
}

template <typename T>
const T &
Stack<T>::get_last() const {
  assert(next > 0 && "stack empty");
  return elements[next - 1];
}

template <typename T>
T &
Stack<T>::get_next() {
  assert(next < elements.size() && "stack full");
  return elements[next];
}

template <typename T>
const T &
Stack<T>::get_next() const {
  assert(next < elements.size() && "stack full");
  return elements[next];
}

template <typename T>
T &
Stack<T>::at(size_t idx) {
  return elements.at(idx);
}

template <typename T>
const T &
Stack<T>::at(size_t idx) const {
  return elements.at(idx);
}

template <typename T>
void
Stack<T>::set_next_element(T &e) {  // NOLINT(runtime/references)
  elements.emplace_back(e);
}

// explicit instantiation
template class Stack<Header>;
template class Stack<HeaderUnion>;

}  // namespace bm
