#include "ipv4.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Número de protocolo experimental en IPv4 usado para identificar los paquetes de este servidor de eco sin interferir con protocolos reales.
#define ECHO_PROTOCOL 253
// Calcula el espacio máximo útil para enviar el mensaje restándole los 20 bytes de la cabecera IPv4 al límite total de la red.
#define ECHO_MAX_PAYLOAD (ETH_MTU - 20)

// Convierte el texto de una dirección IPv4 a un arreglo de 4 bytes, validando estrictamente que no haya caracteres extra y que cada número cumpla la fórmula 0 <= valor <= 255.
static int parse_address(char *text, ipv4_addr_t address) {
   // 'octet' almacena los cuatro números de la IP extraídos del texto, mientras que 'trailing' sirve como trampa para detectar y rechazar cualquier letra o carácter basura sobrante al final de la dirección.
   unsigned int octet[4];
   char trailing;
   // sscanf -> analiza el texto para extraer los cuatro números separados por puntos, y la condición devuelve -1 si no se logran leer exactamente cuatro elementos válidos.
   // Si no logra leer exactamente los 4 números esperados, detiene el proceso devolviendo un error.
   if (sscanf(text, "%u.%u.%u.%u%c", &octet[0], &octet[1], &octet[2], &octet[3], &trailing) != 4) {
      return -1;
   }
   // Limite cada número a un rango válido de 0 a 255.
   for (int i = 0; i < 4; i++) {
      if (octet[i] > 255) {
         return -1;
      }
   // Copia los números validados al arreglo de bytes de la dirección IP.
   address[i] = (unsigned char)octet[i];
   }
   return 0;
}

// La variable argc sirve para contar el número exacto de palabras introducidas en la consola al lanzar el programa (como mínimo deben ser 5: ./programa (0), eth0 (1), la IP de destino (2), el mensaje (3)). Por otro lado, argv es una lista que almacena el texto de cada una de esas palabras para que puedas inyectar la interfaz y la IP directamente en tu código.
int main(int argc, char *argv[]) {
   // Verificar que el usuario incluye los 5 parámetros necesarios: comando de ejecución, archivo de configuración, archivo de rutas, IP de destino y el mensaje.
   if (argc != 5) {
      fprintf(stderr, " Error: Debes escribir al menos 5 elementos para ejecutar el programa (Usage: %s <config> <routes> <server-ip> <message>).\n", argv[0]);
      return EXIT_FAILURE;
   }
   
   // 'server' almacenará la IP de destino en formato de bytes, y 'length' calcula la cantidad de caracteres del mensaje que el usuario quiere enviar.
   ipv4_addr_t server;
   size_t length = strlen(argv[4]);

   // Si la dirección IP es incorrecta o la longitud del mensaje está fuera de los límites permitidos, interrumpe el programa devolviendo un error.
   if (parse_address(argv[3], server) != 0 || length == 0 || length > ECHO_MAX_PAYLOAD) {
      fprintf(stderr, "Invalid IPv4 address or message (1-1480 bytes).\n");
      return EXIT_FAILURE;
   }

   // ipv4_open() -> crea y configura toda la capa de red IPv4 (interfaz, IP y tabla de rutas) leyendo los archivos pasados como parámetro.
   // Reserva memoria para crear la estructura 'layer' y rellena todos sus datos (interfaz, IP y rutas) usando los archivos pasados por consola.
   ipv4_layer_t *layer = ipv4_open(argv[1], argv[2]);
   if (layer == NULL) {
      fprintf(stderr, "Could not initialize IPv4.\n");
      return EXIT_FAILURE;
   }

   // Inicializa el estado de salida del programa como "error" por defecto, el cual cambiará a éxito si todo funciona correctamente.
   int status = EXIT_FAILURE;

   // Intenta enviar el mensaje al destinatario y muestra un error si la cantidad de bytes realmente transmitidos no coincide con la longitud total de los datos.
   if (ipv4_send(layer, server, ECHO_PROTOCOL, (unsigned char *)argv[4], (int)length) != (int)length) {
      fprintf(stderr, "Could not send message.\n");
   } else {
      // 'reply' reserva espacio en memoria para guardar el mensaje de respuesta, mientras que 'sender' almacenará la dirección IP de quien nos ha contestado.
      unsigned char reply[ETH_MTU];
      ipv4_addr_t sender;
      // Escucha la red esperando una respuesta del servidor de eco durante 5 segundos, y guarda el mensaje recibido en 'reply' y la IP del remitente en 'sender'.
      int received = ipv4_recv(layer, sender, ECHO_PROTOCOL, reply, sizeof(reply), 5000);

      if (received == 0) {
         fprintf(stderr, "Echo timed out after 5 seconds.\n");
      } else if (received < 0) {
         fprintf(stderr, "Could not receive echo.\n"); 
         // memcmp() -> Compara byte por byte dos bloques de memoria y devuelve 0 si su contenido es exactamente idéntico.
      } else if (memcmp(sender, server, IPv4_ADDR_SIZE) != 0 || received != (int)length || memcmp(reply, argv[4], length) != 0) {
         fprintf(stderr, "Reply source or payload does not match the request.\n");
      } else {
         printf("IPv4 echo verified: %d bytes from %s\n", received, argv[3]);
         fwrite(reply, 1, (size_t)received, stdout);
         putchar('\n');
         status = EXIT_SUCCESS;
      }
   }
   
   // Cierra la capa IPv4 liberando todos los recursos que había reservado. 
   if (ipv4_close(layer) != 0) {
      fprintf(stderr, "Could not close IPv4 cleanly.\n");
      status = EXIT_FAILURE;
   }
   return status;
}