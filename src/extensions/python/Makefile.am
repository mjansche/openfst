python_LTLIBRARIES = fst_exceptions.la fst_properties.la pywrapfst.la

pyexec_LTILIBRARIES = fst_exceptions.la fst_properties.la pywrapfst.la

fst_exceptions_la_SOURCES = fst_exceptions.cc
fst_exceptions_la_CPPFLAGS = -I$(srcdir)/../../include $(PYTHON_CPPFLAGS)
fst_exceptions_la_LDFLAGS = $(PYTHON_LDFLAGS) -avoid-version -module
fst_exceptions_la_LIBADD = ../../script/libfstscript.la ../../lib/libfst.la \
                           -lfstscript -lfst -lm -ldl

fst_properties_la_SOURCES = fst_properties.cc
fst_properties_la_CPPFLAGS = -I$(srcdir)/../../include $(PYTHON_CPPFLAGS)
fst_properties_la_LDFLAGS = $(PYTHON_LDFLAGS) -avoid-version -module
fst_properties_la_LIBADD = ../../script/libfstscript.la ../../lib/libfst.la \
                           -lfstscript -lfst -lm -ldl

pywrapfst_la_SOURCES = pywrapfst.cc
pywrapfst_la_CPPFLAGS = -I$(srcdir)/../../include $(PYTHON_CPPFLAGS)
pywrapfst_la_LDFLAGS = $(PYTHON_LDFLAGS) -avoid-version -module
pywrapfst_la_LIBADD = ../../script/libfstscript.la ../../lib/libfst.la \
                      -lfstfarscript -lfstfar -lfstscript -lfst -lm -ldl

# NB: we use the Cython-generated .cc, rather than the .pyx source file, so
# modifications to the .pyx files will not influence the build.
