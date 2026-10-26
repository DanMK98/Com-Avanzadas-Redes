#include "udp.h"

#include <arpa/inet.h>
#include <string.h>
#include <timerms.h>
#include <stdlib.h>

#define UDP_PROTOCOL 17
#define UDP_HEADER_SIZE 8
#define IPV4_MIN_HEADER_SIZE 20
#define UDP_MAX_DATAGRAM (ETH_MTU - IPV4_MIN_HEADER_SIZE)
#define UDP_MAX_PAYLOAD (UDP_MAX_DATAGRAM - UDP_HEADER_SIZE)

struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((__packed__));

int udp_send(udp_layer_t *layer, ipv4_addr_t dst,
             uint16_t dst_port, unsigned char *payload,
             int payload_len)
{
    if (layer == NULL || layer->ipv4 == NULL ||
        dst == NULL || layer->local_port == 0 ||
        dst_port == 0 || payload_len < 0 ||
        payload_len > UDP_MAX_PAYLOAD ||
        (payload == NULL && payload_len > 0)) {
        return -1;
    }

    unsigned char datagram[UDP_MAX_DATAGRAM];
    int datagram_len = UDP_HEADER_SIZE + payload_len;

    struct udp_header header = {
        .src_port = htons(layer->local_port),
        .dst_port = htons(dst_port),
        .length = htons(datagram_len),
        .checksum = 0
    };

    memcpy(datagram, &header, sizeof(header));

    if (payload_len > 0) {
        memcpy(datagram + UDP_HEADER_SIZE, payload, payload_len);
    }

    int sent = ipv4_send(layer->ipv4, dst, UDP_PROTOCOL,
                         datagram, datagram_len);

    if (sent != datagram_len) {
        return -1;
    }

    return payload_len;
}

int udp_recv(udp_layer_t *layer, ipv4_addr_t src,
             uint16_t *src_port, unsigned char *buffer,
             int buffer_len, long int timeout)
{
    if (layer == NULL || layer->ipv4 == NULL ||
        layer->local_port == 0 || src == NULL ||
        src_port == NULL || buffer == NULL || buffer_len <= 0) {
        return -1;
    }

    unsigned char datagram[UDP_MAX_DATAGRAM];
    ipv4_addr_t sender;
    timerms_t timer;
    int first_attempt = 1;

    if (timeout >= 0) {
        timerms_reset(&timer, timeout);
    }

    for (;;) {
        long int remaining = -1;

        /* Unrelated packets must not restart the timeout. */
        if (timeout >= 0) {
            remaining = timerms_left(&timer);

            if (remaining <= 0) {
                if (!first_attempt) {
                    return 0;
                }
                remaining = 0;
            }
        }

        first_attempt = 0;

        int received = ipv4_recv(layer->ipv4, sender, UDP_PROTOCOL,
                                 datagram, sizeof(datagram), remaining);

        if (received <= 0) {
            return received;
        }

        if (received < UDP_HEADER_SIZE) {
            continue;
        }

        struct udp_header header;
        memcpy(&header, datagram, sizeof(header));

        int datagram_len = ntohs(header.length);

        if (datagram_len < UDP_HEADER_SIZE ||
            datagram_len > received ||
            ntohs(header.dst_port) != layer->local_port) {
            continue;
        }

        int payload_len = datagram_len - UDP_HEADER_SIZE;

        /*
         * This API reserves zero for timeout, so empty UDP
         * datagrams are ignored on reception.
         */
        if (payload_len == 0) {
            continue;
        }

        if (payload_len > buffer_len) {
            return -1;
        }

        memcpy(buffer, datagram + UDP_HEADER_SIZE, payload_len);
        memcpy(src, sender, IPv4_ADDR_SIZE);
        *src_port = ntohs(header.src_port);

        return payload_len;
    }

}

udp_layer_t *udp_open(char *file_conf, char *file_conf_route,
                      uint16_t local_port)
{
    if (local_port == 0) {
        return NULL;
    }

    udp_layer_t *layer = malloc(sizeof(*layer));
    if (layer == NULL) {
        return NULL;
    }

    layer->ipv4 = ipv4_open(file_conf, file_conf_route);
    if (layer->ipv4 == NULL) {
        free(layer);
        return NULL;
    }

    layer->local_port = local_port;
    return layer;
}

int udp_close(udp_layer_t *layer)
{
    if (layer == NULL) {
        return -1;
    }

    int result = ipv4_close(layer->ipv4);
    free(layer);

    return result;
}
