#ifndef _BMI_PORT_
#define _BMI_PORT_

typedef struct bmi_port_s bmi_port_t;

typedef struct bmi_port_mgr_s bmi_port_mgr_t;

/* the client must make a copy of the buffer memory, this memory cannot be
   modified and is no longer garanteed to be valid after the function
   returns. */
typedef void (*bmi_packet_handler_t)(int port_num, const char *buffer, int len);

int bmi_port_create_mgr(bmi_port_mgr_t **port_mgr);

int bmi_set_packet_handler(bmi_port_mgr_t *port_mgr,
			   bmi_packet_handler_t packet_handler);

int bmi_port_send(bmi_port_mgr_t *port_mgr,
		  int port_num, const char *buffer, int len);

int bmi_port_interface_add(bmi_port_mgr_t *port_mgr,
			   const char *ifname, int port_num,
			   const char *pcap_dump);

int bmi_port_interface_remove(bmi_port_mgr_t *port_mgr, int port_num);

int bmi_port_destroy_mgr(bmi_port_mgr_t *port_mgr);

#endif
