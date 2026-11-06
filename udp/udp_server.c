#include "udp.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

// Variable para avisar al bucle principal de que debe parar el servidor.
static volatile sig_atomic_t stop_requested = 0;

// Se ejecuta al pulsar Ctrl+C para poner la variable a 1 y avisar al bucle de que debe detenerse.
static void request_stop(int signal_number) {
   (void)signal_number;
   stop_requested = 1;
}

// La variable argc sirve para contar el número exacto de palabras introducidas en la consola al lanzar el programa (como mínimo deben ser 3: ./programa (0), eth0 (1), y la IP (2)). Por otro lado, argv es una lista que almacena el texto de cada una de esas palabras para que puedas inyectar la interfaz y la IP directamente en tu código.
int main(int argc, char *argv[]){
   if (argc != 4) {
      fprintf(stderr, "Error: Debes escribir al menos 4 elementos para ejecutar el programa(Usage: %s <config> <routes> <local-port>).\n", argv[0]);
      return EXIT_FAILURE;
  }

   // end: Puntero para saber dónde deja de leer strtol.
   char *end;
   // errno: Resetea la variable global de errores antes de la conversión.
   errno = 0;
   // strtol() -> String TO Long. Convierte el texto a un número entero largo en base 10.
   long port = strtol(argv[3], &end, 10);

   // Comprueba que la conversión fue limpia y que el puerto está en el rango válido (1-65535).
   if (errno || end == argv[3] || *end != '\0' || port < 1 || port > 65535) {
      fprintf(stderr, "Port must be between 1 and 65535.\n");
      return EXIT_FAILURE;
   }
   // signal() -> Le dice al sistema operativo: "Cuando ocurra esta señal, ejecuta esta función".
   // SIGINT: Es la señal que se envía al pulsar Ctrl+C.
   // SIGTERM: Es la señal que se envía cuando el sistema operativo quiere cerrar el programa.
   // Si signal() devuelve SIG_ERR, significa que hubo un problema al registrar el manejador.
   if (signal(SIGINT, request_stop) == SIG_ERR || signal(SIGTERM, request_stop) == SIG_ERR) {
      fprintf(stderr, "Could not install signal handlers.\n");
      return EXIT_FAILURE;
   }

   // udp_open() -> Configura IPv4, abre la interfaz y "reserva" el puerto local para escuchar.
   udp_layer_t *udp = udp_open(argv[1], argv[2], (uint16_t)port);
   if (udp == NULL) {
      fprintf(stderr, "Could not initialize UDP.\n");
      return EXIT_FAILURE;
   }

   printf("UDP echo server listening on port %ld. Press Ctrl+C to stop.\n", port);
   int status = EXIT_SUCCESS;

   // Mientras el interruptor 'stop_requested' siga a 0, el servidor sigue escuchando.
   // payload -> Buffer donde se guardará el mensaje recibido.
   // sender -> Guardará la IP de quien nos envía el mensaje.
   // sender_port -> Guardará el PUERTO de quien nos envía el mensaje.
   while (!stop_requested) {
      unsigned char payload[ETH_MTU];
      ipv4_addr_t sender;
      uint16_t sender_port;

      // udp_recv() -> Escucha el puerto local esperando un datagrama.
      // El timeout es de 500 ms (medio segundo). Esto es clave: si no llega nada, 
      // la función devuelve 0 y el bucle vuelve a comprobar 'stop_requested'.
      int received = udp_recv(udp, sender, &sender_port, payload, sizeof(payload), 500);

      // Si mientras esperábamos recibir algo, el usuario pulsó Ctrl+C...
      if (stop_requested) {
         break;
      }
      // Si 'received' es 0, significa que pasaron los 500 ms sin recibir nada (timeout).
      if (received == 0) {
         continue;
      }
      // Si 'received' es negativo, hubo un error real en la recepción.
      if (received < 0) {
         fprintf(stderr, "Could not receive UDP datagram.\n");
         status = EXIT_FAILURE;
         break;
      }

      // Si hemos llegado aquí, es que hemos recibido un mensaje válido.
      // ipv4_addr_str() -> Convierte la IP binaria del remitente a texto legible.
      char address[IPv4_STR_MAX_LENGTH];
      ipv4_addr_str(sender, address);

      // udp_send() -> Reenvía exactamente el mismo mensaje ('payload') de vuelta al remitente.
      // Fíjate que usamos la IP y el puerto del remitente ('sender', 'sender_port') como destino.
      if (udp_send(udp, sender, sender_port, payload, received) != received) {
         fprintf(stderr, "Could not echo to %s:%u.\n", address, (unsigned int)sender_port);
         continue;
      }

      // Si todo fue bien, mostramos por pantalla a quién le hemos hecho el eco.
      printf("Echoed %d bytes to %s:%u\n", received, address, (unsigned int)sender_port);
   }
   // Se ejecuta cuando el bucle termina (por Ctrl+C o por un error).
   if (udp_close(udp) != 0) {
      fprintf(stderr, "Could not close UDP cleanly.\n");
      status = EXIT_FAILURE;
   }
   return status;
}