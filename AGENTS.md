# Renesas_IRremote Agent Handoff

本文档是给后续 GPT/Codex 会话读取的项目交接说明。当前项目已经完成红外接收调试和 IRremoteESP8266 适配，后续计划是在此基础上继续做 WiFi、蓝牙，以及手机 App 与当前项目通信。

## 项目位置

- RA 工程目录：`/home/zheng/Github/ir_learning/workspace/Renesas_IRremote`
- 当前 pyOCD 配置：`Renesas_IRremote/pyocd.yaml`
- 当前 Renesas pack：`Renesas_IRremote/Renesas.RA_DFP.6.4.0.pack`
- 上游红外库外部依赖：`Renesas_IRremote/third_party/IRremoteESP8266`

`third_party/IRremoteESP8266` 应由用户 clone，不要复制上游库源码进本仓库。该库是 GNU LGPL v2.1，项目当前做法是通过本地 `third_party` checkout 使用它。

## 重要协作偏好

- 用户使用中文沟通。
- 如果确实需要改 FSP 配置，要明确告诉用户改哪里，不要在代码里绕开 FSP 配置问题。
- 不要擅自覆盖 RASC/FSP 生成内容，除非用户明确要求。
- 不要修改 `third_party/IRremoteESP8266` 内的上游源码；需要兼容逻辑时优先写在本项目 RA 适配层或应用层。
- 用户已经用 `uv` 创建过 venv 并安装 pyOCD；激活方式通常是 `source .venv/bin/activate`，但当前 pyOCD 配置文件在 `Renesas_IRremote` 目录内。

## 当前硬件/FSP 事实

- MCU/工程：Renesas RA4M2，FSP/RASC 生成的 CMake 工程。
- 主晶振实际是 12 MHz。之前 FSP 配成 24 MHz 导致红外 raw timing 正好减半，用户已经改正。
- 子时钟曾引起问题，当前调试过程中已关闭。
- heap 和 main stack 曾因 IRremoteESP8266 动态分配不足导致 `_exit(1)`，后续已设到 `0x2000` 级别。调试接收版目前能链接和运行。
- 当前红外 RX demo 默认引脚：`BSP_IO_PORT_00_PIN_05`。
- 当前红外 TX demo 默认引脚：`BSP_IO_PORT_00_PIN_05`。
- 用户说发送脚在 FSP 中有 GPT 定时器配置，但当前 `IRsend` RA port 仍是软件 GPIO 翻转发送，不使用 GPT PWM。
- 当前接收不是 FSP ICU/GPIO 边沿中断，而是 SysTick 50 us 采样 ISR。若后续 WiFi/BLE/RTOS 占用 SysTick，需要重新设计接收后端。

## 外部依赖

如果 `third_party/IRremoteESP8266` 不存在，先执行：

```sh
cd /home/zheng/Github/ir_learning/workspace/Renesas_IRremote
mkdir -p third_party
git clone https://github.com/crankyoldgit/IRremoteESP8266.git third_party/IRremoteESP8266
```

CMake 支持通过 `-DIRREMOTEESP8266_DIR=/path/to/IRremoteESP8266` 指向其他 checkout。

## 构建方式

删掉 `build` 后重新配置时，一定要显式带上 ARM toolchain file。否则 CMake 会选 `/usr/bin/cc`，然后因为不认识 `-mthumb/-mfloat-abi/-mfpu` 报错。

从 workspace 根目录构建当前 App 固件调试版：

```sh
cd /home/zheng/Github/ir_learning/workspace

cmake -S Renesas_IRremote -B Renesas_IRremote/build/App -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/Renesas_IRremote/cmake/gcc.cmake \
  -DARM_TOOLCHAIN_PATH=/usr/bin \
  -DCMAKE_BUILD_TYPE=Debug \
  -DIRREMOTE_RA4M2_RECV_DEMO=OFF \
  -DIRREMOTE_RA4M2_SEND_DEMO=OFF \
  -DIRREMOTE_RA4M2_USE_IRQ_RECV=ON \
  -DIRREMOTE_RA4M2_USE_SYSTICK_RECV=OFF \
  -DIRREMOTE_RA4M2_USE_GPT_SEND=ON \
  -DIRREMOTE_RA4M2_RECV_BUFFER_SIZE=1024 \
  -DIRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE=1024 \
  -DAPP_IR_RX_PIN=BSP_IO_PORT_00_PIN_05 \
  -DAPP_IR_TX_PIN=BSP_IO_PORT_01_PIN_15

cmake --build Renesas_IRremote/build/App -- -j2
```

配置成功时构建命令应使用：

```text
/usr/bin/arm-none-eabi-gcc
/usr/bin/arm-none-eabi-g++
```

不应使用：

```text
/usr/bin/cc
/usr/bin/c++
```

## CMake/协议配置现状

主要配置在 `CMakeLists.txt`：

- `IRREMOTEESP8266_DIR` 默认指向 `third_party/IRremoteESP8266`。
- `IRREMOTE_RA4M2_RECV_DEMO` 默认 OFF，接收调试时设 ON。
- `IRREMOTE_RA4M2_SEND_DEMO` 默认 OFF。
- `IRREMOTE_RA4M2_USE_SYSTICK_RECV` 默认 ON。
- `IRREMOTE_RA4M2_RECV_SYSTICK_US` 默认 `50`。
- `IRREMOTE_RA4M2_RECV_BUFFER_SIZE` 默认 `1024`，为空调长码保留空间。
- `IRREMOTE_RA4M2_RECV_TIMEOUT_MS` 默认 `50`。
- `IRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE` 默认 `1024`，用于 GDB raw dump。
- 几乎所有 IRremoteESP8266 send/decode 协议宏都显式启用，`_IR_ENABLE_DEFAULT_` 保持 false。
- `IRac.cpp` 已加入构建，用于 `IRAcUtils::decodeToState()` 统一空调状态解析。

当前全协议调试版体积大约为：

```text
text ~= 104 KB
bss  ~= 19 KB
```

这是调试版配置。后续做生产固件时，应按需要裁剪协议集合、raw dump、demo globals。

## 代码结构

关键文件：

- `src/hal_entry.cpp`
  - 包含 send demo 和 receive demo。
  - 接收 demo 会更新一组 `volatile` GDB 调试变量。
  - 调用 `IRAcUtils::decodeToState()` 将支持的空调协议转成 `stdAc::state_t`。
  - 本项目额外为 `COOLIX48` 做了本地语义兼容：用 `IRCoolixAC::setRawFromCoolix48()` 转成通用空调状态，因为上游 `decodeToState()` 当前没有直接处理 `COOLIX48`。
  - 维护上一帧空调状态，用于 Coolix/Coolix48 这类 toggle 命令解析。

- `src/ir_ra4m2/Arduino.cpp`
  - 提供 IRremoteESP8266 需要的 Arduino 兼容接口：
    - `pinMode`
    - `digitalWrite`
    - `digitalRead`
    - `micros`
    - `millis`
    - `delayMicroseconds`
    - `delay`
    - `yield`
  - `micros()` 基于 DWT cycle counter 和 `SystemCoreClock`。
  - pin 参数是 FSP pin token，不是 Arduino 数字引脚。

- `src/ir_ra4m2/IRrecv_RA4M2.cpp`
  - RA4M2 的 IRrecv 后端。
  - 默认使用 SysTick，每 50 us 采样一次红外接收脚。
  - active level 默认 `LOW`，适配常见红外接收头的低电平有效输出。
  - 捕获 raw timing 后复用 IRremoteESP8266 的协议 decode 顺序。
  - `DECODE_HASH` 兜底未知协议。

- `src/ir_ra4m2/newlib_stubs.c`
  - newlib stub。早期 heap 不足时会进入 `_exit(1)` 并 `bkpt`，这曾被 GDB watchpoint 误以为接收触发。

## 当前调试变量

基础协议结果：

- `g_irremote_decode_count`
- `g_irremote_last_decode_type`
- `g_irremote_last_value`
- `g_irremote_last_bits`
- `g_irremote_last_rawlen`
- `g_irremote_last_overflow`
- `g_irremote_last_raw_dump_len`
- `g_irremote_last_rawbuf`

空调语义结果：

- `g_irremote_ac_state_valid`
- `g_irremote_ac_protocol`
- `g_irremote_ac_model`
- `g_irremote_ac_power`
- `g_irremote_ac_mode`
- `g_irremote_ac_degrees`
- `g_irremote_ac_celsius`
- `g_irremote_ac_fanspeed`
- `g_irremote_ac_swingv`
- `g_irremote_ac_swingh`
- `g_irremote_ac_quiet`
- `g_irremote_ac_turbo`
- `g_irremote_ac_econo`
- `g_irremote_ac_light`
- `g_irremote_ac_filter`
- `g_irremote_ac_clean`
- `g_irremote_ac_beep`
- `g_irremote_ac_sleep`
- `g_irremote_ac_clock`
- `g_irremote_ac_command`
- `g_irremote_ac_ifeel`
- `g_irremote_ac_sensor_temperature`

枚举含义：

- `mode`: `-1 off`, `0 auto`, `1 cool`, `2 heat`, `3 dry`, `4 fan`
- `fanspeed`: `0 auto`, `1 min`, `2 low`, `3 medium`, `4 high`, `5 max`, `6 mediumHigh`
- `swingv`: `-1 off`, `0 auto`, `1 highest`, `2 high`, `3 middle`, `4 low`, `5 lowest`, `6 upperMiddle`
- `swingh`: `-1 off`, `0 auto`, `1 leftMax`, `2 left`, `3 middle`, `4 right`, `5 rightMax`, `6 wide`
- `sensor_temperature = -100` 表示没有传感器温度字段。

判断层次：

- `g_irremote_last_decode_type != UNKNOWN`：协议层识别成功。
- `g_irremote_ac_state_valid == true`：还能解析成空调通用状态。
- 非空调遥控器没有温度/模式/风速字段，`g_irremote_ac_state_valid == false` 是正常结果。

## pyOCD/GDB 手动调试流程

从 `Renesas_IRremote` 目录启动 pyOCD：

```sh
cd /home/zheng/Github/ir_learning/workspace/Renesas_IRremote
source .venv/bin/activate

pyocd gdbserver --config pyocd.yaml \
  --elf build/App/Renesas_IRremote.elf \
  --persist -M halt
```

另一个终端启动 GDB：

```sh
cd /home/zheng/Github/ir_learning/workspace/Renesas_IRremote
arm-none-eabi-gdb build/App/Renesas_IRremote.elf
```

GDB 内：

```gdb
target remote localhost:3333
monitor reset halt
load
tbreak hal_entry
continue
set var g_irremote_decode_count = 0
watch g_irremote_decode_count
continue
```

看到 `Continuing.` 后按遥控器。收到并成功 decode 一帧后 watchpoint 会停住，通常栈位置可能在 `IRrecv::resume()` 附近，这是正常的。

打印基础结果：

```gdb
p g_irremote_decode_count
p g_irremote_last_decode_type
p/x g_irremote_last_value
p g_irremote_last_bits
p g_irremote_last_rawlen
p g_irremote_last_overflow
```

打印空调语义：

```gdb
p g_irremote_ac_state_valid
p g_irremote_ac_protocol
p g_irremote_ac_power
p g_irremote_ac_mode
p g_irremote_ac_degrees
p g_irremote_ac_celsius
p g_irremote_ac_fanspeed
p g_irremote_ac_swingv
p g_irremote_ac_swingh
p g_irremote_ac_turbo
p g_irremote_ac_light
p g_irremote_ac_sleep
p g_irremote_ac_ifeel
p g_irremote_ac_sensor_temperature
```

打印 raw：

```gdb
set print elements 0
p g_irremote_last_raw_dump_len
p g_irremote_last_rawbuf[0]@g_irremote_last_raw_dump_len
```

继续测下一个按键：

```gdb
continue
```

`monitor reset halt` 的含义：通过 GDB 的 `monitor` 命令让 pyOCD 复位目标 MCU，并在复位后立刻 halt CPU。它用于建立干净调试现场，然后再 `load`、设断点、运行。

`watch g_irremote_decode_count` 的含义：设置数据 watchpoint。只要 decode 成功后代码执行 `g_irremote_decode_count++`，CPU 就停住。

## 已验证的重要解码现象

用户的旧“美的”空调遥控器曾解码成 `COOLIX48`，不是 `MIDEA` 或 `GREE`。这是合理的，不代表接收错误：

- raw header 约 `4300 us, 4450 us`，符合 Coolix/Coolix48。
- 值例子：`0xb24dbf40d02f`。
- byte 对为 `b2 4d bf 40 d0 2f`，每组是 byte 和反码：
  - `0xb2 ^ 0xff = 0x4d`
  - `0xbf ^ 0xff = 0x40`
  - `0xd0 ^ 0xff = 0x2f`
- 这符合 Coolix48 的编码。
- 它不是严格 Gree，因为 Gree header 约 `9000 us, 4500 us`，当前观测首 mark 约一半。
- 它也不是严格 Midea 48-bit 模式，因为 Midea 期望两段完整 48-bit 及特定反码/校验关系。

结论：品牌外壳和协议名不一定一致。后续 App 不应强依赖“品牌名”等于协议名，应展示或使用抽象空调状态。

## VSCode 注意事项

`.vscode/settings.json` 中当前 `cmake.useCMakePresets` 是 `never`，因此 VSCode CMake Tools 不会自动使用 `CMakePresets.json`。若从 VSCode 构建，要确保 CMake Tools 配置阶段包含：

```json
"cmake.configureSettings": {
  "CMAKE_TOOLCHAIN_FILE": "${workspaceFolder}/cmake/gcc.cmake",
  "ARM_TOOLCHAIN_PATH": "/usr/bin",
  "IRREMOTE_RA4M2_RECV_DEMO": "ON",
  "IRREMOTE_RA4M2_SEND_DEMO": "OFF",
  "IRREMOTE_RA4M2_RECV_BUFFER_SIZE": "1024",
  "IRREMOTE_RA4M2_DEBUG_RAW_DUMP_SIZE": "1024"
}
```

如果 VSCode 打开的是上一级 `workspace`，则 `${workspaceFolder}` 路径不同，需改成 `${workspaceFolder}/Renesas_IRremote/...`。

Cortex-Debug 的 Debug Console 中当前不需要加 `-exec`。直接输入 GDB 命令，例如：

```gdb
monitor reset halt
load
tbreak hal_entry
continue
```

## 后续 WiFi/BLE/App 集成建议

当前 `hal_entry.cpp` 是 demo 风格，不适合直接继续堆 WiFi/BLE/App 业务。建议下一阶段先把红外功能抽出稳定模块：

- 新建红外服务层，例如 `src/ir_ra4m2/ir_service.*` 或 `src/app/ir_service.*`。
- 用结构体封装最后一次接收结果，而不是让 App/WiFi/BLE 直接读一堆 GDB demo globals。
- 保留两个层次的数据：
  - 协议层：`decode_type`, `value`, `bits`, `rawlen`, `overflow`, optional raw timings。
  - 空调语义层：`valid`, `protocol`, `power`, `mode`, `degrees`, `fanspeed`, `swingv`, `swingh`, `turbo`, `light`, `sleep`, 等。
- App 协议不要只传品牌名。应传协议名/枚举加通用空调状态。
- 接收事件可以做成队列或单槽 latest-state，供 WiFi/BLE transport 拉取。
- 发送命令建议也走通用空调状态，尽量使用 IRremoteESP8266 的 `IRac`/各协议 class 生成 IR 帧。
- 如果 WiFi/BLE stack 使用 RTOS 或占用 SysTick，当前 SysTick 接收后端会冲突。届时应改成：
  - FSP ICU 外部中断捕获接收脚边沿；
  - GPT/AGT 或 DWT 记录边沿时间；
  - 在非 ISR 上下文调用 decode；
  - ISR 只写 ring buffer/raw timing，避免重活。
- 如果发送要提高稳定性，后续可改 `IRsend` RA backend 使用 GPT PWM 产生 38 kHz carrier；当前是软件延时翻转 GPIO。

建议给手机 App 的状态 JSON 形状：

```json
{
  "kind": "ir_rx",
  "protocol": "COOLIX48",
  "bits": 48,
  "value_hex": "0xb24dbf40d02f",
  "ac": {
    "valid": true,
    "power": true,
    "mode": "cool",
    "degrees": 26,
    "celsius": true,
    "fanspeed": "auto",
    "swingv": "off",
    "swingh": "off",
    "turbo": false,
    "light": false,
    "sleep": -1
  }
}
```

建议 App 下发命令时使用 transport-agnostic schema，例如：

```json
{
  "kind": "ac_set",
  "protocol": "COOLIX48",
  "power": true,
  "mode": "cool",
  "degrees": 26,
  "celsius": true,
  "fanspeed": "auto",
  "swingv": "off"
}
```

WiFi/BLE 只负责传输和鉴权，红外协议生成/解析应保持在 IR service 内。

## 常见坑

- 如果 raw timing 全部约为期望值的一半，优先检查 FSP 主晶振/clock 配置。
- 如果程序停在 `_exit(1)` 或 newlib stub 的 `bkpt`，优先检查 heap/stack 是否不足。
- 如果 `g_irremote_last_overflow == true`，增大 `IRREMOTE_RA4M2_RECV_BUFFER_SIZE`。
- 如果 `g_irremote_ac_state_valid == false` 但 `g_irremote_last_decode_type` 有值，不一定是错误；该协议可能不是空调协议，或上游没有语义转换。
- 如果 CMake 用 `/usr/bin/cc` 构建，说明 toolchain file 没有生效。
- 不要把接收协议名当成真实品牌名；旧遥控器和 OEM 协议经常不一致。
