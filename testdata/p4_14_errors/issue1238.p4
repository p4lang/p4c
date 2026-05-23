/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type palatalising {
  fields {
  }
}

parser start {
  return ingress;
}

action lubbers() {
  subtract(palatalising, palatalising, 1);
}

table terrs {
  reads {
  }
  actions {
    lubbers;
  }
}

control ingress {
  apply(terrs);
}
