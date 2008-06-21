import unittest
import strutil

def slow_escape(s):
	t = s.replace('&', '&amp;')
	t = t.replace('"', '&quot;')
	t = t.replace('\\', '&#39;')
	t = t.replace('<', '&lt;')
	return t.replace('>', '&gt;')

class StrutilTestCase(unittest.TestCase):

	def test_regular_str(self):
		# unescaped strings shouldn't return a new reference
		for s in ('', 'a', 'foo', 'foobarbaz'):
			assert strutil.escape(s) is s
	
	def test_str_needs_escaping(self):
		for s in ('', '&', '\\', '"', 'foo&bar', 'foo&bar"baz\\'):
			assert strutil.escape(s) == slow_escape(s)

	def test_regular_uni(self):
		# unescaped unicodes shouldn't return a new reference
		for s in (u'', u'a', u'foo', u'foobarbaz'):
			assert strutil.escape(s) is s

	def test_uni_needs_escaping(self):
		for s in (u'', u'&', u'\\', u'"', u'foo&bar', u'foo&bar"baz\\'):
			assert strutil.escape(s) == slow_escape(s)

if __name__ == '__main__':
	unittest.main()
