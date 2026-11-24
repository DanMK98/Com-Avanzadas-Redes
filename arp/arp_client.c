#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "../eth/eth.h"
#include "../ipv4/ipv4.h"
#include "../arp/arp.h"

int main(int argc, char *argv[])
{

   if (argc != 3)
   {
      fprintf(stderr, "Error: Debes escribir al menos 3 elementos para ejecutar el programa (ejemplo: %s eth0 192.168.1.20).\n", argv[0]);
      exit(EXIT_FAILURE);
   }
   char *iface_name = argv[1];
   char *dest_ip_str = argv[2];

   ipv4_addr_t dest_ip;

   if (inet_pton(AF_INET, dest_ip_str, dest_ip) != 1)
   {
      fprintf(stderr, "Error: invalid target IPv4 address.\n");
      return EXIT_FAILURE;
   }

   eth_iface_t *iface = eth_open(iface_name);
   if (iface == NULL)
   {
      fprintf(stderr, "Error: No se pudo abrir la interfaz %s\n", iface_name);
      exit(EXIT_FAILURE);
   }

   mac_addr_t mac_resuelta;

   int resultado = arp_resolve(iface, dest_ip, mac_resuelta);

   if (resultado == 0)
   {

      printf("%s -> %02X:%02X:%02X:%02X:%02X:%02X\n",
             dest_ip_str,
             mac_resuelta[0], mac_resuelta[1], mac_resuelta[2],
             mac_resuelta[3], mac_resuelta[4], mac_resuelta[5]);
   }
   else
   {

      fprintf(stderr, "Error: No se recibió respuesta ARP para la IP %s tras 2 segundos.\n", dest_ip_str);
   }

   eth_close(iface);
   return resultado == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}