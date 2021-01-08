#cython: language_level=3

from libcpp.memory cimport unique_ptr

cdef extern from "<fst/compat.h>" namespace "fst" nogil:
    unique_ptr[T] WrapUnique[T](T *)
