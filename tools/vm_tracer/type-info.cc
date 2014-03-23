/*
 * Processing type and code related matters.
 * by richardxx, 2014.3
 */

#include "type-info.hh"
#include "miner.hh"

// Class static vars
Map* Map::notifier = NULL;
Map* Map::null_map = new Map(-1);
Code* Code::null_code = new Code(-1);


static map<int, Map*> all_maps;
static map<int, Code*> all_codes;


void CoreInfo::add_usage(State* user_s)
{
  used_by.push_back(user_s);
}


Map::Map(int new_map)
{
  map_id = new_map;
}


void Map::update_map(int new_id)
{
  // Update global map structure
  all_maps.erase(map_id);
  all_maps[new_id] = this;

  // Now we change the map id to new id
  map_id = new_id;
}


void Map::do_notify(Transition* trans)
{
  if (notifier == this)
    ops_result_in_force_deopt(trans);
}


Code::Code(int new_code)
{
  code_id = new_code;
}


void Code::update_code(int new_code_id)
{
  all_codes.erase(code_id);
  code_id = new_code_id;
  all_codes[new_code_id] = this;
}



// ------------------------------------------
Map*
find_map(int new_map, bool create)
{
  map<int, Map*>::iterator it = all_maps.find(new_map);
  Map* res = Map::null_map;

  if ( it == all_maps.end() ) {
    if ( create == true ) {
      res = new Map(new_map);
      all_maps[new_map] = res;
    }
  }
  else
    res = it->second;

  return res;
}


Code* find_code(int new_code, bool create)
{
  map<int, Code*>::iterator it = all_codes.find(new_code);
  Code* res = Code::null_code;
  
  if ( it == all_codes.end() ) {
    if ( create == true ) {
      res = new Code(new_code);
      all_codes[new_code] = res;
    }
  }
  else
    res = it->second;
  
  return res;
}
