#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "max7219.pio.h"

// PIN_DIN can be any pin, but PIN_CS and PIN_CLK must be sequential, and CS must be on the lower pin
#define PIN_DIN 19
#define PIN_CLK 18
#define PIN_CS 17
// update this to the number of modules you have chained
#define NUMBER_OF_CHAINED_MODULES 4 
// this should be fine for default clock, if you're having problems,  you can raise this number
// at default clock 5 should cause the output to be somewhere in the vicinity of 40mhz
#define SERIAL_CLK_DIV 5.f

void max7219_put_all(PIO pio, uint sm, uint16_t data)
{
    for(int i = 0; i < NUMBER_OF_CHAINED_MODULES; i++)
        max7219_put(pio, sm, data);
}
int main()
{
    stdio_init_all();

    // select which pio unit and state machine to run on
    PIO pio = pio0;
    uint sm = 0;

    // get a reference to the pio program itself
    uint offset = pio_add_program(pio, &max7219_program);

    // initialize the pio state machine with the program
    max7219_program_init(pio, sm, offset, NUMBER_OF_CHAINED_MODULES, PIN_DIN, PIN_CLK, PIN_CS, SERIAL_CLK_DIV);

    max7219_put_all(pio, sm, 0x0F00); // test mode
    max7219_put_all(pio, sm, 0x0B07); // 8 lines
    max7219_put_all(pio, sm, 0x0A03); // brightness = 15/32
    max7219_put_all(pio, sm, 0x0900); // disable 7-seg decoding
    max7219_put_all(pio, sm, 0x0C01); // turn on displays
    sleep_ms(1000);
    
    while (true) {
        // generate a toggling checkerboard pattern
        for(uint16_t i = 1; i <= 8; i++) {
            uint16_t i_offset = i << 8;
            uint16_t data = i % 2 == 0 ? 0x55 : 0xAA;
            uint16_t result = i_offset | data;
            max7219_put_all(pio, sm, result);
        }
        sleep_ms(200);
        for(uint16_t i = 1; i <= 8; i++) {
            uint16_t i_offset = i << 8;
            uint16_t data = i % 2 != 0 ? 0x55 : 0xAA;
            uint16_t result = i_offset | data;
            max7219_put_all(pio, sm, result);
        }
        sleep_ms(200);
    }

    return 0;
}
