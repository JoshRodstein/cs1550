/*************************************************
 * Program: "Graphics.h"
 * Project 1: Graphics LIbrary    
 *       Prof. Midurda's Intro to Oeprating Systems cs1550
 * 
 * By: Joshua Rodstein
 * Email: jor92@pitt.edu
 *
 * graphics header file containing:
 *  typedef, function prototypes, RGB macro
 *
 ******************************************************************/


// typdefs                                                                                    
typedef unsigned short int color_t;



// Function Prototypes
void init_graphics();
void exit_graphics();
char getkey();
void sleep_ms(long);
void clear_screen(void *);
void draw_pixel(void*, int, int, color_t);
void draw_line(void *, int, int, int, int, color_t);
void drawCircle(void *, int, int, color_t);
void * new_offscreen_buffer();
void blit(void *);

// macros
#define RGB(r, g, b) ((color_t) ((r << 11) + ((g & 0x003f) << 5) + (b & 0x001f)))
