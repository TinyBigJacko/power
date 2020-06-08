/*
 *
 * refactor using pigpio
 *
 ***********************************************************************
 */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <pigpio.h>
#include <unistd.h>
#include <time.h>

//timestamp vars
time_t unixtime;
char tstamp[80];

int max_timeout = 15000; //15 sec max interrupt timeout
uint32_t previous_tick = 0; //used for preventing early retriggers
uint32_t slope_skip = 30000; //trigger threshold (30ms, avg length of good pulse)
uint32_t min_interval;

_Bool logging = 0; //set this to 1 to log to file

void myInterrupt(int gpio, int level, uint32_t tick) {
  
  FILE * fptr;

  uint32_t interval_seen = tick - previous_tick;
  
  if (interval_seen < min_interval) 
 // if (((tick < previous_tick + min_interval) && (previous_tick + min_interval <= 4294967295) && previous_tick > 0) || ((previous_tick + min_interval > 4294967295) && (tick < previous_tick + min_interval - 4294967296)))
  {
      level = 99;
  }

  previous_tick = tick;
  
  if (logging)
  {
    // open up the file (if we're logging)
    fptr = fopen("/var/log/gpio-pigtest.log", "a");
  }

  //prepare the timestamps (if we're logging)
  time(&unixtime);
  strftime(tstamp,80,"%x %X %Z",localtime(&unixtime));

  switch(level)
  {
      case 0:
      case 1:
      min_interval = slope_skip;
      printf ("Interrupt\n");
      fflush (stdout);      
      if (logging)
      {
        fprintf(fptr,"%s: (Tick: %u) Interrupted (Gap: %i usec)\n", tstamp, tick, interval_seen);
      }
      break;

      case 2:
      min_interval = 0;
      if (logging)
      {
        fprintf(fptr,"%s: (Tick: %u) Timeout (Gap: %i usec)\n", tstamp, tick, interval_seen);
      }
      break;
      
      case 99:
      min_interval = slope_skip - interval_seen; //bit buggy if there are multiple false-triggers
      if (logging)
      {
        fprintf(fptr,"%s: (Tick: %u) False Retrigger Skipped (Gap: %i usec)\n", tstamp, tick, interval_seen);
      }
      break;
      
      default:
      min_interval = 0;
      if (logging)
      {
        fprintf(fptr,"%s: (Tick: %u) Bad Trigger (Gap: %i usec)\n", tstamp, tick, interval_seen);
      }
  }

  if (logging)
  {
    fclose(fptr);
  }
}

/*
 *********************************************************************************
 * main
 *********************************************************************************
 */

int main (void)
{
  FILE * fptr2;
  
  min_interval = slope_skip; //start off with min_interval equal to a normal pulse duty cycle

  time(&unixtime);
  strftime(tstamp,80,"%x %X %Z",localtime(&unixtime));

  if (logging)
  {
    fptr2 = fopen("/var/log/gpio-pigtest.log", "a");
    fprintf(fptr2,"%s: New Run!\n", tstamp);
  }
  if (gpioInitialise() < 0)
  {
    // pigpio initialisation failed.
    time(&unixtime);
    strftime(tstamp,80,"%x %X %Z",localtime(&unixtime));
    if (logging)
    {
      fprintf(fptr2,"%s: Pigpio initialisation failed; Quitting!\n", tstamp);
      fclose(fptr2);
      return 0;
    }
  }
  else
  {
    // pigpio initialised okay.
    time(&unixtime);
    strftime(tstamp,80,"%x %X %Z",localtime(&unixtime));
    if (logging)
    {
      fprintf(fptr2,"%s: Pigpio initialisation OK\n", tstamp);
      fclose(fptr2);
    }
    gpioSetISRFunc(18, FALLING_EDGE, max_timeout, myInterrupt); 
  }

  for (;;)
  {
    sleep(UINT_MAX);
  }

  gpioTerminate();
  return 0;
}
