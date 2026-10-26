#include "ipv4.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define ECHO_PROTOCOL 253

static volatile sig_atomic_t stop_requested = 0;

static void request_stop(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <config> <routes>\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (signal(SIGINT, request_stop) == SIG_ERR ||
        signal(SIGTERM, request_stop) == SIG_ERR) {
        fprintf(stderr, "Could not install signal handlers.\n");
        return EXIT_FAILURE;
    }
    ipv4_layer_t *layer = ipv4_open(argv[1], argv[2]);
    if (layer == NULL) {
        fprintf(stderr, "Could not initialize IPv4.\n");
        return EXIT_FAILURE;
    }
    printf("IPv4 echo server listening on protocol %d. Press Ctrl+C to stop.\n",
           ECHO_PROTOCOL);

    int status = EXIT_SUCCESS;
    while (!stop_requested) {
        unsigned char payload[ETH_MTU];
        ipv4_addr_t sender;
        int received = ipv4_recv(layer, sender, ECHO_PROTOCOL,
                                 payload, sizeof(payload), 500);
        if (stop_requested) {
            break;
        }
        if (received == 0) {
            continue;
        }
        if (received < 0) {
            fprintf(stderr, "Could not receive IPv4 datagram.\n");
            status = EXIT_FAILURE;
            break;
        }
        char address[IPv4_STR_MAX_LENGTH];
        ipv4_addr_str(sender, address);
        if (ipv4_send(layer, sender, ECHO_PROTOCOL, payload, received) != received) {
            fprintf(stderr, "Could not echo to %s.\n", address);
            continue;
        }
        printf("Echoed %d bytes to %s\n", received, address);
    }
    if (ipv4_close(layer) != 0) {
        fprintf(stderr, "Could not close IPv4 cleanly.\n");
        status = EXIT_FAILURE;
    }
    return status;
}
