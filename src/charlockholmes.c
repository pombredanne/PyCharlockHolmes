#include "Python.h"
#include "charlockholmes.h"

static PyMethodDef charlockholmes_methods[] = {
    {"detect", (PyCFunction)charlockholmes_encoding_detect, METH_VARARGS | METH_KEYWORDS,
        "Attempt to detect the encoding of this string."},
    {"detect_all", (PyCFunction)charlockholmes_encoding_detect_all, METH_VARARGS | METH_KEYWORDS,
        "Attempt to detect the encoding of this string, and return "
        "a list with all the possible encodings that match it."},
    {"get_supported_encodings", (PyCFunction)charlockholmes_get_supported_encodings, METH_VARARGS | METH_KEYWORDS,
        "Get list of supported encodings."},
    {NULL, NULL, 0, NULL}   /* sentinel */
};

void
initpycharlockholmes(void)
{
    /* Create the module and add the functions */
    Py_InitModule("pycharlockholmes", charlockholmes_methods);
    charlockholmes_init_encoding();
}
