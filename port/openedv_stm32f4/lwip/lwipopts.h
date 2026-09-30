/**
 * @file    lwipopts.h
 * @brief   lwIP options for the ALIENTEK F429 board (FreeRTOS, netconn/socket).
 *
 * IP/UDP/TCP/ICMP checksums are computed in software (the STM32F4 Tx checksum
 * offload does not transmit DHCP DISCOVERs). NO_SYS=0 build with a TCP/IP thread.
 */

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#include <stdint.h>

/* SNTP time hook, implemented by the port/app (see SNTP_SET_SYSTEM_TIME). */
void lwip_sntp_set_time(uint32_t sec);

/* The lwIP port uses the raw FreeRTOS port (no CMSIS-OS): NO_SYS=0. */
#define NO_SYS                          0
#define SYS_LIGHTWEIGHT_PROT            1

#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (24 * 1024)
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_UDP_PCB                8
#define MEMP_NUM_TCP_PCB                10
#define MEMP_NUM_TCP_PCB_LISTEN         4
#define MEMP_NUM_TCP_SEG                120
#define MEMP_NUM_SYS_TIMEOUT            12
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

/* Multicast (IGMP) and socket-level broadcast. */
#define LWIP_IGMP                       1
#define IP_SOF_BROADCAST                1
#define IP_SOF_BROADCAST_RECV           1

/* MQTT client (lwIP apps/mqtt). It uses the altcp layer; TLS is NOT enabled,
 * the ALIENTEK examples connect in plain text. */
#define LWIP_ALTCP                      1
#define LWIP_MQTT                       1

/* SNTP client with DNS-based server name. The app provides the hook. */
#define LWIP_SNTP                       1
#define SNTP_SERVER_DNS                 1
#define SNTP_SET_SYSTEM_TIME            lwip_sntp_set_time

#define LWIP_STATS                      0
#define LWIP_NETIF_API                  1
#define LWIP_COMPAT_MUTEX               0

/* LWIP_RAND() source (used for TCP ISN, DHCP/DNS IDs): 1 = STM32 hardware
 * RNG (drv_rng), 0 = C library rand(). Default: hardware. */
#ifndef LWIP_RAND_USE_HW
#define LWIP_RAND_USE_HW                1
#endif

/* Software checksums. The STM32F4 Tx checksum offload does not transmit DHCP
 * DISCOVERs (source 0.0.0.0, destination 255.255.255.255), so lwIP generates
 * and verifies the IP/UDP/TCP/ICMP checksums in software. */
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1

#define LWIP_NETCONN                    1
#define LWIP_SOCKET                     1

#define LWIP_NETIF_LINK_CALLBACK        1

#define LWIP_SO_RCVTIMEO                1

#define TCPIP_THREAD_NAME               "tcpip"
#define TCPIP_THREAD_STACKSIZE          2048
#define TCPIP_MBOX_SIZE                 8
#define DEFAULT_UDP_RECVMBOX_SIZE       6
#define DEFAULT_TCP_RECVMBOX_SIZE       6
#define DEFAULT_ACCEPTMBOX_SIZE         6
#define DEFAULT_THREAD_STACKSIZE        1024
#define TCPIP_THREAD_PRIO               5

#endif /* LWIPOPTS_H */
