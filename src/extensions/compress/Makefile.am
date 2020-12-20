AM_CPPFLAGS = -I$(srcdir)/../../include $(ICU_CPPFLAGS)

if HAVE_BIN
bin_PROGRAMS = fstcompress fstrandmod

LDADD = libfstcompressscript.la \
        ../../script/libfstscript.la \
        ../../lib/libfst.la \
        -lz -lm -ldl

fstcompress_SOURCES = fstcompress.cc
fstrandmod_SOURCES = fstrandmod.cc
endif

if HAVE_SCRIPT
libfstcompressscript_la_SOURCES = compress-script.cc
libfstcompressscript_la_LDFLAGS = -version-info 2:5:0
endif

if HAVE_SCRIPT
libfst_LTLIBRARIES = libfstcompressscript.la
endif
