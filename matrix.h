#include "LedControl.h"

void matrix_init(LedControl lc, int brightness)
{
  lc.shutdown(0, false); // disable power saving, enable display
  lc.setIntensity(0, brightness);
  lc.clearDisplay(0);
}

void matrix_on(LedControl lc)
{
  for (int row=0; row<8; row++)
  {
    lc.setRow(0, row, B11111111);
  }
}

void matrix_off(LedControl lc)
{
  lc.clearDisplay(0);
}

// returns 1 if LED is on, 0 if off.
// handles sequence states 0 (off), 1 (on), and 2 (random)

byte led_on(byte state)
{
  if (state == 1)
  {
    return 1;
  }
  else if (state == 2)
  {
    if (random(2) >= 1)
    {
      return 1;
    }
  }

  return 0;
}

void matrix_row_play(LedControl lc, byte method, byte * seqrow)
{
  if (method == 0) // setLed()
  {
    for (int row=0; row<8; row++)
    {
      lc.setLed(0, row, 0, led_on(seqrow[(row*8)+6]));
      lc.setLed(0, row, 1, led_on(seqrow[(row*8)+5]));
      lc.setLed(0, row, 2, led_on(seqrow[(row*8)+4]));
      lc.setLed(0, row, 3, led_on(seqrow[(row*8)+3]));
      lc.setLed(0, row, 4, led_on(seqrow[(row*8)+2]));
      lc.setLed(0, row, 5, led_on(seqrow[(row*8)+1]));
      lc.setLed(0, row, 6, led_on(seqrow[(row*8)+0]));
      lc.setLed(0, row, 7, led_on(seqrow[(row*8)+7]));
    }
  }
  else if (method == 1) // setRow()
  {
    for (int row=0; row<8; row++)
    {
      byte rowbyte = 0;
 
      // swapping segA with segDP

      if (led_on(seqrow[(row*8)+6]))
      {
        rowbyte += 1;
      }
      if (led_on(seqrow[(row*8)+5]))
      {
        rowbyte += 2;
      }
      if (led_on(seqrow[(row*8)+4]))
      {
        rowbyte += 4;
      }
      if (led_on(seqrow[(row*8)+3]))
      {
        rowbyte += 8;
      }
      if (led_on(seqrow[(row*8)+2]))
      {
        rowbyte += 16;
      }
      if (led_on(seqrow[(row*8)+1]))
      {
        rowbyte += 32;
      }
      if (led_on(seqrow[(row*8)+0]))
      {
        rowbyte += 64;
      }
      if (led_on(seqrow[(row*8)+7]))
      {
        rowbyte += 128;
      }
 
      lc.setRow(0, row, rowbyte);
    }
  }
  else if (method == 2) // setColumn()
  {
    for (int col=0; col<8; col++)
    {
      byte colbyte = 0;
      byte coltrue = 0; // compensate for shifted columns

      if (col == 6)
      {
        coltrue = 1;
      }
      if (col == 5)
      {
        coltrue = 2;
      }
      if (col == 4)
      {
        coltrue = 3;
      }
      if (col == 3)
      {
        coltrue = 4;
      }
      if (col == 2)
      {
        coltrue = 5;
      }
      if (col == 1)
      {
        coltrue = 6;
      }
      if (col == 0)
      {
        coltrue = 7;
      }
      if (col == 7)
      {
        coltrue = 0;
      }

      if (led_on(seqrow[(0*8)+coltrue]))
      {
        colbyte += 1;
      }
      if (led_on(seqrow[(1*8)+coltrue]))
      {
        colbyte += 2;
      }
      if (led_on(seqrow[(2*8)+coltrue]))
      {
        colbyte += 4;
      }
      if (led_on(seqrow[(3*8)+coltrue]))
      {
        colbyte += 8;
      }
      if (led_on(seqrow[(4*8)+coltrue]))
      {
        colbyte += 16;
      }
      if (led_on(seqrow[(5*8)+coltrue]))
      {
        colbyte += 32;
      }
      if (led_on(seqrow[(6*8)+coltrue]))
      {
        colbyte += 64;
      }
      if (led_on(seqrow[(7*8)+coltrue]))
      {
        colbyte += 128;
      }

      lc.setColumn(0, col, colbyte);
    }
  }
}
