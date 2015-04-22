#ifndef _BM_SIMPLE_ROUTER_H_
#define _BM_SIMPLE_ROUTER_H_

int packet_accept(int port_num, const char *buffer, int len);

typedef void (*transmit_fn_t)(int port_num, const char *buffer, int len);

void start_processing(transmit_fn_t transmit_fn);

#endif
