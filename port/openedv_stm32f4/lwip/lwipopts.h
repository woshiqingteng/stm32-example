/**
 * @file    lwipopts.h
 * @brief   lwIP options for the ALIENTEK F429 board (FreeRTOS, netconn/socket).
 *
 * Hardware checksum offload is enabled (the STM32F4 ETH MAC computes IP/UDP/
 * TCP/ICMP checksums). The port is a full NO_SYS=0 build with a TCP/IP thread.
 */

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/* The lwIP port uses the raw FreeRTOS port (no CMSIS-OS): NO_SYS=0. */
#define NO_SYS                          0
#define SYS_LIGHTWEIGHT_PROT            1

#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16 * 1024)
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_UDP_PCB                8
#define MEMP_NUM_TCP_PCB                8
#define MEMP_NUM_TCP_PCB_LISTEN         4
#define MEMP_NUM_TCP_SEG                120
#define MEMP_NUM_SYS_TIMEOUT            8
#define PBUF_POOL_SIZE                  8
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(TCP_MSS + 40 + PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + 4)

#define LWIP_TCP                        1
#define TCP_TTL                         255
#define TCP_QUEUE_OOSEQ                 0
#define TCP_MSS                         (1500 - 40)
#define TCP_SND_BUF                     (11 * TCP_MSS)
#define TCP_SND_QUEUELEN                (8 * TCP_SND_BUF / TCP_MSS)
#define TCP_WND                         (4 * TCP_MSS)

#define LWIP_ICMP                       1
#define LWIP_UDP                        1
#define UDP_TTL                         255

#define LWIP_DHCP                       1
#define LWIP_DNS                        1

#define LWIP_STATS                      0
#define LWIP_NETIF_API                  1
#define LWIP_COMPAT_MUTEX               0

/* STM32F4xx ETH MAC checksum offload. */
#define CHECKSUM_BY_HARDWARE            1
#ifdef CHECKSUM_BY_HARDWARE
#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_GEN_ICMP               0
#endif

#define LWIP_NETCONN                    1
#define LWIP_SOCKET                     1

#define LWIP_NETIF_LINK_CALLBACK        1

#define LWIP_SO_RCVTIMEO                1

#define TCPIP_THREAD_NAME               "tcpip"
#define TCPIP_THREAD_STACKSIZE          1000
#define TCPIP_MBOX_SIZE                 8
#define DEFAULT_UDP_RECVMBOX_SIZE       6
#define DEFAULT_TCP_RECVMBOX_SIZE       6
#define DEFAULT_ACCEPTMBOX_SIZE         6
#define DEFAULT_THREAD_STACKSIZE        512
#define TCPIP_THREAD_PRIO               5

#endif /* LWIPOPTS_H */
