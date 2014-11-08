;;;; cl-messagepack.lisp

(in-package #:messagepack)

(declaim (optimize (debug 3)))

(eval-when (:compile-toplevel :load-toplevel :execute)
  (defun mkstr (&rest args)
    (format nil "~{~a~}" args))
  (defun mksymb (&rest args)
    (intern (apply #'mkstr args))))

(defmacro signed-unsigned-convertors (size)
  (let ((speed (if (< size 32) 3 0)))
    `(progn
       (defun ,(mksymb 'sb size '-> 'ub size) (sb)
         (declare (optimize (debug 0) (safety 0) (speed ,speed))
                  (type (integer ,(- (expt 2 (1- size))) ,(1- (expt 2 (1- size)))) sb))
         (if (< sb 0)
             (ldb (byte ,size 0) sb)
             sb))
       (defun ,(mksymb 'ub size '-> 'sb size) (sb)
         (declare (optimize (debug 0) (safety 0) (speed ,speed))
                  (type (mod ,(expt 2 size)) sb))
         (if (logbitp (1- ,size) sb)
             (- (1+ (logxor (1- (expt 2 ,size)) sb)))
             sb)))))

(signed-unsigned-convertors 8)
(signed-unsigned-convertors 16)
(signed-unsigned-convertors 32)
(signed-unsigned-convertors 64)

(defun write-hex (data)
  (let (line)
    (loop
       for i from 0 to (1- (length data))
       do (push (elt data i) line)
       when (= (length line) 16)
       do
         (format t "~{~2,'0x ~}~%" (nreverse line))
         (setf line nil))
    (when line
      (format t "~{~2,'0x ~}~%" (nreverse line)))))

(defun encode (data)
  (flexi-streams:with-output-to-sequence (stream)
    (encode-stream data stream)))

(defun make-hash (data)
  (let ((result (make-hash-table)))
    (dolist (kv data)
      (cond ((consp (cdr kv))
             (setf (gethash (first kv) result) (second kv)))
            (t
             (setf (gethash (car kv) result) (cdr kv)))))
    result))

(defun is-byte-array (data-type)
  (and (vectorp data-type)
       (equal '(unsigned-byte 8) (array-element-type data-type))))

(defun encode-stream (data stream)
  (cond ((floatp data) (encode-float data stream))
        ((numberp data) (encode-integer data stream))
        ((null data) (write-byte #xc0 stream))
        ((eq data t) (write-byte #xc3 stream))
        ((stringp data)
         (encode-string data stream))
        ((is-byte-array data)
         (encode-raw-bytes data stream))
        ((or (consp data) (vectorp data))
         (encode-array data stream))
        ((hash-table-p data)
         (encode-hash data stream))
        ((symbolp data)
         (encode-string (symbol-name data) stream))
        (t (error "Cannot encode data."))))

(defun encode-string (data stream)
  (encode-raw-bytes (babel:string-to-octets data) stream))

#+sbcl (defun sbcl-encode-float (data stream)
         (cond ((equal (type-of data) 'single-float)
                (write-byte #xca stream)
                (store-big-endian (sb-kernel:single-float-bits data) stream 4))
               ((equal (type-of data) 'double-float)
                (write-byte #xcb stream)
                (store-big-endian (sb-kernel:double-float-high-bits data) stream 4)
                (store-big-endian (sb-kernel:double-float-low-bits data) stream 4)))
         t)

(defun encode-float (data stream)
  (or #+sbcl (sbcl-encode-float data stream)
      #-(or sbcl) (error "No floating point support yet.")))

(defun encode-each (data stream &optional (encoder #'encode-stream))
  (cond ((hash-table-p data)
         (maphash (lambda (key value)
                    (funcall encoder key stream)
                    (funcall encoder value stream))
                  data))
        ((or (vectorp data) (consp data))
         (mapc (lambda (subdata)
                 (funcall encoder subdata stream))
               (coerce data 'list)))
        (t (error "Not sequence or hash table."))))

(defun encode-sequence (data stream
                        short-prefix short-length
                        typecode-16 typecode-32
                        &optional (encoder #'encode-stream))
  (let ((len (if (hash-table-p data)
                 (hash-table-count data)
                 (length data))))
    (cond ((<= 0 len short-length)
           (write-byte (+ short-prefix len) stream)
           (encode-each data stream encoder))
          ((<= 0 len 65535)
           (write-byte typecode-16 stream)
           (store-big-endian len stream 2)
           (encode-each data stream encoder))
          ((<= 0 len (1- (expt 2 32)))
           (write-byte typecode-32 stream)
           (store-big-endian len stream 4)
           (encode-each data stream encoder)))))

(defun encode-hash (data stream)
  (encode-sequence data stream #x80 15 #xdc #xdd))

(defun encode-array (data stream)
  (encode-sequence data stream #x90 15 #xdc #xdd))

(defun encode-raw-bytes (data stream)
  (encode-sequence data stream #xa0 31 #xda #xdb #'write-byte))

(defun encode-integer (data stream)
  (cond ((<= 0 data 127) (write-byte data stream))
        ((<= -32 data -1) (write-byte (sb8->ub8 data) stream))
        ((<= 0 data 255)
         (write-byte #xcc stream)
         (write-byte data stream))
        ((<= 0 data 65535)
         (write-byte #xcd stream)
         (store-big-endian data stream 2))
        ((<= 0 data (1- (expt 2 32)))
         (write-byte #xce stream)
         (store-big-endian data stream 4))
        ((<= 0 data (1- (expt 2 64)))
         (write-byte #xcf stream)
         (store-big-endian data stream 8))
        ((<= -128 data 127)
         (write-byte #xd0 stream)
         (write-byte (sb8->ub8 data) stream))
        ((<= -32768 data 32767)
         (write-byte #xd1 stream)
         (write-byte (sb16->ub16 data) stream))
        ((<= (- (expt 2 31)) data (1- (expt 2 31)))
         (write-byte #xd2 stream)
         (write-byte (sb32->ub32 data) stream))
        ((<= (- (expt 2 63)) data (1- (expt 2 63)))
         (write-byte #xd3 stream)
         (write-byte (sb64->ub64 data) stream))
        (t (error "Integer too large or too small."))))

(defun store-big-endian (number stream byte-count)
  (let (byte-list)
    (loop
       while (> number 0)
       do
         (push (rem number 256)
               byte-list)
         (setf number (ash number -8)))
    (loop
       while (< (length byte-list) byte-count)
       do (push 0 byte-list))
    (when (> (length byte-list) byte-count)
      (error "Number too large."))
    (write-sequence byte-list stream)))

(defun decode (byte-array)
  (flexi-streams:with-input-from-sequence (stream byte-array)
    (decode-stream stream)))

(defun decode-stream (stream)
  (let ((byte (read-byte stream)))
    (cond ((= 0 (ldb (byte 1 7) byte))
           byte)
          ((= 7 (ldb (byte 3 5) byte))
           (ub8->sb8 byte))
          ((= #xcc byte)
           (read-byte stream))
          ((= #xcd byte)
           (load-big-endian stream 2))
          ((= #xce byte)
           (load-big-endian stream 4))
          ((= #xcf byte)
           (load-big-endian stream 8))
          ((= #xd0 byte)
           (ub8->sb8 (read-byte stream)))
          ((= #xd1 byte)
           (ub16->sb16 (load-big-endian stream 2)))
          ((= #xd2 byte)
           (ub32->sb32 (load-big-endian stream 4)))
          ((= #xd3 byte)
           (ub64->sb64 (load-big-endian stream 8)))
          ((= #xc0 byte)
           nil)
          ((= #xc3 byte)
           t)
          ((= #xc2 byte)
           nil)
          ((= #xca byte)
           (or #+sbcl (sb-kernel:make-single-float (load-big-endian stream 4))
               #-(or sbcl) (error "No floating point support yet.")))
          ((= #xcb byte)
           (or #+sbcl (sb-kernel:make-double-float (load-big-endian stream 4)
                                                   (load-big-endian stream 4))
               #-(or sbcl) (error "No floating point support yet.")))
          ((= 5 (ldb (byte 3 5) byte))
           (decode-raw-sequence (ldb (byte 5 0) byte) stream))
          ((= #xda byte)
           (decode-raw-sequence (load-big-endian stream 2) stream))
          ((= #xdb byte)
           (decode-raw-sequence (load-big-endian stream 4) stream))
          ((= 9 (ldb (byte 4 4) byte))
           (decode-array (- byte #x90) stream))
          ((= #xdc byte)
           (decode-array (load-big-endian stream 2) stream))
          ((= #xdd byte)
           (decode-array (load-big-endian stream 4) stream))
          ((= 8 (ldb (byte 4 4) byte))
           (decode-map (- byte #x80) stream))
          ((= #xde byte)
           (decode-map (load-big-endian stream 2) stream))
          ((= #xdf byte)
           (decode-map (load-big-endian stream 4) stream)))))

(defun decode-map (length stream)
  (let ((hash-table (make-hash-table :test #'equal)))
    (loop repeat length
       do (let ((key (decode-stream stream))
                (value (decode-stream stream)))
            (setf (gethash key hash-table) value)))
    hash-table))

(defun decode-array (length stream)
  (let ((array (make-array length)))
    (dotimes (i length)
      (setf (aref array i) (decode-stream stream)))
    array))

(defun decode-raw-sequence (length stream)
  (let ((seq (make-array length :element-type '(mod 256))))
    (read-sequence seq stream)
    (babel:octets-to-string seq)))

(defun load-big-endian (stream byte-count)
  (let ((result 0))
    (loop
       repeat byte-count
       do (setf result (+ (ash result 8)
                          (read-byte stream))))
    result))
