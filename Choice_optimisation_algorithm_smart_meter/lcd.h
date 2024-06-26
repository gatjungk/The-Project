/*  Author: Steve Gunn
 * Licence: This work is licensed under the Creative Commons Attribution License.
 *           View this license at http://creativecommons.org/about/licenses/
 */
 
#include <stdbool.h>
#include <stdint.h>
#include "ili934x.h"

#define LCDWIDTH	240
#define LCDHEIGHT	320

/* Colour definitions RGB565 */
#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F      
#define GREEN       0x07E0      
#define CYAN        0x07FF      
#define RED         0xF800      
#define MAGENTA     0xF81F      
#define YELLOW      0xFFE0
#define GREY        0x8430
#define ORCHID      0xDB9A
#define ABSOLUTE_ZERO 0x0257
#define YES			0xEA60 //addition color from Ihatefamous
#define SHOCKINGPINK  0xF897  

typedef enum {North, West, South, East} orientation;

typedef enum {
  NormalMode,
  IdleMode,
  PartialMode
} ili934x_mode;

typedef struct {
	uint16_t width, height;
	orientation orient;
	uint16_t x, y;
	uint16_t foreground, background;
} lcd;

typedef struct 
{
	uint16_t left, right;
	uint16_t top, bottom;
} rectangle;

extern lcd display;

uint16_t start_x, start_y;

void init_lcd();
void set_orientation(orientation o);
void clear_screen();
void fill_rectangle(rectangle r, uint16_t col);
void fill_rectangle_indexed(rectangle r, uint16_t* col);
void display_char(char c, uint8_t size);
void display_string(char *str, uint8_t size);

void add_spacing(uint16_t x, uint16_t sp, uint16_t ep);
void change_position(uint16_t x, uint16_t y);
void change_background(uint16_t colour);
void change_foreground(uint16_t colour);

void init_table(uint16_t start_x, uint16_t end_x );
void update_table(uint16_t row, uint16_t col, char *str);

void setup_Brightness();
void return_PWM_Brightness(float volts);