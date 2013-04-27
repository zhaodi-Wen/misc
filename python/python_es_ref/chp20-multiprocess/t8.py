#!/usr/bin/env python
#coding: utf-8
import threading
import time

a = []
l = threading.Lock()
c = threading.Condition(l)

def pop(interval):
	while True:
		l.acquire()
		if len(a) > 0:
			print "pop",
			while len(a):
				print "%d" % a.pop(),
			print ""
		c.wait()
		l.release()
def main():
	t = threading.Thread(target=pop, args=(3,))
	t.daemon = True
	t.start()
	
	while True:
		l.acquire()
		print 'push',
		for x in range(10):
			print x,
			a.append(x)

		c.notify()
		l.release()
		time.sleep(1)
	t.join()

if __name__ == "__main__":
	main()
