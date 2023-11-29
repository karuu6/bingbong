FROM --platform=linux/amd64 ubuntu:rolling
COPY app/ /app
RUN mkdir -p /app/bin
RUN mkdir -p /app/log

RUN apt-get update
RUN apt-get -y upgrade
RUN apt-get -y install build-essential zlib1g-dev
RUN apt-get -y install libmlpack-dev libarmadillo-dev libensmallen-dev

RUN make -C /app q

WORKDIR /app
ENTRYPOINT [ "./bin/q" ]