# IPv4 echo programs

These programs exercise the IPv4 library directly using protocol number 253.
They do not use UDP or implement ICMP ping. Nonempty messages up to 1480 bytes
are supported. The client verifies the source address and exact reply bytes
with a five-second receive timeout. Stop the server with Ctrl+C.

Build from the project root in the course Linux environment:

```sh
rawnetcc /tmp/ipv4_client ipv4/ipv4_client.c ipv4/ipv4.c ipv4/ipv4_config.c ipv4/ipv4_route_table.c arp/arp.c eth/eth.c
rawnetcc /tmp/ipv4_server ipv4/ipv4_server.c ipv4/ipv4.c ipv4/ipv4_config.c ipv4/ipv4_route_table.c arp/arp.c eth/eth.c
```

Run on the server VM (192.168.1.200), from the project root:

```sh
/tmp/ipv4_server ipv4/ipv4_config_server.txt ipv4/ipv4_route_table_server.txt
```

Run on the client VM (192.168.1.100), from the project root:

```sh
/tmp/ipv4_client ipv4/ipv4_config_client.txt ipv4/ipv4_route_table_client.txt 192.168.1.200 "Hello IPv4"
```

Use two VMs connected to the same lab network, with distinct MAC addresses.
Match the configured interfaces and IP addresses to each VM. The OS must have
the corresponding addresses assigned so it can answer ARP; the library has no
ARP responder. Configuration files alone do not assign OS addresses.

Expected client output includes:

```text
IPv4 echo verified: 10 bytes from 192.168.1.200
Hello IPv4
```
