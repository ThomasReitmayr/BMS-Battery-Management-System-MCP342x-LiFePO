BMS-LiFePO Battery Management System MCP3424 or MCP3428
Battery Management System für LiFePO

This sketch controls a 4S LiFePO battery pack using an Arduino Nano, a voltage divider and 3 relays. The code is held simple and may be optimized and customized according to the required parameter settings und specs.
Single cell voltage measure.
The battery pack includes 3 relays switching load, charge and balancing based on threshold voltages.
Inculded libraries:
   Wire.h
   MCP342x.h
   U8g2lib.h

Details:
- Arduino Nano
- I2C bus for A/D converter and OLED
- DC Voltage measurement of each cell using MCP3424 or MCP3428 with 16 bit 4 Channel ADC (--> MCP342x.h)
- All 4 cell of the battery pack are monitored individually
- External voltage divider with 180k & 20k resistors 0.1% tolerance - if lower tolerance is used individual software implemented calibration per cell might be needed
- Battery pack is a LiFePO 4S
- Relays will be switched on and off based on voltage thresholds and hystersis
- Results are displayed on OLED 1.3" SH1106 (--> U8g2lib.h) including monitoring of
  * voltage per cell
  * battery pack voltage
  * status of relays (load, charge, balance)
- Controls 3 relays for battery pack control (load - charge - balance)
- Emergency shut down, if due to any failure min or max voltage is reached 
