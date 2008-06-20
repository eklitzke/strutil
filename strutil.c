#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Python.h>

/* This is a highly optimized HTML escape method for strings. It directly
 * accesses the character buffers of the input/ouput strings to minimize memory
 * copies. Right now it only works with ASCII strings, but it should be simple
 * to make it work with UTF-8 as well (since all replacement
 * characters/sequences are ASCII). Getting it to work quickly with wide
 * character encodings (e.g. UCS-2) would be a little bit more tricky but still
 * doable.
 */
static PyObject* escape(PyObject *self, PyObject *args) {

	PyStringObject *input;

	if (!PyArg_ParseTuple(args, "S", &input))
		return NULL;

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
			case '\'':
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

	char *amp_sym = "&amp;";
	char *lt_sym = "&lt;";
	char *gt_sym = "&gt;";
	char *slash_sym = "&#39;";
	char *quot_sym = "&quot;";

#define UPDATE_STRING(a,b)\
	Py_MEMCPY(new_str + npos, old + opos, i - opos);\
	Py_MEMCPY(new_str + npos + i - opos, a, b);\
	npos += (i - opos) + b;\
	opos = i + 1;\
	break;

	for (i = 0; i < len; i++) {
		switch (old[i]) {
			case '&':  UPDATE_STRING(amp_sym, 5);
			case '<':  UPDATE_STRING(lt_sym, 4);
			case '>':  UPDATE_STRING(gt_sym, 4);
			case '\\': UPDATE_STRING(slash_sym, 5);
			case '\"': UPDATE_STRING(quot_sym, 6);
		}
	}
	if (opos < len)
		Py_MEMCPY(new_str + npos, old + opos, len - opos);

	return (PyObject *) new_pystr;
}

PyDoc_STRVAR(module_doc, "String utilities.");

static PyMethodDef strutil_methods[] = {
	{"escape", escape, METH_VARARGS, "HTML-escape a string"},
	{NULL, NULL, 0, NULL} /* Sentinel */
};

PyMODINIT_FUNC initstrutil(void) 
{
    PyObject* m;
    m = Py_InitModule3("strutil", strutil_methods, module_doc);

}
