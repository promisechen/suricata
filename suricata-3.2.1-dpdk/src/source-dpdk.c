#include "stdio.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <rte_config.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_malloc.h>
#include <rte_cycles.h>
#include <rte_errno.h>

#define NB_MBUF   8192

struct rte_ring *packetRing;
#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 128
#define RTE_TEST_TX_DESC_DEFAULT 512
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;
struct rte_mempool * l2fwd_pktmbuf_pool = NULL;
static const struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 0, /**< IP checksum offload disabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 0, /**< CRC stripped by hardware */
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};
static int
l2fwd_launch_one_lcore(__attribute__((unused)) void *dummy)
{
     struct rte_mbuf *pkts_burst[32];
    int ret = 0;
    int enq = 0;
    while(1)
    {
        ret = rte_eth_rx_burst(0, 0, (struct rte_mbuf **)&pkts_burst, 1);      
        if(ret!=0) 
        {
            struct rte_mbuf *m = pkts_burst[0];
            enq = rte_ring_enqueue_burst(packetRing, (void *)&m, 1);
            if(enq!=1)
            {
                printf("ring error\n",enq);
                rte_pktmbuf_free(m);
            }
        }
    }
    return 0;
}
int init_dpdk()
{
    int ret = 0;
    uint8_t nb_ports;
    char arg[16][1024];
    char * dargv[16] = {arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], \
        arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12],arg[13],arg[14],arg[15]};
    int nRC = 0;
    uint64_t lCoreMask = 0; 
    int i = 0 ;
    sprintf(dargv[0], "ddtest");
    sprintf(dargv[1], "-c");
    sprintf(dargv[2], "0x%lx", 3);
    sprintf(dargv[3], "-n");
    sprintf(dargv[4], "4");
    printf("abcde\n");
    ret = rte_eal_init(5, dargv);

    nb_ports = rte_eth_dev_count();
    if (nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

    //内存池 
    /* create the mbuf pool */
    l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF,
            MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
            rte_socket_id());
    if (l2fwd_pktmbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
    //网卡配置
    ret = rte_eth_dev_configure(0, 1, 1, &port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
                ret, (unsigned) 0);
    // 初始化队列
    ret = rte_eth_rx_queue_setup(0, 0, nb_rxd,
            rte_eth_dev_socket_id(0),
            NULL,
            l2fwd_pktmbuf_pool);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
                ret, (unsigned) 0);

    /* init one TX queue on each port */
    fflush(stdout);
    ret = rte_eth_tx_queue_setup(0, 0, nb_txd,
            rte_eth_dev_socket_id(0),
            NULL);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
                ret, (unsigned) 0);
    // 启动端口抓包 
    ret = rte_eth_dev_start(0);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
                ret, (unsigned) 0);
    rte_eth_promiscuous_enable(0);
    //队列 
    packetRing = rte_ring_create("packetRing", 8192, 
            0, 0);
//RING_F_SC_DEQ
    /* launch per-lcore init on every lcore */
    rte_eal_mp_remote_launch( l2fwd_launch_one_lcore, NULL, SKIP_MASTER);
#if 0
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0) {
            ret = -1;
            break;
        }
    }

    while(1)
    {
//printf("abc\n");
            ;
    }
#endif
    return 0;
}

