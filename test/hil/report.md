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

## Environment (observed)
- Board: **link up**, MAC DMA running; router DHCP did **not** grant a lease, so
  the demos fell back to their static address.
- PC: 192.168.2.8/24 (gw 192.168.2.1).
- For the run below the board's fallback was temporarily set to **192.168.2.100**
  (same subnet as the PC) and a temporary static DNS (192.168.2.1) was added;
  the client demos' peer was temporarily set to the PC (192.168.2.8). All of
  these temporary edits were **reverted** and are not committed.

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

## Not fully exercised
- **Board-initiated traffic** (TCP clients 8/11, UDP clients 7/10,
  broadcast 10-1, multicast 10-2): the Windows firewall blocks unsolicited
  inbound connections/datagrams to the PC-side peers (needs an admin rule to
  verify delivery). The send trigger itself is confirmed (KEY0 watchpoint).
- **MQTT 17/18**: placeholders credentials; only DNS/TCP reachability applies.
- Router DHCP: not diagnosed (pktmon needs admin); the static fallback was used.

## Tools
`hil_fb_ocr.py`, `hil_key.py`, `pc_peer.py`, `hil_run.py` (see `README.md`).
