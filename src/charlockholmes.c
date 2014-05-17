#include "Python.h"
#include "charlockholmes.h"

static PyMethodDef charlockholmes_methods[] = {
    {"detect", (PyCFunction)charlockholmes_encoding_detect, METH_VARARGS,
        "Attempt to detect the encoding of this string."},
    {"detect_all", (PyCFunction)charlockholmes_encoding_detect_all, METH_VARARGS,
        "Attempt to detect the encoding of this string, and return "
        "a list with all the possible encodings that match it."},
    {"get_strip_tags", (PyCFunction)charlockholmes_get_strip_tags, METH_NOARGS,
        "Returns whether or not the strip_tags flag is set on this detector."},
    {"set_strip_tags", (PyCFunction)charlockholmes_set_strip_tags, METH_VARARGS,
        "Enable or disable the stripping of HTML/XML tags from the input before attempting any detection"},
    {"get_supported_encodings", (PyCFunction)charlockholmes_get_supported_encodings, METH_NOARGS,
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
