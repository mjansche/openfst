AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)
AM_LDFLAGS = $(ICU_LIBS) -lm -ldl

libfstdir = @libfstdir@
libfst_LTLIBRARIES = libfstfar.la libfstfarscript.la

libfstfar_la_SOURCES = sttable.cc
libfstfar_la_LDFLAGS = -version-info 0:0:0

libfstfarscript_la_SOURCES = farscript.cc compile-strings.cc main.cc
libfstfarscript_la_LDFLAGS = -version-info 0:0:0

bin_PROGRAMS = farcompilestrings farcreate farextract farinfo farprintstrings

farbindeps =  ../../lib/libfst.la ../../script/libfstscript.la \
     libfstfar.la libfstfarscript.la

farcompilestrings_SOURCES = farcompilestrings.cc
farcompilestrings_LDADD = $(farbindeps)

farcreate_SOURCES = farcreate.cc
farcreate_LDADD = $(farbindeps)

farextract_SOURCES = farextract.cc
farextract_LDADD = $(farbindeps)

farinfo_SOURCES = farinfo.cc
farinfo_LDADD = $(farbindeps)

farprintstrings_SOURCES = farprintstrings.cc
farprintstrings_LDADD = $(farbindeps)
