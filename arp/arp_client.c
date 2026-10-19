#include <stdio.h>  // Proporciona funciones de entrada y salida como printf() para interactuar con la terminal.
#include <stdlib.h> // Contiene funciones de gestión del sistema como exit() para abortar la ejecución de forma segura si hay un error.
#include "eth.h"    // Permite controlar el puerto de red para mandar y leer mensajes físicos.
#include "ipv4.h"   // Facilita las funciones necesarias para manejar direcciones IP, como traducir cadenas de texto al formato binario que entiende la red.
#include "arp.h"    // Importa las estructuras (como arp_header) y la función arp_resolve() definidas en la cabecera.

// La variable argc sirve para contar el número exacto de palabras introducidas en la consola al lanzar el programa (como mínimo deben ser 3: ./programa (0), eth0 (1), y la IP (2)). Por otro lado, argv es una lista que almacena el texto de cada una de esas palabras para que puedas inyectar la interfaz y la IP directamente en tu código.
int main(int argc, char *argv[]) {
   // PASO 1: Validación de parámetros. Se comprueba que el usuario ejecuta el programa pasando la interfaz y la IP como argumentos.
   // 1. Verificar que el usuario pasa la interfaz y la IP al ejecutar
   if (argc < 3) { // Este es el caso en el que el usuario no ha pasado los parámetros correctos al ejecutar el programa (que son al menos 3).
      fprintf(stderr, "Error: Debes escribir al menos 3 elementos para ejecutar el programa (ejemplo: %s eth0 192.168.1.20).\n", argv[0]);
      exit(EXIT_FAILURE);
   }
   char *iface_name = argv[1]; // Se guarda el primer argumento escrito por el usuario (el nombre de la interfaz física, ej. "eth0") en un puntero de texto.
   char *ip_str = argv[2];     // Se guarda el segundo argumento (la IP de destino en formato legible, ej. "192.168.1.20") en otro puntero.

   // PASO 2: Apertura del hardware. Abrir la interfaz de red para poder comunicarnos físicamente cn el exterior.
   // eth_open()-> Inicia la conexión con la interfaz de red y devuelve una "llave" o manejador para poder usarla en los posteriores envíos y recepciones.
   eth_iface_t *iface = eth_open(iface_name);
   if (iface == NULL) {
      fprintf(stderr, "Error: No se pudo abrir la interfaz %s\n", iface_name);
      exit(EXIT_FAILURE);
   }

   // PASO 3: Conversión de formatos. Pasar la IP de un texto legible por humanos (string) al formato interno de 4 bytes que usa la lógica de red.
   // ipv4_str_addr(recibido, traducido).
   ipv4_addr_t dest_ip;            // Crea una variable vacía llamada 'dest_ip' usando el tipo 'ipv4_addr_t' (importado de "ipv4.h"), el cual funciona como un array de 4 casillas exactas para almacenar los 4 bytes reales de una dirección IP.
   ipv4_str_addr(ip_str, dest_ip); // Traduce la IP desde el formato de texto humano (ej. "192.168.1.20" guardado en 'ip_str') hacia el formato numérico de red, guardando cada uno de esos 4 valores matemáticos dentro de las casillas de 'dest_ip'.

   // PASO 4: Variable para almacenar la MAC que nos devuelva arp_resolve.
   // mac_addr_t es un tipo de dato personalizado que funciona como un array de 6 casillas, el tamaño exacto necesario para almacenar los 6 bytes de una dirección física.
   mac_addr_t mac_resuelta;

   // PASO 5: Ejecución del protocolo. Invoca la función central encargada de lanzar la petición ARP a la red, arrancar el temporizador y escuchar la respuesta.
   // arp_resolve(Llave_de_interfaz, IP_buscada, Contenedor_vacio)
   int resultado = arp_resolve(iface, dest_ip, mac_resuelta); // Se pasa la llave de la interfaz de red (iface), la IP objetivo ya convertida a formato binario (dest_ip), y el contenedor vacío (mac_resuelta) para que lo rellene.

   // PASO 6: Evaluación y muestra del resultado en pantalla.
   if (resultado == 0) {
      // Éxito, se imprime la IP y la MAC descubierta en formato hexadecimal con dos dígitos por byte.
      printf("%s -> %02X:%02X:%02X:%02X:%02X:%02X\n", 
         ip_str,
         mac_resuelta[0], mac_resuelta[1], mac_resuelta[2], 
         mac_resuelta[3], mac_resuelta[4], mac_resuelta[5]);
   } else {
      // Error / Timeout
      fprintf(stderr, "Error: No se recibió respuesta ARP para la IP %s tras 2 segundos.\n", ip_str);
   }

   // PASO 7: Limpieza del sistema.
   // eth_close()-> Libera el uso de la interfaz de red, cerrando ordenadamente la "llave" que habíamos solicitado en el Paso 2 para no dejar recursos bloqueados en el sistema operativo.
   eth_close(iface);
   return 0;
}