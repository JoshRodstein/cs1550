/*****************************************************************************************
 * Driver Program for library.c Graphics Library *                                       *
 *                                                                                       * 
 * By: Joshua Rodstein
 *   Bresnan's Circle Drawing Algorithm: 
 *     obtained from http://www.geeksforgeeks.org/bresenhams-circle-drawing-algorithm/
 *   Modified by: Joshua Rodstein
 *  
 *Functionality: 
 *   Compile: gcc driver.c -o driver
 *   '+' to draw a circle increased by radius size 10 each time
 *   'q' to quit 
 *****************************************************************************************/


#include "library.c"


// Function for circle-generation
// using Bresenham's algorithm

void circleBres(void * img, int r, color_t c)
{
  int x = 0, y = r;
  int d = 3 - 2 * r;
  while (y >= x)
    {
      // for each pixel we will
      // draw all eight pixels
      drawCircle(img, x, y, c);
      x++;
 
      // check for decision parameter
      // and correspondingly 
      // update d, x, y
      if (d > 0)
        {
	  y--; 
	  d = d + 4 * (x - y) + 10;
        }
      else
	d = d + 4 * x + 6;
      drawCircle(img, x, y, c);
      sleep_ms(50);
    }
}



/*
 Modify color by adjusting the RGB parameters of the color variable.
 */

int main(int argc, char **argv){
  // initialize graphics lib frame buffer
  init_graphics();

  // create new off screen buffer
  // initialize radius to size 25
  void *buf = new_offscreen_buffer();
  int r = 25;
  char key;
  color_t color = RGB(31, 0, 0); //set desired color parameter


  /*
    polling loop to obtain key, utilizing
    blit(), clear_screen, getkey() and exit_graphics()
   */
  do {
    key = getkey();
    if(key == 'q'){
      clear_screen(buf);
      blit(buf);
      break;
    } else if (key == '+'){
   	r += 10;
	clear_screen(buf);
	circleBres(buf, r, color);
	blit(buf);
    } 

  } while(1);

  exit_graphics();
  return 0;

}
