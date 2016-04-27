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

#ifndef _BF_LPM_TRIE_H
#define _BF_LPM_TRIE_H

#ifdef __cplusplus
extern "C"{
#endif 
  
#include <stddef.h>
#include <stdbool.h>
  
typedef struct bf_lpm_trie_s bf_lpm_trie_t;

typedef unsigned long value_t;

bf_lpm_trie_t *bf_lpm_trie_create(size_t key_width_bytes, bool auto_shrink);

void bf_lpm_trie_destroy(bf_lpm_trie_t *t);

void bf_lpm_trie_insert(bf_lpm_trie_t *trie,
			const char *prefix, int prefix_length,
			const value_t value);

bool bf_lpm_trie_has_prefix(const bf_lpm_trie_t *trie,
			    const char *prefix, int prefix_length);

bool bf_lpm_trie_lookup(const bf_lpm_trie_t *trie, const char *key,
			value_t *pvalue);

bool bf_lpm_trie_delete(bf_lpm_trie_t *trie, const char *prefix,
			int prefix_length);

#ifdef __cplusplus
}
#endif

#endif
