#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "max7219.pio.h"

// PIN_DIN can be any pin, but PIN_CS and PIN_CLK must be sequential, and CS must be on the lower pin
#define PIN_DIN 19
#define PIN_CLK 18
#define PIN_CS 17

// update this to the number of modules you have chained
#define NUMBER_OF_CHAINED_MODULES 4 

// this should be fine for default clock, if you're having problems,  you can raise this number
// at default clock 5. should cause the output to be somewhere in the vicinity of 8mhz
// anything below 5 tends to glitch
#define SERIAL_CLK_DIV 6.f

// this function is useful to directly write out a command to all modules
// does not use DMA
void max7219_put_all(PIO pio, uint sm, uint16_t data)
{
    for(int i = 0; i < NUMBER_OF_CHAINED_MODULES; i++)
        max7219_put(pio, sm, data);
}

// somewhat decently optimized conversion of a uint8_t array to commands
// command_row specifies which register to write to on the modules (which row)
void max7219_buffer_row(uint8_t *row, uint16_t *commands, uint len, uint16_t command_row) {
    uint command_high = command_row << 8;
    for(uint i = 0; i < len; i++) {
        commands[i] = row[i] | command_high;
    }
}

// somewhat decently optimized conversion of a uint8_t framebuffer to commands
// row_len is the number of modules in a row, num_rows is the number of rows to convert
// (e.g. row_len is the number of pixels wide / 8, num_rows is the number of pixels tall)
void max7219_buffer_frame(uint8_t *frame, uint16_t *commands, uint row_len, uint num_rows) {
    uint offset = 0;
    for(uint16_t i = 1; i <= num_rows; i++) {
        max7219_buffer_row(frame + offset, commands + offset, row_len, i);
        offset += row_len;
    }
}

// dumps a buffer of commands to printf, with similar signature to buffer_frame
void max7219_dump_buffer(uint16_t *buf, uint row_len, uint row_count) {
    uint i = 0;
    for(uint row = 0; row < row_count; row++) {
        printf("%d", row);
        for(uint col = 0; col < row_len; col++) {
            printf(" %#06x", buf[i]);
            i++;
        }
        printf("\n");
    }
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

    // get an unused DMA channel
    int chan = dma_claim_unused_channel(true);

    // configure the DMA channel
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);

    ////////////// INITIALIZATION VIA DMA [START]
    uint16_t src[20] = {
        0x0F00, 0x0F00, 0x0F00, 0x0F00, // test mode
        0x0B07, 0x0B07, 0x0B07, 0x0B07, // 8 lines
        0x0A03, 0x0A03, 0x0A03, 0x0A03, // brightness = 15/32
        0x0900, 0x0900, 0x0900, 0x0900, // disable 7-seg decoding
        0x0C01, 0x0C01, 0x0C01, 0x0C01  // turn on displays
    };

    // dump initialization data to serial
    // printf("\n\ninitialization:\n");
    // max7219_dump_buffer(src, 4, 5);

    // DMA the initialization to the PIO
    dma_channel_configure(chan, &c, &pio->txf[sm], src, 20, true);
    // wait for it to finish
    dma_channel_wait_for_finish_blocking(chan);
    ////////////// INITIALIZATION VIA DMA [END]

    ////////////// INITIALIZATION VIA DIRECT WRITES [START]
    /*
    max7219_put_all(pio, sm, 0x0F00); // test mode
    max7219_put_all(pio, sm, 0x0B07); // 8 lines
    max7219_put_all(pio, sm, 0x0A03); // brightness = 15/32
    max7219_put_all(pio, sm, 0x0900); // disable 7-seg decoding
    max7219_put_all(pio, sm, 0x0C01); // turn on displays
    */
    ////////////// INITIALIZATION VIA DIRECT WRITES [END]

    // sleep after init (not really necessary)
    sleep_ms(1000);


    ////////////// ANIMATE FROM FRAMEBUFFER VIA DMA [START]
    // store a couple of frames of animation in memory
    uint8_t frame1[32] = {
        0b00111000, 0b00111000, 0b00111000, 0b00111000,
        0b01000100, 0b01000100, 0b01000100, 0b01000100,
        0b10101010, 0b10101010, 0b10101010, 0b10101010,
        0b10000010, 0b10000010, 0b10000010, 0b10000010,
        0b10101010, 0b10010010, 0b10101010, 0b10010010,
        0b10010010, 0b10101010, 0b10010010, 0b10101010,
        0b01000100, 0b01000100, 0b01000100, 0b01000100,
        0b00111000, 0b00111000, 0b00111000, 0b00111000
    };
    uint8_t frame2[32] = {
        0b00111000, 0b00111000, 0b00111000, 0b00111000,
        0b01000100, 0b01000100, 0b01000100, 0b01000100,
        0b10101010, 0b10101010, 0b10101010, 0b10101010,
        0b10000010, 0b10000010, 0b10000010, 0b10000010,
        0b10010010, 0b10101010, 0b10010010, 0b10101010,
        0b10101010, 0b10010010, 0b10101010, 0b10010010,
        0b01000100, 0b01000100, 0b01000100, 0b01000100,
        0b00111000, 0b00111000, 0b00111000, 0b00111000
    };

    // allocate memory for command buffer (one 16bit command is required for each byte of data)
    uint16_t command_buffer[32];

    bool toggle = true;
    while(true) {
        toggle = !toggle;
        sleep_ms(1000);

        // calculate the command_buffer from the current framebuffer
        max7219_buffer_frame(toggle ? frame1 : frame2, command_buffer, 4, 8);

        // dump the command buffer contents to serial
        // printf("\n\nframe:\n");
        // max7219_dump_buffer(command_buffer, 4, 8);

        // DMA the command buffer out to the PIO
        dma_channel_configure(chan, &c, &pio->txf[sm], command_buffer, 32, true);
        dma_channel_wait_for_finish_blocking(chan);
    }
    ////////////// ANIMATE FROM FRAMEBUFFER VIA DMA [END]


    ////////////// SIMPLE CHECKERBOARD ANIMATION LOOP VIA DIRECT WRITES [START]
    /*
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
    */
    ////////////// SIMPLE CHECKERBOARD ANIMATION LOOP VIA DIRECT WRITES [END]

    return 0;
}






