/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "vector.h"

struct raw_vector {
    int         capacity, size;
    void        *data;
};

int init_raw_vector(void *vec, size_t elsize, int mincap)
{
    struct raw_vector *v = (struct raw_vector *)vec;
    v->size = 0;
    v->capacity = 32 / elsize;
    if (v->capacity < 4) v->capacity = 4;
    if (v->capacity < mincap) v->capacity = mincap;
    if (!(v->data = malloc(elsize * v->capacity)))
	v->capacity = 0;
    return v->data ? 0 : -1;
}

int erase_raw_vector(void *vec, size_t elsize, int i, unsigned cnt)
{
    struct raw_vector *v = (struct raw_vector *)vec;
    if (i < 0 && i >= v->size) return -1;
    if (cnt == 0) cnt = 1;
    if (i + cnt >= (unsigned)v->size) {
	v->size = i;
    } else {
	char *p = (char *)v->data + i*elsize;
	memmove(p, p + elsize*cnt, elsize * (v->size - i - cnt));
	v->size -= cnt; }
    return 0;
}

int expand_raw_vector(void *vec, size_t elsize)
{
    struct raw_vector *v = (struct raw_vector *)vec;
    size_t ncap = v->capacity * 2U;
    void *n;
    if (ncap == 0) {
	ncap = 32 / elsize;
	if (ncap < 4) ncap = 4; }
    if (ncap > (size_t)INT_MAX && (int)(ncap = INT_MAX) == v->capacity) {
	errno = ERANGE;
	return -1; }
    if (!(n = realloc(v->data, elsize * ncap))) return -1;
    v->capacity = ncap;
    v->data = n;
    return 0;
}

int insert_raw_vector(void *vec, size_t elsize, int i, unsigned cnt)
{
    struct raw_vector *v = (struct raw_vector *)vec;
    if (i < 0 && i > v->size) return -1;
    if (cnt == 0) cnt = 1;
    if (v->size + cnt > (unsigned)INT_MAX) {
	errno = ERANGE;
	return -1; }
    if ((int)(v->size + cnt) > v->capacity) {
	int newsz = v->size + cnt;
	void *n;
	if (newsz < v->capacity * 2) newsz = v->capacity * 2;
	if (!(n = realloc(v->data, elsize * newsz))) return -1;
	v->capacity = newsz;
	v->data = n; }
    if (i < v->size) {
	char *p = (char *)v->data + i*elsize;
	memmove(p + cnt*elsize, p, elsize * (v->size - i)); }
    v->size += cnt;
    return 0;
}

int reserve_raw_vector(void *vec, size_t elsize, int size, int shrink)
{
    struct raw_vector *v = (struct raw_vector *)vec;
    void *n;
    if (v->capacity < size || (shrink && v->capacity > size)) {
	if (!(n = realloc(v->data, elsize * size))) return -1;
	v->capacity = size;
	if (size < v->size)
	    v->size = size;
	v->data = n; }
    return 0;
}

int shrink_raw_vector(void *vec, size_t elsize)
{
    struct raw_vector *v = (struct raw_vector *)vec;
    void *n;
    if (v->size < v->capacity) {
	if (!(n = realloc(v->data, elsize * v->size))) return -1;
	v->capacity = v->size;
	v->data = n; }
    return 0;
}
