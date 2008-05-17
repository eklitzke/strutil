#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Python.h>

static PyObject* escape(PyObject *self, PyObject *args) {

	int len;
	char *old;

	if (!PyArg_ParseTuple(args, "s#", &old, &len))
		return NULL;

	/* We optimize for the common case, which is that no changes need to be
	 * made to the string. */
	int amps = 0;
	int lt = 0;
	int gt = 0;
	int slash = 0;
	int quot = 0;
	int i;
	for (i = 0; i < len; i++) {
		switch (old[i]) {
			case '&': amps++; break;
			case '<': lt++; break;
			case '>': gt++; break;
			case '\\': slash++; break;
			case '\'': quot++; break;
		}
	}

	/* The common, optimized case */
	if (!(amps | lt | gt | slash | quot))
		return PyString_FromStringAndSize(old, len); //FIXME: can we just return a reference to the old string?

	int newlen = len + (4 * amps) + (3 * (slash + lt + gt)) + (5 * quot);
	char* new_str = malloc(newlen);

	int opos = 0;
	int npos = 0;

	char *amp_sym = "&amp;";
	char *lt_sym = "&lt;";
	char *gt_sym = "&gt;";
	char *slash_sym = "&#39;";
	char *quot_sym = "&quot;";

#define update_string(a,b) memcpy(new_str + npos, old + opos, i - opos); memcpy(new_str + npos + i - opos, a, b); npos += (i - opos) + b; opos = i + 1; break;

	for (i = 0; i < len; i++) {
		switch (old[i]) {
			case '&': update_string(amp_sym, 5);
			case '<': update_string(lt_sym, 4);
			case '>': update_string(gt_sym, 4);
			case '\\': update_string(slash_sym, 4);
			case '\"': update_string(quot_sym, 6);
		}
	}
	if (opos < len)
		memcpy(new_str + npos, old + opos, len - opos);

	/* null terminate the string */
	new_str[newlen] = '\0';

	return PyString_FromStringAndSize(new_str, newlen);
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
