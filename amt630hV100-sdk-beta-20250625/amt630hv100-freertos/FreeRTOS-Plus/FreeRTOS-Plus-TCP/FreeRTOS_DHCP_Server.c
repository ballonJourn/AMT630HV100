/*
*   porting from lwip
*/
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_DHCP_Server.h"
#include "NetworkBufferManagement.h"

#if (DHCP_DEBUG == LWIP_DBG_ON)
#define debug(s, ...) printf("%s: " s "\n", "DHCP", ## __VA_ARGS__)
#else
#define debug(s, ...)
#endif

/* Grow the size of the lwip dhcp_msg struct's options field, as LWIP
   defaults to a 68 octet options field for its DHCP client, and most
   full-sized clients send us more than this. */
#define DHCP_OPTIONS_LEN 312
#define NETIF_MAX_HWADDR_LEN 6U
#define DHCP_FILE_LEN     128U
#define DHCP_SNAME_LEN    64U
#define DHCP_CHADDR_LEN   16U
#define LWIP_IANA_HWTYPE_ETHERNET 1

/* DHCP op codes */
#define DHCP_BOOTREQUEST            1
#define DHCP_BOOTREPLY              2

/* DHCP message types */
#define DHCP_DISCOVER               1
#define DHCP_OFFER                  2
#define DHCP_REQUEST                3
#define DHCP_DECLINE                4
#define DHCP_ACK                    5
#define DHCP_NAK                    6
#define DHCP_RELEASE                7
#define DHCP_INFORM                 8
#define DHCP_MAGIC_COOKIE           0x63825363UL

/* This is a list of options for BOOTP and DHCP, see RFC 2132 for descriptions */

/* BootP options */
#define DHCP_OPTION_PAD             0
#define DHCP_OPTION_SUBNET_MASK     1 /* RFC 2132 3.3 */
#define DHCP_OPTION_ROUTER          3
#define DHCP_OPTION_DNS_SERVER      6
#define DHCP_OPTION_HOSTNAME        12
#define DHCP_OPTION_IP_TTL          23
#define DHCP_OPTION_MTU             26
#define DHCP_OPTION_BROADCAST       28
#define DHCP_OPTION_TCP_TTL         37
#define DHCP_OPTION_NTP             42
#define DHCP_OPTION_END             255

/* DHCP options */
#define DHCP_OPTION_REQUESTED_IP    50 /* RFC 2132 9.1, requested IP address */
#define DHCP_OPTION_LEASE_TIME      51 /* RFC 2132 9.2, time in seconds, in 4 bytes */
#define DHCP_OPTION_OVERLOAD        52 /* RFC2132 9.3, use file and/or sname field for options */
#define DHCP_OPTION_MESSAGE_TYPE    53 /* RFC 2132 9.6, important for DHCP */
#define DHCP_OPTION_MESSAGE_TYPE_LEN 1
#define DHCP_OPTION_SERVER_ID       54 /* RFC 2132 9.7, server IP address */
#define DHCP_OPTION_PARAMETER_REQUEST_LIST  55 /* RFC 2132 9.8, requested option types */
#define DHCP_OPTION_MAX_MSG_SIZE    57 /* RFC 2132 9.10, message size accepted >= 576 */
#define DHCP_OPTION_MAX_MSG_SIZE_LEN 2
#define DHCP_OPTION_T1              58 /* T1 renewal time */
#define DHCP_OPTION_T2              59 /* T2 rebinding time */
#define DHCP_OPTION_US              60
#define DHCP_OPTION_CLIENT_ID       61
#define DHCP_OPTION_TFTP_SERVERNAME 66
#define DHCP_OPTION_BOOTFILE        67
/* possible combinations of overloading the file and sname fields with options */
#define DHCP_OVERLOAD_NONE          0
#define DHCP_OVERLOAD_FILE          1
#define DHCP_OVERLOAD_SNAME         2
#define DHCP_OVERLOAD_SNAME_FILE    3

#define IPADDR_ANY          ((uint32_t)0x00000000UL)
#define ip4_addr_get_byte(ipaddr, idx) (((const uint8_t*)(&(ipaddr)->addr))[idx])
#define ip4_addr1(ipaddr) ip4_addr_get_byte(ipaddr, 0)
#define ip4_addr2(ipaddr) ip4_addr_get_byte(ipaddr, 1)
#define ip4_addr3(ipaddr) ip4_addr_get_byte(ipaddr, 2)
#define ip4_addr4(ipaddr) ip4_addr_get_byte(ipaddr, 3)
#define ip4_addr_copy(dest, src) ((dest).addr = (src).addr)
#define ip4_addr_set_zero(ipaddr)     ((ipaddr)->addr = 0)
#define ip4_addr_isany_val(addr1)   ((addr1).addr == IPADDR_ANY)
#define ip4_addr_cmp(addr1, addr2) ((addr1)->addr == (addr2)->addr)
#define ip_addr_cmp(addr1, addr2)               ip4_addr_cmp(addr1, addr2)

#define swap16(A) ((((uint16_t)(A) & 0xff00) >> 8) | (((uint16_t)(A) & 0x00ff) << 8))
#define swap32(A) ((((uint32_t)(A) & 0xff000000) >> 24) | \
				   (((uint32_t)(A) & 0x00ff0000) >>  8) | \
				   (((uint32_t)(A) & 0x0000ff00) <<  8) | \
				   (((uint32_t)(A) & 0x000000ff) << 24))
#define IPADDR_BROADCAST    ((uint32_t)0xffffffffUL)

static union {   
    char c[4];   
    unsigned long l;   
} endian_test = {{ 'l', '?', '?', 'b' } };  
 
#define ENDIANNESS ((char)endian_test.l)  

uint16_t htons(uint16_t hs)
{
	return (ENDIANNESS=='l') ? swap16(hs): hs;
}

uint32_t htonl(uint32_t hl)
{
	return (ENDIANNESS=='l') ? swap32(hl): hl;
}

uint16_t ntohs(uint16_t ns)
{
	return (ENDIANNESS=='l') ? swap16(ns): ns;	
}

uint32_t ntohl(uint32_t nl)
{
	return (ENDIANNESS=='l') ? swap32(nl): nl;	
}

#include "pack_struct_start.h"
struct ip4_addr {
	uint32_t addr;
};
#include "pack_struct_end.h"

typedef struct ip4_addr ip4_addr_t;
typedef ip4_addr_t ip_addr_t;


#include "pack_struct_start.h"
struct dhcp_msg
{
	uint8_t op;
	uint8_t htype;
	uint8_t hlen;
	uint8_t hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	ip4_addr_t ciaddr;
	ip4_addr_t yiaddr;
	ip4_addr_t siaddr;
	ip4_addr_t giaddr;
	uint8_t chaddr[DHCP_CHADDR_LEN];
	uint8_t sname[DHCP_SNAME_LEN];
	uint8_t file[DHCP_FILE_LEN];
	uint32_t cookie;
	uint8_t options[DHCP_OPTIONS_LEN];
};
#include "pack_struct_end.h"


typedef struct {
    uint8_t hwaddr[NETIF_MAX_HWADDR_LEN];
    uint8_t active;
    uint32_t expires;
} dhcp_lease_t;

typedef struct {
    //struct netconn *nc;
    Socket_t  server_handle;
    uint8_t max_leases;
    ip4_addr_t first_client_addr;
    //struct netif *server_if;
    dhcp_lease_t *leases; /* length max_leases */
    /* Optional router */
    ip4_addr_t router;
    /* Optional DNS server */
    ip4_addr_t dns;
} server_state_t;

struct dhcp_client_ctx
{
	int32_t tid;
	ListItem_t item;
	TimerHandle_t timer;
	server_state_t* state;
	struct dhcp_msg msg;
};

static TaskHandle_t dhcpserver_task_handle = NULL;
static server_state_t *gState;
static List_t  gdhcp_client_list;
static uint32_t  gDefaultIp = 0;

uint32_t FreeRTOS_GetIPAddressSafe( void )
{
	uint32_t ip = FreeRTOS_GetIPAddress();
	if (0 == ip) {
		return gDefaultIp;
	}
	return ip;
}


/* Handlers for various kinds of incoming DHCP messages */
static void handle_dhcp_discover(server_state_t* state, struct freertos_sockaddr *xaddr, struct dhcp_msg *received);
static void handle_dhcp_request(server_state_t* state, struct freertos_sockaddr *xaddr, struct dhcp_msg *dhcpmsg);
static void handle_dhcp_release(server_state_t* state, struct dhcp_msg *dhcpmsg);

static void send_dhcp_nak(server_state_t* state, struct freertos_sockaddr *xaddr, struct dhcp_msg *dhcpmsg);

static void dhcpserver_task(void *pxParameter);

/* Utility functions */
static uint8_t *find_dhcp_option(struct dhcp_msg *msg, uint8_t option_num, uint8_t min_length, uint8_t *length);
static uint8_t *add_dhcp_option_byte(uint8_t *opt, uint8_t type, uint8_t value);
static uint8_t *add_dhcp_option_bytes(uint8_t *opt, uint8_t type, void *value, uint8_t len);
static dhcp_lease_t *find_lease_slot(server_state_t* state, uint8_t *hwaddr);

/* Copy IP address as dotted decimal to 'dest', must be at least 16 bytes long */
inline static void sprintf_ipaddr(const ip4_addr_t *addr, char *dest)
{
    if (addr == NULL)
        sprintf(dest, "NULL");
    else
        sprintf(dest, "%d.%d.%d.%d", ip4_addr1(addr),
                ip4_addr2(addr), ip4_addr3(addr), ip4_addr4(addr));
}

inline static void sprintf_ipaddr1(uint32_t addr, uint8_t *dest)
{
    ip4_addr_t tmp  = {0};
	tmp.addr = addr;
	sprintf_ipaddr(&tmp, (char*)dest);
}


void dhcpserver_start(const uint8_t ucIPAddress[4], uint32_t first_client_addr, uint8_t max_leases)
{
    /* Stop any existing running dhcpserver */
    if (dhcpserver_task_handle) {
        return;
    }
	gDefaultIp = (ucIPAddress[0]) | (ucIPAddress[1] << 8) | (ucIPAddress[2] << 16) | (ucIPAddress[3] << 24);
	printf("dhcpserver_start-->gDefaultIp:%x\r\n", gDefaultIp);
	vListInitialise(&gdhcp_client_list);
    gState = (server_state_t*)pvPortMalloc(sizeof(server_state_t));
    memset(gState, 0, sizeof(*gState));
    gState->max_leases = max_leases;
    gState->leases = (dhcp_lease_t *)pvPortMalloc(max_leases * sizeof(dhcp_lease_t));
    memset((void*)gState->leases, 0, max_leases * sizeof(dhcp_lease_t));
    // state->server_if is assigned once the task is running - see comment in dhcpserver_task()
    //ip4_addr_copy(gState->first_client_addr, *first_client_addr);
	gState->first_client_addr.addr = first_client_addr;

    /* Clear options */
    ip4_addr_set_zero(&gState->router);
    ip4_addr_set_zero(&gState->dns);

    xTaskCreate(dhcpserver_task, "DHCP Server", configMINIMAL_STACK_SIZE * 4, (void*)gState, 5, &dhcpserver_task_handle);
}

void dhcpserver_stop(void)
{
    if (dhcpserver_task_handle) {
        vTaskDelete(dhcpserver_task_handle);
        dhcpserver_task_handle = NULL;
        vPortFree((void*)gState->leases);
        vPortFree((void*)gState);
        gState = NULL;
    }
}

void dhcpserver_set_router(uint32_t router)
{
    //ip4_addr_copy(gState->router, router);
    gState->router.addr = router;
}

void dhcpserver_set_dns(uint32_t dns)
{
    //ip4_addr_copy(gState->dns, dns);
	gState->dns.addr = dns;
}

static void dhcpserver_task(void *pxParameter)
{
	
	SocketSet_t xFD_Set;
	BaseType_t xResult;
	Socket_t xSockets;
	struct freertos_sockaddr xAddress;
	uint32_t xClientLength = sizeof( struct freertos_sockaddr );
	int32_t lBytes;
	const TickType_t xRxBlockTime = 0;
	uint8_t rcv_buf[4096] = {0};
	server_state_t* state = (server_state_t*)pxParameter;
	uint8_t ipAddrStr[32] = {0};

	xFD_Set = FreeRTOS_CreateSocketSet();
	xSockets = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
    xAddress.sin_port = FreeRTOS_htons( 67 );
    FreeRTOS_bind(xSockets, &xAddress, sizeof( struct freertos_sockaddr ));
    FreeRTOS_setsockopt(xSockets, 0, FREERTOS_SO_RCVTIMEO, &xRxBlockTime, sizeof( xRxBlockTime ));

	state->server_handle = xSockets;
    while(1) {
		FreeRTOS_FD_CLR(xSockets, xFD_Set, eSELECT_READ);
		FreeRTOS_FD_SET( xSockets, xFD_Set, eSELECT_READ );
    	xResult = FreeRTOS_select( xFD_Set, portMAX_DELAY );
        if (xResult == 0) {
			continue;
        }
        struct dhcp_msg received = { 0 };

        /* Receive a DHCP packet */
		if( !FreeRTOS_FD_ISSET ( xSockets, xFD_Set ) ) {
              
        }
		/* Read from the socket. */
		lBytes = FreeRTOS_recvfrom( xSockets, (void*)rcv_buf, sizeof(rcv_buf), 0, &xAddress,
                                &xClientLength );
		if (lBytes <= 0)
			continue;
		if (lBytes < (sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN))
			continue;

        /* expire any leases that have passed */
        uint32_t now = xTaskGetTickCount();
        for (int i = 0; i < state->max_leases; i++) {
            if (state->leases[i].active) {
                uint32_t expires = state->leases[i].expires - now;
                if (expires >= 0x80000000) {
                    state->leases[i].active = 0;
                }
            }
        }
		memcpy((void*)&received, (void*)rcv_buf, lBytes);

        uint8_t *message_type = find_dhcp_option(&received, DHCP_OPTION_MESSAGE_TYPE,
                                                 DHCP_OPTION_MESSAGE_TYPE_LEN, NULL);
        if(!message_type) {
            debug("DHCP Server Error: No message type field found");
            continue;
        }

#if (DHCP_DEBUG == LWIP_DBG_ON)
		sprintf_ipaddr1(xAddress.sin_addr, ipAddrStr);
        debug("State dump. Message type %d from %s port:%d\r\n", *message_type, ipAddrStr, xAddress.sin_port);
        for(int i = 0; i < state->max_leases; i++) {
#if 0
            dhcp_lease_t *lease = &state->leases[i];
            debug("lease slot %d expiry %d hwaddr %02x:%02x:%02x:%02x:%02x:%02x \r\n", i, lease->expires, lease->hwaddr[0],
                   lease->hwaddr[1], lease->hwaddr[2], lease->hwaddr[3], lease->hwaddr[4],
                   lease->hwaddr[5]);
#endif
        }
#endif

		//xAddress.sin_addr = 0xffffffffUL;
    	//xAddress.sin_port = ( uint16_t ) 68;
        switch(*message_type) {
        case DHCP_DISCOVER:
			printf("DHCP_DISCOVER\r\n");
            handle_dhcp_discover(state, &xAddress, &received);
            break;
        case DHCP_REQUEST:
			printf("DHCP_REQUEST\r\n");
            handle_dhcp_request(state, &xAddress, &received);
            break;
        case DHCP_RELEASE:
			printf("DHCP_RELEASE\r\n");
            handle_dhcp_release(state, &received);
            break;
        default:
            debug("DHCP Server Error: Unsupported message type %d\r\n", *message_type);
            break;
        }
    }
}

/*static void send_dhcp_msg(server_state_t* state, struct freertos_sockaddr *xaddr, struct dhcp_msg *dhcpmsg)
{
	NetworkBufferDescriptor_t * pxNetworkBuffer;
	uint8_t * pucUDPPayloadBuffer = NULL;
	uint32_t xClientLength = sizeof( struct freertos_sockaddr );

	pxNetworkBuffer = pxGetNetworkBufferWithDescriptor( sizeof( UDPPacket_t ) + sizeof(struct dhcp_msg), 0U );

    if( pxNetworkBuffer == NULL ) {
		debug("DHCP Server: malloc net buf desc err.\r\n");
		return;
    }
	pucUDPPayloadBuffer = &( pxNetworkBuffer->pucEthernetBuffer[ ipUDP_PAYLOAD_OFFSET_IPv4 ] );
	memcpy(pucUDPPayloadBuffer, (void*)dhcpmsg, sizeof(struct dhcp_msg));

	FreeRTOS_sendto(state->server_handle, (void*)pucUDPPayloadBuffer, sizeof(struct dhcp_msg), FREERTOS_ZERO_COPY, xaddr, xClientLength);
}*/
#define dhcpdTIMER_SCAN_FREQUENCY_MS			pdMS_TO_TICKS( 300UL )

static void handle_dhcp_discover(server_state_t* state, struct freertos_sockaddr *xaddr, struct dhcp_msg *dhcpmsg)
{
	uint32_t xClientLength = sizeof( struct freertos_sockaddr );
	int32_t ret = -1;
	struct dhcp_client_ctx *client = NULL;
    if (dhcpmsg->htype != LWIP_IANA_HWTYPE_ETHERNET)
        return;
    if (dhcpmsg->hlen > NETIF_MAX_HWADDR_LEN)
        return;

    dhcp_lease_t *freelease = find_lease_slot(state, dhcpmsg->chaddr);
    if(!freelease) {
        debug("DHCP Server: All leases taken.\r\n");
        return; /* Nothing available, so do nothing */
    }

    /* Reuse the DISCOVER buffer for the OFFER response */
    dhcpmsg->op = DHCP_BOOTREPLY;
	dhcpmsg->htype = LWIP_IANA_HWTYPE_ETHERNET;
    //bzero(dhcpmsg->options, DHCP_OPTIONS_LEN);
	memset((void *)dhcpmsg->options, 0, sizeof(dhcpmsg->options));

    dhcpmsg->yiaddr.addr = FreeRTOS_htonl(FreeRTOS_ntohl(state->first_client_addr.addr) + (freelease - state->leases));
    //dhcpmsg->yiaddr.addr = ((state->first_client_addr.addr) + (freelease - state->leases));

    uint8_t *opt = (uint8_t *)&dhcpmsg->options;
    opt = add_dhcp_option_byte(opt, DHCP_OPTION_MESSAGE_TYPE, DHCP_OFFER);
    uint32_t ser_addr = FreeRTOS_GetIPAddressSafe();
    opt = add_dhcp_option_bytes(opt, DHCP_OPTION_SERVER_ID, &ser_addr, 4);
	uint32_t ser_netmask = FreeRTOS_GetNetmask();
    opt = add_dhcp_option_bytes(opt, DHCP_OPTION_SUBNET_MASK, &ser_netmask, 4);//
    uint32_t ser_broadcast = FreeRTOS_GetIPAddressSafe();
    uint8_t* tmp_ptr_addr = (uint8_t*)&ser_broadcast;
	*(tmp_ptr_addr + 3) = 0xff;
	opt = add_dhcp_option_bytes(opt, DHCP_OPTION_BROADCAST, &ser_broadcast, 4);
	if (1) {
		uint8_t addr_str[32] = {0};
		sprintf_ipaddr1(ser_addr, addr_str);
		debug("server ip:%s   \r\n", addr_str);
		sprintf_ipaddr1(xaddr->sin_addr, addr_str);
		debug("##client ip:%s   port:%d\r\n", addr_str, FreeRTOS_htons(xaddr->sin_port));
		memset(addr_str, 0, sizeof(addr_str));
		sprintf_ipaddr1(ser_netmask, addr_str);
		debug("server netmask:%s  \r\n", addr_str);
		printf("send len:%d\r\n", sizeof(struct dhcp_msg));
	}
    //if (!ip4_addr_isany_val(state->router)) {
    	uint8_t addr_str11[32] = {0};
    	state->router.addr = FreeRTOS_GetGatewayAddress();
		sprintf_ipaddr1(state->router.addr, addr_str11);
		debug("router ip:%s   \r\n", addr_str11);
		
        opt = add_dhcp_option_bytes(opt, DHCP_OPTION_ROUTER, &state->router, 4);
    //}
    //if (!ip4_addr_isany_val(state->dns)) {
    	state->dns.addr = FreeRTOS_GetGatewayAddress();
		sprintf_ipaddr1(state->dns.addr, addr_str11);
		debug("dns ip:%s   \r\n", addr_str11);
        opt = add_dhcp_option_bytes(opt, DHCP_OPTION_DNS_SERVER, &state->dns, 4);
    //}

    uint32_t expiry = htonl(DHCPSERVER_LEASE_TIME);
    opt = add_dhcp_option_bytes(opt, DHCP_OPTION_LEASE_TIME, &expiry, 4);

    opt = add_dhcp_option_bytes(opt, DHCP_OPTION_END, NULL, 0);

	if (0) {
		int i;
		uint8_t *tmpp = (uint8_t *)dhcpmsg;
		for (i = 0; i < sizeof(struct dhcp_msg); i++) {
			printf("%02x ", tmpp[i]);
		}printf("\r\n");
	}
	//xaddr->sin_addr = FreeRTOS_htonl(dhcpmsg->yiaddr.addr);
	xaddr->sin_addr = 0xffffffff;//
	xaddr->sin_port = ( uint16_t ) FreeRTOS_htons(68);
	if (0 && NULL != client) {
		memcpy((void *)&client->msg, (void *)dhcpmsg, sizeof(struct dhcp_msg));
		client->state = state;
		xTimerStart(client->timer, 0);
	}
	
	ret = FreeRTOS_sendto(state->server_handle, (void*)dhcpmsg, sizeof(struct dhcp_msg), 0, xaddr, xClientLength);
	printf("len out:%d ret:%d\r\n", sizeof(struct dhcp_msg), ret);
	if (ret != sizeof(struct dhcp_msg)) {
		printf("udhcpd ret:%d\r\n", ret);
	}
	//send_dhcp_msg(state, xaddr, dhcpmsg);
}

static void handle_dhcp_request(server_state_t* state, struct freertos_sockaddr *xaddr, struct dhcp_msg *dhcpmsg)
{
	uint32_t xClientLength = sizeof( struct freertos_sockaddr );
    static char ipbuf[16];
    if (dhcpmsg->htype != LWIP_IANA_HWTYPE_ETHERNET) {
		debug("DHCP Server Error: htype err.\r\n");
        return;
    }
    if (dhcpmsg->hlen > NETIF_MAX_HWADDR_LEN) {
		debug("DHCP Server Error: hlen err.\r\n");
        return;
    }

    ip4_addr_t requested_ip;
	ip4_addr_t any_ip = {0};
	any_ip.addr = IPADDR_ANY;
    uint8_t *requested_ip_opt = find_dhcp_option(dhcpmsg, DHCP_OPTION_REQUESTED_IP, 4, NULL);
    if (requested_ip_opt) {
        memcpy(&requested_ip.addr, requested_ip_opt, 4);
    } else if (ip4_addr_cmp(&requested_ip, &any_ip)) {
        ip4_addr_copy(requested_ip, dhcpmsg->ciaddr);
    } else {
        debug("DHCP Server Error: No requested IP");
        send_dhcp_nak(state, xaddr, dhcpmsg);
        return;
    }

    /* Test the first 4 octets match */
    if (ip4_addr1(&requested_ip) != ip4_addr1(&state->first_client_addr)
       || ip4_addr2(&requested_ip) != ip4_addr2(&state->first_client_addr)
       || ip4_addr3(&requested_ip) != ip4_addr3(&state->first_client_addr)) {
        sprintf_ipaddr(&requested_ip, ipbuf);
        debug("DHCP Server Error: %s not an allowed IP\r\n", ipbuf);
        send_dhcp_nak(state, xaddr, dhcpmsg);
        return;
    }
    /* Test the last octet is in the MAXCLIENTS range */
    int16_t octet_offs = ip4_addr4(&requested_ip) - ip4_addr4(&state->first_client_addr);
    if(octet_offs < 0 || octet_offs >= state->max_leases) {
        debug("DHCP Server Error: Address out of range\r\n");
        send_dhcp_nak(state, xaddr, dhcpmsg);
        return;
    }

    dhcp_lease_t *requested_lease = state->leases + octet_offs;
    if (requested_lease->active && memcmp(requested_lease->hwaddr, dhcpmsg->chaddr,dhcpmsg->hlen))
    {
        debug("DHCP Server Error: Lease for address already taken\r\n");
        send_dhcp_nak(state, xaddr, dhcpmsg);
        return;
    }

    memcpy(requested_lease->hwaddr, dhcpmsg->chaddr, dhcpmsg->hlen);
    sprintf_ipaddr(&requested_ip, ipbuf);
    debug("DHCP lease addr %s assigned to MAC %02x:%02x:%02x:%02x:%02x:%02x\r\n", ipbuf, requested_lease->hwaddr[0],
           requested_lease->hwaddr[1], requested_lease->hwaddr[2], requested_lease->hwaddr[3], requested_lease->hwaddr[4],
           requested_lease->hwaddr[5]);
    uint32_t now = xTaskGetTickCount();
    requested_lease->expires = now + DHCPSERVER_LEASE_TIME * configTICK_RATE_HZ;
    requested_lease->active = 1;

    //sdk_wifi_softap_set_station_info(requested_lease->hwaddr, &requested_ip);

    /* Reuse the REQUEST message as the ACK message */
    dhcpmsg->op = DHCP_BOOTREPLY;
    memset((void *)dhcpmsg->options, 0, sizeof(dhcpmsg->options));

    ip4_addr_copy(dhcpmsg->yiaddr, requested_ip);

    uint8_t *opt = (uint8_t *)&dhcpmsg->options;
    opt = add_dhcp_option_byte(opt, DHCP_OPTION_MESSAGE_TYPE, DHCP_ACK);
    uint32_t expiry = htonl(DHCPSERVER_LEASE_TIME);
    opt = add_dhcp_option_bytes(opt, DHCP_OPTION_LEASE_TIME, &expiry, 4);
	uint32_t ser_addr = FreeRTOS_GetIPAddressSafe();
    opt = add_dhcp_option_bytes(opt, DHCP_OPTION_SERVER_ID, &ser_addr, 4);
	uint32_t ser_netmask = FreeRTOS_GetNetmask();
    opt = add_dhcp_option_bytes(opt, DHCP_OPTION_SUBNET_MASK, &ser_netmask, 4);
    //if (!ip4_addr_isany_val(state->router)) {
    	state->router.addr = FreeRTOS_GetGatewayAddress();
        opt = add_dhcp_option_bytes(opt, DHCP_OPTION_ROUTER, &state->router, 4);
    //}
    //if (!ip4_addr_isany_val(state->dns)) {
    	state->dns.addr = FreeRTOS_GetGatewayAddress();
        opt = add_dhcp_option_bytes(opt, DHCP_OPTION_DNS_SERVER, &state->dns, 4);
    //}

    opt = add_dhcp_option_bytes(opt, DHCP_OPTION_END, NULL, 0);
	xaddr->sin_addr = 0xffffffff;
	xaddr->sin_port = ( uint16_t ) FreeRTOS_htons(68);
	FreeRTOS_sendto(state->server_handle, (void*)dhcpmsg, sizeof(struct dhcp_msg), 0, xaddr, xClientLength);
	//send_dhcp_msg(state, xaddr, dhcpmsg);
}

static void handle_dhcp_release(server_state_t* state, struct dhcp_msg *dhcpmsg)
{
    dhcp_lease_t *lease = find_lease_slot(state, dhcpmsg->chaddr);
    if (lease) {
        lease->active = 0;
        lease->expires = 0;
    }
}

static void send_dhcp_nak(server_state_t* state, struct freertos_sockaddr *xaddr, struct dhcp_msg *dhcpmsg)
{
	uint32_t xClientLength = sizeof( struct freertos_sockaddr );

    /* Reuse 'dhcpmsg' for the NAK */
    dhcpmsg->op = DHCP_BOOTREPLY;
    memset((void *)dhcpmsg->options, 0, sizeof(dhcpmsg->options));

    uint8_t *opt = (uint8_t *)&dhcpmsg->options;
    opt = add_dhcp_option_byte(opt, DHCP_OPTION_MESSAGE_TYPE, DHCP_NAK);
    uint32_t ser_addr = FreeRTOS_GetIPAddressSafe();
    opt = add_dhcp_option_bytes(opt, DHCP_OPTION_SERVER_ID, &ser_addr, 4);
    opt = add_dhcp_option_bytes(opt, DHCP_OPTION_END, NULL, 0);
	xaddr->sin_addr = 0xffffffff;
	xaddr->sin_port = ( uint16_t ) FreeRTOS_htons(68);
	FreeRTOS_sendto(state->server_handle, (void*)dhcpmsg, sizeof(struct dhcp_msg), 0, xaddr, xClientLength);
	//send_dhcp_msg(state, xaddr, dhcpmsg);
}

static uint8_t *find_dhcp_option(struct dhcp_msg *msg, uint8_t option_num, uint8_t min_length, uint8_t *length)
{
    uint8_t *start = (uint8_t *)&msg->options;
    uint8_t *msg_end = (uint8_t *)msg + sizeof(struct dhcp_msg);

    for (uint8_t *p = start; p < msg_end-2;) {
        uint8_t type = *p++;
        uint8_t len = *p++;
        if (type == DHCP_OPTION_END)
            return NULL;
        if (p+len >= msg_end)
            break; /* We've overrun our valid DHCP message size, or this isn't a valid option */
        if (type == option_num) {
            if (len < min_length)
                break;
            if (length)
                *length = len;
            return p; /* start of actual option data */
        }
        p += len;
    }
    return NULL; /* Not found */
}

static uint8_t *add_dhcp_option_byte(uint8_t *opt, uint8_t type, uint8_t value)
{
    *opt++ = type;
    *opt++ = 1;
    *opt++ = value;
    return opt;
}

static uint8_t *add_dhcp_option_bytes(uint8_t *opt, uint8_t type, void *value, uint8_t len)
{
    *opt++ = type;
    if (len) {
        *opt++ = len;
        memcpy(opt, value, len);
    }
    return opt+len;
}

/* Find a free DHCP lease, or a lease already assigned to 'hwaddr' */
static dhcp_lease_t *find_lease_slot(server_state_t* state, uint8_t *hwaddr)
{
    dhcp_lease_t *empty_lease = NULL;
    for (int i = 0; i < state->max_leases; i++) {
        if (!state->leases[i].active && !empty_lease)
            empty_lease = &state->leases[i];
        else if (memcmp(hwaddr, state->leases[i].hwaddr, 6) == 0)
            return &state->leases[i];
    }
    return empty_lease;
}

