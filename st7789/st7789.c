/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <string.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "st7789.h"

#include "consts.c"

#define KHZ 1000UL

static struct st7789_config st7789_cfg;
static u16 st7789_width;
static u16 st7789_height;
static bool_t st7789_data_mode = false;

static void st7789_cmd(u08 cmd, const u08* data, BaseSize_t len) {
    spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    st7789_data_mode = false;

    gpio_put(st7789_cfg.gpio_dc, 0);
    while(!spi_is_writable(st7789_cfg.spi));
    spi_write_blocking(st7789_cfg.spi, &cmd, sizeof(cmd));
    gpio_put(st7789_cfg.gpio_dc, 1);
    
    if (len && data != NULL) {    
        while(!spi_is_writable(st7789_cfg.spi));        
        spi_write_blocking(st7789_cfg.spi, data, len);
    }
}

static void st7789_caset(u16 xs, u16 xe) {
    u08 data[] = {
        xs >> 8,
        xs & 0xff,
        xe >> 8,
        xe & 0xff,
    };

    // CASET (2Ah): Column Address Set
    st7789_cmd(ST7789_CASET, data, sizeof(data));
}

static void st7789_raset(u16 ys, u16 ye){
    u08 data[] = {
        ys >> 8,
        ys & 0xff,
        ye >> 8,
        ye & 0xff,
    };

    // RASET (2Bh): Row Address Set
    st7789_cmd(ST7789_RASET, data, sizeof(data));
}

static void st7789_ramwr(){
    gpio_put(st7789_cfg.gpio_dc, 0);

    u08 cmd = ST7789_RAMWR;
    while(!spi_is_writable(st7789_cfg.spi));  
    spi_write_blocking(st7789_cfg.spi, &cmd, sizeof(cmd));
    gpio_put(st7789_cfg.gpio_dc, 1);
}

void st7789_init(const struct st7789_config* config, u16 width, u16 height)
{
    memcpy(&st7789_cfg, config, sizeof(st7789_cfg));
    st7789_width = width;
    st7789_height = height;

    gpio_set_function(st7789_cfg.gpio_din, GPIO_FUNC_SPI);
    gpio_set_function(st7789_cfg.gpio_clk, GPIO_FUNC_SPI);

    gpio_init(st7789_cfg.gpio_dc);
    gpio_init(st7789_cfg.gpio_rst);
    gpio_init(st7789_cfg.gpio_bl);

    gpio_set_dir(st7789_cfg.gpio_dc, GPIO_OUT);
    gpio_set_dir(st7789_cfg.gpio_rst, GPIO_OUT);
    gpio_set_dir(st7789_cfg.gpio_bl, GPIO_OUT);

    gpio_put(st7789_cfg.gpio_rst, 0);
    sleep_ms(250);
    gpio_put(st7789_cfg.gpio_rst, 1);
    sleep_ms(250);
    
    gpio_put(st7789_cfg.gpio_dc, 1);
    sleep_ms(150);
    
    spi_init(st7789_cfg.spi, st7789_cfg.clk_perif_khz * KHZ / 100);
    spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    sleep_ms(150);

    // SWRESET (01h): Software Reset
    st7789_cmd(ST7789_SWRESET, NULL, 0);
    sleep_ms(150);

    // SLPOUT (11h): Sleep Out
    st7789_cmd(ST7789_SLPOUT, NULL, 0);
    sleep_ms(150);

    // COLMOD (3Ah): Interface Pixel Format
    // - RGB interface color format     = 65K of RGB interface
    // - Control interface color format = 16bit/pixel
    st7789_cmd(
        ST7789_COLMOD, 
        (u08[]){ ST7789_COLOR_MODE_65K | ST7789_COLOR_MODE_16BIT }, 
        1
    );
    sleep_ms(150);

    // MADCTL (36h): Memory Data Access Control
    // - Page Address Order            = Top to Bottom
    // - Column Address Order          = Left to Right
    // - Page/Column Order             = Normal Mode
    // - Line Address Order            = LCD Refresh Top to Bottom
    // - RGB/BGR Order                 = RGB
    // - Display Data Latch Data Order = LCD Refresh Left to Right
    st7789_cmd(ST7789_MADCTL, (u08[]){ ST7789_MADCTL_RGB }, 1);
    sleep_ms(150);

    // INVON (21h): Display Inversion On
    st7789_cmd(ST7789_INVON, NULL, 0);
    sleep_ms(150);

    // NORON (13h): Normal Display Mode On
    st7789_cmd(ST7789_NORON, NULL, 0);
    sleep_ms(150);

    // DISPON (29h): Main screen turned on
    st7789_cmd(ST7789_DISPON, NULL, 0);
    sleep_ms(150);

    st7789_caset(0, width);
    st7789_raset(0, height);

    gpio_put(st7789_cfg.gpio_bl, 1);

    spi_set_baudrate(st7789_cfg.spi, st7789_cfg.clk_perif_khz * KHZ);
}

void st7789_write(const void* data, BaseSize_t len) {
    if (!st7789_data_mode) {
        st7789_ramwr();
        spi_set_format(st7789_cfg.spi, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        st7789_data_mode = true;
    }
    while(!spi_is_writable(st7789_cfg.spi));
    BaseSize_t n = (spi_write16_blocking(st7789_cfg.spi, data, len>>1))<<1;
    if( n != len ) {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        spi_write_blocking(st7789_cfg.spi, data+n, 1);
    }
}

void st7789_put(u16 pixel) {
    st7789_write(&pixel, sizeof(pixel));
}

void st7789_fill(u16 pixel) {
    int num_pixels = st7789_width * st7789_height;

    st7789_set_cursor(0, 0);

    for (int i = 0; i < num_pixels; i++) {
        st7789_put(pixel);
    }
}

void st7789_invert_colors(bool_t invert) {
    if(invert) st7789_cmd(ST7789_INVON, NULL, 0);
    else  st7789_cmd(ST7789_INVOFF, NULL, 0);
}

void st7789_set_cursor(u16 x, u16 y) {
    st7789_select_window(x, y, st7789_width, st7789_height);
}

void st7789_select_window(u16 x0, u16 y0, u16 x1, u16 y1) {
    st7789_caset(x0, x1);
    st7789_raset(y0, y1);
}

void st7789_vertical_scroll(u16 row) {
    u08 data[] = {
        (row >> 8) & 0xff,
        row & 0x00ff
    };
    // VSCSAD (37h): Vertical Scroll Start Address of RAM 
    st7789_cmd(ST7789_VSCSAD, data, sizeof(data));
}

/**
 * Rotate the display clockwise or anti-clockwie set by `rotation`
 * @param rotation Type of rotation. Supported values 0, 1, 2, 3
 */
void st7789_rotate_display(u08 rotation)
{
	/*
	* 	(u08)rotation :	Rotation Type
	* 					0 : Default landscape
	* 					1 : Potrait 1
	* 					2 : Landscape 2
	* 					3 : Potrait 2
	*/
	// Set max rotation value to 4
	rotation = rotation % 4;
    u16 temp_width = 0;
	st7789_cmd(ST7789_MADCTL, NULL, 0);		//Memory Access Control
	switch (rotation)
	{
		case 0:
			st7789_cmd(ST7789_MADCTL_RGB, NULL, 0);	// Default
            temp_width = (u16)st7789_width;
            st7789_width = st7789_height;
			st7789_height = temp_width;
			break;
		case 1:
			st7789_cmd(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB, NULL, 0);
            temp_width = (u16)st7789_width;
            st7789_width = st7789_height;
			st7789_height = temp_width;
			break;
		case 2:
			st7789_cmd(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB, NULL, 0);
            temp_width = (u16)st7789_width;
			st7789_width = st7789_height;
			st7789_height = temp_width;
			break;
		case 3:
			st7789_cmd(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB, NULL, 0);
            temp_width = (u16)st7789_width;
            st7789_width = st7789_height;
			st7789_height = temp_width;
			break;
	}
}

static void st7789_write_char(u16 x, u16 y, char ch, FontDef font, u16 color, u16 bgcolor){
    st7789_select_window(x,y, x + font.width - 1, y + font.height - 1);
    
	for (u32 i = 0; i < font.height; i++) {
		u32 b = font.data[(ch - 32) * font.height + i];
		for (u32 j = 0; j < font.width; j++) {
			if ((b << j) & 0x8000) {
				st7789_write(&color, sizeof(color));
			}
			else {
				st7789_write(&bgcolor, sizeof(bgcolor));
			}
		}
	}
}

void st7789_write_string(u16 x, u16 y, const char *str, FontDef font, u16 color, u16 bgcolor) {
	while (*str) {
		if (x + font.width >= st7789_width) {
			x = 0;
			y += font.height;
			if (y + font.height >= st7789_height) {
				break;
			}

			if (*str == ' ') {
				// skip spaces in the beginning of the new line
				str++;
				continue;
			}
		}
		st7789_write_char(x, y, *str, font, color, bgcolor);
		x += font.width;
		str++;
	}
}

void st7789_draw_line(u16 x0, u16 y0, u16 x1, u16 y1, u16 color) {
	u16 swap;
    u16 steep = ABS(y1 - y0) > ABS(x1 - x0);

    if (steep) {
		swap = x0;
		x0 = y0;
		y0 = swap;

		swap = x1;
		x1 = y1;
		y1 = swap;
    }

    if (x0 > x1) {
		swap = x0;
		x0 = x1;
		x1 = swap;

		swap = y0;
		y0 = y1;
		y1 = swap;
    }

    int16_t dx = x1 - x0;
    int16_t dy = ABS(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) ystep = 1;
    else ystep = -1;

    for (; x0<=x1; x0++) {
        if (steep) st7789_select_window(y0, x0, y0, x0);
        else st7789_select_window(x0, y0, x0, y0);
        st7789_put(color);
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void st7789_draw_filled_rectangle(u16 x, u16 y, u16 w, u16 h, u16 color) {
	/* Check input parameters */
	if (x >= st7789_width || y >= st7789_height) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= st7789_width) {
		w = st7789_width - x;
	}
	if ((y + h) >= st7789_height) {
		h = st7789_height - y;
	}

	/* Draw lines */
	for (u16 i = 0; i <= h; i++) {
		/* Draw lines */
		st7789_draw_line(x, y + i, x + w, y + i, color);
	}
}
