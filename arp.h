#include "eth.h"  // Importa las funciones y estructuras necesarias para manejar la capa Ethernet y enviar tramas por la red.
#include "ipv4.h" // Proporciona los tipos de datos específicos para almacenar y operar con direcciones IPv4.

struct arp_header {
   uint16_t hw_type;    // Tipo de hardware (Ethernet = 1).
   uint16_t proto_type; // Tipo de protocolo (IPv4 = 0x0800).
   uint8_t  hw_len;     // Longitud de dir. MAC (6 bytes), hw_len especifica que la dirección física MAC de la tarjeta de red ocupa exactamente 6 bytes.
   uint8_t  proto_len;  // Longitud de dir. IP (4 bytes), proto_len indica que la dirección lógica IPv4 tiene un tamaño fijo de 4 bytes.
   uint16_t opcode;     // Operación (Request = 1, Reply = 2), opcode define el propósito del paquete, valiendo 1 si estamos preguntando (Request) o 2 si estamos respondiendo (Reply).
   mac_addr_t src_mac;  // Dirección MAC origen.
   ipv4_addr_t src_ip;  // Dirección IP origen.
   mac_addr_t dst_mac;  // Dirección MAC destino (En request es 00:00:00:00:00:00).
   ipv4_addr_t dst_ip;  // Dirección IP destino.
} __attribute__((__packed__));

// eth_iface_t * iface: Es la puerta (o interfaz) por la cual vas a enviar la pregunta.
// ipv4_addr_t dest: Es la IP objetivo (la dirección lógica) de la que necesitas averiguar su MAC.
// mac_addr_t mac: Es la variable que pasas como contenedor vacío para que la función, al recibir la respuesta, escriba ahí dentro la MAC física descubierta.
// int: Indica que la función devuelve un número (normalmente un 1 si todo salió bien y logró encontrar la MAC, o un -1 si pasaron los 2 segundos y nadie respondió).
int arp_resolve ( eth_iface_t * iface, ipv4_addr_t dest, mac_addr_t mac );

