int ipv4_send (ipv4_layer_t * layer, ipv4_addr_t dst, uint8_t protocol, unsigned char * payload, int payload_len) {
/* 1. Rellenar datagrama IPv4 (cabecera + datos) */
/* 2. Obtener siguiente salto con ipv4_route_table_lookup () */
/* 3. Averiguar dirección MAC sig. salto con arp_resolve() */
/* ¡OJO! Gestionar caso destino directamente conectado */
/* 4. Enviar datagrama con eth_send() */
/* 5. Devolver longitud datos (payload) enviados */
}