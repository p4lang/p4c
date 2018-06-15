/* SPDX-License-Identifier: LGPL-2.1 */

/*
 * common eBPF ELF operations.
 *
 * Copyright (C) 2013-2015 Alexei Starovoitov <ast@kernel.org>
 * Copyright (C) 2015 Wang Nan <wangnan0@huawei.com>
 * Copyright (C) 2015 Huawei Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License (not later!)
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not,  see <http://www.gnu.org/licenses>
 */
#ifndef __BPF_BPF_H
#define __BPF_BPF_H

enum libbpf_errno {
    __LIBBPF_ERRNO__START = 4000,

    /* Something wrong in libelf */
    LIBBPF_ERRNO__LIBELF = __LIBBPF_ERRNO__START,
    LIBBPF_ERRNO__FORMAT,   /* BPF object format invalid */
    LIBBPF_ERRNO__KVERSION, /* Incorrect or no 'version' section */
    LIBBPF_ERRNO__ENDIAN,   /* Endian mismatch */
    LIBBPF_ERRNO__INTERNAL, /* Internal error in libbpf */
    LIBBPF_ERRNO__RELOC,    /* Relocation failed */
    LIBBPF_ERRNO__LOAD, /* Load program failure for unknown reason */
    LIBBPF_ERRNO__VERIFY,   /* Kernel verifier blocks program loading */
    LIBBPF_ERRNO__PROG2BIG, /* Program too big */
    LIBBPF_ERRNO__KVER, /* Incorrect kernel version */
    LIBBPF_ERRNO__PROGTYPE, /* Kernel doesn't support this program type */
    LIBBPF_ERRNO__WRNGPID,  /* Wrong pid in netlink message */
    LIBBPF_ERRNO__INVSEQ,   /* Invalid netlink sequence */
    __LIBBPF_ERRNO__END,
};


#include <linux/bpf.h>
#include <stddef.h>

int bpf_create_map_node(enum bpf_map_type map_type, const char *name,
			int key_size, int value_size, int max_entries,
			__u32 map_flags, int node);
int bpf_create_map_name(enum bpf_map_type map_type, const char *name,
			int key_size, int value_size, int max_entries,
			__u32 map_flags);
int bpf_create_map(enum bpf_map_type map_type, int key_size, int value_size,
		   int max_entries, __u32 map_flags);
int bpf_create_map_in_map_node(enum bpf_map_type map_type, const char *name,
			       int key_size, int inner_map_fd, int max_entries,
			       __u32 map_flags, int node);
int bpf_create_map_in_map(enum bpf_map_type map_type, const char *name,
			  int key_size, int inner_map_fd, int max_entries,
			  __u32 map_flags);

/* Recommend log buffer size */
#define BPF_LOG_BUF_SIZE (256 * 1024)
int bpf_load_program_name(enum bpf_prog_type type, const char *name,
			  const struct bpf_insn *insns,
			  size_t insns_cnt, const char *license,
			  __u32 kern_version, char *log_buf,
			  size_t log_buf_sz);
int bpf_load_program(enum bpf_prog_type type, const struct bpf_insn *insns,
		     size_t insns_cnt, const char *license,
		     __u32 kern_version, char *log_buf,
		     size_t log_buf_sz);
int bpf_verify_program(enum bpf_prog_type type, const struct bpf_insn *insns,
		       size_t insns_cnt, int strict_alignment,
		       const char *license, __u32 kern_version,
		       char *log_buf, size_t log_buf_sz, int log_level);

int bpf_map_update_elem(int fd, const void *key, const void *value,
			__u64 flags);

int bpf_map_lookup_elem(int fd, const void *key, void *value);
int bpf_map_delete_elem(int fd, const void *key);
int bpf_map_get_next_key(int fd, const void *key, void *next_key);
int bpf_obj_pin(int fd, const char *pathname);
int bpf_obj_get(const char *pathname);
int bpf_prog_attach(int prog_fd, int attachable_fd, enum bpf_attach_type type,
		    unsigned int flags);
int bpf_prog_detach(int attachable_fd, enum bpf_attach_type type);
int bpf_prog_detach2(int prog_fd, int attachable_fd, enum bpf_attach_type type);
int bpf_prog_test_run(int prog_fd, int repeat, void *data, __u32 size,
		      void *data_out, __u32 *size_out, __u32 *retval,
		      __u32 *duration);
int bpf_prog_get_next_id(__u32 start_id, __u32 *next_id);
int bpf_map_get_next_id(__u32 start_id, __u32 *next_id);
int bpf_prog_get_fd_by_id(__u32 id);
int bpf_map_get_fd_by_id(__u32 id);
int bpf_obj_get_info_by_fd(int prog_fd, void *info, __u32 *info_len);
int bpf_prog_query(int target_fd, enum bpf_attach_type type, __u32 query_flags,
		   __u32 *attach_flags, __u32 *prog_ids, __u32 *prog_cnt);

struct perf_event_attr;
int perf_event_open(struct perf_event_attr *attr, int pid, int cpu,
            int group_fd, unsigned long flags);

#endif