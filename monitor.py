#!/usr/bin/python

"""
	Modified by KieranC to submit pulse count to Open Energy Monitor EmonCMS API

	Power Monitor
	Logs power consumption to an SQLite database, based on the number
	of pulses of a light on an electricity meter.

	Copyright (c) 2012 Edward O'Regan

	Permission is hereby granted, free of charge, to any person obtaining a copy of
	this software and associated documentation files (the "Software"), to deal in
	the Software without restriction, including without limitation the rights to
	use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
	of the Software, and to permit persons to whom the Software is furnished to do
	so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
"""

import time, os, subprocess, httplib, datetime
from apscheduler.scheduler import Scheduler

# The next 2 lines enable logging for the scheduler. Uncomment for debugging.
import logging
logging.basicConfig(filename='/var/log/monitorpy.log', level=logging.WARNING)

pulsecount=0
power=0

# Start the scheduler
sched = Scheduler()
sched.start()


# This function monitors the output from gpio-irq C app
# Code from vartec @ http://stackoverflow.com/questions/4760215/running-shell-command-from-python-and-capturing-the-output
def runProcess(exe):
    logging.debug('runProcess started')
    p = subprocess.Popen(exe, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    while(True):
      logging.debug('runProcess while-loop entered')
      retcode = p.poll() #returns None while subprocess is running
      logging.debug('runProcess poll ended, retcode: %s', retcode)
      line = p.stdout.readline()
      logging.debug('runProcess readline ended, line: %s', line)
      yield line
      if(retcode is not None):
        break


# Every minute this function converts the number of pulses over the last minute into a power value and sends it to EmonCMS
@sched.interval_schedule(minutes=1)
def SendPulses():
	global pulsecount
	global power
	logging.debug('Pulses: %s', pulsecount) # Uncomment for debugging.
	# The next line calculates a power value in watts from the number of pulses, my meter is 1000 pulses per kWh, you'll need to modify this if yours is different.
	power = pulsecount * 60
	logging.debug('Power: %sW', power) # Uncomment for debugging.
	pulsecount = 0;
	timenow = time.strftime('%s')
	logging.debug('Time: %s (%s)', timenow, time.strftime('%c',time.localtime(float(timenow)))) # Uncomment for debugging.
        url = ("/input/post?time=%s&node=pulsepi&json={power1:%i}&apikey=4613f98e5be0f9d5d2000530fde51573") % (timenow, power) # You'll need to put in your API key here from EmonCMS
        connection = httplib.HTTPConnection("localhost")
        connection.request("GET", url)


for line in runProcess(["/usr/local/bin/gpio-pigtest"]):
    pulsecount += 1
