# WiFi Inverter — Connection & Polling Cycle Design

This document describes the connection-handling strategy used by `DtuTcpClient`
(the TCP client that talks to an HMS-xxxW-2T WiFi inverter's built-in DTU on port
10081) and explains why the current *connect-on-demand* approach replaced the
original *persistent-connection* approach.

Both approaches produce identical downstream data (they push into the
protocol-agnostic `StatisticsParser`); they differ only in how the TCP socket to
the DTU is managed.

---

## Approach A — Persistent connection + keepalive (original)

Connect once and stay connected for the lifetime of the session.

- A `Ticker` (`_loopTimer`, every `DTU_LOOP_SEC` = 5 s) drives `_loop()`, which
  runs the poll cycle over the already-open socket.
- A second `Ticker` (`_keepAliveTimer`, every `DTU_KEEPALIVE_SEC` = 10 s) writes a
  single null byte to keep the socket from going idle.
- AppInfo is re-requested periodically (`DTU_APPINFO_INTERVAL_MS`), staggered to
  fire ~8 s after connect.
- **Any** disconnect is treated as a failure: `_onDisconnect` clears
  `_appInfoReceived` and fires `_connectCallback(false)`, which increments the
  inverter's reachability failure count.

### Poll cycle (state: `_txrxState`)

```
AppInfo -> RealData -> GetConfig -> GetAlarms
```

Each step advances on the next ticker fire, so a full cycle is paced by the 5 s
ticker regardless of how fast the DTU actually replies.

### Problems observed

1. **Stall / connection reset.** The DTU firmware resets an idle-but-open
   connection on its own schedule. While the socket was held open between polls,
   the DTU would eventually drop it; until that drop happened, data updates
   stalled.
2. **False failure counts.** Because every disconnect — including the DTU's own
   routine resets — went through the failure path, the reachability counter was
   noisy and could trip watchdog/availability logic even when the inverter was
   perfectly healthy.
3. **Slow cycle.** Ticker-paced step advancement made the effective poll interval
   roughly 3× longer than the DTU's response latency required.

---

## Approach B — Connect-on-demand + planned disconnect (current)

Stay **disconnected** between polls. Open the socket only when there is work to
do, run one complete poll cycle as fast as the DTU replies, deliver the data,
then close the socket cleanly.

- `_loop()` still runs every `DTU_LOOP_SEC`, but while `OFFLINE` it only connects
  when work is actually due:
  - `pollDue` — `millis() - _lastPollCompletedAt >= DTU_WIFI_POLL_SEC * 1000`
    (`DTU_WIFI_POLL_SEC` = 30 s), or
  - a pending command (`_pendingLimit || _pendingPower || _pendingRestart`).
  - Otherwise it returns immediately and stays disconnected.
- On connect, `_onConnect` calls `_loop()` right away instead of waiting for the
  next ticker fire, and `_onData` chains AppInfo → RealData directly — so a full
  cycle no longer waits multiple ticker intervals (~3× faster).
- When the cycle completes (`_readRespGetAlarms`), the async-TCP task sets two
  atomics: `_dataReady` and `_pendingDisconnect`. It does **not** close the
  socket itself.
- The main task's `tick()` sees `_dataReady`, delivers the fresh data first, then
  sees `_pendingDisconnect`, sets `_plannedDisconnect`, and closes the socket.
  Delivering before closing means stats are already fresh when the
  connection-closed event fires.
- `_onDisconnect` checks `_plannedDisconnect`:
  - **planned** — intentional close after a completed poll; no failure callback,
    and `_appInfoReceived` is preserved so the next session can skip AppInfo.
  - **unplanned** — a real error/timeout; reset `_appInfoReceived` and fire
    `_connectCallback(false)` (the genuine failure path).
- The keepalive timer and its helpers (`_keepAliveTimer`, `_keepAliveCb`,
  `_keepAlive`) are removed entirely — there is no idle socket to keep alive.

### Why the two atomics matter

`_pendingDisconnect` moves the socket teardown off the async-TCP callback and onto
the main task, so the close happens at a safe point after data delivery.
`_plannedDisconnect` lets `_onDisconnect` distinguish an intentional close from a
genuine failure. That pair is the crux of the fix: it eliminates both the
idle-reset stall and the false failure counts.

### State machine

```
OFFLINE --(pollDue || pending cmd)--> CONNECTING --> CONNECTED
   ^                                                     |
   |                                              (poll cycle: AppInfo
   |                                               -> RealData -> Config
   |                                               -> Alarms)
   |                                                     |
   |                                          tick(): deliver data,
   |                                          then planned close
   |                                                     |
   +----------- OFFLINE (idle ~DTU_WIFI_POLL_SEC) <------+

Real error / timeout at any point -> OFFLINE via failure path
(_connectCallback(false), _appInfoReceived reset, retry/backoff).
```

---

## Trade-offs

| Aspect                | A: Persistent            | B: Connect-on-demand         |
|-----------------------|--------------------------|------------------------------|
| Socket lifetime       | Always open              | Open only during a poll      |
| Idle DTU reset / stall| Yes (the core bug)       | Avoided — never idle-open    |
| Failure counter       | Noisy (counts DTU resets)| Accurate (real errors only)  |
| Poll latency          | Ticker-paced (~3× slower)| Chained, response-paced      |
| TCP handshakes        | One per session          | One per poll (~every 30 s)   |
| State complexity      | Simpler                  | +3 flags/timestamp           |
| Keepalive machinery   | Required                 | Removed                       |

Approach B trades a slightly more complex state (three extra fields:
`_pendingDisconnect`, `_plannedDisconnect`, `_lastPollCompletedAt`) and one TCP
handshake per poll for correctness with how the DTU firmware actually wants to be
talked to. Field testing showed multi-day stable operation with no stalls and
accurate reachability reporting.

---

## Key constants

| Constant             | Value  | Meaning                                        |
|----------------------|--------|------------------------------------------------|
| `DTU_TCP_PORT`       | 10081  | DTU TCP port                                   |
| `DTU_LOOP_SEC`       | 5 s    | Ticker interval; how often `_loop()` checks    |
| `DTU_WIFI_POLL_SEC`  | 30 s   | Minimum seconds between full data polls (B)    |
| `DTU_TXRX_TIMEOUT_MS`| 30000  | Per-exchange timeout                           |
| `DTU_RECONNECT_MAX`  | 5      | Retries before a backoff pause                 |
| `DTU_RECONNECT_PAUSE_MS` | 60000 | Pause after a retry burst                   |

(`DTU_KEEPALIVE_SEC` existed only in Approach A and has been removed.)
