#include "ipv4.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Shared by the two echo applications; not ICMP or UDP. */
#define ECHO_PROTOCOL 253
#define ECHO_MAX_PAYLOAD (ETH_MTU - 20)

static int parse_address(char *text, ipv4_addr_t address)
{
    unsigned int octet[4];
    char trailing;
    if (sscanf(text, "%u.%u.%u.%u%c", &octet[0], &octet[1],
               &octet[2], &octet[3], &trailing) != 4) {
        return -1;
    }
    for (int i = 0; i < 4; i++) {
        if (octet[i] > 255) {
            return -1;
        }
        address[i] = (unsigned char)octet[i];
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <config> <routes> <server-ip> <message>\n", argv[0]);
        return EXIT_FAILURE;
    }
    ipv4_addr_t server;
    size_t length = strlen(argv[4]);
    if (parse_address(argv[3], server) != 0 ||
        length == 0 || length > ECHO_MAX_PAYLOAD) {
        fprintf(stderr, "Invalid IPv4 address or message (1-1480 bytes).\n");
        return EXIT_FAILURE;
    }

    ipv4_layer_t *layer = ipv4_open(argv[1], argv[2]);
    if (layer == NULL) {
        fprintf(stderr, "Could not initialize IPv4.\n");
        return EXIT_FAILURE;
    }
    int status = EXIT_FAILURE;
    if (ipv4_send(layer, server, ECHO_PROTOCOL,
                  (unsigned char *)argv[4], (int)length) != (int)length) {
        fprintf(stderr, "Could not send message.\n");
    } else {
        unsigned char reply[ETH_MTU];
        ipv4_addr_t sender;
        int received = ipv4_recv(layer, sender, ECHO_PROTOCOL,
                                 reply, sizeof(reply), 5000);
        if (received == 0) {
            fprintf(stderr, "Echo timed out after 5 seconds.\n");
        } else if (received < 0) {
            fprintf(stderr, "Could not receive echo.\n");
        } else if (memcmp(sender, server, IPv4_ADDR_SIZE) != 0 ||
                   received != (int)length ||
                   memcmp(reply, argv[4], length) != 0) {
            fprintf(stderr, "Reply source or payload does not match the request.\n");
        } else {
            printf("IPv4 echo verified: %d bytes from %s\n", received, argv[3]);
            fwrite(reply, 1, (size_t)received, stdout);
            putchar('\n');
            status = EXIT_SUCCESS;
        }
    }
    if (ipv4_close(layer) != 0) {
        fprintf(stderr, "Could not close IPv4 cleanly.\n");
        status = EXIT_FAILURE;
    }
    return status;
}
