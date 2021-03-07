# deej_controller

Custom controller for use with IcyRespawn's deej software. Reads four rotary encoders with edge-driven interrupts to increment/decrement volume per channel. Volume data is sent over serial to the FT230X which appears as a serial port on the host PC. 

Known issues:
* FT230X and MCP1700 both output 3.3V onto the same rail. Need to separate to prevent voltage mis-match and resultant backwards current flow. 
* 5.3x10.2 SOIC16W footprints for SN74HC595N shift registers are too wide - Need to switch to SOIC16
* LED Bargraph footprints are too wide - Need to reduce row-to-row spacing, must likely by 2.54mm or some multiple
