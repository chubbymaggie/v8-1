CC=g++
CFLAGS= -w -g #-O2
.PHONY: all install clean
.DEFAULT: all


all: tracer


state-machine.o: state-machine.hh state-machine.cc
	${CC} ${CFLAGS} -c state-machine.cc

type-info.o: type-info.hh state-machine.hh type-info.cc
	${CC} ${CFLAGS} -c type-info.cc

correlation-miner.o: state-machine.hh miner.hh correlation-miner.cc
	$(CC) $(CFLAGS) -c correlation-miner.cc

sm-builder.o: sm-builder.hh sm-builder.cc jsweeter_events.h state-machine.hh miner.hh
	${CC} ${CFLAGS} -c sm-builder.cc

tracer: tracer.cc options.h state-machine.o type-info.o sm-builder.o correlation-miner.o
	${CC} ${CFLAGS} tracer.cc state-machine.o type-info.o correlation-miner.o sm-builder.o -o tracer


install:
	cp tracer ${HOME}/bin/


clean:
	-rm tracer *.o


