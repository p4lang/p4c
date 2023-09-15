/*
Copyright 2023-present Intel

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef LIB_HASHVEC_H_
#define LIB_HASHVEC_H_

#include <cstdint>

#include "bitvec.h"
#ifdef DEBUG
#include <iostream>
#endif

class hash_vector_base {
    struct internal;
    internal *info;
    union {
        uint8_t *s1;
        uint16_t *s2;
        uint32_t *s3;
        // FIXME -- add uint64_t for vectors of more than 2**32 elements?
    } hash;
    static uint32_t gethash_s1(const hash_vector_base *, size_t);
    static uint32_t gethash_s2(const hash_vector_base *, size_t);
    static uint32_t gethash_s3(const hash_vector_base *, size_t);
    static void sethash_s1(hash_vector_base *, size_t, uint32_t);
    static void sethash_s2(hash_vector_base *, size_t, uint32_t);
    static void sethash_s3(hash_vector_base *, size_t, uint32_t);
    void allochash();
    void freehash();

    size_t hashsize;                 /* size of the hash array - always a power-of-2-minus-1 */
    size_t collisions, log_hashsize; /* log(base 2) of hashsize+1 */

 public:
    struct lookup_cache {
        size_t slot;
        size_t collisions;
        uint32_t getidx(const hash_vector_base *);
        const void *getkey(const hash_vector_base *);
        void *getval(hash_vector_base *);
    };
#ifdef DEBUG
    void dump(std::ostream &);
#endif

 protected:
    size_t inuse;  /* number of elements in use in the table */
    bitvec erased; /* elements that have been erased */
    hash_vector_base(bool ismap, bool ismulti, size_t capacity);
    hash_vector_base(const hash_vector_base &);
    hash_vector_base(hash_vector_base &&);
    hash_vector_base &operator=(const hash_vector_base &);
    hash_vector_base &operator=(hash_vector_base &&);
    virtual ~hash_vector_base() { freehash(); }
    virtual size_t hashfn(const void *) const = 0;
    virtual bool cmpfn(const void *, const void *) const = 0;
    virtual bool cmpfn(const void *, size_t) const = 0;
    virtual const void *getkey(uint32_t) const = 0;
    virtual void *getval(uint32_t) = 0;
    virtual uint32_t limit() = 0;
    virtual void resizedata(size_t) = 0;
    virtual void moveentry(size_t, size_t) = 0;

    void clear();
    size_t find(const void *key, lookup_cache *cache) const;
    size_t find_next(const void *key, lookup_cache *cache) const;
    void *lookup(const void *key, lookup_cache *cache = nullptr);
    void *lookup_next(const void *key, lookup_cache *cache = nullptr);
    size_t hv_insert(const void *key, lookup_cache *cache = nullptr);
    // returns data index caller needs to store value in; if == data.size(), need emplace_back
    // also need to clear erased[index]
    size_t remove(const void *key, lookup_cache *cache = nullptr);
    // returns data index caller needs to clear (if < data.size()) or -1 if nothing removed
    void redo_hash();
};

#endif /* LIB_HASHVEC_H_ */
