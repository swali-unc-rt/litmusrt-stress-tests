from __future__ import print_function
import subprocess as sp
import time
import signal
import random
import os
import csv
#import numpy as np

LIBLITMUS = "../../liblitmus"
FEATHER_TRACE = "../../feather-trace-tools"

def main():
    numpages = 500
    m = 8
    schedname = 'GSN-EDF'
    taskset = []

    def cleanup_rtspin(*args):
        for task in taskset:
            task.kill()
        exit()

    signal.signal(signal.SIGINT, cleanup_rtspin)
    signal.signal(signal.SIGSEGV, cleanup_rtspin)
    signal.signal(signal.SIGTERM, cleanup_rtspin)

    ss = sp.Popen([LIBLITMUS + '/setsched ' + schedname], shell=True)
    ss.wait()
    time.sleep(2.0)

    # First create the m background tasks, emulates cold cache
    for i in range(m):
        bkgtask = sp.Popen([LIBLITMUS + '/rtspin', '-B', '-m', str(numpages)])
        taskset.append(bkgtask)
    time.sleep(2.0)

    trace = sp.Popen([FEATHER_TRACE + '/st-trace-schedule','test-fsmlp'], stdin=sp.PIPE)
    print('starting up tracing framework...')
    time.sleep(5.0)

    fsmlptask = sp.Popen(['../bin/fsmlptest'], shell=True)
    fsmlptask.wait()
    time.sleep(2.0)
    trace.communicate(b'\n')
    trace.wait()

    print('letting system settle down')

    for task in taskset:
        task.kill()
        time.sleep(1.5)
    
    trace = sp.Popen([LIBLITMUS + '/setsched', 'Linux'], shell=False)
    trace.wait()

    # st-job-stats schedule_host=*_trace=my-trace-experiment_cpu=*.bin > response_times.csv
    stats = sp.Popen([FEATHER_TRACE +
												'/st-job-stats schedule_host=*_trace=test-fsmlp_cpu=*.bin > response_times.csv'], shell=True)
    stats.wait()
    print('Stats finished')
    
    cleaner = sp.Popen(['rm schedule_host=*'], shell=True)
    cleaner.wait()

if __name__ == '__main__':
	main()
