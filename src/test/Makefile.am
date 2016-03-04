AM_CPPFLAGS = -I$(srcdir)/../include $(ICU_CPPFLAGS)
LDADD = ../lib/libfst.la -lm $(DL_LIBS)

check_PROGRAMS = fst_test weight_test algo_test

fst_test_SOURCES = fst_test.cc fst_test.h

weight_test_SOURCES = weight_test.cc weight-tester.h

algo_test_SOURCES = algo_test.cc algo_test.h rand-fst.h

TESTS = fst_test weight_test algo_test
