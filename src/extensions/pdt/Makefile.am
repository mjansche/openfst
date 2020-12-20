AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)
AM_LDFLAGS = -lm -ldl

if HAVE_BIN
bin_PROGRAMS = pdtcompose pdtexpand pdtinfo pdtreplace pdtreverse pdtshortestpath

pdtbindeps = libfstpdtscript.la ../../script/libfstscript.la \
             ../../lib/libfst.la

pdtcompose_SOURCES = pdtcompose.cc
pdtcompose_LDADD = $(pdtbindeps)

pdtexpand_SOURCES = pdtexpand.cc
pdtexpand_LDADD = $(pdtbindeps)

pdtinfo_SOURCES = pdtinfo.cc
pdtinfo_LDADD = $(pdtbindeps)

pdtreplace_SOURCES = pdtreplace.cc
pdtreplace_LDADD = $(pdtbindeps)

pdtreverse_SOURCES = pdtreverse.cc 
pdtreverse_LDADD = $(pdtbindeps)

pdtshortestpath_SOURCES = pdtshortestpath.cc 
pdtshortestpath_LDADD = $(pdtbindeps)
endif

libfstdir = @libfstdir@

if HAVE_SCRIPT
libfst_LTLIBRARIES = libfstpdtscript.la
libfstpdtscript_la_SOURCES = pdtscript.cc
libfstpdtscript_la_LDFLAGS = -version-info 0:0:0
endif
