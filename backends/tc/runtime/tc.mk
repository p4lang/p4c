# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Actual location of the makefile
ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

## Argument for the CLANG compiler
CLANG ?= clang
override INCLUDES+= -I$(ROOT_DIR)/include -I$(ROOT_DIR)/../../ebpf/runtime/
override LIBS+=
P4C ?= p4c-pna-p4tc
# Optimization flags to save space
CFLAGS+= -O2 -g -c -D__KERNEL__ -D__ASM_SYSREG_H -DBTF \
	 -Wno-unused-value  -Wno-pointer-sign \
	 -Wno-compare-distinct-pointer-types \
	 -Wno-gnu-variable-sized-type-not-at-end \
	 -Wno-address-of-packed-member -Wno-tautological-compare \
	 -Wno-unknown-warning-option -Wnoparentheses-equality

TMPL+=$(OUTPUT_DIR)/$(TEMPLATE).template
TMPL+=$(OUTPUT_DIR)/$(TEMPLATE).json
SRCS+=$(OUTPUT_DIR)/$(TEMPLATE)_parser.c
SRCS+=$(OUTPUT_DIR)/$(TEMPLATE)_control_blocks.c
OBJS=$(SRCS:.c=.o)

all: $(SRCS) $(OBJS)

$(SRCS): $(P4_FILE)
	@if ! ($(P4C) --version); then \
		echo "*** ERROR: Cannot find p4c-ebpf"; \
		exit 1;\
	fi;
	$(P4C) $(P4_FILE) -o ${OUTPUT_DIR}

$(OBJS): %.o : %.c
	$(CLANG) $(CFLAGS) $(INCLUDES) --target=bpf -mcpu=probe -c $< -o $@

clean:
	rm -f $(OBJS)

realclean:
	rm -f $(OBJS) $(SRCS) $(TMPL) $(TEMPLATE)_parser.h
