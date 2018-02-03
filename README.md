# I2C Interrupt Watchdog Error - ESP-IDF Issue 1503

https://github.com/espressif/esp-idf/issues/1503

This short application demonstrates a crash in the I2C driver when the I2C bus is unstable.

In most cases, this results in a crash similar to:

```
Guru Meditation Error: Core  0 panic'ed (Interrupt wdt timeout on CPU0)
Register dump:
PC      : 0x40085cbf  PS      : 0x00060034  A0      : 0x800850cf  A1      : 0x3ffb0570  
0x40085cbf: vTaskExitCritical at /Users/david/esp32/esp-idf-v3.0/components/freertos/./tasks.c:4571

A2      : 0x00000001  A3      : 0x00000000  A4      : 0x00000002  A5      : 0x3ffb45b0  
A6      : 0x00000003  A7      : 0x00060923  A8      : 0x3ffb4784  A9      : 0x000000ff  
A10     : 0x00000000  A11     : 0x3ffb05b8  A12     : 0x00000004  A13     : 0x3ffb3450  
A14     : 0x00000001  A15     : 0x00000000  SAR     : 0x00000017  EXCCAUSE: 0x00000005  
EXCVADDR: 0x00000000  LBEG    : 0x4000c2e0  LEND    : 0x4000c2f6  LCOUNT  : 0xffffffff  

Backtrace: 0x40085cbf:0x3ffb0570 0x400850cc:0x3ffb0590 0x40083d42:0x3ffb05b0 0x40084149:0x3ffb05e0 0x400822a1:0x3ffb0610 0x400853b1:0x00000000
0x40085cbf: vTaskExitCritical at /Users/david/esp32/esp-idf-v3.0/components/freertos/./tasks.c:4571

0x400850cc: xQueueGenericSendFromISR at /Users/david/esp32/esp-idf-v3.0/components/freertos/./queue.c:2183

0x40083d42: i2c_master_cmd_begin_static at /Users/david/esp32/esp-idf-v3.0/components/driver/./i2c.c:1027

0x40084149: i2c_isr_handler_default at /Users/david/esp32/esp-idf-v3.0/components/driver/./i2c.c:1027

0x400822a1: _xt_lowint1 at /Users/david/esp32/esp-idf-v3.0/components/freertos/./xtensa_vectors.S:1105

0x400853b1: xQueueGenericReceive at /Users/david/esp32/esp-idf-v3.0/components/freertos/./queue.c:2183


Rebooting...
```

You can use `info line *0x40085cbf` to get the correct line number. In this case, line 1072 of esp-idf-v3.0/components/freertos/./tasks.c:

```
	configASSERT( xCoreID == tskNO_AFFINITY || xCoreID < portNUM_PROCESSORS);
```

However, it should be noted that the crash address seems to change each time. It is often within the I2C ISR code path, but typically not in
the same place twice in a row. It moves around a lot, and might suggest memory corruption.

It is also interesting to note that many times after the initial crash, the ESP32 will reboot but then crash immediately when calling `i2c_driver_install`. This occurs over and over until power is removed or the reset button on the board is pressed.

A workaround for this seems to be to add a call to `i2c_hw_fsm_reset(i2c_num)` at the beginning of `i2c_driver_install` so that any previous I2C crash caused by a frozen hardware state machine can be cleared. With this workaround in place, the ESP32 does not continually crash after the initial crash, but it will still crash the first time.


## How To Reproduce

Connect 20cm wires to pin 18 (SDA) and pin 19 (SCL). No external pull-ups are required.

Use `make menuconfig` to set appropriate serial configuration.

Compile this project against ESP-IDF (tested against branch `release/v3.0`, commit b6b8af498c8cfeb1b209e3035d0531564900952d).

Flash onto an ESP32 board with `make flash`.

Run the application and use `make monitor` to observe the log.

Once you see `app_main: about to run i2c_master_scan` start momentarily but repeatedly shorting SDA and SCL by brushing the ends of your two wires together repeatedly, to simulate a very poor condition I2C bus.

You should see output with random I2C addresses, for example:

```
I (5161) i2c: detected I2C address on master 0 at address 0x72
I (7011) i2c: detected I2C address on master 0 at address 0x14
I (8091) i2c: detected I2C address on master 0 at address 0x64
I (8231) i2c: detected I2C address on master 0 at address 0x07
...
```

After some time (up to a minute in my case), you should eventually witness a crash:

```
I (76531) i2c: detected I2C address on master 0 at address 0x58
I (76681) i2c: detected I2C address on master 0 at address 0x2c
I (76771) i2c: detected I2C address on master 0 at address 0x32
Guru Meditation Error: Core  0 panic'ed (Interrupt wdt timeout on CPU0)
Register dump:
PC      : 0x40085cbf  PS      : 0x00060034  A0      : 0x800850cf  A1      : 0x3ffb0570  
0x40085cbf: vTaskExitCritical at /Users/david/esp32/esp-idf-v3.0/components/freertos/./tasks.c:4571

A2      : 0x00000001  A3      : 0x00000000  A4      : 0x00000002  A5      : 0x3ffb45b0  
A6      : 0x00000003  A7      : 0x00060923  A8      : 0x3ffb4784  A9      : 0x000000ff  
A10     : 0x00000000  A11     : 0x3ffb05b8  A12     : 0x00000004  A13     : 0x3ffb3450  
A14     : 0x00000001  A15     : 0x00000000  SAR     : 0x00000017  EXCCAUSE: 0x00000005  
EXCVADDR: 0x00000000  LBEG    : 0x4000c2e0  LEND    : 0x4000c2f6  LCOUNT  : 0xffffffff  

Backtrace: 0x40085cbf:0x3ffb0570 0x400850cc:0x3ffb0590 0x40083d42:0x3ffb05b0 0x40084149:0x3ffb05e0 0x400822a1:0x3ffb0610 0x400853b1:0x00000000
0x40085cbf: vTaskExitCritical at /Users/david/esp32/esp-idf-v3.0/components/freertos/./tasks.c:4571

0x400850cc: xQueueGenericSendFromISR at /Users/david/esp32/esp-idf-v3.0/components/freertos/./queue.c:2183

0x40083d42: i2c_master_cmd_begin_static at /Users/david/esp32/esp-idf-v3.0/components/driver/./i2c.c:1027

0x40084149: i2c_isr_handler_default at /Users/david/esp32/esp-idf-v3.0/components/driver/./i2c.c:1027

0x400822a1: _xt_lowint1 at /Users/david/esp32/esp-idf-v3.0/components/freertos/./xtensa_vectors.S:1105

0x400853b1: xQueueGenericReceive at /Users/david/esp32/esp-idf-v3.0/components/freertos/./queue.c:2183


Rebooting...
```

Without the crash-loop workaround, your ESP32 will probably crash again, and again, at this point:

```
W (322) app_main: about to run i2c_master_init
D (2332) i2c: i2c_master_init
W (2332) i2c: about to run i2c_param_config
W (2332) i2c: about to run i2c_driver_install
Guru Meditation Error: Core  0 panic'ed (Interrupt wdt timeout on CPU0)
Register dump:
PC      : 0x40084137  PS      : 0x00060034  A0      : 0x400822a4  A1      : 0x3ffb05e0  
0x40084137: i2c_isr_handler_default at /Users/david/esp32/esp-idf-v3.0/components/driver/./i2c.c:1027

0x400822a4: _xt_lowint1 at /Users/david/esp32/esp-idf-v3.0/components/freertos/./xtensa_vectors.S:1105

A2      : 0x3ffb3338  A3      : 0x3ff53000  A4      : 0x00000500  A5      : 0x00000000  
A6      : 0x00000000  A7      : 0x3ffb5ea8  A8      : 0x3ffb3338  A9      : 0x00000003  
A10     : 0x00000000  A11     : 0x00000001  A12     : 0x80087c68  A13     : 0x3ffb4560  
A14     : 0x00000001  A15     : 0x3ffb377d  SAR     : 0x0000001f  EXCCAUSE: 0x00000005  
EXCVADDR: 0x00000000  LBEG    : 0x4000c2e0  LEND    : 0x4000c2f6  LCOUNT  : 0xffffffff  

Backtrace: 0x40084137:0x3ffb05e0 0x400822a1:0x3ffb0610 0x4000bfed:0x00000000
0x40084137: i2c_isr_handler_default at /Users/david/esp32/esp-idf-v3.0/components/driver/./i2c.c:1027

0x400822a1: _xt_lowint1 at /Users/david/esp32/esp-idf-v3.0/components/freertos/./xtensa_vectors.S:1105


Rebooting...
```

Note that this crash occurs before the code reaches the "Scan" function, so it is an earlier crash.

## Boards

I have been able to reproduce this on two ESP32 boards:

* "DOIT" board from [AliExpress](https://www.aliexpress.com/item/ESP-32-ESP-32S-Development-Board-WiFi-Bluetooth-Ultra-Low-Power-Consumption-Dual-Cores-ESP32-Board/32797883648.html)
* Wemos LoLin32 Lite from [AliExpress](https://www.aliexpress.com/item/WEMOS-LOLIN32-Lite-V1-0-0-wifi-bluetooth-board-based-ESP-32-esp32-Rev1-MicroPython-4MB/32831394824.html)

The issue is much easier to reproduce with the DOIT board, and often occurs within a minute of repeatedly shorting SDA and SCL.

The LoLin32 Lite board seems much more stable in this regard and it is more difficult to get the issue to occur. I found it effective to put a [GY-2561](https://www.aliexpress.com/item/Free-Shipping-1pcs-GY-2561-TSL2561-Luminosity-Sensor-Breakout-infrared-Light-Sensor-module-integrating-sensor-AL/32640401005.html) on the end of a 5 metre Ethernet cable (SDA and SCL paired with ground) and attach this to the ESP32's pin 18 and 19 as normal.



