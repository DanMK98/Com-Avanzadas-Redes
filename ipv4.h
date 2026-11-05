#ifndef _IPv4_H
#define _IPv4_H

#include <stdint.h>
#include "eth.h"

// Define el tamaño binario de una IPv4 (4 bytes) y su límite máximo en formato de texto (16 caracteres).
// Logitud máxmima del nombre de un interfaz de red.
#define IPv4_ADDR_SIZE 4
#define IPv4_STR_MAX_LENGTH 16
#define IFACE_NAME_MAX_LENGTH 32

typedef unsigned char ipv4_addr_t [IPv4_ADDR_SIZE];

// Dirección IPv4 a cero "0.0.0.0" 
extern ipv4_addr_t IPv4_ZERO_ADDR;

// Nos quedamos con ipv4_route_table_t
typedef struct ipv4_route_table ipv4_route_table_t;
 
typedef struct ipv4_layer {
   eth_iface_t * iface;                // iface: La interfaz de red física (la tarjeta) por donde entran y salen los paquetes.
   ipv4_addr_t addr;                   // addr: La dirección IP propia que tiene asignada esta máquina para identificarse.
   ipv4_addr_t netmask;                // netmask: La máscara de subred, usada matemáticamente para calcular si una IP de destino está en tu misma red local o en una externa.
   ipv4_route_table_t * routing_table; // routing_table: La tabla de rutas (el mapa) que le dice al sistema por qué camino debe enviar los paquetes cuando el destino está fuera de tu red local.
 */
} ipv4_layer_t;

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
void ipv4_addr_str ( ipv4_addr_t addr, char* str );


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
int ipv4_str_addr ( char* str, ipv4_addr_t addr );

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

uint16_t ipv4_checksum ( unsigned char * data, int len );




ipv4_layer_t* ipv4_open(char * file_conf, char * file_conf_route);

int ipv4_send(ipv4_layer_t *layer, ipv4_addr_t dst,uint8_t protocol, unsigned char *payload,int payload_len);

int ipv4_recv(ipv4_layer_t * layer, ipv4_addr_t sender, uint8_t protocol, unsigned char buffer[], int buf_len, long int timeout);

int ipv4_close(ipv4_layer_t * layer);

#endif /* _IPv4_H */
