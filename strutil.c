/* This module is just a proof of concept. It is not fully tested. Do not use it in production. */

#include <Python.h> /* includes strutil.h */

/* This assumes little-endianness */
#define ucs_amp		(Py_UNICODE) '&'
#define ucs_lt		(Py_UNICODE) '<'
#define ucs_gt		(Py_UNICODE) '>'
#define ucs_slash	(Py_UNICODE) '\\'
#define ucs_quot	(Py_UNICODE) '\"'

static Py_UNICODE *amp_buf;
static Py_UNICODE *lt_buf;
static Py_UNICODE *gt_buf;
static Py_UNICODE *slash_buf;
static Py_UNICODE *quot_buf;

/* Forward declarations */
static PyObject* escape_str(PyStringObject *input);
static PyObject* escape_uni(PyUnicodeObject *input);

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
	Py_UNICODE *input_buf = PyUnicode_AS_UNICODE(input);;

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

PyDoc_STRVAR(module_doc, "String utilities.");

static PyMethodDef strutil_methods[] = {
	{"escape", escape, METH_VARARGS, "HTML-escape a string"},
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
}
