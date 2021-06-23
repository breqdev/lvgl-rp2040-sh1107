#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "lvgl/lvgl.h"

#include "pico/binary_info.h"
#include "hardware/i2c.h"


#define ADDR 0x3C

#define WIDTH 64
#define HEIGHT 128

#define BUFFER_SIZE (WIDTH * HEIGHT)

#define I2C i2c1


enum Command {
    COL_ADDR_LOW = 0x00,
    COL_ADDR_HIGH = 0x10,
    ADDRESS_MODE = 0x20,
    CONTRAST = 0x81,
    SEGMENT_REMAP = 0xA0,
    MULTIPLEX = 0xA8,
    ENTIRE_DISPLAY_ON = 0xA4,
    REVERSE = 0xA6,
    OFFSET = 0xD3,
    DC_DC = 0xAD,
    ON_OFF = 0xAE,
    PAGE_ADDR = 0xB0,
    COM_SCAN_DIR = 0xC0,
    INTERNAL_CLOCK = 0xD5,
    DISCHARGE_PRECHARGE = 0xD9,
    VCOM_DESELECT = 0xDB,
    START_LINE = 0xDC,
    READ_MODIFY_WRITE = 0xE0,
    READ_MODIFY_WRITE_END = 0xEE,
    NOP = 0xE3,
};

#define PAGES (HEIGHT / 8)
#define BYTES_PER_PAGE WIDTH

const uint8_t startup[] = {
    ON_OFF | 0,
    START_LINE, 0,
    CONTRAST, 0x2f,
    ADDRESS_MODE | 0,
    SEGMENT_REMAP | 0,
    COM_SCAN_DIR | 0xF,
    MULTIPLEX, 0x7F,
    OFFSET, 0x60,
    INTERNAL_CLOCK, 0x51,
    DISCHARGE_PRECHARGE, (2) << 4 + (2),
    VCOM_DESELECT, 0x35,
    PAGE_ADDR | 0,
    ENTIRE_DISPLAY_ON | 0,
    REVERSE | 0,
    ON_OFF | 1
};

void init_i2c() {
    i2c_init(I2C, 100 * 1000);

    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);

    gpio_pull_up(2);
    gpio_pull_up(3);
}

void send_command(uint8_t command) {
    uint8_t data[] = {0x80, command};

    i2c_write_blocking(
        I2C,
        ADDR,
        data,
        2,
        false
    );
}


void send_data(const uint8_t* data, size_t length) {
    uint8_t* buffer = malloc(length + 1);

    memcpy(buffer + 1, data, length);
    buffer[0] = 0x40;

    i2c_write_blocking(
        I2C,
        ADDR,
        buffer,
        length + 1,
        false
    );

    free(buffer);
}

void my_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    printf("Flushing area x1=%02x x2=%02x y1=%02x y2=%02x\n", area->x1, area->x2, area->y1, area->y2);

    uint8_t start_page = area->y1 >> 3;
    uint8_t end_page = area->y2 >> 3;

    uint8_t start_col = area->x1;
    uint8_t end_col = area->x2 + 1;

    uint8_t transfer_size = end_col - start_col;

    for (uint8_t page = start_page; page <= end_page; ++page) {
        send_command(COL_ADDR_HIGH | 0);
        send_command(COL_ADDR_LOW | 0);

        send_command(PAGE_ADDR | page);

        void* data = color_p + page * WIDTH + start_col;

        send_data(data, transfer_size);
    }

    lv_disp_flush_ready(disp_drv);
}

void my_rounder_cb(lv_disp_drv_t* disp_drv, lv_area_t* area) {
    area->y1 &= ~0x7;
    area->y2 &= ~0x7;
    area->y2 += 7;

    // area->y1 = 0;
    // area->y2 = HEIGHT - 1;

    // area->x1 = 0;
    // area->x2 = WIDTH - 1;
}

void my_set_px_cb(lv_disp_drv_t* disp_drv, uint8_t* buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa) {

    uint16_t page = y >> 3;
    uint16_t column = x;
    uint8_t bit = y & 0x7;

    uint8_t mask = 1 << bit;

    uint16_t buffer_index = page * WIDTH + column;

    if (color.full == 0) {
        buf[buffer_index] |= mask;
    } else {
        buf[buffer_index] &= ~mask;
    }
}


void do_tick_inc() {
    lv_tick_inc(5);
}


int main() {
    stdio_init_all();

    sleep_ms(5000);
    printf("setting up i2c\n");

    init_i2c();

    for (uint8_t i = 0; i < sizeof(startup)/sizeof(*startup); ++i) {
        uint8_t command = startup[i];
        // printf("sending command %02x\n", command);
        send_command(command);
    }

    // static lv_color_t buffer[BUFFER_SIZE];

    // // for (int x = 0; x < WIDTH; x += 10) {
    // //     for (int y = 0; y < HEIGHT; y += 10) {
    // //         my_set_px_cb(NULL, buffer, WIDTH, x, y, lv_color_white(), 0);
    // //     }
    // // }

    // for (int row = 0; row < HEIGHT; row += 10) {
    //     for (int y = row; y < row + 5; ++y) {
    //         for (int x = 0; x < WIDTH; ++x) {
    //             my_set_px_cb(NULL, buffer, WIDTH, x, y, lv_color_white(), 0);
    //         }
    //     }
    // }

    // for (int y = 0; y < HEIGHT; ++y) {
    //     int x = (WIDTH / 2.0) + sin(y / 20.0) * (WIDTH / 2.0);

    //     my_set_px_cb(NULL, buffer, WIDTH, x, y, lv_color_white(), 0);
    // }

    // for (int y = 0; y < HEIGHT; ++y) {
    //     int x = (WIDTH / 2.0) + cos(y / 20.0) * (WIDTH / 2.0);

    //     my_set_px_cb(NULL, buffer, WIDTH, x, y, lv_color_black(), 0);
    // }

    // lv_area_t everywhere;
    // everywhere.x1 = 0;
    // everywhere.x2 = WIDTH - 1;
    // everywhere.y1 = 0;
    // everywhere.y2 = HEIGHT - 1;

    // my_flush_cb(NULL, &everywhere, buffer);

    lv_init();

    static lv_disp_draw_buf_t disp_buf;

    static lv_color_t buf_1[BUFFER_SIZE];
    // static lv_color_t buf_2[BUFFER_SIZE];

    lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, BUFFER_SIZE);

    static lv_disp_drv_t disp_drv;

    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;

    disp_drv.flush_cb = my_flush_cb;
    disp_drv.rounder_cb = my_rounder_cb;
    disp_drv.set_px_cb = my_set_px_cb;

    disp_drv.hor_res = WIDTH;
    disp_drv.ver_res = HEIGHT;

    lv_disp_t* disp;
    disp = lv_disp_drv_register(&disp_drv);


    // lv_theme_t* theme = lv_theme_mono_init(disp, true, NULL);


    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // lv_obj_t* rectangle = lv_obj_create(lv_scr_act());
    // lv_obj_set_size(rectangle, 32, 32);
    // lv_obj_set_pos(rectangle, 32, 32);

    // lv_style_t style;
    // lv_style_init(&style);

    // lv_style_set_bg_color(&style, lv_color_white());
    // lv_style_set_border_width(&style, 2);
    // lv_style_set_border_color(&style, lv_color_black());

    // lv_obj_add_style(rectangle, &style, 0);

    printf("Label created\n");

    repeating_timer_t timer;
    add_repeating_timer_ms(5, do_tick_inc, NULL, &timer);

    printf("Tick timer initialized\n");

    while (true) {
        lv_task_handler();
        sleep_ms(500);
    }

    return 0;
}