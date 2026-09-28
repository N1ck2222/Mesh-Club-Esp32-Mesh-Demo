> **AI disclaimer:** The Arduino sketches in this repository were written by an AI assistant (Claude, made by Anthropic). The author directed the design, uploaded the code to real hardware, and tested it, but the code itself is AI-generated. Review it before relying on it, and expect that it may contain mistakes.

A small ESP32 demo that shows how a wireless mesh network works, using nothing but a few dev boards and their onboard LEDs.

One board acts as a **transmitter** that sends out a heartbeat. Two **receiver** boards flash their LEDs when they hear it. Because the boards form a mesh, a receiver that is out of the transmitter's direct radio range still gets the heartbeat, relayed through the other receiver.

## What it does

| Board | Behavior |
|-------|----------|
| **Transmitter** | Broadcasts a heartbeat every 1 second and blinks its onboard LED slowly, like a pulse |
| **Receiver** (x2) | Flashes its onboard LED **fast** while heartbeats are arriving, and stays **dark** when they stop |

A set is 3 boards: 1 transmitter + 2 receivers. You can run several sets side by side by giving each its own mesh name (see [Configuration](#configuration)).

## How the mesh part works

The sketches use the [painlessMesh](https://gitlab.com/painlessMesh/painlessMesh) library, which handles routing automatically. The receiver code contains no relay logic at all: when the transmitter broadcasts a message, painlessMesh forwards it from node to node until everyone in the mesh has it.

That means you can place one receiver in the middle and the other one farther away, out of the transmitter's direct range. The far receiver will still flash, because the middle receiver passes the heartbeat along.

## Hardware

- 3 x ESP32 dev boards per set (tested on generic ESP32 DevKitC-style boards with a WROOM-32 module)
- USB cables and a power source for each board
- No extra wiring: everything uses the onboard LED

The sketches assume the onboard LED is on **GPIO2**, which is correct for most ESP32 dev boards. Some boards only have a power LED and no user LED. If nothing blinks, check your board's pinout and change `LED_PIN` at the top of each sketch.

## Software setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) (2.x recommended).
2. Add ESP32 board support: in **Boards Manager**, install **esp32 by Espressif Systems**.
3. In **Library Manager**, install:
   - `painlessMesh`
   - `ArduinoJson`
   - `TaskScheduler`
   - `AsyncTCP`
4. Select your board (**ESP32 Dev Module** works for most generic boards) and the correct serial port.

If you hit compile errors, an `ArduinoJson` version mismatch is the most common cause.

## Uploading

- Open `mesh_transmitter/mesh_transmitter.ino` and upload it to **one** board.
- Open `mesh_receiver/mesh_receiver.ino` and upload the **same sketch** to the other **two** boards. Receivers don't need individual IDs.

Open the Serial Monitor at **115200 baud** on any board to watch what it's doing. Receivers print `RECEIVING` and `LOST` when their state changes.

## Configuration

Each sketch has a config block at the top:

| Setting | Default | Notes |
|---------|---------|-------|
| `MESH_PREFIX` | `heartbeatA` | Mesh name. **Must match** across all 3 boards in a set. Use a different value (e.g. `heartbeatB`) for a second set so the sets stay separate. |
| `MESH_PASSWORD` | `meshdemo1234` | Must match across a set. Change it if you like. |
| `MESH_PORT` | `5555` | Must match across a set. |
| `LED_PIN` | `2` | Onboard LED pin. |
| `HEARTBEAT_INTERVAL_MS` | `1000` | Transmitter only. Time between heartbeats. |
| `LED_ON_MS` | `200` | Transmitter only. How long the LED stays lit per beat. |
| `FAST_BLINK_MS` | `100` | Receiver only. Toggle interval while receiving. |
| `HEARTBEAT_TIMEOUT_MS` | `3000` | Receiver only. Silence longer than this counts as "not receiving". |
| `REDUCE_TX_POWER` | `false` | Set to `true` on all boards to lower radio power (see below). |

## Testing the relay

To see real multi-hop behavior, the far receiver must not be able to hear the transmitter directly:

1. Power on all three boards close together and confirm both receivers flash fast.
2. Move one receiver away from the transmitter, keeping the other receiver somewhere in between.
3. The far receiver should keep flashing, since it is being fed by the middle one.

At close range, ESP32 radios usually reach each other directly, so everything hears everything. If that happens, set `REDUCE_TX_POWER` to `true` on all three boards and re-upload. That lowers transmit power so distance and walls matter much sooner.

Other quick tests:

- **Unplug the transmitter.** Both receivers should go dark after about 3 seconds.
- **Plug it back in.** The transmitter rejoins the mesh and sends a heartbeat as soon as it connects, so the receivers resume flashing without waiting for the next scheduled beat. Total reconnect time is mostly ESP32 boot time plus painlessMesh's join time.

## Why ESP32 and not the Arduino MKR WiFi 1010?

This project started on MKR WiFi 1010 boards, but painlessMesh does not run on them. The MKR WiFi 1010's WiFi chip is controlled through Arduino's WiFiNINA firmware, which doesn't give painlessMesh the low-level WiFi access it needs. WiFiNINA also can't run as an access point and a station at the same time, which real multi-hop relaying depends on. Native ESP32 boards have no such limits, so that's what this demo uses.

## Troubleshooting

- **Nothing blinks:** your board may not have a user LED on GPIO2. Check the pinout and change `LED_PIN`.
- **Receivers never flash:** confirm `MESH_PREFIX`, `MESH_PASSWORD`, and `MESH_PORT` are identical on all three boards, and check the Serial Monitor for `New connection` messages.
- **Two sets interfering:** give each set a different `MESH_PREFIX`.
- **Everything flashes even when boards are far apart:** the radios are still in direct range. Try `REDUCE_TX_POWER` or add more distance.
- **Compile errors:** update or reinstall `painlessMesh`, `ArduinoJson`, `TaskScheduler`, and `AsyncTCP`.

## License

MIT. See [LICENSE](LICENSE).
