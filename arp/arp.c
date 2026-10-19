#include "arp.h"       // Importa la estructura arp_header para manejar mensajes ARP.
#include "timerms.h"   // Habilita el control del flujo del programa mediante temporizadores en milisegundos.  
#include <arpa/inet.h> // Facilita las funciones htons  y ntohs para traducir los números a un formato universal, garantizando que todos los ordenadores los lean de la misma forma.
#include <string.h>    // Proporciona funciones de manipulación a nivel de bytes (memcpy, memset, memcmp) para operar con IPs y MACs.

// Define las instrucciones reales de la función anunciada en el archivo .h, utilizando la interfaz y la IP recibidas para buscar la MAC y devolver el resultado.
int arp_resolve ( eth_iface_t * iface, ipv4_addr_t dest, mac_addr_t mac ) {
    
   // Creamos una variable vacía llamada 'peticion' utilizando el formato arp_header como molde para organizar y rellenar los datos del mensaje antes de enviarlo.
   struct arp_header peticion;
 
   // PASO 1: Rellenamos los campos de 'peticion'.
   peticion.hw_type = htons(1);         // 1 = Red Ethernet.
   peticion.proto_type = htons(0x0800); // 0x0800 = Protocolo IPv4.
   peticion.hw_len = 6;                 // Tamaño de la dirección MAC (6 bytes).
   peticion.proto_len = 4;              // Tamaño de la dirección IP (4 bytes).   
   peticion.opcode = htons(1);          // 1 = Esto es una pregunta (Request).

   // PASO 2: Asigna nuestra MAC física y la IP objetivo a la petición, rellenando con ceros nuestra IP (aún no asignada) y la MAC de destino (el dato a descubrir).
   // Nos dan la ip del destino, pero no nos dan el nuestro mac, tenemos que buscarlo nosotros mismos.
   mac_addr_t mi_mac;          // Creamos una variable local y vacía diseñada específicamente para almacenar una dirección MAC.
   eth_getaddr(iface, mi_mac); // Lee la MAC física real de tu interfaz de red y la guarda dentro de la variable 'mi_mac'.
   // memcpy(Destino, Origen, Tamaño)-> sirve para copiar un bloque exacto de bytes de un lugar de la memoria a otro.
   memcpy(peticion.src_mac, mi_mac, sizeof(mac_addr_t)); // Copia los bytes de tu MAC hacia el campo "MAC de origen" del paquete ARP para que sepan quién pregunta.
   memcpy(peticion.dst_ip, dest, sizeof(ipv4_addr_t));   // Copia la IP objetivo (la variable 'dest' que recibimos como parámetro) hacia el campo dst_ip del mensaje ARP para indicar por quién preguntamos.

   // Como todavía no se dispone de una dirección IPv4 configurada, la dirección de red origen del mensaje ARP debe ser 0.0.0.0, y con MAC pasa lo mismo.
   // memset(destino, valor, tamaño)-> Rellena un bloque de memoria contiguo de un tamaño determinado escribiendo un mismo valor en todos sus bytes.
   memset(peticion.src_ip, 0, sizeof(ipv4_addr_t)); // Inicializa los 4 bytes del campo src_ip con ceros, configurando la dirección 0.0.0.0. 
   memset(peticion.dst_mac, 0, sizeof(mac_addr_t)); // Inicializa los 6 bytes del campo dst_mac con ceros (00:00:00:00:00:00) porque la dirección física del destinatario es el dato que estamos buscando.
   
   // PASO 3: Transmisión. Difunde la petición ARP a toda la red local usando la MAC universal.
   mac_addr_t mac_broadcast = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Define la dirección MAC universal de difusión (Broadcast, todo a FF) para poder enviar la petición a todos los equipos de la red a la vez.
    
   // eth_send()-> Empaqueta un bloque de datos en crudo dentro de una trama Ethernet estándar y lo transmite físicamente por la interfaz.
   eth_send(iface, mac_broadcast, 0x0806, (unsigned char *)&peticion, sizeof(struct arp_header)); // Enviamos usando eth_send: interfaz, MAC destino, protocolo ARP (0x0806), datos, y longitud.

   // PASO 4: Iniciar un temporizador de 2 segundos y entrar en un bucle de escucha.
   // timerms_reset(puntero, tiempo)-> Configura y arranca un cronómetro pasándole la dirección de memoria de la variable temporal y el tiempo límite en milisegundos.
   timerms_t timer;
   timerms_reset(&timer, 2000); // Crea un temporizador y lo programa a 2 segundos (2000 ms) para evitar que el programa se quede esperando infinitamente si el paquete ARP se pierde en la red.
    
   mac_addr_t mac_remota;       // Crea un recipiente vacío para guardar la dirección MAC física del equipo que nos acaba de enviar el paquete.
   struct arp_header respuesta; // Prepara un "molde" con la estructura de un mensaje ARP para organizar y leer correctamente los datos que lleguen.
   int bytes_leidos = 0;        // Inicializa un contador a cero para registrar cuántos bytes se han recibido y saber si la lectura ha capturado algo (si devuelve un número negativo es que han pasado los 2 segundos).

   // Mientras quede tiempo en el cronómetro...
   // timerms_left(puntero)-> Consulta el reloj interno y devuelve la cantidad exacta de milisegundos que le restan al temporizador antes de agotarse.
   // Aunque la MAC llega repetida en dos partes distintas del mensaje, creamos 'mac_remota' exclusivamente porque eth_recv() nos obliga a darle un recipiente para guardar el "sobre" exterior.
   // Luego no usaremos esta variable para nada; el dato que realmente nos interesa se guarda por separado y de forma automática dentro de 'respuesta' al encajar con los parámetros del arp_header.
   while (timerms_left(&timer) > 0) {
       
      // PASO 5: Recepción. Escuchar la red esperando capturar un paquete antes de que el reloj llegue a cero.
      // eth_recv(interfaz, parte de fuera del sobre, protocolo, destino, tamaño, tiempo)-> Escucha la red, guarda el remitente exterior y el mensaje en las variables, y devuelve únicamente un número: los bytes capturados (o -1 si se agota el tiempo).
      bytes_leidos = eth_recv(iface, mac_remota, 0x0806, (unsigned char *)&respuesta, sizeof(struct arp_header), timerms_left(&timer)); // Escucha el puerto esperando un paquete ARP (0x0806) como máximo el tiempo que le quede al reloj, guardando los datos en 'respuesta' y la MAC de origen en 'mac_remota'.
       
      // PASO 6: Validación. Confirmar que la captura fue exitosa, que el paquete es una respuesta y que proviene de la IP correcta.
      // Si hemos recibido una trama ARP (leído > 0)
      // memcmp(A, B, tamaño)-> compara la memoria de A con B. Si son idénticas, devuelve 0.
      // ntohs()-> actúa como un traductor automático: reordena los bytes que llegan por la red para que nuestro procesador no los lea al revés y pueda entender correctamente el mensaje del otro ordenador.
      if (bytes_leidos > 0) {  
         if (ntohs(respuesta.opcode) == 2 && memcmp(respuesta.src_ip, dest, sizeof(ipv4_addr_t)) == 0) { // Verifica si el mensaje recibido es una respuesta (Opcode igual a 2) y si la IP de quien lo envía coincide exactamente con la IP del destinatario que estábamos buscando.
            // PASO 7: Extracción y Éxito. Clonar el dato útil hacia la variable de salida y frenar la ejecución confirmando que todo salió bien.
            memcpy(mac, respuesta.src_mac, sizeof(mac_addr_t)); // Copia la información real de la dirección descubierta desde el paquete recibido (respuesta.src_mac) hacia la variable de salida (mac) para entregársela al programa principal.
            return 0; 
         }
      }
   }
   // PASO 8: Fracaso. Si el reloj se agota, el bucle termina y el programa confiesa al sistema principal que no pudo encontrar la MAC.
   return -1;
}