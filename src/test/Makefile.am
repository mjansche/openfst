AM_CPPFLAGS = -I$(srcdir)/../include $(ICU_CPPFLAGS)
LDADD = ../lib/libfst.la -lm $(DL_LIBS)

check_PROGRAMS = fst_test weight_test

fst_test_SOURCES = fst_test.cc fst_test.h

weight_test_SOURCES = weight_test.cc weight-tester.h

algo_test_SOURCES = algo_test.cc algo_test.h rand-fst.h

check_PROGRAMS += algo_test_log
algo_test_log_SOURCES = $(algo_test_SOURCES)
algo_test_log_CPPFLAGS = -DTEST_LOG

check_PROGRAMS += algo_test_tropical
algo_test_tropical_SOURCES = $(algo_test_SOURCES)
algo_test_tropical_CPPFLAGS = -DTEST_TROPICAL

check_PROGRAMS += algo_test_minmax
algo_test_minmax_SOURCES = $(algo_test_SOURCES)
algo_test_minmax_CPPFLAGS = -DTEST_MINMAX

# check_PROGRAMS += algo_test_left_string
# algo_test_left_string_SOURCES = $(algo_test_SOURCES)
# algo_test_left_string_CPPFLAGS = -DTEST_LEFT_STRING

# check_PROGRAMS += algo_test_right_string
# algo_test_right_string_SOURCES = $(algo_test_SOURCES)
# algo_test_right_string_CPPFLAGS = -DTEST_RIGHT_STRING

# check_PROGRAMS += algo_test_gallic
# algo_test_gallic_SOURCES = $(algo_test_SOURCES)
# algo_test_gallic_CPPFLAGS = -DTEST_GALLIC

check_PROGRAMS += algo_test_lexicographic
algo_test_lexicographic_SOURCES = $(algo_test_SOURCES)
algo_test_lexicographic_CPPFLAGS = -DTEST_LEXICOGRAPHIC

check_PROGRAMS += algo_test_power
algo_test_power_SOURCES = $(algo_test_SOURCES)
algo_test_power_CPPFLAGS = -DTEST_POWER

TESTS = $(check_PROGRAMS)
