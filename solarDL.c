#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <time.h>
#include <unistd.h>

#include "solarDL.h"

// Use GPIO Pin 18, which is Pin 1 for wiringPi library
#define DL_PIN 1

#define TIME_DIFF_LONG   1800
#define TIME_DIFF_SHORT  1400

#define NUM_OF_SYNC_BITS 17

//#define DEBUG_READ_BIT
//#define DEBUG_BLD_BIT
//#define DEBUG_BLD_BYTE
//#define DEBUG_DL_DATA

union
{
  uint8_t data_raw[DL_NUM_OF_BYTES];
  struct dl_data data_new;
} dl;

// the event counter
volatile int32_t t_diff;
volatile int32_t t_new;
volatile int32_t t_old;
volatile uint8_t bit;
volatile uint8_t bit_old_short;
volatile uint8_t flgNew;
volatile uint8_t data[DL_NUM_OF_BYTES];
volatile uint8_t cntSync = 0;
volatile int8_t cntByte = 0;
volatile uint8_t idx;
volatile uint8_t flgOnce = 1;
volatile uint8_t flgNewData = 0;
volatile uint8_t flgStopStrtBitOk = 1;


// -------------------------------------------------------------------------
// isr:  called every time an event occurs
// -------------------------------------------------------------------------
void isrDlPin(void) {

  struct timespec tp_new;

  clock_gettime(CLOCK_REALTIME, &tp_new);
  t_new = tp_new.tv_nsec / 1000;

  t_diff = t_new - t_old;
  if( t_diff < 0 )
    t_diff = 1000000L + t_diff;

  t_old = t_new;

#ifdef DEBUG_READ_BIT
  printf("1: %ld;%d\n", t_diff, digitalRead(DL_PIN));
#endif

  flgNew = 0;
  if( digitalRead(DL_PIN) == 0 )
  {
    if( t_diff > TIME_DIFF_LONG )
    {
      bit = 1;
      flgNew = 1;
    }

    if( t_diff < TIME_DIFF_SHORT && bit_old_short == 1 )
    {
      bit = 1;
      bit_old_short = 0;
      flgNew = 1;
    }
    else if( t_diff < TIME_DIFF_SHORT )
    {
      bit_old_short = 1;
    }
    else
    {
      bit_old_short = 0;
    }
  }
  else
  {
    if( t_diff > TIME_DIFF_LONG )
    {
      bit = 0;
      flgNew = 1;
    }

    if( t_diff < TIME_DIFF_SHORT && bit_old_short == 1 )
    {
      bit = 0;
      bit_old_short = 0;
      flgNew = 1;
    }
    else if( t_diff < TIME_DIFF_SHORT )
    {
      bit_old_short = 1;
    }
    else
    {
      bit_old_short = 0;
    }
  }

  if( flgNew == 1 )
  {
#ifdef DEBUG_BLD_BIT
    printf("%d\n", bit);
#endif

#ifdef DEBUG_BLD_BYTE
    if( idx == 0 && cntByte == 0 && ((cntSync == (NUM_OF_SYNC_BITS + 1) && flgOnce == 1) || cntSync == NUM_OF_SYNC_BITS) )
      printf("\n");


    printf("2: %d",bit);
#endif

    if( ((cntSync == (NUM_OF_SYNC_BITS + 1) && flgOnce == 1) || cntSync == NUM_OF_SYNC_BITS) )
    {
      flgOnce = 0;

      if( (cntByte == 0 && bit == 1) /*|| (cntByte == 9 || bit == 0)*/ )
      {
        // cancel because stop or start bit wrong
#ifdef DEBUG_BLD_BYTE
        printf("#");
#endif
        flgStopStrtBitOk = 0;
      }


      if( cntByte > 0 && cntByte < 9 )
      {
        data[idx] = data[idx] | (bit << (cntByte - 1));
      }
      else
      {
        // Start and Stop Bit
        if( cntByte >= 9 )
        {
          if( idx < CHECKSUM_INDEX )
          {
            data[CHECKSUM_NEW_INDEX] = data[CHECKSUM_NEW_INDEX] + data[idx];
          }

#ifdef DEBUG_BLD_BYTE
          printf(" - %02x\n", data[idx]);
#endif

          if( idx == CHECKSUM_INDEX )
          {
#ifdef DEBUG_BLD_BYTE
            printf("\nCheck: %02x = %02x\n",  data[CHECKSUM_INDEX], data[CHECKSUM_NEW_INDEX]);
#endif

            if( data[CHECKSUM_INDEX] == data[CHECKSUM_NEW_INDEX] && flgStopStrtBitOk == 1 )
            {
              for( idx = 0; idx < DL_NUM_OF_BYTES; idx++ )
              {
                dl.data_raw[idx] = data[idx];
              }
              flgNewData = 1;
            }
            flgStopStrtBitOk = 1;

            for( idx = 0; idx < DL_NUM_OF_BYTES; idx++ )
            {
              data[idx] = 0;
            }

            idx = 0;
            cntSync = 0;
          }
          else
          {
            idx++;
          }
        }
      }
      if( cntByte < 9 )
        cntByte++;
      else
        cntByte = 0;
    }
    else
    {
      if( bit == 1 )
        cntSync++;
      else
        cntSync = 0;
    }
  }
}


// -------------------------------------------------------------------------
// main
// -------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  uint8_t i;

  /* source file in char source[512] and dest file in char dest[512] */
  char str[512];
  char str2[32];
  char pathRrdUpdate[512];
  char pathHtmlUpdate[512];

  FILE *file = NULL;
  float fMWh, fMWhOld, fMWhDiff;
  char bOnce = 0;

  if (argc > 0)
  {
    // Open file whose path is passed as an argument
    file = fopen( argv[1], "w" );

    // fopen returns NULL pointer on failure
    if ( file == NULL)
    {
      printf("Error: Could not open file.");
    }
    else
    {
      memset(pathRrdUpdate, '\0', sizeof(512));
      sprintf(pathRrdUpdate, "%s", argv[1]);
      memset(pathHtmlUpdate, '\0', sizeof(512));
      sprintf(pathHtmlUpdate, "%s.js", argv[1]);

      // Closing file
      fclose(file);
    }
  }
  else
  {
    printf("Error: rrd update script missing.");
    return 1;
  }


  // sets up the wiringPi library
  if (wiringPiSetup () < 0) {
      printf ("Error: Unable to setup wiringPi.");
      return 1;
  }

  // set Pin 17/0 generate an interrupt on high-to-low transitions
  // and attach myInterrupt() to the interrupt
  if ( wiringPiISR (DL_PIN, INT_EDGE_BOTH, &isrDlPin) < 0 ) {
      printf ("Error: Unable to setup ISR.");
      return 1;
  }

  // set pullup
  pullUpDnControl(DL_PIN, PUD_UP);

  // display counter value every second.
  while (1)
  {
    if( flgNewData == 1 && dl.data_new.Jhr != 10 )
    {
      flgNewData = 0;
      // new line !!!

#ifdef DEBUG_DL_DATA
      for( i=0; i < DL_NUM_OF_BYTES; i++ )
      {
        printf("%02d ", i+1);
      }
      printf("\n");

      for( i=0; i < DL_NUM_OF_BYTES; i++ )
      {
        printf("%02x ", dl.data_raw[i]);
      }
      printf("\n");
#endif


      // int buf[ridx].Temp[6];
      for(i=0; i<6; i++)
      {
        if (dl.data_new.Temp[i] == 0xFFFFA000 || dl.data_new.Temp[i] == 0x2FFF)
        {
          dl.data_new.Temp[i] = 0;
        }
        else
        {
          if (dl.data_new.Temp[i] & 0x8000) dl.data_new.Temp[i] |=  0xF000;
          else                              dl.data_new.Temp[i] &= ~0xF000;
        }
      }

      time_t t = time(NULL);
      struct tm tm = *localtime(&t);

      sprintf(str, "%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%3.1f:%d:%d:%d:%3.3f:%2.1f:%3.1f",
                                                             (float)dl.data_new.Temp[0]/10,
                                                             (float)dl.data_new.Temp[1]/10,
                                                             (float)dl.data_new.Temp[2]/10,
                                                             (float)dl.data_new.Temp[3]/10,
                                                             (float)dl.data_new.Temp[4]/10,
                                                             (float)dl.data_new.Temp[5]/10,
                                                             0x01 & dl.data_new.AusgByte,
                                                             0x01 & (dl.data_new.AusgByte >> 1),
                                                             0x01 & (dl.data_new.AusgByte >> 2),
                                                             (float)dl.data_new.MWh_Lo + (float)dl.data_new.KWh / 10000,
                                                             (float)dl.data_new.KW / 10,
                                                             (float)dl.data_new.Lh
                                                             );

      file = fopen( pathRrdUpdate, "w" );
      fprintf(file, "%s\n", str);
      fclose(file);

      fMWh = (float)dl.data_new.MWh_Lo + (float)dl.data_new.KWh / 10000;
      if( ((tm.tm_hour == 23) && (tm.tm_min == 59)) || (bOnce == 0) )
      {
        bOnce = 1;
        fMWhOld = fMWh;
      }
      fMWhDiff = fMWh - fMWhOld;


      sprintf(str2, ":%d:%1.3f", dl.data_new.DrehzA1, fMWhDiff );
      strcat(str, str2);

      file = fopen( pathHtmlUpdate, "w" );
      fprintf(file, "%s\n", str);
      fclose(file);

      printf("%04d%02d%02d %02d:%02d:%02d - ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
      printf("%s\n", str);
    }

    fflush(stdout);

    delayMicroseconds ( 1000000 );
  }
  return 0;
}
