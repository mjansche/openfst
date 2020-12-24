# OpenFst Docker images

OpenFst (www.openfst.org) is a library for constructing, combining,
optimizing, and searching weighted finite-state transducers (FSTs).

The OpenFst Docker images
(https://hub.docker.com/repository/docker/mjansche/openfst) contain
builds of OpenFst on top of two different Linux distributions.


## Ubuntu

The full Ubuntu-based image is built on top of `ubuntu:latest`, which
is currently 20.04 Focal. It contains a build of the OpenFst dynamic
library and headers, the command-line binaries, the Python module, and
all extensions, including dynamically loadable shared object files for
all specialized FST types defined in OpenFst.

The Python module has been built for the default version of Python
(currently 3.8) in the Ubuntu base image. The OpenFst image does not
include Python itself. It can be installed with `apt update && apt
install -y python3 libpython3.8`. Once Python 3.8 is installed, the
OpenFst Python module can then be loaded with `import pywrapfst`.


## Alpine

The smaller Alpine-based image is built on top of `alpine:latest`. It
contains a build of the OpenFst dynamic library and headers, the
command-line binaries, the Python module, and the minimally needed
extensions for compiling several OpenGrm packages (Ngram and Thrax, in
particular). It does not contain dynamically loadable shared object
files for any of the specialized FST types. The resulting OpenFst
image is very lightweight and can be used as the base image for
developing with OpenFst.

The Python module has been built for the default version of Python
(currently 3.8) in the Alpine base image. The OpenFst image does not
include Python itself. It can be installed with `apk add
python3`. Once Python 3.8 is installed, the OpenFst Python module can
then be loaded with `import pywrapfst`.
