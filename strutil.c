/* This module is just a proof of concept. It is not fully tested. Do not use it in production. */

#include <Python.h> /* includes string.h */

/* This assumes little-endianness */
#define ucs_amp		(Py_UNICODE) '&'
#define ucs_lt		(Py_UNICODE) '<'
#define ucs_gt		(Py_UNICODE) '>'
#define ucs_slash	(Py_UNICODE) '\\'
#define ucs_quot	(Py_UNICODE) '\"'

#define ucs_comma   (Py_UNICODE) ','
#define ucs_semic   (Py_UNICODE) ';'
#define ucs_dot     (Py_UNICODE) '.'
#define ucs_space   (Py_UNICODE) ' '
#define ucs_tab     (Py_UNICODE) '\t'
#define ucs_new     (Py_UNICODE) '\n'
#define ucs_lbrace  (Py_UNICODE) '{'
#define ucs_rbrace  (Py_UNICODE) '}'
#define ucs_openp   (Py_UNICODE) '('
#define ucs_closep  (Py_UNICODE) ')'

/* kind of like isalpha, but only for lower case characters */
#define UCS_ISALPHA (c) ((c >= (Py_UNICODE) 'a') && (c <= (Py_UNICODE) 'z'))

static Py_UNICODE *amp_buf;
static Py_UNICODE *lt_buf;
static Py_UNICODE *gt_buf;
static Py_UNICODE *slash_buf;
static Py_UNICODE *quot_buf;

/* Holds the unicode representation of "://" */
static PyObject *uri_scheme_sep;
static PyObject *href_start;
static PyObject *href_end1;
static PyObject *href_end2;

/* Forward declarations */
static PyObject* escape_str(PyStringObject *input);
static PyObject* escape_uni(PyUnicodeObject *input);
//static PyObject* linkify_uni(PyUnicodeObject *input);

/* HTML-escapes a string. This just calls escape_str or escape_uni (depending
 * on the type of the argument). */
static PyObject* escape(PyObject *self, PyObject *args) {

	PyObject *input;
	if (!PyArg_ParseTuple(args, "O", &input))
		return NULL;

	if (PyString_CheckExact(input))
		return escape_str((PyStringObject *) input);

	if (PyUnicode_CheckExact(input))
		return escape_uni((PyUnicodeObject *) input);

	PyErr_SetString(PyExc_TypeError, "argument to escape must be a string or unicode object");
	return NULL;
}

/* This is a highly optimized HTML escape method for strings. It directly
 * accesses the character buffers of the input/ouput strings to minimize memory
 * copies. Right now it only works with ASCII strings, but it should be simple
 * to make it work with UTF-8 as well (since all replacement
 * characters/sequences are ASCII). Getting it to work quickly with wide
 * character encodings (e.g. UCS-2) would be a little bit more tricky but still
 * doable.
 */
static PyObject* escape_str(PyStringObject *input) {

	Py_ssize_t len = PyString_GET_SIZE(input);

	/* We optimize for the common case, which is that no changes need to be
	 * made to the string. */
	int i;
	int amps = 0;
	int lt = 0;
	int gt = 0;
	int slash = 0;
	int quot = 0;

	for (i = 0; i < len; i++) {
		switch (input->ob_sval[i]) {
			case '&':
				amps++;
				break;
			case '<':
				lt++;
				break;
			case '>':
				gt++;
				break;
			case '\\':
				slash++;
				break;
			case '\"':
				quot++;
				break;
		}
	}

	/* The common, optimized case. When the input string doesn't have any
	 * characters that need to be escaped just return a reference to the
	 * original string. */
	if (!(amps || lt || gt || slash || quot)) {
		Py_INCREF(input);
		return (PyObject *) input;
	}

	/* The size of the new string */
	Py_ssize_t newlen = len + (4 * (amps + slash)) + (3 * (lt + gt)) + (5 * quot);
	PyStringObject *new_pystr = (PyStringObject *) PyString_FromStringAndSize(NULL, newlen);

	/* Pointers to the internal buffers of the Python string objects */
	char *old = input->ob_sval;
	char *new_str = new_pystr->ob_sval;

	int opos = 0;
	int npos = 0;

	static char *amp_sym = "&amp;";
	static char *lt_sym = "&lt;";
	static char *gt_sym = "&gt;";
	static char *slash_sym = "&#39;";
	static char *quot_sym = "&quot;";

#define UPDATE_STRING(a,b)\
	Py_MEMCPY(new_str + npos, old + opos, i - opos);\
	Py_MEMCPY(new_str + npos + i - opos, a, b);\
	npos += (i - opos) + b;\
	opos = i + 1;

	for (i = 0; i < len; i++) {
		switch (old[i]) {
			case '&':
				UPDATE_STRING(amp_sym, 5);
				break;
			case '<':
				UPDATE_STRING(lt_sym, 4);
				break;
			case '>':
				UPDATE_STRING(gt_sym, 4);
				break;
			case '\\':
				UPDATE_STRING(slash_sym, 5);
				break;
			case '"':
				UPDATE_STRING(quot_sym, 6);
				break;
		}
	}
	if (opos < len)
		Py_MEMCPY(new_str + npos, old + opos, len - opos);

	return (PyObject *) new_pystr;
}

static PyObject* escape_uni(PyUnicodeObject *input) {

	/* Points to the internal Py_UNICODE buffer of the input object */
	Py_UNICODE *input_buf = PyUnicode_AS_UNICODE(input);

	/* This is the number of characters in the string */
	Py_ssize_t len = PyUnicode_GET_SIZE(input);

	/* We optimize for the common case, which is that no changes need to be
	 * made to the string. */
	int i;
	int amps = 0;
	int lt = 0;
	int gt = 0;
	int slash = 0;
	int quot = 0;

	for (i = 0; i < len; i++) {
		switch (input_buf[i]) {
			case ucs_amp:
				amps++;
				break;
			case ucs_lt:
				lt++;
				break;
			case ucs_gt:
				gt++;
				break;
			case ucs_slash:
				slash++;
				break;
			case ucs_quot:
				quot++;
				break;
		}
	}

	/* The common, optimized case. When the input string doesn't have any
	 * characters that need to be escaped just return a reference to the
	 * original string. */
	if (!(amps || lt || gt || slash || quot)) {
		Py_INCREF(input);
		return (PyObject *) input;
	}

	Py_ssize_t newlen = len + ((4 * (amps + slash)) + (3 * (lt + gt)) + (5 * quot));
	PyUnicodeObject * new_pyuni = (PyUnicodeObject *) PyUnicode_FromUnicode(NULL, newlen);
	Py_UNICODE *new_buf = PyUnicode_AS_UNICODE(new_pyuni);

	int opos = 0;
	int npos = 0;

#define UPDATE_UNI(a, b)\
	Py_MEMCPY(new_buf + npos, input_buf + opos, (i - opos) * sizeof(Py_UNICODE));\
	Py_MEMCPY(new_buf + (i + npos - opos), a, b * sizeof(Py_UNICODE));\
	npos += (i - opos) + b;\
	opos = i + 1;

	for (i = 0; i < len; i++) {
		switch (input_buf[i]) {
			case ucs_amp:
				UPDATE_UNI(amp_buf, 5);
				break;
			case ucs_lt:
				UPDATE_UNI(lt_buf, 4);
				break;
			case ucs_gt:
				UPDATE_UNI(gt_buf, 4);
				break;
			case ucs_slash:
				UPDATE_UNI(slash_buf, 5);
				break;
			case ucs_quot:
				UPDATE_UNI(quot_buf, 6);
				break;
		}
	}
	if (opos < len) {
		Py_MEMCPY(new_buf + npos, input_buf + opos, (len - opos) * sizeof(Py_UNICODE));
	}

	return (PyObject *) new_pyuni;
}

/* Here's how the find_url function works:
 *
 * 1. Search the string for an occurence of the string "://".
 * 2. Back scan to the beginning of the word and check that the string starts
 *    with http:// or https:// (we won't link to other URI schemas).
 * 3. Scan forward for the end of the string or the next space.
 * 4. Scan backwards and remove trailing punctuation.
 *
 * This is a best guess. It isn't perfect. You're allowed to have a lot of
 * different kinds of punctuation in URLs (including unescaped spaces!), so
 * your valid url that ends in punctuation or contains spaces won't be
 * linkified correctly, but that's just the way of things.
 *
 * This function mutates the parameter `begin' to hold the position of the
 * beginning of the string and returns the length of the string that has been
 * identified as a URL. A return value of 0 indicates that the function failed
 * to find a URL. A return value of -1 means an exception was raised.
 */
static Py_ssize_t find_url(PyObject *input, Py_ssize_t start, Py_ssize_t *begin) {

	Py_UNICODE *input_buf = PyUnicode_AS_UNICODE(input);

	const Py_ssize_t in_len = PyUnicode_GET_SIZE(input);
	Py_ssize_t pos;

	Py_ssize_t end = 0;

	/* Do a forward search for "://", and store the position in pos */
	while ((pos = PyUnicode_Find(input, uri_scheme_sep, start, in_len, 1)) >= 0) {
		printf("pos = %d\n", (int) pos);

		/* Now we need to backscan to the beginning of the word */
		int i;
		for (i = pos - 1; (i >= 0) && (input_buf[i] != ucs_space); i--) ;
		*begin = i + 1;

		/* If the URL doesn't have a valid schema, search for "://" again and
		 * restart the linkify process */
		int diff = pos - *begin;
		if (diff != 4 && diff != 5) {
			printf("bad diff, diff = %d\n", diff);
			start = pos + 1;
			continue;
		}

		/* FIXME: validate against http and https using memcmp */

		/* Now we forward scan for a space */
		for (i = pos + 3; (i < in_len) && (input_buf[i] != ucs_space); i++) ;
		end = i - 1;

		/* And backscan for punctuation... note that we can fold this into the
		 * search for space and maybe it would be a little bit faster, although
		 * I think the usual case will be zero or one punctuation characters
		 * (or ocassionally an ellipsis) so having the extra comparison
		 * operations in the forward scan loop probably aren't worthwhile. */

		int again = 1;
		while (again) {
			switch (input_buf[end]) {
				case (Py_UNICODE) '.':
				case (Py_UNICODE) ',':
				case (Py_UNICODE) '?':
				case (Py_UNICODE) '!':
				case (Py_UNICODE) '>':
				case (Py_UNICODE) ')':
				case (Py_UNICODE) ';':
				case (Py_UNICODE) ':':
				case (Py_UNICODE) ']':
					end--;
					break;

				default:
					again = 0;
					end++;
					break;
			}
		}

		printf("end backscan, *begin = %d, end = %d\n", *begin, end);
		return end - *begin;
	}

	return (pos == -2) ? -1 : 0;
}

static PyObject* linkify_uni(PyObject *input) {

	Py_ssize_t begin = 0;
	Py_ssize_t start = 0;

	Py_ssize_t len = find_url(input, start, &begin);

	if (start == -1)
		return NULL;


	if (len > 0) {
		Py_ssize_t in_len = PyUnicode_GET_SIZE(input);

		PyObject *empty = PyUnicode_DecodeASCII("", 0, NULL);
		PyObject *front = PySequence_GetSlice(input, 0, begin);
		PyObject *url = PySequence_GetSlice(input, begin, begin + len);
		PyObject *end = PySequence_GetSlice(input, begin + len, in_len);

		PyObject *join_tuple = PyTuple_Pack(7, front, href_start, url, href_end1, url, href_end2, end);
		PyObject *new_uni = PyUnicode_Join(empty, join_tuple);

		Py_DECREF(empty);
		Py_DECREF(front);
		Py_DECREF(url);
		Py_DECREF(end);
		Py_DECREF(join_tuple);

		return new_uni;

		/* The commented out code below was my attempt at an optimized join...
		 * it doesn't create unnecessary objects and unrolls the equivalent
		 * join loop. It doesn't work though, which is why there are so many
		 * debugging statements and why it's commented out */

#if 0
		/* Optimized method of creating the string */
		Py_UNICODE *input_buf = PyUnicode_AS_UNICODE(input);


		/* The new length will be:
		 *    begin
		 *  + 9 (length of <a href=")
		 *  + length of url
		 *  + 2 (length of ">)
		 *  + length of url
		 *  + 4 (length of </a>)
		 *  + length of end of string
		 */


		Py_ssize_t new_len = in_len + len + 15;
		printf("slices are [:%d], [%d:%d], [%d:%d]\n", begin, begin, begin + len, begin + len, in_len);
		printf("old size was %d, new size is %d\n", in_len, new_len);

		printf("creating new uni\n");
		PyObject *new_uni = PyUnicode_FromUnicode(NULL, new_len);
		Py_UNICODE *new_buf = PyUnicode_AS_UNICODE(new_uni);

		printf("sizeof unicode is %d\n", sizeof(Py_UNICODE));
#define FAST_COPY(a, b, c, d)\
		printf("copying %d bytes from %p into %p + %d\n", (d) * sizeof(Py_UNICODE), (b) + (c) * sizeof(Py_UNICODE), new_buf, (a) * sizeof(Py_UNICODE));\
		Py_MEMCPY(new_buf + (a) * sizeof(Py_UNICODE), (b) + (c) * sizeof(Py_UNICODE), (d) * sizeof(Py_UNICODE))

		FAST_COPY(0, input_buf, 0, begin);
		FAST_COPY(begin, PyUnicode_AS_UNICODE(href_start), 0, 9);
		FAST_COPY(begin + 9, input_buf, begin, len);
		FAST_COPY(begin + 9 + len, PyUnicode_AS_UNICODE(href_end1), 0, 2);
		FAST_COPY(begin + 11 + len, input_buf, begin, len);
		FAST_COPY(begin + 11 + (2 * len), PyUnicode_AS_UNICODE(href_end2), 0, 4);
		FAST_COPY(begin + 15 + (2 * len), input_buf, begin + len, in_len - begin - len);

		printf("done copying\n");

		Py_INCREF(new_uni);
		Py_INCREF(input);

		return new_uni;
#endif
	}

	Py_INCREF(input);
	return input;
}

static PyObject* linkify(PyObject *self, PyObject *args) {

	PyObject *input;
	if (!PyArg_ParseTuple(args, "O", &input))
		return NULL;

	if (PyUnicode_CheckExact(input))
		return linkify_uni(input);

	PyErr_SetString(PyExc_TypeError, "argument to escape must be a unicode object");
	return NULL;
}

PyDoc_STRVAR(module_doc, "String utilities.");

static PyMethodDef strutil_methods[] = {
	{"escape", escape, METH_VARARGS, "HTML-escape a string"},
	{"linkify", linkify, METH_VARARGS, "returns a linkified copy of a string"},
	{NULL, NULL, 0, NULL} /* Sentinel */
};

int add_unicode_constant(PyObject *m, const char *name, const char *str, Py_UNICODE **g)
{
	PyObject *unistr;
	if ((unistr = PyUnicode_DecodeASCII(str, strlen(str), NULL)) == NULL)
		return -1;
	*g = PyUnicode_AS_UNICODE(unistr); /* this is a borrowed reference! */
	return PyModule_AddObject(m, name, unistr);
}

PyMODINIT_FUNC initstrutil(void) 
{
    PyObject* m;
    m = Py_InitModule3("strutil", strutil_methods, module_doc);

	/* This is kind of a hack. Basically we want unicode versions of the
	 * replacement buffers to exist somewhere in global scope so we don't need
	 * to reconstruct them in the escaping routines. This is done by creating
	 * some module-level variables and stealing references to their internal
	 * unicode buffers.
	 */
	add_unicode_constant(m, "_UNI_AMP", "&amp;", &amp_buf);
	add_unicode_constant(m, "_UNI_LT", "&lt;", &lt_buf);
	add_unicode_constant(m, "_UNI_GT", "&gt;", &gt_buf);
	add_unicode_constant(m, "_UNI_SLASH", "&#39;", &slash_buf);
	add_unicode_constant(m, "_UNI_QUOT", "&quot;", &quot_buf);

	uri_scheme_sep = PyUnicode_DecodeASCII("://", 3, NULL);
	PyModule_AddObject(m, "_URI_SCHEME_SEP", uri_scheme_sep);

	href_start = PyUnicode_DecodeASCII("<a href=\"", 9, NULL);
	href_end1 = PyUnicode_DecodeASCII("\">", 2, NULL);
	href_end2 = PyUnicode_DecodeASCII("</a>", 4, NULL);
	PyModule_AddObject(m, "_HREF_START", href_start);
	PyModule_AddObject(m, "_HREF_END1", href_end1);
	PyModule_AddObject(m, "_HREF_END2", href_end2);
}
