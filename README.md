# To active CLK0 output from avr 

Since MCP2515 has no crystal on board, clock is generated by AVR core
```bash
avrdude -c usbasp -p m328p -U lfuse:w:0xB7:m
```

http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=B7&HIGH=DA&EXTENDED=FD&LOCKBIT=FF  