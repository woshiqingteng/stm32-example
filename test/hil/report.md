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
  DNS 192.168.2.1** (the demos fall back to this when DHCP does not answer).
- Client demos (7/8/10/11) target the PC at **192.168.2.8**.
- The router's DHCP did not answer the board; the static path was used.

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
| MQTT connectivity (18) | DNS resolves `mqtts.heclouds.com` and the TCP connect reaches the MQTT callback (`State:Disconnect`, expected with a placeholder key) |

## Not fully exercised (environment)
- **Board-initiated traffic** (TCP clients 8/11, UDP clients 7/10, broadcast
  10-1, multicast 10-2): the Windows firewall drops unsolicited inbound to the
  PC-side peer. Opening it needs an administrator rule:
  ```
  netsh advfirewall firewall add rule name="lwip-hil-udp" dir=in action=allow protocol=UDP localport=8080
  netsh advfirewall firewall add rule name="lwip-hil-tcp" dir=in action=allow protocol=TCP localport=8080
  ```
  (delete afterwards with `... delete rule name="lwip-hil-udp"`).
  The send trigger itself is confirmed (KEY0 watchpoint `0x80`).
- **MQTT 17 (Aliyun)**: the composed host uses the placeholder ProductKey, so
  it is NXDOMAIN; a real ProductKey is needed to test connectivity.
- **MQTT publish**: placeholder credentials, connectivity only.
- Router DHCP was not granted; pktmon/netsh need administrator (denied).

## Tools
`hil_fb_ocr.py`, `hil_key.py`, `pc_peer.py`, `hil_run.py` (see `README.md`).
