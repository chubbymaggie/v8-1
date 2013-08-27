CC=g++
CFLAGS= -O2
.PHONY: all install clean
.DEFAULT: all


all: tracer


state_machine.o: state_machine.h state_machine.cc
	${CC} ${CFLAGS} -c state_machine.cc

correlation-miner.o: state_machine.h miner.hh correlation-miner.cc
	$(CC) $(CFLAGS) -c correlation-miner.cc

sm-builder.o: sm-builder.hh sm-builder.cc events.h state_machine.h miner.hh
	${CC} ${CFLAGS} -c sm-builder.cc

tracer: tracer.cc options.h state_machine.o sm-builder.o correlation-miner.o
	${CC} ${CFLAGS} tracer.cc state_machine.o correlation-miner.o sm-builder.o -o tracer


install:
	cp tracer.exe ${HOME}/bin/


clean:
	-rm tracer.exe sm-builder.o state_machine.o


