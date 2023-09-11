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

#include "hashvec.h"

#include "exceptions.h"

/* FIXME -- there are almost certainly problems when the hashtable size
 * FIXME -- exceeds INT_MAX elements.  That requires 16GB+ memory... */

struct hash_vector_base::internal {
    uint32_t (*gethash)(const hash_vector_base *, size_t);
    void (*sethash)(hash_vector_base *, size_t, uint32_t);
    size_t maxset;
    char ismap, ismulti, hashelsize;
};

uint32_t hash_vector_base::gethash_s1(const hash_vector_base *ht, size_t i) {
    return ht->hash.s1[i];
}
uint32_t hash_vector_base::gethash_s2(const hash_vector_base *ht, size_t i) {
    return ht->hash.s2[i];
}
uint32_t hash_vector_base::gethash_s3(const hash_vector_base *ht, size_t i) {
    return ht->hash.s3[i];
}
void hash_vector_base::sethash_s1(hash_vector_base *ht, size_t i, uint32_t v) {
    ht->hash.s1[i] = v;
}
void hash_vector_base::sethash_s2(hash_vector_base *ht, size_t i, uint32_t v) {
    ht->hash.s2[i] = v;
}
void hash_vector_base::sethash_s3(hash_vector_base *ht, size_t i, uint32_t v) {
    ht->hash.s3[i] = v;
}

static size_t mod_f(size_t x) { return x % 0xf; }
static size_t mod_1f(size_t x) { return x % 0x1f; }
static size_t mod_3f(size_t x) { return x % 0x3f; }
static size_t mod_7f(size_t x) { return x % 0x7f; }
static size_t mod_ff(size_t x) { return x % 0xff; }
static size_t mod_1ff(size_t x) { return x % 0x1ff; }
static size_t mod_3ff(size_t x) { return x % 0x3ff; }
static size_t mod_7ff(size_t x) { return x % 0x7ff; }
static size_t mod_fff(size_t x) { return x % 0xfff; }
static size_t mod_1fff(size_t x) { return x % 0x1fff; }
static size_t mod_3fff(size_t x) { return x % 0x3fff; }
static size_t mod_7fff(size_t x) { return x % 0x7fff; }
static size_t mod_ffff(size_t x) { return x % 0xffff; }
static size_t mod_1ffff(size_t x) { return x % 0x1ffff; }
static size_t mod_3ffff(size_t x) { return x % 0x3ffff; }
static size_t mod_7ffff(size_t x) { return x % 0x7ffff; }
static size_t mod_fffff(size_t x) { return x % 0xfffff; }
static size_t mod_1fffff(size_t x) { return x % 0x1fffff; }
static size_t mod_3fffff(size_t x) { return x % 0x3fffff; }
static size_t mod_7fffff(size_t x) { return x % 0x7fffff; }
static size_t mod_ffffff(size_t x) { return x % 0xffffff; }
static size_t mod_1ffffff(size_t x) { return x % 0x1ffffff; }
static size_t mod_3ffffff(size_t x) { return x % 0x3ffffff; }
static size_t mod_7ffffff(size_t x) { return x % 0x7ffffff; }
static size_t mod_fffffff(size_t x) { return x % 0xfffffff; }
static size_t mod_1fffffff(size_t x) { return x % 0x1fffffff; }
static size_t mod_3fffffff(size_t x) { return x % 0x3fffffff; }
static size_t mod_7fffffff(size_t x) { return x % 0x7fffffff; }
static size_t mod_ffffffff(size_t x) { return x % 0xffffffff; }

static size_t (*mod_hashsize[])(size_t x) = {
    0,
    0,
    0,
    0,
    mod_f,
    mod_1f,
    mod_3f,
    mod_7f,
    mod_ff,
    mod_1ff,
    mod_3ff,
    mod_7ff,
    mod_fff,
    mod_1fff,
    mod_3fff,
    mod_7fff,
    mod_ffff,
    mod_1ffff,
    mod_3ffff,
    mod_7ffff,
    mod_fffff,
    mod_1fffff,
    mod_3fffff,
    mod_7fffff,
    mod_ffffff,
    mod_1ffffff,
    mod_3ffffff,
    mod_7ffffff,
    mod_fffffff,
    mod_1fffffff,
    mod_3fffffff,
    mod_7fffffff,
    mod_ffffffff,
};

void hash_vector_base::allochash() {
    // FIXME -- all switch cases are doing the same thing (just with different types) so
    // it should be possible to simplify this
    switch (info->hashelsize) {
        case sizeof(uint8_t):
            hash.s1 = new uint8_t[hashsize];
            memset(hash.s1, 0, hashsize * info->hashelsize);
            break;
        case sizeof(uint16_t):
            hash.s2 = new uint16_t[hashsize];
            memset(hash.s2, 0, hashsize * info->hashelsize);
            break;
        case sizeof(uint32_t):
            hash.s3 = new uint32_t[hashsize];
            memset(hash.s3, 0, hashsize * info->hashelsize);
            break;
    }
}

void hash_vector_base::freehash() {
    // FIXME -- all switch cases are doing the same thing (just with different types) so
    // it should be possible to simplify this
    switch (info->hashelsize) {
        case sizeof(uint8_t):
            delete[] hash.s1;
            break;
        case sizeof(uint16_t):
            delete[] hash.s2;
            break;
        case sizeof(uint32_t):
            delete[] hash.s2;
            break;
    }
    hash.s1 = nullptr;
}

hash_vector_base::hash_vector_base(bool ismap, bool ismulti, size_t capacity) {
    static internal formats[] = {{hash_vector_base::gethash_s1, hash_vector_base::sethash_s1,
                                  UINT8_MAX, 0, 0, sizeof(uint8_t)},
                                 {hash_vector_base::gethash_s2, hash_vector_base::sethash_s2,
                                  UINT16_MAX, 0, 0, sizeof(uint16_t)},
                                 {hash_vector_base::gethash_s3, hash_vector_base::sethash_s3,
                                  UINT32_MAX, 0, 0, sizeof(uint32_t)},
                                 {hash_vector_base::gethash_s1, hash_vector_base::sethash_s1,
                                  UINT8_MAX, 1, 0, sizeof(uint8_t)},
                                 {hash_vector_base::gethash_s2, hash_vector_base::sethash_s2,
                                  UINT16_MAX, 1, 0, sizeof(uint16_t)},
                                 {hash_vector_base::gethash_s3, hash_vector_base::sethash_s3,
                                  UINT32_MAX, 1, 0, sizeof(uint32_t)},
                                 {hash_vector_base::gethash_s1, hash_vector_base::sethash_s1,
                                  UINT8_MAX, 0, 1, sizeof(uint8_t)},
                                 {hash_vector_base::gethash_s2, hash_vector_base::sethash_s2,
                                  UINT16_MAX, 0, 1, sizeof(uint16_t)},
                                 {hash_vector_base::gethash_s3, hash_vector_base::sethash_s3,
                                  UINT32_MAX, 0, 1, sizeof(uint32_t)},
                                 {hash_vector_base::gethash_s1, hash_vector_base::sethash_s1,
                                  UINT8_MAX, 1, 1, sizeof(uint8_t)},
                                 {hash_vector_base::gethash_s2, hash_vector_base::sethash_s2,
                                  UINT16_MAX, 1, 1, sizeof(uint16_t)},
                                 {hash_vector_base::gethash_s3, hash_vector_base::sethash_s3,
                                  UINT32_MAX, 1, 1, sizeof(uint32_t)},
                                 {0, 0, 0, 0, 0, 0}};

    hashsize = 15;
    log_hashsize = 4;
    while (hashsize / 2 < capacity) {
        hashsize += hashsize + 1;
        log_hashsize++;
    }
    BUG_CHECK(hashsize + 1 == (1UL << log_hashsize), "hash corrupt");
    info = formats + (ismap ? 3 : 0) + (ismulti ? 6 : 0);
    BUG_CHECK(info->ismap == ismap, "corrupt");
    while (capacity > info->maxset) {
        info++;
        BUG_CHECK(info->ismap == ismap, "capacity %d exceeds max", capacity);
    }
    allochash();
    inuse = collisions = 0;
}

hash_vector_base::hash_vector_base(const hash_vector_base &a)
    : info(a.info),
      hashsize(a.hashsize),
      collisions(a.collisions),
      log_hashsize(a.log_hashsize),
      inuse(a.inuse),
      erased(a.erased) {
    allochash();
    memcpy(hash.s1, a.hash.s1, hashsize * info->hashelsize);
}

hash_vector_base::hash_vector_base(hash_vector_base &&a)
    : info(a.info),
      hashsize(a.hashsize),
      collisions(a.collisions),
      log_hashsize(a.log_hashsize),
      inuse(a.inuse),
      erased(a.erased) {
    hash.s1 = a.hash.s1;
    a.hash.s1 = nullptr;
}

hash_vector_base &hash_vector_base::operator=(const hash_vector_base &a) {
    freehash();
    info = a.info;
    hashsize = a.hashsize;
    inuse = a.inuse;
    collisions = a.collisions;
    log_hashsize = a.log_hashsize;
    erased = a.erased;
    allochash();
    memcpy(hash.s1, a.hash.s1, hashsize * info->hashelsize);
    return *this;
}

hash_vector_base &hash_vector_base::operator=(hash_vector_base &&a) {
    freehash();
    info = a.info;
    hashsize = a.hashsize;
    inuse = a.inuse;
    collisions = a.collisions;
    log_hashsize = a.log_hashsize;
    erased = a.erased;
    hash.s1 = a.hash.s1;
    a.hash.s1 = nullptr;
    return *this;
}

void hash_vector_base::clear() {
    freehash();
    while (info->hashelsize > 1) --info;
    hashsize = 15;
    collisions = 0;
    log_hashsize = 4;
    inuse = 0;
    erased.clear();
    allochash();
}

size_t hash_vector_base::find(const void *key, lookup_cache *cache) const {
    size_t hash = mod_hashsize[log_hashsize](hashfn(key));
    size_t idx, collisions = 0;
    while ((idx = info->gethash(this, hash)) && (erased[idx - 1] || !cmpfn(key, idx - 1))) {
        if (++collisions >= hashsize) {
            idx = 0;
            break;
        }
        if (++hash == hashsize) hash = 0;
    }
    cache->slot = hash;
    cache->collisions = collisions;
    return idx;
}

size_t hash_vector_base::find_next(const void *key, lookup_cache *cache) const {
    size_t hash = cache->slot;
    size_t idx, collisions = cache->collisions;
    if (!(idx = info->gethash(this, hash))) return idx;
    if (++hash == hashsize) hash = 0;
    while ((idx = info->gethash(this, hash)) && (erased[idx - 1] || !cmpfn(key, idx - 1))) {
        if (++collisions >= hashsize) {
            idx = 0;
            break;
        }
        if (++hash == hashsize) hash = 0;
    }
    cache->slot = hash;
    cache->collisions = collisions;
    return idx;
}

void *hash_vector_base::lookup(const void *key, lookup_cache *cache) {
    size_t idx;
    lookup_cache local;
    if (!cache) cache = &local;
    idx = find(key, cache);
    return idx ? getval(idx - 1) : 0;
}

void *hash_vector_base::lookup_next(const void *key, lookup_cache *cache) {
    size_t idx = 0;
    if (info->ismulti) idx = find_next(key, cache);
    return idx ? getval(idx - 1) : 0;
}

void hash_vector_base::redo_hash() {
    size_t i, j;
    lookup_cache cache;
    memset(hash.s1, 0, hashsize * info->hashelsize);
    collisions = 0;
    size_t limit = this->limit();
    auto erased = this->erased;
    this->erased.clear();
    for (i = j = 0; i < limit; i++) {
        if (erased[i]) continue;
        const void *k = getkey(i);
        if (find(k, &cache)) {
            if (info->ismulti)
                while (find_next(k, &cache))
                    ;
            else
                BUG("dupliacte keys in hash_vector_base::redo_hash");
        }
        if (j != i) {
            moveentry(j, i);
        }
        info->sethash(this, cache.slot, ++j);
        collisions += cache.collisions;
    }
    resizedata(j);
    inuse = j;
}

size_t hash_vector_base::hv_insert(const void *key, lookup_cache *cache) {
    lookup_cache local;
    size_t idx;
    if (cache) {
#ifndef NDEBUG
        if (find(key, &local) && info->ismulti)
            while (find_next(key, &local))
                ;
        BUG_CHECK(local.slot == cache->slot && local.collisions == cache->collisions,
                  "invalid cache in hash_vector_base::hv_insert");
#endif
    } else {
        cache = &local;
        if (find(key, cache) && info->ismulti)
            while (find_next(key, cache))
                ;
    }
    if ((collisions += cache->collisions) > hashsize / 2) {
        /* Too many collisions -- redo hash table */
        if (inuse * 3 < limit() * 2) {
            /* Many empty slots from removals, so don't expand anything */
        } else if (/* !ht->hashfn[1] && */ hashsize > inuse * 10) {
            /* Hashtable 90% empty and no alternate hash function.   Hash
             * function isn't working but expanding further will likely
             * make things worse, not better.
             * Nothing we can do, so punt */
        } else /* if (!ht->hashfn[1] || ht->hashsize < ht->limit*3) */ {
            /* No alternate hashfn, or hashtable pretty full, expand hash */
            hashsize += hashsize + 1;
            log_hashsize++;
            BUG_CHECK((hashsize & (hashsize + 1)) == 0, "hash corruption");
            BUG_CHECK(hashsize + 1 == (1UL << log_hashsize), "hash corruption");
            freehash();
            allochash();
#if 0
        } else {
            // FIXME -- figure out a way to allow changing the hash function in this case
            ht->hashfn++;
#endif
        }
        redo_hash();
        if (find(key, cache) && info->ismulti)
            while (find_next(key, cache))
                ;
    }
    if ((idx = info->gethash(this, cache->slot)) == 0) {
        bool need_redo = false;
        if (limit() >= hashsize) {
            hashsize += hashsize + 1;
            log_hashsize++;
            BUG_CHECK((hashsize & (hashsize + 1)) == 0, "hash corruption");
            BUG_CHECK(hashsize + 1 == (1UL << log_hashsize), "hash corruption");
            freehash();
            need_redo = true;
        }
        if (limit() >= info->maxset) {
            BUG_CHECK(info->ismap == info[1].ismap, "hash_vector overflow %d entries", limit() + 1);
            freehash();
            info++;
            need_redo = true;
        }
        if (need_redo) {
            allochash();
            redo_hash();
            if (find(key, cache) && info->ismulti)
                while (find_next(key, cache))
                    ;
        }
        info->sethash(this, cache->slot, limit() + 1);
        inuse++;
        return -1;
    } else if (erased[idx - 1]) {
        inuse++;
    }
    return idx - 1;
}

size_t hash_vector_base::remove(const void *key, lookup_cache *cache) {
    lookup_cache local;
    size_t idx;
    if (cache) {
#ifndef NDEBUG
        if (find(key, &local) && info->ismulti)
            while (local.slot != cache->slot && find_next(key, cache))
                ;
        BUG_CHECK(local.slot == cache->slot && local.collisions == cache->collisions,
                  "invalid cache in hash_vector_base::remove");
#endif
    } else {
        cache = &local;
        find(key, cache);
    }
    if ((idx = info->gethash(this, cache->slot)) != 0) {
        if (!erased[idx - 1]) inuse--;
        if (idx == limit()) {
            resizedata(idx - 1);
            info->sethash(this, cache->slot, 0);
            collisions -= cache->collisions;
        } else {
            erased[idx - 1] = 1;
        }
    }
    return idx - 1;
}

uint32_t hash_vector_base::lookup_cache::getidx(const hash_vector_base *ht) {
    BUG_CHECK(slot < ht->hashsize, "invalid cache");
    return ht->info->gethash(ht, slot) - 1;
}

const void *hash_vector_base::lookup_cache::getkey(const hash_vector_base *ht) {
    size_t idx;
    BUG_CHECK(slot < ht->hashsize, "invalid cache");
    idx = ht->info->gethash(ht, slot);
    if (idx > 0) return ht->getkey(idx - 1);
    return 0;
}

void *hash_vector_base::lookup_cache::getval(hash_vector_base *ht) {
    size_t idx;
    BUG_CHECK(slot < ht->hashsize, "invalid cache");
    idx = ht->info->gethash(ht, slot);
    if (idx > 0) return ht->getval(idx - 1);
    return 0;
}

#ifdef DEBUG
#include <iomanip>

void hash_vector_base::dump(std::ostream &out) {
    int fs = 4, ls = 18;
    out << "hash_vector " << (void *)this << ": " << (info->ismap ? "map" : "set")
        << static_cast<int>(info->hashelsize) << "\n";
    out << "hashsize=" << hashsize << ", limit=" << limit() << ", inuse=" << inuse
        << ", collisions=" << collisions;
    size_t limit = this->limit();
    if (limit > 999999) {
        fs = 8;
        ls = 9;
    } else if (limit > 99999) {
        fs = 7;
        ls = 10;
    } else if (limit > 9999) {
        fs = 6;
        ls = 12;
    } else if (limit > 999) {
        fs = 5;
        ls = 15;
    }
    for (size_t i = 0; i < hashsize; i++) {
        if (i % ls == 0) out << "\n";
        out << std::setw(fs) << info->gethash(this, i);
    }
    out << "\nerased=" << erased << std::endl;
}
#endif /* DEBUG */
