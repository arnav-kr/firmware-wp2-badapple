# Hardware port

This is a reconstructed board description from the stock application and live
register captures, not the original devicetree or a PCB schematic. Zephyr's
devicetree/configuration was compiled into driver structures in stock.

| Device | Port detail |
|---|---|
| Display | CO5300/JX07003 driver; 466x466, RGB565/BGR configuration, 48 MHz / 55 Hz baseline |
| Panel power/reset | GPIO A4 power, B0 reset; physical levels deliberately used |
| Touch | CST820, I2C0; A5 power, A26 reset, A27 IRQ |
| Crown rotation | MT3511, I2C2 address 0x75; stock-derived initialization table |
| Battery/charger | Internal PMU charger and BATV ADC; reconstructed stock profile |
| Speaker | DAC/PMUDAC route, external PA/rail setup; stock-matching 16 kHz FIR mode |

The crown's first register 0x06 reset write can return -EIO. The driver tolerates
that known step, applies the stock-derived table and clears the wake bit.
Displacement accumulates to 80 counts per volume step, with direction and limits
covered by host tests. No crown calibration values are written to NVRAM.

## Panel lifecycle correction

The failed v12 path differed from compiled stock in its reset waveform and
shutdown waits. `drivers/display/panel_stock_sequence.h` holds the sequence
shared by the driver and host tests:

1. Hold reset low, enable A4 power, wait 20 ms.
2. Drive B0 high / low / high, with a 20 ms wait after each transition.
3. Sleep: DCS 0x28, wait 20 ms; DCS 0x10, wait 120 ms.
4. Common shutdown waits 100 ms, then reset low / wait 20 ms, power low / wait 20 ms.

Brightness initialization uses the pending runtime value. Lifecycle, TE watchdog,
ESD reset and transfer counters distinguish recovery paths in shell status.
The command/config comparison found the normal stock and custom display init
tables already matched. The initial white/black failure trigger was not captured;
these lifecycle corrections are justified, but do not establish its exact cause.

`board/wp2/stock-profile.json` records the reconstructed charger settings and
stock source identity. Drivers retain diagnostic probes useful for further
validation. No electrical netlist or universal recovery guarantee is claimed.
