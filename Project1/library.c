/***********************************************************************************
* Graphics Library - Project 1 for Prof. Misurda's                                 *
* Intro to Operating Systems  class - cs1550                                       *
*                                                                                  *
* Program: "Library.c"                                                             *         
*  By: Joshua Rodstein                                                             *
*  Email: jor94@pitt.edu                                                           *
*                                                                                  *
*  Bresnan's Line Drawing Algorithm obtained from Rosetta Code                     *           
*  @  https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C           *
*                                                                                  *
*  Modified By: Joshua Rodstein                                                    *
*                                                                                  * 
*  Implementation of Bresnan's Circle Drawing ALgorithm obtained geeksforgeeks     *
*  @ http://www.geeksforgeeks.org/bresenhams-circle-drawing-algorithm/             *  
*                                                                                  *
*  Modified by: Joshua Rodstein                                                    * 
***********************************************************************************/



// Header File Inclusions //
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include "graphics.h"

int fd;
color_t * frame_buffer;
color_t * offscreen_frame_buffer;
int size;
struct termios term;
struct fb_var_screeninfo var_scrinf;
struct fb_fix_screeninfo fix_scrinf;



void init_graphics() {
  /*fd- file descriptor for frame buffer with read & write privlages
    ioctl- syscall to io control to retrieve screen resolution information
           and assign values to corresponding struct variable. */
  fd = open("/dev/fb0", O_RDWR);

  if(fd >= 0){
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var_scrinf) >= 0 
	&& ioctl(fd, FBIOGET_FSCREENINFO, &fix_scrinf) >= 0)
    {
      size = var_scrinf.yres_virtual * fix_scrinf.line_length;
    }
  }

  /*syscall mmap maps a file into a shared portion of our address 
    returns a void pointer as an unsigned int (size f color_t data typedef)
    to the address */
  frame_buffer = (color_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  /*Disable keypress echo and canonical mode*/
  ioctl(STDIN_FILENO, TCGETS, &term);
  term.c_lflag &= ~ECHO;
  term.c_lflag &= ~ICANON;
  ioctl(STDIN_FILENO, TCSETS, &term);

  write(1, "\033[2J", 8);

}

void clear_screen(void * img) {
  //loop over offscreen buffer and set each byte to 0
  int x, y;
  for(y = 0; y < var_scrinf.yres_virtual; y++){
    for(x = 0; x < fix_scrinf.line_length/2; x++){
      draw_pixel((color_t*)img, x, y, (color_t) 0);
    }
  }

  write(1, "\033[2J", 7);
  
  return;
}

/*Un-map memory, restore key-press echo and 
  canonical mode, close file descriptor */
void exit_graphics(){
  munmap(frame_buffer, size);
  munmap(offscreen_frame_buffer, size);

  
  if(ioctl(STDIN_FILENO, TCGETS, &term) >= 0){
    term.c_lflag |= ECHO;
    term.c_lflag |= ICANON;
    ioctl(STDIN_FILENO, TCSETS, &term);
  }

  close(fd);
}

// getkey() to allow for keypress polling
// 
char getkey(){
  char key;
  fd_set fdset;
  struct timeval tval;
  int rval = 0;

  tval.tv_sec = 0;
  tval.tv_usec = 0;
  
  /* Empties the set */
  FD_ZERO(&fdset);  
  /* Watch STDIN for input */
  FD_SET(STDIN_FILENO, &fdset);

  rval = select(STDOUT_FILENO, &fdset, NULL, NULL, &tval);

  if(rval > 0){
    read(STDIN_FILENO, &key, sizeof(char));
    return key;
  } else {
    return ;
  }
}


void sleep_ms(long ms){
  struct timespec timer;
  timer.tv_sec = 0;
  timer.tv_nsec = ms*(1000000);

  nanosleep(&timer, NULL);
  
}

// draw pixel
void draw_pixel(void *img, int x, int y, color_t color){

  // check x and y to assure within bounds of the framebuffer
  if(x < 0 || x > fix_scrinf.line_length/2) 
    return;
  if(y < 0 || y > var_scrinf.yres_virtual)
    return;
  //calculate offset, assign color information to that address
  color_t * temp_fb = (color_t *)img + (y*fix_scrinf.line_length/2 + x);
  *temp_fb = color;
  
}

// Implementation of Bresnan's Line drawing algorithm
void draw_line(void *img, int x1, int y1, int x2, int y2, color_t c){
  int dx = abs(x2-x1), sx = x1<x2 ? 1 : -1;
  int dy = abs(y2-y1), sy = y1<y2 ? 1 : -1; 
  int err = (dx>dy ? dx : -dy)/2, e2;
 
  for(;;){
    draw_pixel((color_t *)img,x1,y1, c);
    if (x1==x2 && y1==y2) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x1 += sx; }
    if (e2 < dy) { err += dx; y1 += sy; }
  }
}

// Implementation of Bresnan's circle drawing algorithm
void drawCircle(void *img, int x, int y, color_t c)
{
  int xc = (var_scrinf.xres_virtual/2);
  int yc = (var_scrinf.yres_virtual/2);

  draw_pixel((color_t *)img, xc+x, yc+y, c);
  draw_pixel((color_t *)img, xc-x, yc+y, c);
  draw_pixel((color_t *)img, xc+x, yc-y, c);
  draw_pixel((color_t *)img, xc-x, yc-y, c);
  draw_pixel((color_t *)img, xc+y, yc+x, c);
  draw_pixel((color_t *)img, xc-y, yc+x, c);
  draw_pixel((color_t *)img, xc+y, yc-x, c);
  draw_pixel((color_t *)img, xc-y, yc-x, c);
}


// allocate memory for offscreen frame buffer
// Returns void * to base address of memory allocation
void * new_offscreen_buffer(){
 
  offscreen_frame_buffer = (color_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, 
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return offscreen_frame_buffer;
}

// Loop over off screen frame buffer, calculate offset
// and copy pixel to corresponsing offset(pixel) of
// the onscreen buffer
void blit(void *src){
  int x,y;
  color_t * temp_buf;

  // for loop for iterating over y(rows), and
  // x(offset in row) coordinates
  for(y = 0; y < var_scrinf.yres_virtual; y++){
    for(x = 0; x < fix_scrinf.line_length/2; x++){
      // calculate offset and store address in temp_buf
      // derefernce temp_buf to assign color to corresponding
      // address in the onscreen frame buffer
      temp_buf = (color_t *)src + (y*fix_scrinf.line_length/2 + x);
      *(frame_buffer + (y*fix_scrinf.line_length/2 + x)) = *temp_buf;
    
    }
  }
}
