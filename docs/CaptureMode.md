# RF Capture Mode

Capture mode enables passive RF sniffing on the CMT2300A radio, logging all received frames from Hoymiles inverters to the serial console (WebSocket). This is used for protocol reverse-engineering of unsupported inverter models.

## Enabling Capture Mode

### Via Web API

```bash
# Enable capture mode
curl -X POST "http://admin:<password>@<dtu-ip>/api/dtu/capture" \
  --data-urlencode 'data={"capture_mode":true}'

# Check capture mode status
curl -s "http://admin:<password>@<dtu-ip>/api/dtu/capture"
# Returns: {"capture_mode":true}

# Disable capture mode
curl -X POST "http://admin:<password>@<dtu-ip>/api/dtu/capture" \
  --data-urlencode 'data={"capture_mode":false}'
```

**Important:** The API uses form-encoded POST with a `data` parameter containing JSON. This is the standard OpenDTU `parseRequestData()` pattern — do **not** send a raw JSON body with `Content-Type: application/json`.

### API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/dtu/capture` | Returns current capture mode state |
| POST | `/api/dtu/capture` | Enable/disable capture mode |

POST body (form-encoded):
```
data={"capture_mode":true}
```

Both endpoints require HTTP Basic authentication (`admin:<password>`).

## Reading Capture Output

Capture frames are logged to the serial console, accessible via WebSocket:

```
ws://<dtu-ip>/console
```

### Python Example

```python
import websocket, re

ws = websocket.create_connection("ws://<dtu-ip>/console", timeout=10)
while True:
    try:
        ws.settimeout(3)
        msg = ws.recv()
        if 'CAPTURE' in msg and 'src=' in msg:
            print(msg.strip())
    except websocket.WebSocketTimeoutException:
        continue
ws.close()
```

### Log Format

```
I (timestamp) hoymiles: CAPTURE 863.00 MHz | -82 dBm | src=a025566b dst=a00e4ff1 len=27 | 95 A0 25 56 6B A0 0E 4F F1 01 ...
```

Fields:
- **Frequency**: RF channel (863.00-870.00 MHz for EU)
- **RSSI**: Signal strength in dBm
- **src**: Source inverter serial (last 4 bytes)
- **dst**: Destination DTU serial (last 4 bytes)
- **len**: Total frame length in bytes
- **Hex dump**: Complete RF frame

### Frame Structure

Each RF frame is 27 bytes:

| Offset | Length | Field |
|--------|--------|-------|
| 0 | 1 | Preamble (0x95) |
| 1-4 | 4 | Source serial (inverter) |
| 5-8 | 4 | Destination serial (DTU) |
| 9 | 1 | Frame ID / fragment count |
| 10-25 | 16 | Payload data |
| 26 | 1 | CRC8 (per-frame) |

Frame IDs:
- **0x01-0x05**: Inverter data response fragments
- **0x80**: DTU data request
- **0x81**: DTU ping/keepalive
- **0x86**: Inverter final data fragment (shorter payload, includes CRC16)

### Reassembly

The OpenDTU framework reassembles response fragments:

```c
// InverterAbstract.cpp
memcpy(_rxFragmentBuffer[fragmentId - 1].fragment, &fragment[10], len - 11);
_rxFragmentBuffer[fragmentId - 1].len = len - 11;
```

This strips the 10-byte header and 1-byte CRC8, storing **16 data bytes per fragment**.

## Channel Hopping

When capture mode is active, the CMT radio sweeps through channels 12-40 (863.00-870.00 MHz in EU) to find active inverters. The hopping rate is automatic — it dwells on each channel briefly before moving to the next.

Known active frequencies for the MIT-5000-8T: **863.00 MHz** and **863.25 MHz**.

## Notes

- Capture mode and normal inverter polling are **mutually exclusive**. While capture mode is active, the DTU will not poll configured inverters.
- The WebSocket console has a **single-client limit**. If a browser tab has the console open, a Python client may receive no data.
- Capture mode state does **not persist** across reboots. It must be re-enabled after each power cycle or firmware update.
- Low-power conditions (winter, early morning) result in sporadic frame transmission. A complete 6-frame capture may take several minutes.
