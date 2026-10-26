#ifndef UDP_H
#define UDP_H

#include "../ipv4/ipv4.h"

typedef struct udp_layer {
    ipv4_layer_t *ipv4;
    uint16_t local_port; /* Host byte order */
} udp_layer_t;

int udp_send(udp_layer_t *layer, ipv4_addr_t dst,
             uint16_t dst_port, unsigned char *payload,
             int payload_len);

int udp_recv(udp_layer_t *layer, ipv4_addr_t src,
             uint16_t *src_port, unsigned char *buffer,
             int buffer_len, long int timeout);

/* local_port uses host byte order; zero is not supported. */
udp_layer_t *udp_open(char *file_conf, char *file_conf_route,
                      uint16_t local_port);

/* Frees the layer even if closing Ethernet fails. */
int udp_close(udp_layer_t *layer);

#endif
