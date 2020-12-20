AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)
AM_LDFLAGS = -lm -ldl

bin_PROGRAMS = pdtcompose pdtexpand pdtreplace

pdtbindeps = ../../lib/libfst.la ../../script/libfstscript.la \
     libfstpdt.la

pdtcompose_SOURCES = pdtcompose.cc
pdtcompose_LDADD = $(pdtbindeps)

pdtexpand_SOURCES = pdtexpand.cc
pdtexpand_LDADD = $(pdtbindeps)

pdtreplace_SOURCES = pdtreplace.cc
pdtreplace_LDADD = $(pdtbindeps)


libfstdir = @libfstdir@
libfst_LTLIBRARIES = libfstpdt.la
libfstpdt_la_SOURCES = pdtscript.cc
libfstpdt_la_LDFLAGS = -version-info 0:0:0
