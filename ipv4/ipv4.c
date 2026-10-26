#include "ipv4.h"
#include "ipv4_route_table.h"
#include "../eth/eth.h"
#include "../arp/arp.h"
#include <stdio.h>
#include <stdlib.h>

/* Dirección IPv4 a cero: "0.0.0.0" */
ipv4_addr_t IPv4_ZERO_ADDR = { 0, 0, 0, 0 };

struct ipv4_header {
  uint8_t version_ihl;
  uint8_t tos;
  uint16_t total_length;
  uint16_t identification;
  uint16_t flags_fragment;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t checksum;
  ipv4_addr_t src;
  ipv4_addr_t dst;
} __attribute__((__packed__));


/* void ipv4_addr_str ( ipv4_addr_t addr, char* str );
 *
 * DESCRIPCIÓN:
 *   Esta función genera una cadena de texto que representa la dirección IPv4
 *   indicada.
 *
 * PARÁMETROS:
 *   'addr': La dirección IP que se quiere representar textualente.
 *    'str': Memoria donde se desea almacenar la cadena de texto generada.
 *           Deben reservarse al menos 'IPv4_STR_MAX_LENGTH' bytes.
 */
void ipv4_addr_str ( ipv4_addr_t addr, char* str ) {
   if (str != NULL) {
      sprintf(str, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
   }
}

/* int ipv4_str_addr ( char* str, ipv4_addr_t addr );
 *
 * DESCRIPCIÓN:
 *   Esta función analiza una cadena de texto en busca de una dirección IPv4.
 *
 * PARÁMETROS:
 *    'str': La cadena de texto que se desea procesar.
 *   'addr': Memoria donde se almacena la dirección IPv4 encontrada.
 *
 * VALOR DEVUELTO:
 *   Se devuelve 0 si la cadena de texto representaba una dirección IPv4.
 *
 * ERRORES:
 *   La función devuelve -1 si la cadena de texto no representaba una
 *   dirección IPv4.
 */
int ipv4_str_addr ( char* str, ipv4_addr_t addr ) {
   int err = -1;

   if (str != NULL) {
      unsigned int addr_int[IPv4_ADDR_SIZE];
      int len = sscanf(str, "%d.%d.%d.%d", &addr_int[0], &addr_int[1], &addr_int[2], &addr_int[3]); 
      if (len == IPv4_ADDR_SIZE) {
         int i;
         for (i=0; i<IPv4_ADDR_SIZE; i++) {
           addr[i] = (unsigned char) addr_int[i];
         }
         err = 0;
      }
   }
  return err;
}

/*
 * uint16_t ipv4_checksum ( unsigned char * data, int len )
 *
 * DESCRIPCIÓN:
 *   Esta función calcula el checksum IP de los datos especificados.
 *
 * PARÁMETROS:
 *   'data': Puntero a los datos sobre los que se calcula el checksum.
 *    'len': Longitud en bytes de los datos.
 *
 * VALOR DEVUELTO:
 *   El valor del checksum calculado.
 */
uint16_t ipv4_checksum ( unsigned char * data, int len ) {
   int i;
   uint16_t word16;
   unsigned int sum = 0;
    
   /* Make 16 bit words out of every two adjacent 8 bit words in the packet
   * and add them up */
   for (i=0; i<len; i=i+2) {
      word16 = ((data[i] << 8) & 0xFF00) + (data[i+1] & 0x00FF);
      sum = sum + (unsigned int) word16;	
   }

   /* Take only 16 bits out of the 32 bit sum and add up the carries */
   while (sum >> 16) {
      sum = (sum & 0xFFFF) + (sum >> 16);
   }

   /* One's complement the result */
   sum = ~sum;

   return (uint16_t) sum;
}

int ipv4_send (ipv4_layer_t * layer, ipv4_addr_t dst, uint8_t protocol, unsigned char * payload, int payload_len) {
  if ((layer == NULL) || (dst == NULL) || (payload == NULL) || (payload_len <= 0)) {
    return -1;
  }
  /* 1. Rellenar datagrama IPv4 (cabecera + datos) */
  if (payload_len > ETH_MTU - (int) sizeof(struct ipv4_header)) {
    return -1;
  }

  struct ipv4_header header = {0};
  int datagram_len = sizeof(header) + payload_len;
  unsigned char datagram[ETH_MTU];

  header.version_ihl = 0x45; /* IPv4, cabecera de 5 palabras (20 bytes) */
  header.total_length = htons(datagram_len);
  header.ttl = 64;
  header.protocol = protocol;
  memcpy(header.src, layer->src_addr, IPv4_ADDR_SIZE);
  memcpy(header.dst, dst, IPv4_ADDR_SIZE);
  header.checksum = htons(ipv4_checksum((unsigned char *) &header, sizeof(header)));

  memcpy(datagram, &header, sizeof(header));
  memcpy(datagram + sizeof(header), payload, payload_len);

  /* 2. Obtener siguiente salto con ipv4_route_table_lookup () */
  ipv4_route_t * route = ipv4_route_table_lookup(layer->route_table, dst);

  if (route == NULL) {
    return -1; //If no route found, return error
  }

  ipv4_addr_t next_hop;
  if (memcmp(route->gateway_addr, IPv4_ZERO_ADDR, IPv4_ADDR_SIZE) == 0) {
    memcpy(next_hop, dst, IPv4_ADDR_SIZE); //Directly connected, use destination address
  } else {
    memcpy(next_hop, route->gateway_addr, IPv4_ADDR_SIZE); //Use gateway
  }

  /* 3. Averiguar dirección MAC sig. salto con arp_resolve() */
  eth_iface_t * iface = eth_open(route->iface);
  if (iface == NULL) {
    return -1;
  }


  mac_addr_t next_hop_mac;
  int arp_result = arp_resolve(iface, next_hop, next_hop_mac);

  if (arp_result != 0) {
  eth_close(iface);
  return -1;
  }

  /* ¡OJO! Gestionar caso destino directamente conectado */
  /* 4. Enviar datagrama con eth_send() */
  int sent = eth_send(iface, next_hop_mac, 0x0800, datagram, datagram_len);
  eth_close(iface);
  if (sent != datagram_len) {
    return -1;
  }

  /* 5. Devolver longitud datos (payload) enviados */
  return payload_len;
}