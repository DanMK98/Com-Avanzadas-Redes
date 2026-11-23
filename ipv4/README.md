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
Use each VM's interface name in its config and routes. The server's `IPv4Address`
must match an address assigned to the server in Linux. The client detects its
local IPv4 address on the configured interface. If that interface has multiple
addresses, the client prefers the most specific subnet that contains the next
hop and favors `IPv4Address` when it matches an assigned address with the same
preference. Keep the `IPv4Address` and `SubnetMask` fields in the client config
so the file can be parsed. Linux answers ARP for the assigned addresses.

Expected client output includes:

```text
IPv4 echo verified: 10 bytes from 192.168.1.200
Hello IPv4
```

## Experiment with the server at 192.168.1.2

With `192.168.1.2/24` assigned to `eth0` in the server VM, start the server
with the lab config.

```sh
/tmp/ipv4_server ipv4/ipv4_config_server_lab.txt ipv4/ipv4_route_table_server.txt
```

Run the client from the other VM. The client detects its local address on the
interface named in its config.

```sh
/tmp/ipv4_client ipv4/ipv4_config_client.txt ipv4/ipv4_route_table_client.txt 192.168.1.2 "Hello IPv4"
```

The server should print `Echoed 10 bytes` and the client should print
`IPv4 echo verified: 10 bytes from 192.168.1.2`.
