#include <arpa/inet.h>
#include <string.h>
#include <timerms.h>
#include <stdlib.h>
#include "udp.h"

#define UDP_PROTOCOL 17
#define UDP_HEADER_SIZE 8
#define IPV4_MIN_HEADER_SIZE 20
#define UDP_MAX_DATAGRAM (ETH_MTU - IPV4_MIN_HEADER_SIZE)
#define UDP_MAX_PAYLOAD (UDP_MAX_DATAGRAM - UDP_HEADER_SIZE)

// layer, heredado de udp_open.
// dst: Dirección IPv4 de la máquina de destino.
// dst_port: Puerto de la aplicación de destino (en formato host).
// payload: Puntero a los datos de usuario que queremos enviar.
// payload_len: Longitud en bytes de esos datos.
// Encapsula los datos de la aplicación añadiendo la cabecera UDP y los pasa a la capa IPv4 para su envío.
int udp_send(udp_layer_t *layer, ipv4_addr_t dst, uint16_t dst_port, unsigned char *payload, int payload_len) {
   // Validaciones de seguridad: comprueba punteros nulos, puertos inválidos o datos demasiado grandes/pequeños.
   if (layer == NULL || layer->ipv4 == NULL ||
      dst == NULL || layer->local_port == 0 ||
      dst_port == 0 || payload_len < 0 ||
      payload_len > UDP_MAX_PAYLOAD ||
      (payload == NULL && payload_len > 0)) {
      return -1;
   }

   // Preparan un buffer temporal en memoria para ensamblar el datagrama completo y calculan su tamaño total sumando los bytes de la cabecera a los datos útiles.
   unsigned char datagram[UDP_MAX_DATAGRAM];
   int datagram_len = UDP_HEADER_SIZE + payload_len;
    
   // Construir la cabecera UDP
   struct udp_header header;
   // htons() -> Convierte el número de puerto del formato de bytes de tu ordenador (host) al estándar de la red (Big-Endian).
   header.src_port = htons(layer->local_port);
   header.dst_port = htons(dst_port);
   header.length = htons(datagram_len);
   // En IPv4 el cálculo del checksum UDP es opcional. Asignar 0 significa que no se utiliza, ahorrando ciclos de procesador.
   header.checksum = 0;

   // memcpy() -> Copia la cabecera UDP recién configurada al principio del datagrama.
   memcpy(datagram, &header, sizeof(header));

   // Si hay datos que enviar, los copia justo después de la cabecera (desplazando UDP_HEADER_SIZE bytes).
   if (payload_len > 0) {
      memcpy(datagram + UDP_HEADER_SIZE, payload, payload_len);
 }

   // ipv4_send() -> Pasa el datagrama completo a la capa inferior, indicando que el protocolo encapsulado es UDP (17).
   int sent = ipv4_send(layer->ipv4, dst, UDP_PROTOCOL, datagram, datagram_len);

   // Si la capa IPv4 no pudo enviar todos los bytes exactos de nuestro datagrama, hubo un error de transmisión.
   if (sent != datagram_len) {
      return -1;
   }
   // Devuelve la longitud de los datos útiles que se han enviado con éxito.
   return payload_len;
}

// layer, heredado de udp_open.
// src: Variable donde se guardará automáticamente la IP del remitente que nos envió el paquete.
// src_port: Variable donde se guardará el puerto de origen del remitente (en formato host).
// buffer: Espacio de memoria para guardar los datos útiles extraídos.
// buffer_len: Capacidad máxima del buffer para evitar desbordamientos.
// timeout: Tiempo límite en milisegundos para esperar un paquete.
// Escucha la red filtrando el tráfico que vaya dirigido exclusivamente a nuestro puerto local y extrae los datos útiles.
int udp_recv(udp_layer_t *layer, ipv4_addr_t src, uint16_t *src_port, unsigned char *buffer, int buffer_len, long int timeout) {
   if (layer == NULL || layer->ipv4 == NULL ||
      layer->local_port == 0 || src == NULL ||
      src_port == NULL || buffer == NULL || buffer_len <= 0) {
      return -1;
   }

   // 'datagram' actuará como contenedor para atrapar el paquete entero, y 'sender' atrapará la IP origen.
   unsigned char datagram[UDP_MAX_DATAGRAM];
   ipv4_addr_t sender;
   timerms_t timer;
   // Actúa como una bandera para garantizar que el bucle intente capturar un paquete al menos una vez antes de considerar que el tiempo de espera se ha agotado.
   int first_attempt = 1;

   // timerms_reset() -> Configura y arranca el cronómetro solo si se proporcionó un timeout válido.
   if (timeout >= 0) {
      timerms_reset(&timer, timeout);
   }

   // Bucle infinito: atrapamos paquetes hasta que llegue el nuestro o se acabe el tiempo.
   for (;;) {
      // Almacena los milisegundos de espera restantes para la próxima captura de paquetes, usando -1 como valor por defecto para indicar una espera infinita si no hay temporizador.
      long int remaining = -1;

      // timerms_left() -> Consulta el reloj para ver cuántos milisegundos le quedan al temporizador.
      if (timeout >= 0) {
         remaining = timerms_left(&timer);

         // Si expiró el tiempo y ya iteramos antes, devolvemos 0 indicando timeout cumplido.
         if (remaining <= 0) {
            if (!first_attempt) {
               return 0;
            }
         remaining = 0;
            }
      }
      first_attempt = 0;

      // ipv4_recv() -> Escucha la capa inferior buscando solo paquetes que utilicen el protocolo UDP (17).
      int received = ipv4_recv(layer->ipv4, sender, UDP_PROTOCOL, datagram, sizeof(datagram), remaining);

      // Si se agotó el tiempo (0) o hubo un error de tarjeta (-1), salimos del bucle devolviendo ese valor.
      if (received <= 0) {
          return received;
      }

      // Verifica que el paquete tenga al menos el tamaño de una cabecera UDP para evitar lecturas de basura.
      if (received < UDP_HEADER_SIZE) {
         continue;
      }

      // Copiamos la cabecera desde el datagrama crudo a la estructura para leer sus campos de forma sencilla.
      struct udp_header header;
      memcpy(&header, datagram, sizeof(header));

      // Evalúa la diferencia entre 'datagram_len' (el tamaño que promete la cabecera) y 'received' (los bytes físicos reales que llegaron), descartando el paquete si afirma traer más datos de los que realmente entraron.
      // ntohs() -> Traduce el número del tamaño (que viaja por la red) para que lo entienda nuestro procesador.
      int datagram_len = ntohs(header.length);

      // Filtros: validamos longitud mínima, que no prometa ser más grande que lo que entró, y que vaya dirigido a NUESTRO puerto.
      // Si no es para nosotros o está mal formado, ignoramos y seguimos esperando el correcto.
      if (datagram_len < UDP_HEADER_SIZE || datagram_len > received ||ntohs(header.dst_port) != layer->local_port) {
         continue;
      }

      // Calculamos el tamaño real de la carga útil restando el tamaño del sobre (la cabecera UDP).
      int payload_len = datagram_len - UDP_HEADER_SIZE;

      // Paquete sin datos (solo cabecera), volvemos a escuchar.
      if (payload_len == 0) {
         continue;
      }

      // Protección contra desbordamiento de memoria si el paquete trae más datos de los que el usuario puede guardar.
      if (payload_len > buffer_len) {
         return -1;
      }

      // Copiamos los datos extraídos al buffer, saltándonos el espacio de la cabecera.
      memcpy(buffer, datagram + UDP_HEADER_SIZE, payload_len);
      // Trasvasamos la IP de origen que guardó ipv4_recv hacia la variable src del usuario.
      memcpy(src, sender, IPv4_ADDR_SIZE);
      // Guardamos el puerto de origen (traduciéndolo antes al formato del ordenador con ntohs).
      *src_port = ntohs(header.src_port);

      return payload_len;
   }
}

// file_conf: Archivo de configuración que contiene la interfaz, IP y máscara de red.
// file_conf_route: Archivo de configuración que contiene la tabla de rutas.
// local_port: Puerto local por el que esta capa va a recibir y enviar información.
// Inicializa la capa UDP, reservando memoria y levantando a la vez la conexión con la capa IPv4 inferior.
udp_layer_t *udp_open(char *file_conf, char *file_conf_route, uint16_t local_port) {
   if (local_port == 0) {
      return NULL;
   }

   // 'malloc' solicita al sistema operativo reservar memoria RAM para albergar nuestra estructura UDP.
   udp_layer_t *layer = malloc(sizeof(*layer));
   if (layer == NULL) {
      return NULL;
   }

   // ipv4_open() -> Arranca el procesamiento IP con las rutas especificadas y devuelve un puntero a su control.
   // Vinculamos la capa inferior recién creada (ipv4) como base de nuestra capa UDP.
   // Si la parte de red falla, cancelamos la creación y liberamos la memoria UDP.
   layer->ipv4 = ipv4_open(file_conf, file_conf_route);
   if (layer->ipv4 == NULL) {
      free(layer);
      return NULL;
   }

   layer->local_port = local_port;
   return layer;
}

// layer, heredado de udp_open.
// Destruye todo el stack de red hacia abajo ordenadamente, liberando los recursos y la memoria ocupada.
int udp_close(udp_layer_t *layer) {
   if (layer == NULL) {
      return -1;
   }

   // ipv4_close() -> Indica a la capa inferior que debe apagar la tarjeta y destruir sus propias tablas/rutas.
   int result = ipv4_close(layer->ipv4);
   // free() -> Devuelve al sistema operativo el bloque de memoria RAM que se reservó para esta capa UDP en el open().
   free(layer);

   return result;
}