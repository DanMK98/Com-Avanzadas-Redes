#include "udp.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Función auxiliar para convertir un texto (string) en un número de puerto válido.
// Recibe el texto y un puntero donde guardar el resultado.
// end: Puntero para saber dónde deja de leer strtol.
// errno: Resetea la variable global de errores antes de la conversión.
// strtol() -> String TO Long. Convierte el texto a un número entero largo en base 10.
static int parse_port(char *text, uint16_t *port){
   char *end;
   errno = 0;
   long value = strtol(text, &end, 10);
   if (errno || end == text || *end != '\0' || value < 1 || value > 65535) {
      return -1;
   }
   *port = (uint16_t)value;
   return 0;
}

// La variable argc sirve para contar el número exacto de palabras introducidas en la consola al lanzar el programa (como mínimo deben ser 3: ./programa (0), eth0 (1), y la IP (2)). Por otro lado, argv es una lista que almacena el texto de cada una de esas palabras para que puedas inyectar la interfaz y la IP directamente en tu código.
int main(int argc, char *argv[]) {
   // Para UDP necesitamos 7 argumentos: programa, config, rutas, puerto_local, ip_servidor, puerto_servidor, mensaje.
   if (argc != 7) {
      fprintf(stderr, "Error: Debes escribir al menos 7 elementos para ejecutar el programa (Usage: %s <config> <routes> <local-port> <server-ip> <server-port> <message>).\n", argv[0]);
      return EXIT_FAILURE;
   }

   // Variables para almacenar los puertos (16 bits).
   uint16_t local_port, server_port;
   // Arreglo de 4 bytes para la IP del servidor.
   ipv4_addr_t server;
   // Longitud del mensaje a enviar (argumento 6).
   size_t length = strlen(argv[6]);

   // Se valida que los puertos sean correctos, que la IP se pueda convertir con inet_pton,
   // y que el mensaje no esté vacío ni supere el MTU (1500 - 20 cabecera IP - 8 cabecera UDP = 147
   // inet_pton -> es la función que traduce el texto de una IP a los 4 bytes binarios reales.
   if (parse_port(argv[3], &local_port) != 0 ||parse_port(argv[5], &server_port) != 0 ||inet_pton(AF_INET, argv[4], server) != 1 ||length == 0 || length > ETH_MTU - 20 - 8) {
      fprintf(stderr, "Invalid ports, server IPv4 address, or message (1-1472 bytes).\n");
      return EXIT_FAILURE;
   }

    // udp_open() -> Configura la capa de red inferior (IPv4) y reserva el puerto local para recibir respuestas.
   udp_layer_t *udp = udp_open(argv[1], argv[2], local_port);
   if (udp == NULL) {
      fprintf(stderr, "Could not initialize UDP.\n");
      return EXIT_FAILURE;
   }

   int status = EXIT_FAILURE;

   // udp_send() -> Empaqueta el mensaje con la cabecera UDP (añadiendo puertos) y lo envía a la IP destino.
   if (udp_send(udp, server, server_port, (unsigned char *)argv[6], (int)length) != (int)length) {
      fprintf(stderr, "Could not send message.\n");
   } else {
      // Buffer para guardar el mensaje recibido.
      unsigned char reply[ETH_MTU];
      // Guardará la IP de quien nos responde.
      ipv4_addr_t sender;
      // Guardará el PUERTO de quien nos responde (dato clave en UDP).
      uint16_t sender_port;

      // udp_recv() -> Escucha la red esperando un paquete UDP dirigido a nuestro puerto local.
      // El último parámetro (5000) es el timeout en milisegundos (5 segundos).
      int received = udp_recv(udp, sender, &sender_port, reply, sizeof(reply), 5000);
      if (received == 0) {
         fprintf(stderr, "Echo timed out after 5 seconds.\n");
      } else if (received < 0) {
         fprintf(stderr, "Could not receive echo.\n");
         // Aquí está la diferencia clave respecto a IPv4: además de comprobar que la IP de origen es la correcta,
         // se comprueba que el PUERTO de origen (sender_port) coincide con el puerto del servidor al que enviamos.
      } else if (memcmp(sender, server, IPv4_ADDR_SIZE) != 0 || sender_port != server_port || received != (int)length || memcmp(reply, argv[6], length) != 0) {
         fprintf(stderr, "Reply source or payload does not match the request.\n");
      } else {
         // Se imprime la IP y el PUERTO del servidor (formato IP:PUERTO).
         printf("Echo verified: %d bytes from %s:%u\n", received, argv[4], (unsigned int)sender_port);
         fwrite(reply, 1, (size_t)received, stdout);
         putchar('\n');
         status = EXIT_SUCCESS;
      }  
   }
   // Cierre y liberación de recursos.
   if (udp_close(udp) != 0) {
      fprintf(stderr, "Could not close UDP cleanly.\n");
      status = EXIT_FAILURE;
   }
   return status;
}