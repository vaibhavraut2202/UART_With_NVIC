# UART_With_NVIC — STM32L4 Interrupt-Driven UART Echo

Interrupt-driven UART example for **STM32L4R5ZIT6** using STM32 HAL + NVIC. Receives bytes on USART2 via 1-byte RX interrupts, buffers them, and echoes back 1024 bytes at a time via chained TX interrupts. Also blinks an LED on PB7.

## Hardware / MCU

- MCU: `STM32L4R5ZIT6`, LQFP144, Cortex-M4
- CubeMX: 6.12.0, Firmware `STM32Cube FW_L4 V1.18.1`
- Toolchain: `MDK-ARM V5.32` (Keil uVision)
- Clock: MSI 4 MHz (`RCC_MSIRANGE_6`), no PLL, HCLK/SYSCLK = MSI, APB1/APB2 DIV1, Flash latency 0
- UART instance: `USART2` on `PA2 (TX)` / `PA3 (RX)`, AF7, very-high speed
- LED: `PB7` push-pull output, toggled in main loop with 500 ms delays
- Debug: Keil `DebugConfig/UART_With_NVIC_STM32L4R5ZITx.dbgconf`

## UART / NVIC Configuration

From `UART_With_NVIC.ioc` and `Core/Src/stm32l4xx_hal_msp.c`:

```
huart2.Instance = USART2
BaudRate = 300  // note: code says 300; //9600; //115200;
WordLength = 8B, StopBits = 1, Parity = None
Mode = TX_RX, HwFlowCtl = None
OverSampling = 16, OneBitSampling = Disable, Prescaler = DIV1
Clock source: PCLK1
NVIC: USART2_IRQn enabled, priority 0,0, PriorityGroup 4
SysTick_IRQn priority 15
```

> Change `huart2.Init.BaudRate` in `Core/Src/main.c:MX_USART2_UART_Init()` to 115200 to match a normal terminal. Both sides must use the same baud.

## Repository Layout

```
UART_With_NVIC - Original/
  UART_With_NVIC.ioc        CubeMX source of truth
  .mxproject
  Core/
    Inc/main.h, stm32l4xx_it.h, stm32l4xx_hal_conf.h
    Src/main.c              app entry, UART init, RX/TX callbacks, LED
    Src/stm32l4xx_it.c      USART2_IRQHandler -> HAL_UART_IRQHandler
    Src/stm32l4xx_hal_msp.c GPIO + NVIC setup for USART2
    Src/system_stm32l4xx.c
  Application/
    UART_Learning.h/.c      CyclicBuffer + rxBuffer/txBuffer globals
  Drivers/                  STM32L4 HAL + CMSIS (Cube-generated, do not edit)
  MDK-ARM/
    UART_With_NVIC.uvprojx/uvoptx
    startup_stm32l4r5xx.s
    UART_With_NVIC/         build outputs (.o/.d/.axf/.hex/.map/.htm)
```

## How It Works

1. `main()` in `Core/Src/main.c:61`:
   `HAL_Init()` → `SystemClock_Config()` → `MX_GPIO_Init()` → `MX_USART2_UART_Init()` → blocking transmit `"hello vaibhav\r\n"` → arm `HAL_UART_Receive_IT(&huart2, rxBuffer.buffer, 1)` → `while(1){Blink_LED2(1);}`.

2. Custom buffers in `Application/UART_Learning.h`:
   ```c
   #define BUFFER_SIZE 1024
   typedef struct { uint8_t buffer[1024]; volatile uint16_t head, tail; } CyclicBuffer;
   extern CyclicBuffer rxBuffer, txBuffer;
   ```
   Simple ring buffer with overwrite protection (drops on full). `head==tail` means empty; `Read` on empty returns 0.

3. RX path `HAL_UART_RxCpltCallback()` in `Core/Src/main.c:203`:
   - Reads 1 byte from `rxBuffer.buffer[0]`
   - Appends to `AccumulationBuffer[1024]` via `AccumulationIndex++`
   - When 1024 bytes accumulated: copies all 1024 into `txBuffer` via `CyclicBuffer_Write()`, resets index, kicks `HAL_UART_Transmit_IT(1 byte)` if `UART_FLAG_TXE` set
   - Re-arms `HAL_UART_Receive_IT(..., 1)` every byte

4. TX path `HAL_UART_TxCpltCallback()` in `Core/Src/main.c:233`:
   - While `head != tail`, reads one byte and calls `HAL_UART_Transmit_IT(1 byte)` again — chained 1-byte interrupt transmission until buffer empties.

5. Interrupt routing `Core/Src/stm32l4xx_it.c:204`:
   `USART2_IRQHandler()` → `HAL_UART_IRQHandler(&huart2)` → HAL dispatches to Rx/Tx complete callbacks.

Net effect: board prints greeting once, then echoes back in 1024-byte blocks. Send <1024 bytes and nothing echoes yet — by design.

## Build / Flash

1. Open `MDK-ARM/UART_With_NVIC.uvprojx` in Keil uVision 5.32+
2. Select target `UART_With_NVIC`, Build (F7)
3. Flash via ST-Link with `UART_With_NVIC_STM32L4R5ZITx.dbgconf` settings
4. Outputs: `MDK-ARM/UART_With_NVIC/UART_With_NVIC.axf/.hex/.map`

Or regenerate with CubeMX: open `UART_With_NVIC.ioc` → Generate Code → open in Keil.

## Test With Serial Terminal

1. Connect USB-TTL to PA2/PA3 (cross TX/RX) + GND, or ST-Link VCP if routed to USART2
2. Open Tera Term / PuTTY / RealTerm at **300 baud** (or whatever you set), 8N1, no flow
3. Reset board → expect `hello vaibhav`
4. Send exactly 1024 characters → expect same 1024 back
5. PB7 LED blinks continuously independent of UART

## Customization

- Baud: `Core/Src/main.c:156`
- Block size: `BUFFER_SIZE` in `Core/Src/main.c:36` and `Application/UART_Learning.h:6` (keep in sync)
- LED pin: `MX_GPIO_Init()` + `Blink_LED2()` in `Core/Src/main.c:28`
- Make echo immediate: call TX logic per byte instead of waiting for 1024

## Known Quirks / TODO

- `HAL_UART_Receive_IT()` is called twice at startup (once in `main()` USER CODE 2, once at end of `MX_USART2_UART_Init()`); harmless but redundant.
- `rxBuffer` is misused as a 1-byte mailbox (`rxBuffer.buffer[0]`) rather than a true ring — `head/tail` of `rxBuffer` are never advanced.
- `AccumulationBuffer` + `txBuffer` double-buffering wastes RAM; could write directly to `txBuffer` per byte.
- No overrun/error handling (`HAL_UART_ErrorCallback` not implemented).
- `Blink_LED2(char)` ignores its argument; blocking `HAL_Delay()` stalls nothing else here but blocks low-power use.
- Build artifacts (`.o/.axf/.hex/.map`) are committed — consider adding `.gitignore` for `MDK-ARM/UART_With_NVIC/`.
