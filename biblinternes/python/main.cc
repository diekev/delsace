#if 0
#include <Python.h>

extern "C" {

static PyObject *spam_system(PyObject */*self*/, PyObject *args)
{
	const char *command;

	if (!PyArg_ParseTuple(args, "s", &command)) {
		return nullptr;
	}

	int sts = system(command);

	return PyLong_FromLong(sts);
}

PyMethodDef SpamMethods[] = {
    { "system", spam_system, METH_VARARGS, "Execute a shell command." },
    { nullptr, nullptr, 0, nullptr }
};

static PyModuleDef spammodule = {
	PyModuleDef_HEAD_INIT,
	"spam",            /* name of the module */
	nullptr,           /* documentation */
	-1,
	SpamMethods,
    nullptr,
    0,
    0,
    0
};

PyMODINIT_FUNC PyInit_spam()
{
	return PyModule_Create(&spammodule);
}

}

int main(int /*argc*/, char** /**argv[]*/)
{
	PyImport_AppendInittab("spam", PyInit_spam);

//	Py_SetProgramName(argv[0]);

	Py_Initialize();

	PyImport_ImportModule("spam");
}
#endif
