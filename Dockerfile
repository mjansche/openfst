FROM ubuntu:focal AS builder

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update
RUN apt-get -y install autoconf g++ make python3-dev
RUN rm -fr /usr/local
RUN mkdir -p /tmp/build

COPY . /tmp/openfst

WORKDIR /tmp/build
RUN /tmp/openfst/configure \
    --prefix=/usr/local \
    --enable-compact-fsts \
    --enable-compress \
    --enable-const-fsts \
    --enable-far \
    --enable-linear-fsts \
    --enable-lookahead-fsts \
    --enable-mpdt \
    --enable-ngram-fsts \
    --enable-pdt \
    --enable-python \
    --enable-special \
    --enable-bin \
    --enable-fsts \
    --enable-grm
RUN make
RUN make install


FROM ubuntu:focal AS image

MAINTAINER Martin Jansche <mjansche+openfst@gmail.com>

COPY --from=builder /usr/local /usr/local

ENV LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib/fst
ENV PYTHONPATH=/usr/local/lib/python3.8/site-packages
