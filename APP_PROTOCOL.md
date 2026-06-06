# RA4M2 IR Remote App Protocol

本工程默认分支在 `IRREMOTE_RA4M2_SEND_DEMO=OFF` 且
`IRREMOTE_RA4M2_RECV_DEMO=OFF` 时进入 App 主循环：

```text
hal_entry -> RemoteController -> EspAtTransport + AirconIrService
```

## 硬件串口

- RA4M2 `P601/RXD9` 接 ESP32-C3-MINI `TXD`
- RA4M2 `P602/TXD9` 接 ESP32-C3-MINI `RXD`
- 默认波特率：`115200`
- SCI9 pin mux 已在当前 FSP 生成的 `ra_gen/pin_data.c` 中配置为
  `IOPORT_PERIPHERAL_SCI1_3_5_7_9`

## ESP-AT 初始化

固件启动后会通过 SCI9 发送一组 AT 命令：

- `AT`
- `ATE0`
- `AT+CWMODE=3`
- `AT+CWSAP="RA4M2_IR","12345678",5,3`
- `AT+CIPMUX=1`
- `AT+CIPSERVER=1,8765`
- `AT+CIFSR`
- `AT+BLEINIT=0`
- `AT+BLEINIT=2`
- `AT+BLENAME="RA4M2_IR"`
- `AT+BLEGATTSSRVCRE`
- `AT+BLEGATTSSRVSTART`
- `AT+BLEGATTSCHAR?`
- `AT+BLEADVSTOP`
- `AT+BLEADVDATAEX="RA4M2_IR","A002","5241344D325F4952",1`
- `AT+BLEADVSTART`

WiFi TCP server 默认端口是 `8765`。BLE 使用 ESP-AT 默认 GATT server，
MCU 解析 `+WRITE:` 事件，回复使用 `AT+BLEGATTSNTFY`。App 优先使用
ESP-AT 默认服务/特征：

- Service UUID: `0xA002`
- Write characteristic: `0xC304`
- Notify characteristic: `0xC305`
- Indicate fallback: `0xC306`

`AT+BLEINIT=0` 和 `AT+BLEADVSTOP` 只是为了清理 ESP32-C3 可能残留的上一次
状态；如果 ESP-AT 返回 `ERROR`，固件不会把这两条计入诊断错误。App 端会
接受标准 16-bit UUID 和完整 Bluetooth base 128-bit UUID 两种形式。

固件启动时会用 `AT+BLEGATTSCHAR?` 读取 ESP-AT GATT 表，并根据 UUID
自动更新运行时 index：`0xC304` 用作 App 写入，`0xC305` 用作通知。默认
index 和板级参数在 `src/app/app_board_config.h` 中维护。当前硬件红外配置为：

- 红外接收头：`P005` / `BSP_IO_PORT_00_PIN_05` / ICU `IRQ10`
- 红外发射头：`P115` / `BSP_IO_PORT_01_PIN_15` / GPT4 `GTIOCA`

BLE 和 TCP 输入都按结尾 `\n` 分帧。App 可能因为 BLE MTU 较小把一条 JSON
命令拆成多次 GATT write，MCU 会缓存分片，直到收到带 `\n` 的最后一片后
再交给上层处理。MCU 通过 BLE 返回 JSON 时也会按
`APP_BLE_NOTIFY_CHUNK_SIZE` 分片发送，App 同样按 `\n` 重组成完整 JSON。
BLE 输入只接受配置的 write characteristic index；ESP-AT 在开启通知时可能
产生 CCCD/descriptor 写入事件，固件会忽略这些非命令写入，避免污染 JSON
命令缓存。
如果 TCP 或 BLE 一次输入里带了连续多条 JSON，固件会尽量按同一个连接继续
处理后续以 `{` 开头的换行分帧 payload；App 端也会串行发送命令，降低粘包
和 ESP-AT busy 的概率。

## FSP 红外外设配置

当前固件默认使用硬件闭环需要的两组 FSP 实例，不再用 SysTick 采样接收，也
不在发送载波期间用 CPU 翻转 GPIO：

- `g_ir_rx_irq`: `Input -> External IRQ (r_icu)`
  - Channel: `10`
  - Pin: `P005`
  - Trigger: `Both Edge`
  - Callback: `irremote_ra4m2_recv_external_irq_callback`
  - Digital filter: enabled, `PCLK / 8`
  - Interrupt priority: `2`
- `g_ir_tx_gpt`: `Timers -> Timer, General PWM (r_gpt)`
  - Channel: `4`
  - Pin: `P115`, `GTIOCA`
  - Mode: `Saw-wave PWM`
  - Period unit: `Hertz`
  - Period: `38000`
  - Duty cycle: `50`
  - `GTIOCA Output Enabled`: `true`
  - Stop level: `Low`
  - Timer callback/interrupt: disabled

构建开关：

```sh
-DIRREMOTE_RA4M2_USE_IRQ_RECV=ON
-DIRREMOTE_RA4M2_USE_SYSTICK_RECV=OFF
-DIRREMOTE_RA4M2_USE_GPT_SEND=ON
```

生成内容应包含 `ra/fsp/src/r_icu/r_icu.c`、`ra/fsp/src/r_gpt/r_gpt.c`，
`ra_gen/common_data.*` 里的 `g_ir_rx_irq`，以及 `ra_gen/hal_data.*` 里的
`g_ir_tx_gpt`。`ra_gen/pin_data.c` 中 `P115` 生成的
`IOPORT_PERIPHERAL_GPT1` 是该脚的 PFS 功能选择枚举名；实际 GPT 通道以
`g_ir_tx_gpt_cfg.channel = 4` 和 `configuration.xml` 的 `gpt4.gtioca.p115`
为准。

## JSON 命令

所有 payload 都是单行 UTF-8 JSON，以 `\n` 结束。WiFi TCP 和 BLE GATT
共用同一套 payload。

查询状态：

```json
{"kind":"get_state"}
```

查询固件诊断配置：

```json
{"kind":"diag"}
```

开始学习一帧红外：

```json
{"kind":"learn_start"}
```

停止学习：

```json
{"kind":"learn_stop"}
```

连接路由器：

```json
{"kind":"wifi_set","ssid":"your-ap","password":"your-password"}
```

固件收到 `wifi_set` 后会发送 `AT+CWJAP`，随后定时查询 `AT+CIFSR`。
普通状态响应里的 `wifi.station_ip` 是 ESP32-C3 在局域网中的 IP；
`wifi.ap_ip` 是 SoftAP 地址。固件检测到 `ap_ip` 或 `station_ip` 变化时
会自动广播一次 `kind=state`。`kind=wifi_connecting` 只表示开发板已经开始
执行连接流程，最终是否入网以随后状态里的 `wifi.station_ip` 为准。

重置 WiFi 或 BLE：

```json
{"kind":"wifi_reset"}
{"kind":"ble_reset"}
```

设置并发射空调状态：

```json
{
  "kind": "ac_set",
  "protocol": "COOLIX48",
  "model": -1,
  "power": true,
  "mode": "cool",
  "degrees": 26,
  "celsius": true,
  "fanspeed": "auto",
  "swingv": "off",
  "swingh": "off",
  "quiet": false,
  "turbo": false,
  "econo": false,
  "light": false,
  "filter": false,
  "clean": false,
  "beep": false,
  "sleep": -1,
  "clock": -1,
  "command": 0,
  "ifeel": false,
  "sensor_temperature": -100.0
}
```

## JSON 响应

学习完成时广播：

```json
{
  "kind": "ir_rx",
  "learning": false,
  "frame": {
    "protocol": "COOLIX48",
    "bits": 48,
    "value_hex": "0xb24dbf40d02f",
    "rawlen": 101,
    "overflow": false
  },
  "ac": {
    "valid": true,
    "protocol": "COOLIX48",
    "model": -1,
    "power": true,
    "mode": "cool",
    "degrees": 26.0,
    "celsius": true,
    "fanspeed": "auto",
    "swingv": "off",
    "swingh": "off",
    "quiet": false,
    "turbo": false,
    "econo": false,
    "light": false,
    "filter": false,
    "clean": false,
    "beep": false,
    "sleep": -1,
    "clock": -1,
    "command": 0,
    "ifeel": false,
    "sensor_temperature": -100.0
  },
  "wifi": {
    "ap_ip": "192.168.4.1",
    "station_ip": "192.168.1.37"
  }
}
```

发射完成时返回 `kind=ir_tx`，普通查询返回 `kind=state`。固件只有在
`IRac::sendAc()` 成功后才提交新的空调状态；如果 `ac_set` 返回 `kind=error`，
后续 `state/get_state` 仍保持上一次已确认状态。

诊断响应示例：

```json
{
  "kind": "diag",
  "protocol_version": 1,
  "app_mode": "remote_controller",
  "transport": {
    "uart_baud": 115200,
    "tcp_port": 8765,
    "ble_service_index": 1,
    "ble_write_char_index": 5,
    "ble_notify_char_index": 6,
    "ble_notify_chunk_size": 20,
    "at": {
      "error_count": 0,
      "last_error_command": "",
      "last_result": "OK"
    }
  },
  "ir": {
    "rx_pin": "BSP_IO_PORT_00_PIN_05",
    "tx_pin": "BSP_IO_PORT_01_PIN_15",
    "recv_backend": "external_irq",
    "send_backend": "gpt_pwm",
    "recv_buffer_size": 1024,
    "recv_timeout_ms": 50
  },
  "wifi": {
    "ap_ip": "192.168.4.1",
    "station_ip": "192.168.1.37"
  }
}
```

## 实机联调顺序

1. 刷入 `Renesas_IRremote/build/App/Renesas_IRremote.srec`。
2. 确认 ESP32-C3-MINI 已烧录官方 ESP-AT 固件，串口为 `115200`，接线为
   RA4M2 `P601/RXD9 <- ESP TXD`、`P602/TXD9 -> ESP RXD`。
3. 手机或电脑连接 SoftAP `RA4M2_IR`，密码 `12345678`，在 App 的 WiFi
   页连接 `192.168.4.1:8765`。
4. 点 App 里的“诊断”，确认返回的 UART/TCP/BLE/IR 配置与本次烧录一致。
5. 在 BLE 页扫描并连接 `RA4M2_IR`；如果能连接但无响应，先核对诊断里的
   `ble_service_index`、`ble_write_char_index` 和 `ble_notify_char_index`
   是否匹配当前 ESP-AT GATT 表。
6. 点“学习”，用真实空调遥控器对准接收头发一帧，确认 App 的“学习栏位”
   变为“已保存”，并显示协议、温度、模式和风速等状态。
7. 点 App 遥控器上的温度、模式、风速、摆风等按键，确认每次按键后返回
   `kind=ir_tx`，并且真实空调响应。

## 构建

从固件项目目录构建 App 固件：

```sh
cd Renesas_IRremote
cmake --preset Debug
cmake --build --preset Debug
```

输出：

```text
Renesas_IRremote/build/App/Renesas_IRremote.elf
Renesas_IRremote/build/App/Renesas_IRremote.srec
```
