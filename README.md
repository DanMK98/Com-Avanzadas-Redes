# Advanced Network Communications Project

An educational C project for the **Comunicaciones de Redes Avanzadas** course at **Universidad Carlos III de Madrid (UC3M)**. It explores Ethernet frames, ARP address resolution, and the foundations of an IPv4 layer through direct interaction with network interfaces.

## Current status

- **Ethernet:** interface management, frame transmission and reception, EtherType filtering, polling, MAC address conversion, and packet dumps.
- **Ethernet examples:** an echo server and a client that sends a generated payload and waits for a reply.
- **ARP:** broadcast requests and IPv4-to-MAC resolution with a two-second timeout.
- **IPv4 utilities:** address conversion, checksum calculation, configuration-file parsing, and routing-table management with longest-prefix matching.
- **IPv4 transmission:** work in progress. `ipv4/ipv4_testing.c` contains an unfinished `ipv4_send()` outline; it is not a runnable test program.

This repository is a learning project, not a complete TCP/IP stack.

## Repository structure

```text
.
|-- eth/
|   |-- eth.c / eth.h                 Ethernet library
|   |-- eth_client.c                 Ethernet echo client
|   `-- eth_server.c                 Ethernet echo server
|-- arp/
|   |-- arp.c / arp.h                 ARP request and reply handling
|   `-- arp_client.c                 Command-line address resolver
|-- ipv4/
|   |-- ipv4.c / ipv4.h               IPv4 address and checksum utilities
|   |-- ipv4_config.c / ipv4_config.h  Interface configuration parser
|   |-- ipv4_route_table.c / .h        Routing-table utilities
|   |-- ipv4_config_*.txt              Example client/server configurations
|   |-- ipv4_route_table_*.txt         Example client/server routes
|   `-- ipv4_testing.c                Unfinished IPv4 send outline
|-- rng.c                             Standalone pseudo-random number example
`-- README.md
```

## Requirements

- A Linux/POSIX networking environment with an Ethernet interface accessible to the programs.
- A C compiler such as GCC and the standard C development headers.
- The external **rawnet** and **timerms** headers and libraries used by the course environment. These dependencies are not included in the repository.
- The course-provided `rawnetcc` command, which compiles the networking programs and grants their executables the required raw-network permissions. Programs built with it can be run directly as a regular user.

The source uses POSIX networking headers and is not ready to build natively with Windows/MSVC. Use the course Linux environment or a suitable Linux VM with access to the required network.

## Building

There is currently no Makefile or automated build configuration. Run these example commands from the repository root in a Linux shell, after installing the external dependencies.

Build the networking examples with the course-provided wrapper. Its syntax is:

```sh
rawnetcc /tmp/<program> <source1.c> <source2.c> ...
```

The first argument is the output executable, followed by the source files. The wrapper invokes GCC, links `rawnet`, and grants the executable the permissions needed to access raw network interfaces. Depending on the course installation, it uses Linux capabilities or a setuid configuration. Run the resulting programs directly as a regular user.

The examples below run from each module's directory so its local headers can be found. `CPATH` supplies the sibling header directories needed by the ARP program without passing compiler options as wrapper arguments.

```sh
# Ethernet echo programs
(cd eth && rawnetcc /tmp/eth_client eth.c eth_client.c)
(cd eth && rawnetcc /tmp/eth_server eth.c eth_server.c)

# ARP resolver, using the IPv4 address utilities
(cd arp && CPATH="../eth:../ipv4${CPATH:+:$CPATH}" \
    rawnetcc /tmp/arp_client arp_client.c arp.c ../eth/eth.c ../ipv4/ipv4.c)

# Standalone random-number example (does not require rawnetcc)
gcc -std=gnu99 -Wall -Wextra rng.c -o /tmp/rng
```

These commands assume the course installation also makes the timer functions used by `eth.c` and `arp.c` available to the linker. If it does not, check the course tooling's instructions for linking `timerms`.

Do not compile every `.c` file into one executable: several files define their own `main()`, and `ipv4_testing.c` is unfinished.

## Running the examples

### Ethernet echo

Use two hosts or VMs connected to the same Ethernet segment. Replace `eth0` with the appropriate interface on each host and the example MAC address with the server's actual MAC address.

Start the server:

```sh
/tmp/eth_server eth0 0x88B5
```

On the client host:

```sh
/tmp/eth_client eth0 0x88B5 02:00:00:00:00:02 64
```

Both programs must use the same EtherType. The client sends a 64-byte payload and waits up to two seconds for the echoed frame. The optional payload length defaults to 1500 bytes and accepts values from 0 to 1500. EtherTypes may be supplied in hexadecimal or decimal.

The programs print frame and payload dumps. The server continues listening until interrupted with `Ctrl+C`.

### ARP resolution

Resolve the MAC address of an IPv4 peer on the local Ethernet segment:

```sh
/tmp/arp_client eth0 192.168.1.200
```

On success, the program prints a mapping such as:

```text
192.168.1.200 -> 02:00:00:00:00:02
```

The resolver broadcasts an ARP request and waits up to two seconds for a reply from the requested IPv4 address. Its current implementation uses `0.0.0.0` as the sender IPv4 address and does not maintain an ARP cache or provide an ARP responder.

### Random-number example

```sh
/tmp/rng 10
```

Prints ten pseudo-random integers using `rand()` seeded with the current time. This example is independent of the networking modules.

## IPv4 configuration

The example configuration files use one key/value pair per line:

```text
Interface eth0
IPv4Address 192.168.1.100
SubnetMask 255.255.255.0
```

The client example uses `192.168.1.100`; the server example uses `192.168.1.200`. Both use a `/24` subnet mask. Lines beginning with `#` are comments.

Routing-table files contain four whitespace-separated fields:

```text
# SubnetAddr    SubnetMask      Iface    Gateway
0.0.0.0         0.0.0.0         eth0     192.168.1.1
192.168.2.0     255.255.255.0   eth0     192.168.1.2
192.168.1.0     255.255.255.0   eth0     0.0.0.0
```

The entries describe a default route, a route to `192.168.2.0/24` through a gateway, and a directly connected `192.168.1.0/24` network. A gateway of `0.0.0.0` indicates direct delivery. Route lookup selects the matching route with the longest prefix.

Adapt the interface names, addresses, and gateways to your lab topology. These files are inputs for the IPv4 utilities; they do not configure the operating system, and the current Ethernet and ARP examples do not load them automatically.

## Verification and troubleshooting

There is no automated test suite in the repository. To check the networking examples manually, run the Ethernet echo pair and resolve a known local peer with the ARP client. A packet capture tool such as Wireshark can help inspect the exchanged frames.

- **Missing `rawnet.h` or `timerms.h`:** install the course dependencies and configure the compiler's include paths.
- **Linker cannot find a library:** check its installed name and library search path.
- **`rawnetcc` not found:** check that the course tooling is installed and its command directory is on your `PATH`.
- **Cannot open an interface:** check the interface name and its availability, confirm the program was built with `rawnetcc`, and check the course tooling configuration for packet/socket creation.
- **Ethernet reply timeout:** check connectivity, the destination MAC address, and that the server is running with the same EtherType.
- **ARP timeout:** check that the requested address belongs to a reachable peer on the local Ethernet segment and that the peer answers ARP requests.

The build commands require the external course libraries and have not been verified in the Windows workspace used to prepare this README.
