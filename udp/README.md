# UDP echo example

Build from the project root in the course Linux environment (with rawnet and
timerms installed):

```sh
rawnetcc /tmp/udp_client udp/udp_client.c udp/udp.c ipv4/ipv4.c ipv4/ipv4_config.c ipv4/ipv4_route_table.c arp/arp.c eth/eth.c
rawnetcc /tmp/udp_server udp/udp_server.c udp/udp.c ipv4/ipv4.c ipv4/ipv4_config.c ipv4/ipv4_route_table.c arp/arp.c eth/eth.c
```

On the server host, from the project root:

```sh
/tmp/udp_server ipv4/ipv4_config_server.txt ipv4/ipv4_route_table_server.txt 5000
```

On the client host, from the project root:

```sh
/tmp/udp_client ipv4/ipv4_config_client.txt ipv4/ipv4_route_table_client.txt 5001 192.168.1.200 5000 "Hello UDP"
```

Adapt interface names, addresses, and routes to the lab network. Configuration
files configure this library, not the host operating system. The current stack
has an ARP resolver but no ARP responder: peers must be able to resolve the
configured addresses through the lab host setup or static neighbor entries.

The client checks the reply's source IP, port, length, and bytes, with a five-second
receive timeout. The server echoes nonempty datagrams and closes its layer on
Ctrl+C. Both programs accept ports from 1 through 65535. Messages must contain
1 through 1472 bytes. This implementation supports local unicast reception,
omits UDP checksum generation/verification, and does not reassemble IP fragments.

Live network behavior must be verified in the course environment.
