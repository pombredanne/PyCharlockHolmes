#ifndef __CHARLOCK_HOLMES_H
#define __CHARLOCK_HOLMES_H

int charlockholmes_init_encoding();
PyObject *charlockholmes_get_supported_encodings(PyObject *self);
PyObject *charlockholmes_encoding_detect(PyObject *self, PyObject *args, PyObject *keywds);
PyObject *charlockholmes_encoding_detect_all(PyObject *self, PyObject *args, PyObject *keywds);

#endif /* __CHARLOCK_HOLMES_H */
