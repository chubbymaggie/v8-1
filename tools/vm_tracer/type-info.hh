// Describe the type and code
// richardxx, 2014


#ifndef TYPE_INFO_H
#define TYPE_INFO_H

#include "state-machine.hh"

// Find or create a map structure from given map_id
Map* 
find_map(int new_map, bool create=true);

// Find or create a code structure
Code* 
find_code(int new_code, bool create=true);


#endif
