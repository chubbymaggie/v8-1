CC=g++
CFlags= -g -std=c++0x

.PHONY: all install clean
.DEFAULT: all


all: tracer

tracer: tracer.cc state_machine.h options.h events.h
	${CC} ${CFlags} tracer.cc -o tracer


install:
	cp tracer.exe ${HOME}/bin/

clean:
	-rm tracer.exe


