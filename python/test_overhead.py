from __future__ import print_function
import subprocess as sp
import time
import signal
import random
import os
import csv
import numpy as np

LIBLITMUS = "../../liblitmus"
FEATHER_TRACE = "../../feather-trace-tools"

def main():
	numpages = 500
	m = 8
	taskset = []
	U = 0.20
	Tmin = 10
	Tmax = 100
	Tg = 0.1
	Duration = 300
	outtaskcsv = 'taskcsv.csv'
	schedname = 'GSN-EDF'
	locking_protocol = 'SMLP'
	lockmask = 15
	initmask = 4095

	def cleanup_rtspin(*args):
		for task in taskset:
			task.kill()
		exit()

	signal.signal(signal.SIGINT, cleanup_rtspin)
	signal.signal(signal.SIGSEGV, cleanup_rtspin)
	signal.signal(signal.SIGTERM, cleanup_rtspin)

	#n_array = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
	n_array = [100]
	#num_tasks = 200

	for num_tasks in n_array: 
		taskset = []
		print('Begin test for n = ' + str(num_tasks))

		# Create real-time taskset
		tgen = sp.Popen(['../taskgen/taskgen', str(m), str(Tmin), str(Tmax), str(num_tasks), str(U), str(Tg), outtaskcsv])
		tgen.wait()
		time.sleep(1.0)

		ss = sp.Popen([LIBLITMUS + '/setsched ' + schedname], shell=True)
		ss.wait()
		time.sleep(2.0)

		# First create the m background tasks, emulates cold cache
		for i in range(m):
			bkgtask = sp.Popen([LIBLITMUS + '/rtspin', '-B', '-m', str(numpages)])
			taskset.append(bkgtask)
		
		time.sleep(2.0)

		trace = sp.Popen([FEATHER_TRACE + '/ft-trace-overheads','test-overheads'], stdin=sp.PIPE)
		print('starting up tracing framework...')
		time.sleep(5.0)

		##################################################
		# Taskset Setup
		##################################################

		print('Creating tasks...')

		index = 0
		with open(outtaskcsv, 'r') as f:
			reader = csv.DictReader(f)
			for line in reader:
				Ci = float(line['cost'])
				Ti = line['period']
				Di = line['deadline']
				task = sp.Popen([LIBLITMUS + '/rtspin', '-w', '-m', str(numpages), '-X', locking_protocol, '-x', str(initmask), '-M', str(lockmask), '-L', str(Ci / 10),
					'-d', Di, str(Ci), Ti, str(Duration+11+1.5*index)])
				taskset.append(task)
				index += 1
		time.sleep(10)

		releaser = sp.Popen([LIBLITMUS + '/release_ts', '-q', '1000'])
		releaser.wait()

		time.sleep(Duration)
		print('specified time has passed, ending tracing')
		trace.communicate(b'\n')
		trace.wait()

		print('letting system settle down')

		for task in taskset:
			task.kill()
			time.sleep(1.5)

		time.sleep(3)

		trace = sp.Popen([LIBLITMUS + '/setsched', 'Linux'], shell=False)
		trace.wait()

		##################################################
		# Process overhead data
		##################################################

		sorter = sp.Popen([FEATHER_TRACE +
												'/ft-sort-traces overheads_*.bin 2>&1'], shell=True)
		sorter.wait()
		print('Sorter finished')

		extractor = sp.Popen([FEATHER_TRACE +
												'/ft-extract-samples overheads_*.bin 2>&1'], shell=True)
		extractor.wait()
		print('Extractor finished')

		combiner = sp.Popen([FEATHER_TRACE +
												'/ft-combine-samples --std overheads_*.float32 2>&1'], shell=True)
		combiner.wait()
		print('Combiner finished')

		counter = sp.Popen([FEATHER_TRACE +
												'/ft-count-samples combined-overheads_*.float32 > counts.csv'],
												shell=True)
		counter.wait()
		print('Counter finished')

		selector = sp.Popen([FEATHER_TRACE +
												'/ft-select-samples counts.csv combined-overheads_*.float32 2>&1'],
												shell=True)
		selector.wait()
		print('Selector finished')

		stats = sp.Popen([FEATHER_TRACE +
												'/ft-compute-stats combined-overheads_*.sf32 > '
												+ str(num_tasks) + '_' + schedname + '_stats.csv'], shell=True)
		stats.wait()
		print('Stats computation finished')

		cleaner = sp.Popen(['rm overheads_*'], shell=True)
		cleaner.wait()
		cleaner = sp.Popen(['rm combined-overheads_*'], shell=True)
		cleaner.wait()

		time.sleep(2)
	
	os.remove('rtspin-locks') # Clean up lock file

if __name__ == '__main__':
	main()
