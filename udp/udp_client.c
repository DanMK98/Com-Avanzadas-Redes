#include "udp.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_port(char *text, uint16_t *port)
{
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno || end == text || *end != '\0' || value < 1 || value > 65535) {
        return -1;
    }
    *port = (uint16_t)value;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 7) {
        fprintf(stderr, "Usage: %s <config> <routes> <local-port> <server-ip> <server-port> <message>\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t local_port, server_port;
    ipv4_addr_t server;
    size_t length = strlen(argv[6]);
    if (parse_port(argv[3], &local_port) != 0 ||
        parse_port(argv[5], &server_port) != 0 ||
        inet_pton(AF_INET, argv[4], server) != 1 ||
        length == 0 || length > ETH_MTU - 20 - 8) {
        fprintf(stderr, "Invalid ports, server IPv4 address, or message (1-1472 bytes).\n");
        return EXIT_FAILURE;
    }

    udp_layer_t *udp = udp_open(argv[1], argv[2], local_port);
    if (udp == NULL) {
        fprintf(stderr, "Could not initialize UDP.\n");
        return EXIT_FAILURE;
    }

    int status = EXIT_FAILURE;
    if (udp_send(udp, server, server_port,
                 (unsigned char *)argv[6], (int)length) != (int)length) {
        fprintf(stderr, "Could not send message.\n");
    } else {
        unsigned char reply[ETH_MTU];
        ipv4_addr_t sender;
        uint16_t sender_port;
        int received = udp_recv(udp, sender, &sender_port,
                                reply, sizeof(reply), 5000);
        if (received == 0) {
            fprintf(stderr, "Echo timed out after 5 seconds.\n");
        } else if (received < 0) {
            fprintf(stderr, "Could not receive echo.\n");
        } else if (memcmp(sender, server, IPv4_ADDR_SIZE) != 0 ||
                   sender_port != server_port || received != (int)length ||
                   memcmp(reply, argv[6], length) != 0) {
            fprintf(stderr, "Reply source or payload does not match the request.\n");
        } else {
            printf("Echo verified: %d bytes from %s:%u\n", received,
                   argv[4], (unsigned int)sender_port);
            fwrite(reply, 1, (size_t)received, stdout);
            putchar('\n');
            status = EXIT_SUCCESS;
        }
    }
    if (udp_close(udp) != 0) {
        fprintf(stderr, "Could not close UDP cleanly.\n");
        status = EXIT_FAILURE;
    }
    return status;
}
