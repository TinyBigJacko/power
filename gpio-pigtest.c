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

int max_timeout = 15000; //15 sec max interrupt timeout
uint32_t previous_tick = 0; //used for preventing early retriggers
uint32_t pulse_duty_cycle = 30000; //trigger threshold (30ms, avg length of good pulse)
uint32_t min_interval;

_Bool logging = 0; //set this to 1 to log to file

/*
 *********************************************************************************
 * getTimeStamp (updates a byref buffer to a desired dateformat)
 *********************************************************************************
 */
void getTimeStamp(char* buff, size_t buffersize) {
	time_t t = time(NULL);
	strftime(buff, buffersize, "%x %X %Z", localtime(&t));
}

/*
 *********************************************************************************
 * myInterrupt (triggered every interval once main loop proceeding)
 *********************************************************************************
 */
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

	//prepare the timestamp (used if we're logging)
	char tstampbuf[80];
	getTimeStamp(tstampbuf, sizeof tstampbuf);

	switch(level)
	{
			case 0:
			case 1:
			min_interval = pulse_duty_cycle;
			printf ("Interrupt\n");
			fflush (stdout);
			if (logging)
			{
				fprintf(fptr,"%s: (Tick: %u) Interrupted (Gap: %i usec)\n", tstampbuf, tick, interval_seen);
			}
			break;

			case 2:
			min_interval = 0;
			if (logging)
			{
				fprintf(fptr,"%s: (Tick: %u) Timeout (Gap: %i usec)\n", tstampbuf, tick, interval_seen);
			}
			break;

			case 99:
			min_interval = pulse_duty_cycle - interval_seen; //bit buggy if there are multiple false-triggers
			if (logging)
			{
				fprintf(fptr,"%s: (Tick: %u) False Retrigger Skipped (Gap: %i usec)\n", tstampbuf, tick, interval_seen);
			}
			break;

			default:
			min_interval = 0;
			if (logging)
			{
				fprintf(fptr,"%s: (Tick: %u) Bad Trigger (Gap: %i usec)\n", tstampbuf, tick, interval_seen);
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

	min_interval = pulse_duty_cycle; //start off with min_interval equal to a normal pulse duty cycle

	char tstampbuf[80];
	getTimeStamp(tstampbuf, sizeof tstampbuf);

	puts(tstampbuf);

	if (logging)
	{
		fptr2 = fopen("/var/log/gpio-pigtest.log", "a");
		fprintf(fptr2,"%s: New Run!\n", tstampbuf);
	}
	if (gpioInitialise() < 0)
	{
		// pigpio initialisation failed.
		if (logging)
		{
			fprintf(fptr2,"%s: Pigpio initialisation failed; Quitting!\n", tstampbuf);
			fclose(fptr2);
			return 0;
		}
	}
	else
	{
		// pigpio initialised okay.
		if (logging)
		{
			fprintf(fptr2,"%s: Pigpio initialisation OK\n", tstampbuf);
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
