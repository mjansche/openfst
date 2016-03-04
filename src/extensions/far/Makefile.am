AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)

if HAVE_SCRIPT
lib_LTLIBRARIES = libfstfarscript.la libfstfar.la
else
lib_LTLIBRARIES = libfstfar.la
endif

libfstfar_la_SOURCES = sttable.cc stlist.cc
libfstfar_la_LDFLAGS = -version-info 2:5:0

if HAVE_SCRIPT
libfstfarscript_la_SOURCES = far-class.cc farscript.cc strings.cc main.cc
libfstfarscript_la_LDFLAGS = -version-info 2:5:0
endif

if HAVE_BIN
bin_PROGRAMS = farcompilestrings farcreate farequal farextract farinfo \
	farisomorphic farprintstrings

LDADD = libfstfarscript.la libfstfar.la ../../script/libfstscript.la \
        ../../lib/libfst.la -lm $(DL_LIBS)

farcompilestrings_SOURCES = farcompilestrings.cc

farcreate_SOURCES = farcreate.cc

farequal_SOURCES = farequal.cc

farextract_SOURCES = farextract.cc

farinfo_SOURCES = farinfo.cc

farisomorphic_SOURCES = farisomorphic.cc

farprintstrings_SOURCES = farprintstrings.cc
endif
