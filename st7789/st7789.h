
#ifndef _PICO_ST7789_H_
#define _PICO_ST7789_H_

#include "font.h"
#include "../femtox/TaskMngr.h"

struct st7789_config {
    spi_inst_t* spi;
    u32 clk_perif_khz;
    u08 gpio_din;
    u08 gpio_clk;
    u08 gpio_dc;
    u08 gpio_rst;
    u08 gpio_bl;
};

void st7789_init(const struct st7789_config* config, u16 width, u16 height);
void st7789_write(const void* data, BaseSize_t len);
void st7789_put(u16 pixel);
void st7789_fill(u16 pixel);
void st7789_select_window(u16 x0, u16 y0, u16 x1, u16 y1);
void st7789_set_cursor(u16 x, u16 y);
void st7789_vertical_scroll(u16 row);
void st7789_rotate_display(u08 rotation); // @param rotation Type of rotation. Supported values 0, 1, 2, 3
void st7789_write_string(u16 x, u16 y, const char *str, FontDef font, u16 color, u16 bgcolor);
void st7789_draw_line(u16 x0, u16 y0, u16 x1, u16 y1, u16 color);
void st7789_draw_filled_rectangle(u16 x, u16 y, u16 w, u16 h, u16 color);

// Color definitions
#define	ST_R_POS_RGB   11	// Red last bit position for RGB display
#define	ST_G_POS_RGB   5 	// Green last bit position for RGB display
#define	ST_B_POS_RGB   0	// Blue last bit position for RGB display

#define	ST_RGB(R,G,B) \
	(((u16)(R >> 3) << ST_R_POS_RGB) | \
	((u16)(G >> 2) << ST_G_POS_RGB) | \
	((u16)(B >> 3) << ST_B_POS_RGB))

#define ST_COLOR_BLACK       ST_RGB(0,     0,   0)
#define ST_COLOR_NAVY        ST_RGB(0,     0, 123)
#define ST_COLOR_DARKGREEN   ST_RGB(0,   125,   0)
#define ST_COLOR_DARKCYAN    ST_RGB(0,   125, 123)
#define ST_COLOR_MAROON      ST_RGB(123,   0,   0)
#define ST_COLOR_PURPLE      ST_RGB(123,   0, 123)
#define ST_COLOR_OLIVE       ST_RGB(123, 125,   0)
#define ST_COLOR_LIGHTGREY   ST_RGB(198, 195, 198)
#define ST_COLOR_DARKGREY    ST_RGB(123, 125, 123)
#define ST_COLOR_BLUE        ST_RGB(0,     0, 255)
#define ST_COLOR_GREEN       ST_RGB(0,   255,   0)
#define ST_COLOR_CYAN        ST_RGB(0,   255, 255)
#define ST_COLOR_RED         ST_RGB(255,   0,   0)
#define ST_COLOR_MAGENTA     ST_RGB(255,   0, 255)
#define ST_COLOR_YELLOW      ST_RGB(255, 255,   0)
#define ST_COLOR_WHITE       ST_RGB(255, 255, 255)
#define ST_COLOR_ORANGE      ST_RGB(255, 165,   0)
#define ST_COLOR_GREENYELLOW ST_RGB(173, 255,  41)
#define ST_COLOR_PINK        ST_RGB(255, 130, 198)

#endif