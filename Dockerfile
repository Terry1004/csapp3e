FROM debian:bullseye-slim

RUN apt update && \
    apt install -y make gcc-multilib && \
    apt install -y gdb-multiarch && \
    apt install -y flex bison tk-dev tcl-dev && \
    apt install -y valgrind && \
    apt install -y python && \
    apt install -y procps && \
    apt install -y net-tools && \
    apt install -y curl

WORKDIR /root/workspace