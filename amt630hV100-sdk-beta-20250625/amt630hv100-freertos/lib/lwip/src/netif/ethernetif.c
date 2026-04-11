
#include <stdio.h>
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "ethernet.h"


#define DUMP_LWIP_NCM_TX_DATA 0
#define DUMP_LWIP_NCM_RX_DATA 0

/* Define those to better describe your network interface. */
#define IFNAME0 'n'
#define IFNAME1 'c'

struct ethernetif {
  struct eth_addr *ethaddr;
};

void ncm_net_set_intf(void* intf);
void gether_send_ext(void * const pxDescriptor);
int g_ncm_register(const char *name);
int ncm_get_mac_address(char mac[6]);


/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
ncm_low_level_init(struct netif *netif)
{
  struct ethernetif *ethernetif = netif->state;
  //int ret = -1;
  char mac[6] = {0xdc, 0x0d, 0x30, 0xa2, 0x70, 0xcd};
  
  /*ret = ncm_get_mac_address(mac);
  if (ret < 0) {
	printf("get wlan mac failed\r\n");
  }*/

  printf("%s:%d str_mac: %02x:%02x:%02x:%02x:%02x:%02x \r\n", __func__, __LINE__, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  (void)ethernetif;

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = mac[0];  
  netif->hwaddr[1] = mac[1];  
  netif->hwaddr[2] = mac[2];  
  netif->hwaddr[3] = mac[3];  
  netif->hwaddr[4] = mac[4];  
  netif->hwaddr[5] = mac[5];

  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP;

#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  /* Do whatever else is needed to initialize interface. */
}


static err_t
ncm_low_level_output(struct netif *netif, struct pbuf *p)
{
  struct ethernetif *ethernetif = netif->state;
  struct pbuf *q;

  (void)ethernetif;

#if ETH_PAD_SIZE
  pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

  for (q = p; q != NULL; q = q->next) {
  	pbuf_ref(q);
  	gether_send_ext((void*)q);
#if DUMP_LWIP_NCM_RX_DATA
	if (1) {
		int i;
		char *tmpbuf = q->payload;
		printf("[lwip] [send]-->");
		for (i = 0; i < q->len; i++) {
			printf("%02x ", tmpbuf[i]);
		}printf("\r\n");
	}
#endif
  }

  //signal that packet should be sent();

  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  if (((u8_t *)p->payload)[0] & 1) {
    /* broadcast or multicast packet*/
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  } else {
    /* unicast packet */
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  }
  /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
  pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

void ncm_ethernetif_input(void *h, struct pbuf* p)
{
    struct netif *netif = (struct netif *)h;
    
    //printf("netif->input:%p\r\n", netif->input);
#if DUMP_LWIP_NCM_RX_DATA
	if (1) {
		int i;
		char *tmpbuf = p->payload;
		printf("[lwip] [recv]-->");
		for (i = 0; i < p->len; i++) {
			printf("%02x ", tmpbuf[i]);
		}printf("\r\n");
	}
#endif
    /* full packet send to tcpip_thread to process */
    if (netif->input(p, netif) != ERR_OK) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
        pbuf_free(p);
    }
    /* the pbuf will be free in upper layer, eg: ethernet_input */
}

err_t
ncm_ethernetif_init(struct netif *netif)
{
  struct ethernetif *ethernetif;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

  ethernetif = mem_malloc(sizeof(struct ethernetif));
  if (ethernetif == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
    return ERR_MEM;
  }

#if LWIP_NETIF_HOSTNAME
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->state = ethernetif;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = ncm_low_level_output;

  g_ncm_register("ncm");

  ethernetif->ethaddr = (struct eth_addr *) & (netif->hwaddr[0]);
  ncm_low_level_init(netif);

  ncm_net_set_intf((void*)netif);

  return ERR_OK;
}

