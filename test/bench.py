import time
import strutil
import cgi

text = open('/Users/evan/code/strutil/test/xanadu.txt').read()

print 'xanadu'
print '------'

t = time.time()
strutil.escape(text)
print 'fast = %s' % (time.time() - t)

t = time.time()
cgi.escape(text)
print 'slow = %s' % (time.time() - t)

text = open('/Users/evan/code/strutil/test/yelp-front-page.txt').read()

print 'yelp html'
print '---------'

t = time.time()
strutil.escape(text)
print 'fast = %s' % (time.time() - t)

t = time.time()
cgi.escape(text)
print 'slow = %s' % (time.time() - t)

