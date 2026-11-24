#include "../arp/arp.h"
#include "timerms.h"
#include <arpa/inet.h>
#include <string.h>

int arp_resolve(eth_iface_t *iface, ipv4_addr_t dest, mac_addr_t mac)
{
    ipv4_addr_t src_ip = {0};
    return arp_resolve_from(iface, src_ip, dest, mac);
}

int arp_resolve_from(eth_iface_t *iface, ipv4_addr_t src_ip, ipv4_addr_t dest, mac_addr_t mac)
{

   struct arp_header peticion;

   peticion.hw_type = htons(1);
   peticion.proto_type = htons(0x0800);
   peticion.hw_len = 6;
   peticion.proto_len = 4;
   peticion.opcode = htons(1);

   mac_addr_t mi_mac;
   eth_getaddr(iface, mi_mac);

   memcpy(peticion.src_mac, mi_mac, sizeof(mac_addr_t));
   memcpy(peticion.dst_ip, dest, sizeof(ipv4_addr_t));

   memcpy(peticion.src_ip, src_ip, sizeof(peticion.src_ip));
   memset(peticion.dst_mac, 0, sizeof(mac_addr_t));

   mac_addr_t mac_broadcast = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

   eth_send(iface, mac_broadcast, 0x0806, (unsigned char *)&peticion, sizeof(struct arp_header));

   timerms_t timer;
   timerms_reset(&timer, 2000);

   mac_addr_t mac_remota;
   struct arp_header respuesta;
   int bytes_leidos = 0;

   while (timerms_left(&timer) > 0)
   {

      bytes_leidos = eth_recv(iface, mac_remota, 0x0806, (unsigned char *)&respuesta, sizeof(struct arp_header), timerms_left(&timer));

      if (bytes_leidos >= (int)sizeof(respuesta))
      {
         if (ntohs(respuesta.opcode) == 2 && memcmp(respuesta.src_ip, dest, sizeof(ipv4_addr_t)) == 0)
         {

            memcpy(mac, respuesta.src_mac, sizeof(mac_addr_t));
            return 0;
         }
      }
   }

   return -1;
}