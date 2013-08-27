// Mining facility for performance bug hunting and reasoning
// By richardxx, 2013.8

#ifndef MINER_H
#define MINER_H

#include <deque>
#include "state_machine.h"

using std::deque;


void
infer_deoptimization_reason(int, Map*);


#endif
