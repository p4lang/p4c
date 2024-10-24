#!/bin/bash

HUGE_DEFAULT=192
HUGE_TO_SET=${NUM_HUGEPAGES:-${1:-$HUGE_DEFAULT}}
HUGE_KEY=vm.nr_hugepages
HUGE_ACTUAL=$(sysctl --values $HUGE_KEY)
CONF=/etc/sysctl.conf

if [[ $HUGE_TO_SET -gt $HUGE_ACTUAL ]]; then
    if grep -q "^$HUGE_KEY\>" $CONF; then
        sed -i "s/^$HUGE_KEY\>.*$/$HUGE_KEY = $HUGE_TO_SET/" $CONF
    else
        echo "$HUGE_KEY = $HUGE_TO_SET" >> $CONF
    fi
    sysctl -p /etc/sysctl.conf
fi
if [[ ! -d /mnt/huge ]]; then
    mkdir /mnt/huge
    mount -t hugetlbfs nodev /mnt/huge
    echo -e "nodev /mnt/huge hugetlbfs\n" >> /etc/fstab
fi
