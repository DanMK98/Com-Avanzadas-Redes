#include "udp.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static volatile sig_atomic_t stop_requested = 0;

static void request_stop(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <config> <routes> <local-port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *end;
    errno = 0;
    long port = strtol(argv[3], &end, 10);
    if (errno || end == argv[3] || *end != '\0' || port < 1 || port > 65535) {
        fprintf(stderr, "Port must be between 1 and 65535.\n");
        return EXIT_FAILURE;
    }
    if (signal(SIGINT, request_stop) == SIG_ERR ||
        signal(SIGTERM, request_stop) == SIG_ERR) {
        fprintf(stderr, "Could not install signal handlers.\n");
        return EXIT_FAILURE;
    }

    udp_layer_t *udp = udp_open(argv[1], argv[2], (uint16_t)port);
    if (udp == NULL) {
        fprintf(stderr, "Could not initialize UDP.\n");
        return EXIT_FAILURE;
    }
    printf("UDP echo server listening on port %ld. Press Ctrl+C to stop.\n", port);
    int status = EXIT_SUCCESS;
    while (!stop_requested) {
        unsigned char payload[ETH_MTU];
        ipv4_addr_t sender;
        uint16_t sender_port;
        int received = udp_recv(udp, sender, &sender_port,
                                payload, sizeof(payload), 500);
        if (stop_requested) {
            break;
        }
        if (received == 0) {
            continue;
        }
        if (received < 0) {
            fprintf(stderr, "Could not receive UDP datagram.\n");
            status = EXIT_FAILURE;
            break;
        }

        char address[IPv4_STR_MAX_LENGTH];
        ipv4_addr_str(sender, address);
        if (udp_send(udp, sender, sender_port, payload, received) != received) {
            fprintf(stderr, "Could not echo to %s:%u.\n", address,
                    (unsigned int)sender_port);
            continue;
        }
        printf("Echoed %d bytes to %s:%u\n", received, address,
               (unsigned int)sender_port);
    }
    if (udp_close(udp) != 0) {
        fprintf(stderr, "Could not close UDP cleanly.\n");
        status = EXIT_FAILURE;
    }
    return status;
}
