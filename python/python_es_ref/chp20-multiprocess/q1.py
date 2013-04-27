#!/usr/bin/env python
#coding: utf-8

import multiprocessing
import time

def consumer(input_q):
	while True:
		item = input_q.get()
		print("consumer ", item)
		input_q.task_done()

def producer(sequence, output_q):
	for item in sequence:
		print("producer ", item)
		output_q.put(item)

if __name__ == "__main__":
	#Queue in two process
	q = multiprocessing.JoinableQueue()
	cons_p = multiprocessing.Process(target=consumer, args=(q,))
	cons_p.daemon = True
	cons_p.start()

	sequence = [1, 2, 3, 4]
	producer(sequence, q)

	#with queue end
	q.join()
