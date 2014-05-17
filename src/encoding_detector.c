#include "Python.h"
#include "unicode/ucsdet.h"
#include "magic.h"

static magic_t ch_magic;
static UCharsetDetector *ch_ucd;

/*
static int detect_binary_content(PyObject *content) {
    const char *binary_result;
    binary_result = magic_buffer(ch_magic, PyString_AsString(content), PyString_Size(content));
    if (binary_result) {
        if (!strstr(binary_result, "text")) {
            return 1;
        }
    }
    return 0;
}
*/

static int detect_binary_content(PyObject *content) {
    const char *buf;
    size_t buf_len, max_scan_len;

    buf = PyString_AsString(content);
    buf_len = PyString_Size(content);
    max_scan_len = 1024;

	if (buf_len > 10) {
		// application/postscript
		if (!memcmp(buf, "%!PS-Adobe-", 11))
			return 0;
	}

	if (buf_len > 7) {
		// image/png
		if (!memcmp(buf, "\x89PNG\x0D\x0A\x1A\x0A", 8))
			return 1;
	}

	if (buf_len > 5) {
		// image/gif
		if (!memcmp(buf, "GIF87a", 6))
			return 1;

		// image/gif
		if (!memcmp(buf, "GIF89a", 6))
			return 1;
	}

	if (buf_len > 4) {
		// application/pdf
		if (!memcmp(buf, "%PDF-", 5))
			return 1;
	}

	if (buf_len > 3) {
		// UTF-32BE
		if (!memcmp(buf, "\0\0\xfe\xff", 4))
			return 0;

		// UTF-32LE
		if (!memcmp(buf, "\xff\xfe\0\0", 4))
			return 0;
	}

	if (buf_len > 2) {
		// image/jpeg
		if (!memcmp(buf, "\xFF\xD8\xFF", 3))
			return 1;
	}

	if (buf_len > 1) {
		// UTF-16BE
		if (!memcmp(buf, "\xfe\xff", 2))
			return 0;

		// UTF-16LE
		if (!memcmp(buf, "\xff\xfe", 2))
			return 0;
	}

    if (max_scan_len > buf_len) {
        max_scan_len = buf_len;
    }

    /*
    int i;
	for (i=0; i<max_scan_len; i++)
	{
		if (buf[i] == NULL) {
            return 1;
        }
	}
	*/

	if (strlen(buf) != buf_len) {
	    return 1;
	}

    return 0;
}

int
charlockholmes_init_encoding()
{
    UErrorCode status = U_ZERO_ERROR;

    // ch_magic = magic_open(MAGIC_NO_CHECK_SOFT);
    ch_magic = magic_open(0);
    if (ch_magic == NULL) {
        PyErr_SetString(PyExc_StandardError, magic_error(ch_magic));
        return -1;
    }
    magic_load(ch_magic, NULL);

    ch_ucd = ucsdet_open(&status);
    if (U_FAILURE(status)) {
        PyErr_SetString(PyExc_StandardError, u_errorName(status));
        return -1;
    }

    return 0;
}

/*
 * The list of detectable encodings supported by this library
 *
 * Returns: an list of strings
 */
PyObject *
charlockholmes_get_supported_encodings(PyObject *self)
{
	UCharsetDetector *csd;
	UErrorCode status = U_ZERO_ERROR;
	UEnumeration *encoding_list;
    PyObject *result;
	int32_t enc_count;
	int32_t i;
	const char *enc_name;
	int32_t enc_name_len;

    csd = ucsdet_open(&status);
    encoding_list = ucsdet_getAllDetectableCharsets(csd, &status);
    enc_count = uenum_count(encoding_list, &status);

    result = PyTuple_New(enc_count);
    if (!result)
        return NULL;

    for(i=0; i < enc_count; i++) {
        enc_name = uenum_next(encoding_list, &enc_name_len, &status);
        PyTuple_SetItem(result, i, PyString_FromStringAndSize(enc_name, enc_name_len));
    }
    ucsdet_close(csd);

    return result;
}

/*
 * Attempt to detect the encoding of this string
 *
 * str      - a String, what you want to detect the encoding of
 * hint_enc - an optional String (like "UTF-8"), the encoding name which will
 *            be used as an additional hint to the charset detector
 *
 * Returns: a dict with encoding, language, type and confidence parameters
 */
PyObject *
charlockholmes_encoding_detect(PyObject *self, PyObject *args)
{
    PyObject *content;
    UErrorCode status = U_ZERO_ERROR;
    const UCharsetMatch *match;
    const char *mname;
    const char *mlang;
    const char *hint_enc = NULL;
    int mconfidence;

    if (!PyArg_ParseTuple(args, "S|s", &content, &hint_enc)) {
        return NULL;
    }

    if (detect_binary_content(content)) {
        return Py_BuildValue("{ss,si}", "type", "binary", "confidence", 100);
    }

    if (hint_enc != NULL) {
        ucsdet_setDeclaredEncoding(ch_ucd, hint_enc, strlen(hint_enc), &status);
    }

    ucsdet_setText(ch_ucd, PyString_AsString(content), (int32_t)PyString_Size(content), &status);
    match = ucsdet_detect(ch_ucd, &status);
    if (match) {
        mname = ucsdet_getName(match, &status);
        mlang = ucsdet_getLanguage(match, &status);
        mconfidence = ucsdet_getConfidence(match, &status);
        if (mlang && mlang[0])
            return Py_BuildValue("{ss,ss,si,ss}",
                    "type", "text",
                    "encoding", mname,
                    "confidence", mconfidence,
                    "language", mlang);
        else
            return Py_BuildValue("{ss,ss,si}",
                    "type", "text",
                    "encoding", mname,
                    "confidence", mconfidence);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/*
 * Attempt to detect the encoding of this string, and return
 * a list with all the possible encodings that match it.
 *
 *
 * str      - a String, what you want to detect the encoding of
 * hint_enc - an optional String (like "UTF-8"), the encoding name which will
 *            be used as an additional hint to the charset detector
 *
 * Returns: an list with zero or more dicts
 *          each one of them with with encoding, language, type and confidence
 *          parameters
 */
PyObject *
charlockholmes_encoding_detect_all(PyObject *self, PyObject *args)
{
    PyObject *lst;
    PyObject *content;
    UErrorCode status = U_ZERO_ERROR;
    const UCharsetMatch **matches;
    const char *mname;
    const char *mlang;
    const char *hint_enc = NULL;
    int mconfidence;
    int i, match_count;

    if (!PyArg_ParseTuple(args, "S|s", &content, &hint_enc)) {
        return NULL;
    }

    if (detect_binary_content(content)) {
        lst = PyList_New(1);
        if (!lst)
            return NULL;

        content = Py_BuildValue("{ss,si}", "type", "binary", "confidence", 100);
        PyList_SET_ITEM(lst, 0, content);
        return lst;
    }

    if (hint_enc != NULL) {
        ucsdet_setDeclaredEncoding(ch_ucd, hint_enc, strlen(hint_enc), &status);
    }

    ucsdet_setText(ch_ucd, PyString_AsString(content), (int32_t)PyString_Size(content), &status);
    matches = ucsdet_detectAll(ch_ucd, &match_count, &status);

    if (matches) {
        lst = PyList_New(match_count);
        if (!lst)
            return NULL;

    	for (i = 0; i < match_count; ++i) {
            mname = ucsdet_getName(matches[i], &status);
            mlang = ucsdet_getLanguage(matches[i], &status);
            mconfidence = ucsdet_getConfidence(matches[i], &status);
            if (mlang && mlang[0])
                content = Py_BuildValue("{ss,ss,si,ss}",
                        "type", "text",
                        "encoding", mname,
                        "confidence", mconfidence,
                        "language", mlang);
            else
                content = Py_BuildValue("{ss,ss,si}",
                        "type", "text",
                        "encoding", mname,
                        "confidence", mconfidence);

            PyList_SET_ITEM(lst, i, content);
        }

        return lst;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

/*
 * Returns whether or not the strip_tags flag is set on this detector
 *
 * Returns: Boolean
 */
PyObject *
charlockholmes_get_strip_tags(PyObject *self)
{
	return PyBool_FromLong(ucsdet_isInputFilterEnabled(ch_ucd));
}

/*
 * Enable or disable the stripping of HTML/XML tags from the input before
 * attempting any detection
 *
 * Returns: Boolean, the value passed
 */
PyObject *
charlockholmes_set_strip_tags(PyObject *self, PyObject *args)
{
    int val;

    if (!PyArg_ParseTuple(args, "i", &val)) {
        return NULL;
    }

	val = val > 0 ? 1 : 0;

	ucsdet_enableInputFilter(ch_ucd, val);

	return PyBool_FromLong(val);
}