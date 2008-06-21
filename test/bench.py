import time
import strutil
import cgi
import os
import test

text = unicode(open('xanadu.txt').read())

print 'xanadu'
print '------'

t = time.time()
strutil.escape(text)
print 'fast = %s' % (time.time() - t)

t = time.time()
test.slow_escape(text)
print 'slow = %s' % (time.time() - t)

text = unicode(open('yelp.txt').read())

# make the unicode string object really big so lots of replacements will happen
for x in range(7):
	text += text

print
print 'yelp html'
print '------------'

t = time.time()
f = strutil.escape(text)
print 'fast = %s' % (time.time() - t)

t = time.time()
s = test.slow_escape(text)
print 'slow = %s' % (time.time() - t)
