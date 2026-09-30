# lwIP demo HIL report

## Critical fix (found by HIL)
`ethernetif.c:low_level_init()` used to call `HAL_ETH_Start_IT()` **before the
scheduler** (during `netif_add`). With the Ethernet cable connected, an ETH RX
interrupt fired pre-scheduler and called `xSemaphoreGiveFromISR()`:
`configASSERT` at `port.c:904` then a UsageFault (INVSTATE) escalated to a
HardFault, leaving the panel blank.

Fix: ETH interrupts are enabled from the `eth_rx` task (`ethernetif_input`),
i.e. only once the scheduler is running. After the fix the UI renders and no
fault occurs.

## Configuration used
- Board static fallback (permanent): **192.168.2.100/24, gw 192.168.2.1,
  DNS 192.168.2.1** (used when DHCP does not answer).
- Client demos (7/8/10/11) target the PC at **192.168.2.8**.
- For the board-initiated checks the PC firewall was opened for inbound
  TCP/UDP 8080 (rules removed afterwards).

## Verified
| check | result |
|---|---|
| ping board (192.168.2.100) | 4/4 replies, ARP `02-00-00-12-34-56` |
| framebuffer OCR, **all 15 apps** | title / STM32 / ATOM@ALIENTEK / lwIP Init Successed / IP / Ethernet Speed / KEY0:Send data / Receive Data: decode correctly (portrait 480x800) |
| TCP server (9) | PC sent `PC2BOARD-TEST-1234`; board displayed it on the RX line |
| HTTP server (16) | PC `GET /` returned `HTTP/1.1 200 OK` + page |
| KEY0 (7) | PH3 injection; write-watchpoint on `g_lwip_send_flag` hit with `0x80` |
| NTP (13) | LCD `2026-09-30 22:57:12`; RTC `TR=0x00225729`, `DR=0x00268930` |
| SNTP (14) | LCD `2026-09-30 22:58:00`; RTC `TR=0x00225815`, `DR=0x00268930` |
| MQTT connectivity (18) | DNS resolves `mqtts.heclouds.com`; TCP connect reaches the MQTT callback (`State:Disconnect`, expected with a placeholder key) |

### Board-initiated (PC firewall opened for the test)
| app | peer | result |
|---|---|---|
| 7 netconn UDP | PC `udp-echo` | PC received `ALIENTEK DATA`; board RX echoed it |
| 10 socket UDP | PC `udp-echo` | PC received; board RX echoed |
| 8 netconn TCP client | PC `tcp-echo` | PC accepted the board; board RX echoed `ALIENTEK DATA` |
| 11 socket TCP client | PC `tcp-echo` | PC accepted the board; board RX echoed |
| 10-1 UDP broadcast | PC `udp-listen` | PC received `ALIENTEK DATA` |
| 10-2 UDP multicast | PC `mcast-listen` (join on 192.168.2.8) | PC received `ALIENTEK DATA` |
| 12 socket TCP server | PC connect | board RX showed `SERVER12-RX-TEST` |
| 12-1 multi-connection | PC 2x connect | board RX showed `MULTI-A` |

## Not exercised
- **MQTT 17 (Aliyun)**: the composed host uses the placeholder ProductKey, so it
  is NXDOMAIN; a real ProductKey is required to test connectivity.
- **MQTT publish**: placeholder credentials, connectivity only.

## Tools
`hil_fb_ocr.py`, `hil_key.py`, `pc_peer.py`, `hil_run.py` (see `README.md`).
