#include "ipv4.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

// Número de protocolo experimental en IPv4 usado para identificar los paquetes de este servidor de eco sin interferir con protocolos reales.
#define ECHO_PROTOCOL 253

// Variable para avisar al bucle principal de que debe parar el servidor.
static volatile sig_atomic_t stop_requested = 0;

// Se ejecuta al pulsar Ctrl+C para poner la variable a 1 y avisar al bucle de que debe detenerse.
static void request_stop(int signal_number) {
   (void)signal_number;
   stop_requested = 1;
}
// La variable argc sirve para contar el número exacto de palabras introducidas en la consola al lanzar el programa (como mínimo deben ser 3: ./programa (0), eth0 (1), y la IP (2)). Por otro lado, argv es una lista que almacena el texto de cada una de esas palabras para que puedas inyectar la interfaz y la IP directamente en tu código.
int main(int argc, char *argv[]) {
   // Verificar que el usuario incluye el comando para iniciar el programa, la interfaz de red y la IP de destino.
   if (argc != 3) {
      fprintf(stderr, "Error: Debes escribir al menos 3 elementos para ejecutar el programa (Usage: %s <config> <routes>).\n", argv[0]);
      return EXIT_FAILURE;
   }

   // Pide al sistema que asocie el Ctrl+C a nuestra función de apagado; si el sistema no lo permite, el programa aborta sin llegar a arrancar.
   if (signal(SIGINT, request_stop) == SIG_ERR || signal(SIGTERM, request_stop) == SIG_ERR) {
      fprintf(stderr, "Could not install signal handlers.\n");
      return EXIT_FAILURE;
   }
   
   // ipv4_open() -> crea y configura toda la capa de red IPv4 (interfaz, IP y tabla de rutas) leyendo los archivos pasados como parámetro.
   // Reserva memoria para crear la estructura 'layer' y rellena todos sus datos (interfaz, IP y rutas) usando los archivos pasados por consola.
   ipv4_layer_t *layer = ipv4_open(argv[1], argv[2]);
   if (layer == NULL) {
      fprintf(stderr, "Could not initialize IPv4.\n");
      return EXIT_FAILURE;
   }

   printf("IPv4 echo server listening on protocol %d. Press Ctrl+C to stop.\n", ECHO_PROTOCOL);

   // Establece el estado de salida del programa como "éxito" por defecto, el cual cambiará a error si algo falla en el bucle.
   int status = EXIT_SUCCESS;

   while (!stop_requested) {
      // Un buffer con el tamaño máximo permitido para almacenar el contenido del mensaje que vamos a recibir.
      unsigned char payload[ETH_MTU];
      // Variable donde la función de recepción guardará la dirección IP del remitente para saber a quién tenemos que devolverle el mensaje.
      ipv4_addr_t sender;

      // Devuelve la cantidad de bytes recibidos: 0 significa tiempo agotado sin errores, menor a 0 indica un fallo, y mayor a 0 es un éxito.
      // Usa la red (layer) para escuchar el protocolo 253, guardando la IP de origen en 'sender' y el mensaje en 'payload' (limitado a su tamaño máximo) durante 500ms.
      int received = ipv4_recv(layer, sender, ECHO_PROTOCOL, payload, sizeof(payload), 500);
         if (stop_requested) {
            break;
         }
         if (received == 0) {
            continue;
         }
         if (received < 0) {
            fprintf(stderr, "Could not receive IPv4 datagram.\n");
            status = EXIT_FAILURE;
            break;
         }

         // Prepara un espacio de texto con el tamaño adecuado para escribir una dirección IP en formato legible (como "192.168.1.1").
         char address[IPv4_STR_MAX_LENGTH];
         // ipv4_addr_str() -> Convierte la dirección IP de su formato interno (4 bytes) a un texto legible por humanos. 
         // Traduce la IP de el que envía desde su formato interno (sender) a texto normal y la guarda en el espacio que acabamos de crear.
         ipv4_addr_str(sender, address);

         // Si los bytes recibidos no coinciden con los bytes enviados de vuelta, significa que hubo un error en la transmisión.
         if (ipv4_send(layer, sender, ECHO_PROTOCOL, payload, received) != received) {
            fprintf(stderr, "Could not echo to %s.\n", address);
            continue;
         }

         printf("Echoed %d bytes to %s\n", received, address);
   }

   // Cierra la capa IPv4 liberando todos los recursos que había reservado. 
   if (ipv4_close(layer) != 0) {
      fprintf(stderr, "Could not close IPv4 cleanly.\n");
      status = EXIT_FAILURE;
   }
   return status;
}