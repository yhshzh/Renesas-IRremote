# Renesas_IRremote

Firmware for an RA4M2-based IR learning and transmission board. The board uses an onboard ESP-AT module for WiFi TCP and BLE GATT communication, and works with the companion Flutter app `ir_remote_app`.

## Features

- Learn IR frames from an onboard IR receiver
- Transmit air-conditioner IR commands using IRremoteESP8266
- WiFi AP/TCP control via ESP-AT
- BLE GATT control via ESP-AT
- JSON line protocol shared by WiFi and BLE
- e2studio, VSCode, and command-line CMake build support

## Hardware

Target MCU: `R7FA4M2AC3CFP`

The current board has the IR and ESP-AT peripherals onboard. The firmware uses these default resources:

| Function | RA4M2 pin | FSP instance | Notes |
| --- | --- | --- | --- |
| IR receive | `P005` | `g_ir_rx_irq` / ICU IRQ10 | Both-edge external interrupt |
| IR transmit carrier | `P115` | `g_ir_tx_gpt` / GPT4 GTIOCA | 38 kHz PWM |
| ESP-AT UART RX | `P601/RXD9` | `g_esp_uart` / SCI9 | 115200 8N1 |
| ESP-AT UART TX | `P602/TXD9` | `g_esp_uart` / SCI9 | 115200 8N1 |

Default network names:

- WiFi AP: `RA4M2_IR`, password `12345678`
- TCP server: `192.168.4.1:8765`
- BLE device name: `RA4M2_IR`

## Requirements

- Renesas e2studio with FSP 6.4.0
- ARM GNU toolchain
- CMake 3.20+
- Ninja
- `third_party/IRremoteESP8266`

Clone the third-party IR library after cloning this repository:

```sh
cd Renesas_IRremote
mkdir -p third_party
git clone https://github.com/crankyoldgit/IRremoteESP8266.git third_party/IRremoteESP8266
```

`third_party/IRremoteESP8266` is intentionally not committed to this repository.

## Build

### e2studio

1. Import this repository with `File -> Import -> Existing Projects into Workspace`.
2. Open `configuration.xml`.
3. Generate FSP content if e2studio asks for it.
4. Build the `Renesas_IRremote` project.

Typical e2studio output:

```text
build/cmake.arm.debug.R7FA4M2AC/Renesas_IRremote.elf
build/cmake.arm.debug.R7FA4M2AC/Renesas_IRremote.srec
```

### Command Line

The default preset expects `arm-none-eabi-*` tools under `/usr/bin`:

```sh
cmake --preset Debug
cmake --build --preset Debug
```

Output:

```text
build/App/Renesas_IRremote.elf
build/App/Renesas_IRremote.srec
```

If your toolchain is elsewhere, configure explicitly:

```sh
cmake -S . -B build/App -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/gcc.cmake" \
  -DARM_TOOLCHAIN_PATH="/path/to/arm-none-eabi/bin" \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build/App
```

## Configuration

Main CMake options:

```text
APP_IR_RX_PIN=BSP_IO_PORT_00_PIN_05
APP_IR_TX_PIN=BSP_IO_PORT_01_PIN_15
APP_TRANSPORT_UART_BAUD=115200
APP_TRANSPORT_TCP_PORT=8765
IRREMOTE_RA4M2_USE_IRQ_RECV=ON
IRREMOTE_RA4M2_USE_GPT_SEND=ON
```

If the board hardware changes, update both FSP (`configuration.xml`) and the matching CMake options.

## Flash and Debug

Use e2studio, Renesas Flash Programmer, J-Link tooling, or pyOCD with the generated `.elf`/`.srec`.

pyOCD example:

```sh
pyocd gdbserver --config pyocd.yaml --elf build/App/Renesas_IRremote.elf --persist -M halt
```

Then connect with:

```sh
arm-none-eabi-gdb build/App/Renesas_IRremote.elf
```

## App Protocol

WiFi and BLE use newline-delimited JSON. See [APP_PROTOCOL.md](APP_PROTOCOL.md) for command and response details.

## Companion App

Use the Flutter project `ir_remote_app` to connect over BLE or WiFi, run diagnostics, learn IR frames, and transmit air-conditioner commands.

## License

This project includes LGPL-2.1 licensed IRremoteESP8266 code as an external checkout. See [LICENSE](LICENSE) and `third_party/IRremoteESP8266/LICENSE.txt`.
