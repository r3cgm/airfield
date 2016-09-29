#include <SD.h>;

byte sd_init(Sd2Card card, SdVolume volume, int cs, char * fn)
{
  pinMode(cs, OUTPUT);
  if (! card.init(SPI_HALF_SPEED, cs))
  {
    return 2;
  }

  if (! volume.init(card))
  {
    return 3;
  }

  // repeat SD card init until successful.  should work the first time
  // through, but does not due to unidentified / race condition issues

  while (1)
  {
    if (SD.begin(cs))
    {
      break;
    }
  }

  return 1; // success
}

File sd_open(char * fn)
{
  File seqfile;
  seqfile = SD.open(fn);
  return seqfile;
}

// purge CR/LF and advance file pointer

void crlf(File &sf)
{
  if (sf.peek() == 13)
  {
    sf.read();
  }
  
  if (sf.peek() == 10)
  {
    sf.read();
  }
}

// return control character and advance the file pointer, if found

byte sd_control(File &sf)
{
  while ((sf.peek() == 13) || (sf.peek() == 10))
  {
    sf.read();
  }

  // 48 == '0' == off, 49 == '1' == on, 50 == '2' == random
  if ((sf.peek() != 48) && (sf.peek() != 49) && (sf.peek() != 50))
  {
    byte control = sf.read();
    return control;
  }

  return 0;
}

void sd_row_read(File &sf, byte * seqrow, int numleds)
{
  for (int i=0; i<numleds; i++)
  {
    seqrow[i] = sf.read() - 48; // convert ASCII to decimal
  }

  crlf(sf);
}
