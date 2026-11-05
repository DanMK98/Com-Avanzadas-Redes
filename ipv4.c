#include "ipv4.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
} __attribute__((__packed__)); esta parte no estaba en mi codigo 

/* Dirección IPv4 a cero: "0.0.0.0" */
ipv4_addr_t IPv4_ZERO_ADDR = { 0, 0, 0, 0 };


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


// file_conf: Archivo de configuración que contiene la interfaz, IP y máscara de red.
// file_conf_route: Archivo de configuración que contiene la tabla de rutas.
// Inicializa la capa IPv4 extrayendo tu IP, máscara y mapa de rutas desde los archivos para construir y devolver la estructura de red operativa.
ipv4_layer_t* ipv4_open(char * file_conf, char * file_conf_route) {
   // 'malloc' (memory allocation) solicita al sistema operativo que reserve dinámicamente un bloque de memoria RAM durante la ejecución del programa.
   // Calcula los bytes exactos que necesita la estructura 'ipv4_layer_t', reserva ese espacio libre en memoria, y guarda su dirección en el puntero 'layer' para empezar a almacenar ahí los datos de tu red.
   ipv4_layer_t * layer = malloc(sizeof(ipv4_layer_t));
   if (layer == NULL) {
        return NULL;
   }

   // ipv4_route_table_create() -> Esta función reserva dinámicamente el espacio necesario en la RAM para una tabla de rutas vacía y devuelve sus coordenadas.
   // Esta línea vincula nuestro centro de control con un nuevo mapa de rutas guardando su dirección de memoria en el puntero routing_table.
   layer->routing_table = ipv4_route_table_create();
   if (layer->routing_table == NULL) {
      free(layer); // Si la tabla falla, liberamos la memoria base y escapamos
      return NULL;
   }

   // Nombre de la interfaz que vamos a abrir.
   char iface_name[IFACE_NAME_MAX_LENGTH];

   // ipv4_config_read() -> Lee un archivo de texto especificado para extraer y cargar automáticamente el nombre de la interfaz, tu IP y la máscara en las variables proporcionadas.
   // ipv4_route_table_free() -> Destruye el mapa de rutas indicado y devuelve al sistema operativo toda la memoria dinámica que se había reservado para él.
   // ipv4_config_read devuelve un entero que podemos usar para verificar si falló
   // Aquí se lee file_conf y se añade la información a iface_name y a la estructura layer (addr y netmask).
   if (ipv4_config_read(file_conf, iface_name, layer->addr, layer->netmask) < 0) {
      ipv4_route_table_free(layer->routing_table);
      free(layer);
      return NULL;
   }

   // Aquí se lee file_conf_route y se añade la información a la tabla de rutas de layer.
   if (ipv4_route_table_read(file_conf_route, layer->routing_table) < 0) {
      ipv4_route_table_free(layer->routing_table);
      free(layer);
      return NULL;
   }

   // eth_open()-> Inicia la conexión con la interfaz de red y devuelve una "llave" o manejador para poder usarla en los posteriores envíos y recepciones.
   layer->iface = eth_open(iface_name);

   if (layer->iface == NULL) {
      ipv4_route_table_free(layer->routing_table);
      free(layer);

      return NULL;
   }
   return layer;
}

// Layer, heredado de ipv4_open.
// dst: Dirección IP de destino.
// protocol: Es el número que identifica qué protocolo te está entregando la carga.
// El 'payload' apunta a la dirección de memoria donde empiezan los datos, mientras que 'payload_len' indica la cantidad exacta de bytes a leer para saber dónde termina el mensaje.
// Encapsula los datos de usuario añadiendo la cabecera IPv4 y los transmite hacia la IP de destino consultando previamente el mapa de rutas para saber por dónde enviarlos.
int ipv4_send (ipv4_layer_t * layer, ipv4_addr_t dst, uint8_t protocol, unsigned char * payload, int payload_len) {
   if ((layer == NULL) || (dst == NULL) || (payload == NULL) || (payload_len <= 0)) {
      return -1;
   }

   // ETH_MTU: Tamaño máximo de un paquete Ethernet (1500 bytes). 
   // Comprueba si los datos son demasiado grandes para caber en un solo paquete de red tras descontar el espacio obligatorio de la cabecera IPv4. (cabecera + datos) 
   if (payload_len > ETH_MTU - (int) sizeof(struct ipv4_header)) {
      return -1;
   }

   // Construir el datagrama IPv4
   struct ipv4_header header = {0};
   int datagram_len = sizeof(header) + payload_len;
   unsigned char datagram[ETH_MTU];

   // Configura los parámetros básicos de la cabecera IPv4: versión, longitud total del datagrama, tiempo de vida (TTL) y el protocolo encapsulado.
   header.version_ihl = 0x45; 
   header.total_length = htons(datagram_len);
   header.ttl = 64;
   header.protocol = protocol;
   memcpy(header.src, layer->addr, IPv4_ADDR_SIZE);
   memcpy(header.dst, dst, IPv4_ADDR_SIZE);
   header.checksum = htons(ipv4_checksum((unsigned char *) &header, sizeof(header)));

   memcpy(datagram, &header, sizeof(header));
   memcpy(datagram + sizeof(header), payload, payload_len);

   /* 2. Obtener siguiente salto con ipv4_route_table_lookup () */
   ipv4_route_t * route = ipv4_route_table_lookup(layer->routing_table, dst);

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

   mac_addr_t next_hop_mac;

   int arp_result = arp_resolve(layer->iface, layer->addr, next_hop, next_hop_mac);

   if (arp_result != 0) {
      return -1;
   }

   /* ¡OJO! Gestionar caso destino directamente conectado */
   /* 4. Enviar datagrama con eth_send() */
   int sent = eth_send(layer->iface, next_hop_mac, 0x0800, datagram, datagram_len);
   if (sent != datagram_len) {
      return -1;
   }

   return payload_len;
}

int ipv4_recv(ipv4_layer_t * layer, ipv4_addr_t sender, uint8_t protocol, unsigned char buffer[], int buf_len, long int timeout) {
   if (layer == NULL) return -1;
    
   timerms_t timer;
   timerms_reset(&timer, timeout);
    
   mac_addr_t src_mac;
   unsigned char datagram[ETH_MTU];
    
   // 1. Entrar en bucle (do while) con temporizador
   do {
      long int time_left = timerms_left(&timer);
        
      // 2. Escuchar datagramas IPv4 con eth_recv() (EtherType IPv4 = 0x0800)
      int bytes_leidos = eth_recv(layer->iface, src_mac, 0x0800, datagram, ETH_MTU, time_left);
        
      if (bytes_leidos > 0) {
         struct ipv4_header *header = (struct ipv4_header *) datagram;
            
         // 3. Comprobar dirección destino y protocolo
         if (header->protocol == protocol && memcmp(header->dst, layer->addr, IPv4_ADDR_SIZE) == 0) {
                
            // 4. Copiar datos y dirección origen
            memcpy(sender, header->src, IPv4_ADDR_SIZE);
                
            // Extraer IHL descartando la versión (máscara 0x0F) y multiplicar por 4 bytes
            int header_len = (header->version_ihl & 0x0F) * 4;
            int payload_len = ntohs(header->total_length) - header_len;
                
            // Prevenir desbordamientos adaptando el tamaño de volcado al buffer local
            int bytes_a_copiar = (payload_len < buf_len) ? payload_len : buf_len;
            memcpy(buffer, datagram + header_len, bytes_a_copiar);
                
            // 5. Devolver longitud datos (payload) recibidos
            return bytes_a_copiar;
         }
      }
   // Permitir espera infinita si timeout es negativo
   } while (timerms_left(&timer) > 0 || timeout < 0); 
    
    return -1;
}

int ipv4_close(ipv4_layer_t * layer) {
    if (layer != NULL) {
        // 1. Liberar tabla de rutas (layer -> routing_table)
        ipv4_route_table_free(layer->routing_table);
        
        // 2. Cerrar capa Ethernet con eth_close()
        eth_close(layer->iface);
        free(layer);
        
        return 0;
    }
    return -1;
}
