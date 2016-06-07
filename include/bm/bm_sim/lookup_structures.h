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
 * Gordon Bailey (gb@gordonbailey.net)
 *
 */

//! @file lookup_structures.h
//! This file contains 2 classes: bm::LookupStructure and
//! bm::LookupStructureFactory. When implementing your target, you may wish
//! to provide custom implementations of the data structures used to perform
//! matches. This can be done by inheriting from one or more of
//! bm::ExactLookupStructure, bm::LPMMatchStructure, and
//! bm::TernaryMatchStructure. Each of these is a specialization of the
//! bm::LookupStructure template for the corresponding match key type.
//!
//! Once the implementation of the new lookup structure is complete, you must
//! extend the bm::LookupStructureFactory class to build instances of it. This
//! is relatively simple. An (abridged) example of creating a new bm::Switch
//! using a custom lookup structure is as follows
//!
//! @code
//!
//! // Extend the appropriate LookupStructure type
//! class MyExactLookupStructure : public bm::ExactLookupStructure {
//!  public:
//!   virtual ~MyExactLookupStructure() = default;
//!
//!  bool lookup(const bm::ByteContainer &key_data,
//!              bm::internal_handle_t *handle) const override {
//!    // ...
//!  }
//!
//!  bool entry_exists(const bm::ExactMatchKey &key) const override {
//!    // ...
//!  }
//!
//!  void add_entry(const bm::ExactMatchKey &key,
//!                 bm::internal_handle_t handle) override {
//!    // ...
//!  }
//!
//!  void delete_entry(const bm::ExactMatchKey &key) override {
//!    // ...
//!  }
//!
//!  void clear() override {
//!    // ...
//!  }
//!
//!  private:
//!   // implementation details ...
//! };
//!
//! // Create a factory subclass which creates your new structure
//! class MyFactory : public bm::LookupStructureFactory {
//!  public:
//!   virtual ~MyFactory() = default;
//!
//!   // Note that it's only necessary to override the create function for the
//!   // particular match types you are interested in, the others will use the
//!   // default implementations.
//!   std::unique_ptr<bm::ExactLookupStructure>
//!   create_for_exact(size_t size, size_t nbytes_key) override {
//!     return std::unique_ptr<bm::ExactLookupStructure>(
//!                new MyExactLookupStructure(/* parameters */));
//!   }
//! };
//!
//! // Override bm::Switch to create your target
//! class MySwitch : public bm::Switch {
//!    // ...
//! };
//!
//! int main(int argc, const char *argv[]) {
//!   // Create an instance of your switch object
//!   auto switch = new MySwitch();
//!
//!   // Pass it an instance of your factory class.
//!   switch->set_lookup_factory(std::shared_ptr<bm::LookupStructureFactory>(
//!                                  new MyFactory()));
//!
//!   // Initialize the switch. Note that this must be done *after* calling
//!   // `set_lookup_factory`, otherwise the default lookup structures will
//!   // be used.
//!   int status = switch->init_from_command_line(argc, argv);
//!   // ...
//! }
//!
//! @endcode

#ifndef BM_BM_SIM_LOOKUP_STRUCTURES_H_
#define BM_BM_SIM_LOOKUP_STRUCTURES_H_

#include "match_key_types.h"
#include "bytecontainer.h"

namespace bm {

//! This class defines an interface for all data structures used
//! in Match Units to perform lookups. Custom data strucures can
//! be created by implementing this interface, and creating a
//! LookupStructureFactory subclass which will create them.
template <typename K>
class LookupStructure {
 public:
  virtual ~LookupStructure() = default;

  //! Look up a given key in the data structure.
  //! Return true and set handle to the found value if there is a match
  //! Return false if there is no match (value in handle is undefined)
  virtual bool lookup(const ByteContainer &key_data,
                      internal_handle_t *handle) const = 0;

  //! Check whether an entry exists. This is distinct from a lookup operation
  //! in that this will also match against the prefix length in the case of
  //! an LPM structure, and against the mask and priority in the case of a
  //! Ternary structure.
  virtual bool entry_exists(const K &key) const = 0;

  //! Store an entry in the lookup structure. Associates the given handle
  //! with the given entry.
  virtual void add_entry(const K &key, internal_handle_t handle) = 0;

  //! Remove a given entry from the structure. Has no effect if the entry
  //! does not exist.
  virtual void delete_entry(const K &key) = 0;

  //! Completely remove all entries from the data structure.
  virtual void clear() = 0;
};

// Convenience typedefs to simplify the code needed to override LookupStructure
// and LookupStructureFactory
typedef LookupStructure<ExactMatchKey>   ExactLookupStructure;
typedef LookupStructure<LPMMatchKey>     LPMLookupStructure;
typedef LookupStructure<TernaryMatchKey> TernaryLookupStructure;
typedef LookupStructure<RangeMatchKey>   RangeLookupStructure;

//! This class is used by match units to create instances of the appropriate
//! LookupStructure implementation. In order to use custom data structures in
//! a target, the data structures should be defined, and then a new subclass
//! of LookupStructureFactory should be created, overriding the
//! `create_for_<match type>` function or functions corresponding to the new
//! data structure.
class LookupStructureFactory {
 public:
  virtual ~LookupStructureFactory() = default;

  //! This is a utility to call the correct `create_for_<type>` function based
  //! on the bm::MatchKey subtype passed as the template parameter K. This is
  //! used by bm::MatchUnitGeneric when creating its lookup structure.
  template <typename K>
  static std::unique_ptr<LookupStructure<K> > create(
      LookupStructureFactory *f, size_t size, size_t nbytes_key);

  //! Create a lookup structure for exact matches.
  virtual std::unique_ptr<ExactLookupStructure>
  create_for_exact(size_t size, size_t nbytes_key);

  //! Create a lookup structure for LPM matches.
  virtual std::unique_ptr<LPMLookupStructure>
  create_for_LPM(size_t size, size_t nbytes_key);

  //! Create a lookup structure for ternary matches.
  virtual std::unique_ptr<TernaryLookupStructure>
  create_for_ternary(size_t size, size_t nbytes_key);

  //! Create a lookup structure for range macthes
  virtual std::unique_ptr<RangeLookupStructure>
  create_for_range(size_t size, size_t nbytes_key);
};


}  // namespace bm


#endif  // BM_BM_SIM_LOOKUP_STRUCTURES_H_
