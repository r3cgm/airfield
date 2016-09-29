/*
  Airfield Firmware

  Christopher 'r3' Mann / Jeremy 'jerware' Williams

  (cc) licensed under a Creative Commons Attribution NonCommercial
    ShareAlike 3.0 Unported License: www.ledseq.com/license.html
*/

#define VERSION 20130415

#include <SD.h>
#include "sd.h"
#include "LedControl.h"
#include "matrix.h"

// pin assignments

#define GI             3
#define DEBUG_SWITCH   4 // DIAG
#define MATRIX_CS      5
#define MATRIX_DI      6
#define MATRIX_CLK     7
#define DEBUG_LED      9
#define SD_CS          10
#define SD_DI          11
#define SD_DO          12
#define SD_CLK         13

// settings

#define DIM_DEFAULT    100 // max brightness percent
#define MATRIX_BRIGHT  15  // default bright (0 is dark)
#define MATRIX_COUNT   1   // future option to daisy chain multiple matricies
#define NUM_LEDS       64  // controllable LEDs not including GI
#define RENDER_METHOD  1   // 0 (setLed), 1 (setRow), 2 (setColumn)
#define SPEED_DEFAULT  100 // speed percent 0-999
#define TICK_DEFAULT   100 // milliseconds between rows

// init

unsigned int    brightness = MATRIX_BRIGHT;
Sd2Card         card;
byte            control;
int             debug = 0;
int             debugFlicker = 0; // state variable for row-blinks
unsigned int    dim = DIM_DEFAULT;
unsigned int    gi_level = 0;
LedControl      lc = LedControl(MATRIX_DI, MATRIX_CLK, MATRIX_CS, MATRIX_COUNT);
unsigned int    loop_counter = 0;
unsigned long   loop_pointer = 0;
char            lq_file[] = "airfield.led";
byte            sd_exists = 0;
byte            seqrow[NUM_LEDS];
byte            seqrow_prev[NUM_LEDS];
File            sf;
unsigned int    speed = SPEED_DEFAULT;
unsigned int    tick = TICK_DEFAULT;
SdVolume        volume;

// diagnostic

// ms between pulse code steps
int pulse_step_time = 3;

// steps during the brightness pulse up
int pulse_up = 85;

// how many brightness steps to increase per step
int pulse_up_jump = int(255 / pulse_up);

// steps during the brightness pulse down
int pulse_down = 85;

// how many brightness steps to decrease per step
int pulse_down_jump = int(255 / pulse_down);

// ms delay desired divided by pulse_step_time = number of steps between pulses
int pulse_pause = 150 / pulse_step_time;

// ms delay desired divided by pulse_step_time = number of steps for final pause
int pulse_endpause = 3000 / pulse_step_time;

// calculated at runtime (do not specify here)
int pulse_steps = 0;

// current step (state variable)
int pulse_cur = 0;

// diagnostic settings (chase loop)

int debugled_step_time = 125; // ms between debug LED steps
int debugled_steps = NUM_LEDS * 2; // 2 > lit cycle + unlit cycle
int debugled_cur = 0; // current step

// ------------------------------------------------------------------------
// setup()
// ------------------------------------------------------------------------

void setup()
{
  // setup debug switch
  pinMode(DEBUG_SWITCH, INPUT);
  debug = digitalRead(DEBUG_SWITCH);

  if (debug)
  {
    Serial.begin(115200);
    Serial.print(F("Airfield: "));
    Serial.println(VERSION);
    Serial.println(F("debug mode enabled"));
  }

  if (debug)
  {
    Serial.println(F("set debug led pin"));
  }
  pinMode(DEBUG_LED, OUTPUT);

  if (debug)
  {
    Serial.println(F("init general illumination"));
  }
  pinMode(GI, OUTPUT);
  analogWrite(GI, 255); // off by default

  if (debug)
  {
    Serial.println(F("turn on debug led"));
  }
  digitalWrite(DEBUG_LED, HIGH);

  if (debug)
  {
    Serial.println(F("init matrix brightness"));
  }
  matrix_init(lc, brightness);

  // generate entropy from line noise on (unconnected) analog pin 0

  if (debug)
  {
    Serial.println(F("harvest entropy from analog noise on pin 0"));
  }
  randomSeed(analogRead(0));

  // clear sequence buffers

  if (debug)
  {
    Serial.println(F("clear buffers"));
  }
  memset(seqrow, 0, sizeof(seqrow));
  memset(seqrow_prev, 0, sizeof(seqrow_prev));

  if (debug)
  {
    Serial.println(F("matrix off"));
  }
  matrix_off(lc);
  
  if (debug)
  {
    Serial.println(F("init sd card"));
  }
  sd_exists = sd_init(card, volume, SD_CS, lq_file);

  if (sd_exists != 1)
  {
    abort(sd_exists);
  }

  if (debug)
  {
    Serial.println(F("load sequence file"));
  }
  sf = sd_open(lq_file);

  if (! sf)
  {
    abort(5);
  }
}

// ------------------------------------------------------------------------
// loop()
// ------------------------------------------------------------------------

void loop()
{
  // toggle debug LED each time through the loop for visual feedback

  if (debugFlicker == 1)
  {
    debugFlicker = 0;
    digitalWrite(DEBUG_LED, HIGH); // off
  }
  else
  {
    debugFlicker = 1;
    digitalWrite(DEBUG_LED, LOW); // on
  }

  row_copy(seqrow_prev, seqrow);

  if (! sf.available())
  {
    sf.seek(0);
  }

  while (control = sd_control(sf))
  {
    // optimize control code order; sort most to least common

    if (control == 't') // tick time
    {
      unsigned int tick_new = 0;
      tick_new = control_tick(sf);
      if (debug)
      {
        Serial.print(F("new tick val: "));
        Serial.println(tick_new);
      }
      if (tick_new)
      {
        tick = tick_new;
      }
    }
    else if (control == '/') // comment
    {
      while ((sf.peek() != 13) && (sf.peek() != 10))
      {
        sf.read();
      }
    }
    else if (control == 'b') // brightness
    {
      brightness = control_brightness(sf);
      if (debug)
      {
        Serial.print(F("new brightness: "));
        Serial.print(brightness);
        Serial.print(F(" w/dim: "));
        Serial.println(brightness * dim / 100);
      }
      lc.setIntensity(0, brightness * dim / 100);
    }
    else if (control == 's') // speed
    {
      unsigned int speed_new = 0;
      speed_new = control_speed(sf);
      if (debug)
      {
        Serial.print(F("new speed: "));
        Serial.println(speed_new);
      }
      if (speed_new)
      {
        speed = speed_new;
      }
    }
    else if (control == 'd') // dim
    {
      unsigned int dim_new = 0;
      dim_new = control_dim(sf);
      if (debug)
      {
        Serial.print(F("new dim: "));
        Serial.print(dim_new);
        Serial.print(F(" w/dim: "));
        Serial.println(brightness * dim / 100);
      }
      if (dim_new)
      {
        dim = dim_new;
        lc.setIntensity(0, brightness * dim / 100);
      }
    }
    else if (control == 'l') // loop (start, end)
    {
      control_loop(sf);
    }
    else if (control == 'g') // GI brightness
    {
      gi_level = control_gi(sf);
    }

    crlf(sf);

    // we may have just reached the end of the file and need to return to top
    if (! sf.available())
    {
      sf.seek(0);
    }
  }

  sd_row_read(sf, seqrow, NUM_LEDS);

  // if we just cycled back to the top of the sequence reset the loop start
  // bookmark.  this prevents the condition where an errant 'le' code with no
  // corresponding 'ls' preceeding it could inherit a previous 'ls' pointer
  // from a previous cycle of the sequence file.

  if (sf.position() == 0)
  {
    loop_pointer = 0;
  }

  if (debug)
  {
    Serial.print(F("play row: "));
    for (int i=0; i<NUM_LEDS; i++)
    {
      Serial.print(seqrow[i]);
    }
    Serial.println();
  }

  analogWrite(GI, (255 - (gi_level * dim / 100)));
  matrix_row_play(lc, RENDER_METHOD, seqrow);

  if (! speed)
  {
    while (1)
    {
      delay(1000);
    }
  }
  delay(tick_time(tick, speed));
}

// this function gets called if any problem occurs.  it will show "pulse codes"
// (see Airfield Troubleshooting doc for more details), as well as show a test
// pattern which is useful for making sure every LED is hooked up properly.

void abort(int code)
{
  unsigned long step_ms = 0;

  if (debug)
  {
    Serial.print(F("aborting playback: "));

    if (code == 2)
    {
      Serial.println(F("SD card type identification failed (code 2)"));
    }
    else if (code == 3)
    {
      Serial.println(F("SD could not find FAT16/FAT32 partition (code 3)"));
    }
    /*
      else if (code == 4)
      {
        Serial.println(F("SD library and/or card initialization failed (code 4)"));
      }
    */
    else if (code == 5)
    {
      Serial.println(F("SD could not find airfield.led file (code 5)"));
    }
    else
    {
      Serial.println(F("unknown condition"));
    }
  }

  pulse_steps = debug_led_steps(code);

  while (1)
  {
    if (debug)
    {
      Serial.print(F("step_ms ("));
      Serial.print(step_ms);
      Serial.println(F(")"));
    }

    debug_led_cycle(step_ms, code);
    debug_chase_cycle(step_ms);
    step_ms++;
    delay(1);
  }
}

// calculate number of step necessary to send the debug LED through an entire
// cycle of pulsing plus a delay at the end

int debug_led_steps (int pulses)
{
  return pulses * (pulse_up + pulse_down + pulse_pause) + pulse_endpause;
}

// step through the debug LED pulsing

void debug_led_cycle (unsigned long ms, int pulses)
{
  // sequence: pulse_up steps from dim > bright,
  // pulse_down steps from bright > dim,
  // pulse_pause between pulses,
  // after the last pulse an additional delay of pulse_endpause

  if (! (ms % pulse_step_time))
  {
    if (debug)
    {
      Serial.print(F("debug_led_cycle (step "));
      Serial.print(pulse_cur);
      Serial.println(F(")"));
    }

    if (pulse_cur < pulses * (pulse_up + pulse_down + pulse_pause))
    {
      // figure out where we are within the pulse
      int pulse_mid = pulse_cur % (pulse_up + pulse_down + pulse_pause);
      if (pulse_mid < pulse_up)
      {
        analogWrite(DEBUG_LED, 255 - (pulse_mid * pulse_up_jump));
      }
      else if (pulse_mid < pulse_up + pulse_down)
      {
        analogWrite(DEBUG_LED, (pulse_mid - pulse_up) * pulse_down_jump);
      }
      else
      {
        // keep the LED off during pauses
        analogWrite(DEBUG_LED, 255);
      }
    }
    else
    {
      // we are in the final delay, so do not do anything active
    }

    pulse_cur++;
    if (pulse_cur == pulse_steps)
    {
      pulse_cur = 0;
    }
  }

}

// step through debug test pattern

void debug_chase_cycle (unsigned long ms)
{
  // sequence: turn on LED #1 and turn off the GI
  // then every step afterward advance the active LED to the next one,
  // and after the last (index NUM_LEDS) LED is lit then turn on all LEDs,
  // turn off #1 and turn on the GI
  // then every step afterward advance the off LED

  if (! (ms % debugled_step_time))
  {
    if (debug)
    {
      Serial.print(F("debug_chase_cycle (step "));
      Serial.print(debugled_cur);
      Serial.println(F(")"));
    }

    if (debugled_cur < NUM_LEDS)
    {
      // advance the single lit LED
      memset(seqrow, 0, sizeof(seqrow));
      seqrow[debugled_cur] = '1' - 48;
      matrix_row_play(lc, RENDER_METHOD, seqrow);
      analogWrite(GI, 255);
    }
    else
    {
      // advance the single unlit LED
      memset(seqrow, 1, sizeof(seqrow));
      seqrow[debugled_cur - NUM_LEDS] = '0' - 48;
      matrix_row_play(lc, RENDER_METHOD, seqrow);
      analogWrite(GI, 0);
    }

    debugled_cur++;
    if (debugled_cur == debugled_steps)
    {
      debugled_cur = 0;
    }
  }
}

// determine net tick time (factoring in the speed percentage)

unsigned int tick_time(int base, int percent)
{
  // 0.000001 is "division by 0" protection for the tick_adjusted calculation

  float tick_scale = (float) (percent + 0.000001) / 100;
  float tick_adjusted = base / tick_scale;
  if (debug)
  {
    Serial.print(F("tick base: "));
    Serial.print(base);
    Serial.print(F(" scale: "));
    Serial.print(tick_scale);
    Serial.print(F(" adjusted: "));
    Serial.println(int(tick_adjusted));
  }
  return int(tick_adjusted);
}

// copy a sequence row

void row_copy(byte * seqrow_prev, byte * seqrow)
{
  for (int i=0; i<NUM_LEDS; i++)
  {
    seqrow_prev[i] = seqrow[i];
  }
}

// read new tick time from SD card

unsigned int control_tick(File &sf)
{
  unsigned int tick_new = 0;

  for (int i=4; i>=0; i--)
  {
    unsigned int factor = 1;
    for (int j=0; j<i; j++)
    {
      factor *= 10;
    }
    tick_new += (sf.read() - 48) * factor;
  }

  return tick_new;
}

// read new speed from SD card

unsigned int control_speed(File &sf)
{
  unsigned int speed_new = 0;

  for (int i=2; i>=0; i--)
  {
    unsigned int factor = 1;
    for (int j=0; j<i; j++)
    {
      factor *= 10;
    }
    speed_new += (sf.read() - 48) * factor;
  }

  if (! speed_new)
  {
    // minimum speed is 1 to avoid freezing and division by 0

    speed_new = 1;
  }

  return speed_new;
}

// read new dimming value from SD card

unsigned int control_dim(File &sf)
{
  unsigned int dim_new = 0;

  for (int i=2; i>=0; i--)
  {
    unsigned int factor = 1;
    for (int j=0; j<i; j++)
    {
      factor *= 10;
    }
    dim_new += (sf.read() - 48) * factor;
  }

  if (dim_new > 100)
  {
    dim_new = 100;
  }

  return dim_new;
}

// read new brightness from SD card

unsigned int control_brightness(File &sf)
{
  unsigned int brightness_new = 0;

  for (int i=1; i>=0; i--)
  {
    unsigned int factor = 1;
    for (int j=0; j<i; j++)
    {
      factor *= 10;
    }
    brightness_new += (sf.read() - 48) * factor;
  }

  if (brightness_new > 15)
  {
    return MATRIX_BRIGHT;
  }

  return brightness_new;
}

// handle all sequence loop activities such as: start, end, jumping back, and
// exiting the loop.

void control_loop(File &sf)
{
  if (debug)
  {
    Serial.print(F("loop action: "));
  }

  if (sf.peek() == 's') // 's', i.e. 'ls', i.e. 'loop start'
  {
    sf.read(); // advance the file pointer
    if (debug)
    {
      Serial.println(F("start"));
    }

    // normally purging the CR/LF is done in loop(), but here we need to do it
    // explicitly so that our loop_pointer points to the correct location
    crlf(sf);

    loop_pointer = sf.position();
  }
  else if (sf.peek() == 'e') // 'e', i.e. 'le123', i.e. 'loop end, repeat 123 times'
  {
    sf.read(); // advance the file pointer
    if (debug)
    {
      Serial.println(F("end"));
    }

    if (loop_counter > 0)
    {
      if (debug)
      {
        Serial.print(F("loops to go: "));
        Serial.println(loop_counter);
      }
      loop_counter--;

      if (loop_counter > 0)
      {
        if (debug)
        {
          Serial.print(F("loop jumping back to position: "));
          Serial.println(loop_pointer);
        }
        sf.seek(loop_pointer);
      }
      else
      {
        if (debug)
        {
          Serial.println(F("loop exiting"));
        }
        for (int i=2; i>=0; i--)
        {
          sf.read(); // consume the 123 in 'le123' so we can advance to the next row
        }
      }
    }
    else
    {
      for (int i=2; i>=0; i--)
      {
        unsigned int factor = 1;
        for (int j=0; j<i; j++)
        {
          factor *= 10;
        }
        loop_counter += (sf.read() - 48) * factor;
      }
      if (debug)
      {
        Serial.print(F("establishing loop count: "));
        Serial.println(loop_counter + 1);
      }
      if (debug)
      {
        Serial.print(F("loop jumping back to position: "));
        Serial.println(loop_pointer);
      }
      sf.seek(loop_pointer);
    }
  }
}

// read GI brightness from SD card (0 = bright, 255 = off)

unsigned int control_gi(File &sf)
{
  if (debug)
  {
    Serial.print(F("general illumination: "));
  }

  unsigned int gi_new = 0;

  for (int i=2; i>=0; i--)
  {
    unsigned int factor = 1;
    for (int j=0; j<i; j++)
    {
      factor *= 10;
    }
    gi_new += (sf.read() - 48) * factor;
  }

  if (debug)
  {
    Serial.println(gi_new);
  }

  return gi_new;
}
