
#ifndef _DHCPSERVER_H
#define _DHCPSERVER_H

#ifndef DHCPSERVER_LEASE_TIME
#define DHCPSERVER_LEASE_TIME (864000)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Start DHCP server.

   Static IP of server should already be set and network interface enabled.

   first_client_addr is the IP address of the first lease to be handed
   to a client.  Subsequent lease addresses are calculated by
   incrementing the final octet of the IPv4 address, up to max_leases.
*/
void dhcpserver_start(const uint8_t ucIPAddress[4], uint32_t first_client_addr, uint8_t max_leases);


/* Stop DHCP server.
 */
void dhcpserver_stop(void);

/* Set a router address to send as an option. */
void dhcpserver_set_router(uint32_t router);

/* Set a DNS address to send as an option. */
void dhcpserver_set_dns(uint32_t dns);

#ifdef __cplusplus
}
#endif

#endif
