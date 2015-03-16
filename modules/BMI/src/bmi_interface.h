#ifndef _BMI_INTERFACE_
#define _BMI_INTERFACE_

typedef struct bmi_interface_s bmi_interface_t;

int bmi_interface_create(bmi_interface_t **bmi, const char *device);

int bmi_interface_add_dumper(bmi_interface_t *bmi, const char *filename);

int bmi_interface_destroy(bmi_interface_t *bmi);

int bmi_interface_send(bmi_interface_t *bmi, const char *data, int len);

int bmi_interface_recv(bmi_interface_t *bmi, const char **data);

int bmi_interface_recv_with_copy(bmi_interface_t *bmi, char *data, int max_len);

int bmi_interface_get_fd(bmi_interface_t *bmi);

#endif
