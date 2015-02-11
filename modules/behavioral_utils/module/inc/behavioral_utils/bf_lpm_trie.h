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

bool bf_lpm_trie_get(const bf_lpm_trie_t *trie, const char *key,
		     value_t *pvalue);

bool bf_lpm_trie_delete(bf_lpm_trie_t *trie, const char *prefix,
			int prefix_length);

#ifdef __cplusplus
}
#endif

#endif
