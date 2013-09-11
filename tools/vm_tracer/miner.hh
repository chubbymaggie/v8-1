// Mining facility for performance bug hunting and reasoning
// By richardxx, 2013.8

#ifndef MINER_H
#define MINER_H

#include <deque>
#include "state_machine.h"

using std::deque;

// Used for inferring why functions are forced to deoptimize
void
record_force_deopt_function(FunctionMachine*);


void
print_machine_path(deque<Transition*>&);


void
print_instance_path(vector<TransPacket*> &, int);


void
ops_result_in_force_deopt(Transition*);


void
sys_result_in_force_deopt(const char*);


// Guess why a function is deoptimized eagerly/softly/lazily
bool
infer_deoptimization_reason(int, Map*, FunctionMachine*);


#endif
