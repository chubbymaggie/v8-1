// Implementation of the inline functions of state machine
// by richardxx, 2013

#include <queue>
#include "state_machine.h"
#include "miner.hh"

using namespace std;


// Global data
ObjectState ObjectMachine::temp_o;
FunctionState FunctionMachine::temp_f;

static map<int, Map*> all_maps;
static map<int, Code*> all_codes;

Map* null_map = new Map(-1);
Code* null_code = new Code(-1);

static Map* map_notifier = NULL;


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
  if (map_notifier == this )
    ops_result_in_force_deopt(trans);
}


Map* find_map(int new_map, bool create)
{
  map<int, Map*>::iterator it = all_maps.find(new_map);
  Map* res = null_map;

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


void 
register_map_notifier(Map* r)
{
  if ( map_notifier != NULL )
    // Perhaps we miss recording some operations
    ops_result_in_force_deopt(NULL);

  map_notifier = r;
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


Code* find_code(int new_code, bool create)
{
  map<int, Code*>::iterator it = all_codes.find(new_code);
  Code* res = null_code;
  
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


TransPacket::TransPacket()
{
  trans = NULL;
  reason.clear();
  cost = 0;
  context = NULL;
  count = 0;
}


TransPacket::TransPacket(const char* desc)
{
  trans = NULL;
  reason.assign(desc);
  cost = 0;
  context = NULL;
  count = 0;
}


TransPacket::TransPacket(const char* desc, int c_)
{
  trans = NULL;
  reason.assign(desc);
  cost = c_;
  context = NULL;
  count = 0;
}


bool TransPacket::has_reason()
{
  return reason.size() != 0;
}

  
void TransPacket::describe(stringstream& ss) const
{
  if ( count > 1 ) ss << count << "*";
  ss << "(";
  if ( context != NULL ) {
    ss << context->toString();
    ss << ", ";
  }
  ss << reason << ")";
}


bool TransPacket::operator<(const TransPacket& rhs) const
{
  return reason < rhs.reason;
}


TransPacket& TransPacket::operator=(const TransPacket& rhs)
{
  reason = rhs.reason;
  cost = rhs.cost;
  context = rhs.context;
  return *this;
}


Transition::Transition()
{
  source = NULL;
  target = NULL;
  last_ = NULL;
}


Transition::Transition(State* s_, State* t_)
{
  source = s_;
  target = t_;
  last_ = NULL;
}


void Transition::insert_reason(const char* r, int cost)
{
  TransPacket *tp = new TransPacket(r, cost);
  
  TpSet::iterator it = triggers.find(tp);
  if ( it != triggers.end() ) {
    delete tp;
    TransPacket* old_tp = *it;
    // It does not invalidate the order in triggers
    old_tp->cost += cost;
    tp = old_tp;
  }
  else {
    triggers.insert(tp);
  }

  tp->trans = this;
  tp->count++;
  last_ = tp;
}


void Transition::insert_reason(TransPacket* tp)
{
  TpSet::iterator it = triggers.find(tp);

  if ( it != triggers.end() ) {
    TransPacket* old_tp = *it;
    old_tp->cost += tp->cost;
    tp = old_tp;
  }
  else {
    TransPacket *new_tp = new TransPacket;
    *new_tp = *tp;
    tp = new_tp;
    triggers.insert(tp);
  }
  
  tp->trans = this;
  tp->count++;
  last_ = tp;
}


void Transition::merge_reasons(string& final)
{
  int i = 0;
  int cost = 0;
  stringstream ss;
  
  TpSet::iterator it = triggers.begin();
  TpSet::iterator end = triggers.end();

  for ( ; it != end; ++it ) {
    if ( i > 6 ) break;
    if ( i > 0 ) ss << '+';
    TransPacket* tp = *it;
    tp->describe(ss);
    cost += tp->cost;
    ++i;
  }
  
  if ( it != end )
    ss << "+(More...)";

  if ( cost != 0 )
    ss << "$$" << cost;
    
  final = ss.str();
}


const char* Transition::graphviz_style()
{
  return "style=solid";
}


SummaryTransition::SummaryTransition()
  : Transition()
{
  exit = NULL;
}


SummaryTransition::SummaryTransition( State* s_, State* t_, State* exit_ )
  : Transition(s_,t_)
{
  exit = exit_;
}


const char* SummaryTransition::graphviz_style()
{
  return "style=dotted";
}


State::State() 
{
  out_edges.clear();
  in_edges.clear();
  machine = NULL;
}


int State::size() 
{ 
  return out_edges.size(); 
}


Transition* State::find_transition( State* next_s, bool by_boilerplate )
{
  TransMap::iterator it = out_edges.find(next_s);
  if ( it == out_edges.end() ) return NULL;
  
  Transition* ans = it->second;
  if ( by_boilerplate && ans->type() != Transition::TSummary ) return NULL;
  return ans;
}


Transition* State::add_transition( State* next_s )
{
  Transition* trans = new Transition(this, next_s);
  out_edges[next_s] = trans;
  next_s->in_edges[this] = trans;
  return trans;
}


Transition* State::add_summary_transition( State* next_s, State* exit_s )
{
  Transition* trans = new SummaryTransition(this, next_s, exit_s);
  out_edges[next_s] = trans;
  next_s->in_edges[this] = trans;
  return trans;
}


Transition* 
State::transfer(State* maybe_next_s, ObjectMachine* boilerplate)
{
  Transition* trans;
  State* next_s;

  // Search for the same state
  trans = find_transition(maybe_next_s, boilerplate != NULL);
  
  if ( trans == NULL ) {
    // Not exist
    // We try to search and add to the state machine pool
    next_s = machine->search_state(maybe_next_s);
    if ( boilerplate != NULL ) {
      State* exit_s = boilerplate->exit_state();
      trans = add_summary_transition(next_s, exit_s);
    }
    else {
      trans = add_transition(next_s);
    }
  }
  
  // Update transition
  return trans;
}


void State::set_map(Map* new_map)
{
  map_d = new_map;
  map_d->add_usage(this);
}


Map* State::get_map() const
{
  return map_d;
}


ObjectState::ObjectState() 
{ 
  id = 0;
  machine = NULL;
  map_d = null_map; 
}


ObjectState::ObjectState( int my_id )
{
  id = my_id;
  machine = NULL;
  map_d = null_map;
}


bool ObjectState::less_than(const State* other) const
{
  return map_d->id() < other->get_map()->id();
}


State* ObjectState::clone() const
{
  ObjectState* new_s = new ObjectState;
  new_s->set_map(map_d);
  new_s->machine = machine;
  return new_s;
}


string ObjectState::toString() const 
{
  stringstream ss;

  if ( id == 0 )
    ss << "START";
  else
    ss << hex << map_d->map_id;
  
  return ss.str();
}

const char* ObjectState::graphviz_style() const
{
  const char* style = NULL;

  if ( id == 0 )
    // id = 0 indicates this is the starting state
    style = "shape=diamond, style=rounded";
  else
    style = "shape=egg"; 
  
  return style;
}


FunctionState::FunctionState() 
{ 
  id = 0;
  machine = NULL;
  code_d = null_code;
}


FunctionState::FunctionState( int my_id )
{
  id = my_id;
  machine = NULL;
  code_d = null_code;
}
  

void FunctionState::set_code(Code* new_code)
{
  code_d = new_code;
  code_d->add_usage(this);
}


Code* FunctionState::get_code()
{
  return code_d;
}


// Implement virtual functions
bool FunctionState::less_than(const State* other) const
{
  switch (other->type()) {
  case State::SObject:
    return !other->less_than(this);
    
  case State::SFunction:
    {
      const FunctionState* o_ = (const FunctionState*)other;
      int diff = code_d->id() - o_->code_d->id();
      if ( diff == 0 )
	return map_d->id() < o_->map_d->id();
      return diff < 0;
    }
  }
  
  return this < other;
}


State* FunctionState::clone() const
{
  FunctionState* new_s = new FunctionState;
  new_s->set_map( map_d );
  new_s->set_code( code_d );
  new_s->machine = machine;
  return new_s;
}


string FunctionState::toString() const 
{
  stringstream ss;

  if ( id == 0 )
    ss << "START";
  else
    ss << hex << code_d->code_id;
  
  return ss.str();
}

int StateMachine::id_counter = 0;

StateMachine* StateMachine::NewMachine( StateMachine::Mtype type )
{
  StateMachine* sm = NULL;

  switch( type ) {
  case MBoilerplate:
  case MObject:
    sm = new ObjectMachine(type);
    break;

  case MFunction:
    sm = new FunctionMachine();
    break;

  case MMap:
    break;

  default:
    break;
  }

  sm->id = id_counter++;
  return sm;
}


StateMachine::StateMachine()
{
  id = -1;
  states.clear();
  inst_at.clear();
  m_name.clear();
  start = NULL;
  type = MCount;
  has_changed = false;
}


void StateMachine::set_machine_name(const char* name)
{
  m_name.assign(name);
}


bool StateMachine::has_name()
{
  return m_name.size() != 0;
}


string StateMachine::toString()
{
  stringstream ss;

  if ( m_name.size() != 0 )
    ss << m_name;
  else
    ss << "(G" << id << ")";

  return ss.str();
}


int StateMachine::size() 
{
  int ans = 0;

  for ( StatesPool::iterator it = states.begin(),
	  end = states.end(); it != end; ++it ) {
    State* s = *it;
    ans += s->size();
  }

  return ans + states.size(); 
}


State* StateMachine::search_state(State* s, bool create)
{
  StatesPool::iterator it = states.find(s);
  
  if ( it == states.end() ) {
    if ( create ) {
      State* s_ = s->clone();
      s_->id = states.size();
      states.insert(s_);
      s = s_;
    }
    else {
      s = NULL;
    }
  }
  else
    s = *it;
  
  return s;
}


void StateMachine::add_state(State* s)
{
  states.insert(s);
}


void StateMachine::delete_state(State* s)
{
  // FIX: delete the incoming edges
  states.erase(s);
}


State* StateMachine::get_instance_pos(int d, bool new_instance)
{
  State* s = NULL;
  
  map<int, State*>::iterator it = inst_at.find(d);
  if ( it == inst_at.end() ) {
    // Add this instance
    s = start;
    inst_at[d] = s;
  }
  else {
    // We obtain the old position first
    s = it->second;

    if ( new_instance == true ) {
      // This instance has been reclaimed by GC and it is allocated to the same object again
      s = start;
      inst_at[d] = s;
    }
  }    
  
  return s;
}


void StateMachine::add_instance(int ins_name, State* s)
{
  inst_at[ins_name] = s;
}


void StateMachine::rename_instance(int old_name, int new_name)
{
  map<int, State*>::iterator it = inst_at.find(old_name);
  if ( it == inst_at.end() ) return;
  State* s = it->second;
  inst_at.erase(it);
  inst_at[new_name] = s;
}


void StateMachine::migrate_instance(int ins_d, Transition* trans)
{
  State* src = trans->source;
  State* tgt = trans->target;

  // Update instance <-> state maps
  inst_at[ins_d] = tgt;
  
  // Call monitoring action
  Map* src_map = ((ObjectState*)src)->get_map();
  src_map->do_notify(trans);
}


void
StateMachine::draw_graphviz(FILE* file)
{
  queue<State*> bfsQ;
  set<State*> visited;
  
  State* init_state = this->start;
  fprintf(file, "digraph G%d {\n", id);
  
  // Output global settings
  fprintf(file, "\tnode[nodesep=2.0];\n");
  fprintf(file, "\tgraph[overlap=false];\n");

  // Go over the state machine
  visited.clear();
  visited.insert(init_state);
  bfsQ.push(init_state);
  
  while ( !bfsQ.empty() ) {
    State* cur_s = bfsQ.front();
    bfsQ.pop();
    
    // We first generate the node description
    int id = cur_s->id;
    fprintf(file, 
	    "\t%d [%s, label=\"%s\"];\n", 
	    id, 
	    cur_s->graphviz_style(),
	    cur_s == init_state ? toString().c_str() : cur_s->toString().c_str());
    
    // We draw the transition edges
    State::TransIterator it, end;
    State::TransMap &t_edges = cur_s->out_edges;
    
    for ( it = t_edges.begin(), end = t_edges.end();
	  it != end; ++it ) {
      Transition* trans = it->second;
      State* next_s = trans->target;
      if ( visited.find(next_s) == visited.end() ) {
	bfsQ.push(next_s);
	visited.insert(next_s);
      }
      
      // Generate the transition descriptive string
      string final;
      trans->merge_reasons(final);
      fprintf(file, 
	      "\t%d -> %d [%s, label=\"%s\"];\n",
	      id, 
	      next_s->id,
	      trans->graphviz_style(),
	      final.c_str() 
	      );
    }
  }
  
  fprintf(file, "}\n\n");
}


ObjectMachine::ObjectMachine()
{
  type = StateMachine::MObject;
  is_boilerplate = false;
  start = new ObjectState(0);
  start->machine = this;
  states.insert(start);
}


ObjectMachine::ObjectMachine(StateMachine::Mtype type_)
{
  type = type_;
  is_boilerplate = (type_ == StateMachine::MBoilerplate);
  start = new ObjectState(0);
  start->machine = this;
  states.insert(start);
}


State* ObjectMachine::search_state(Map* exp_map, bool create)
{
  temp_o.map_d = exp_map;
  temp_o.machine = this;
  return StateMachine::search_state(&temp_o, create);
}


State* ObjectMachine::exit_state()
{
  State* exit_s = NULL;

  for ( map<int, State*>::iterator it = inst_at.begin();
	it != inst_at.end(); ++it ) {
    State* cur_s = it->second;
    if ( exit_s == NULL )
      exit_s = cur_s;
    else if ( exit_s != cur_s )
      return NULL;
  }
  
  return exit_s;
}


ObjectState*
ObjectMachine::jump_to_state_with_map(InstanceDescriptor* i_desc, int exp_map_id, bool new_instance)
{
  int ins_id = i_desc->id;
  ObjectState *cur_s = (ObjectState*)get_instance_pos(ins_id, new_instance);
  if ( exp_map_id == -1 ) return cur_s;

  Map* exp_map = find_map(exp_map_id);

  if ( cur_s->get_map() != exp_map ) {
    State* exp_s = search_state(exp_map, false);
    
    if ( exp_s == NULL ) {
      // We might miss something
      // Let's build this state
      exp_s = temp_o.clone();
      exp_s->id = states.size();
      states.insert(exp_s);
      
      // And we make a link to this state
      Transition* trans = cur_s->transfer(exp_s, NULL);
      trans->insert_reason("?", 0);
    }

    // Next, we check if some operations in middle migrate objects to this state
    vector<TransPacket*> &history = i_desc->history;
    int i;
    int size = history.size();
    int end = (size > 10 ? size - 10 : -1 );
    for ( i = size - 1; i > end; --i ) {
      TransPacket* tp = history[i];
      if ( exp_s == tp->trans->source ) break;
    }
    if ( i > end ) {
      // Yes, some recorded operations transfer this object
      // It's not a missing link
      // But currently I don't know how to deal with it
    }
    else {
      inst_at[ins_id] = exp_s;
      cur_s = (ObjectState*)exp_s;
    }
  }

  return cur_s;
}


Transition* ObjectMachine::set_instance_map(InstanceDescriptor* i_desc, int map_id, bool make_transition)
{
  int ins_id = i_desc->id;

  // Obtain current position of this instance
  State *cur_s = get_instance_pos(ins_id);
  
  // Build the target state
  // Using set_map will register this state to the map
  // We do not register for temporary variable
  temp_o.map_d = find_map(map_id);
  temp_o.machine = this;
  
  Transition* trans = NULL;
  if ( make_transition ) {
    // We do not insert any transfer packet for this transition
    trans = cur_s->transfer( &temp_o, NULL );
  }
  
  return trans;
}


Transition* 
ObjectMachine::evolve(InstanceDescriptor* i_desc, int old_map_id, int map_id, 
		      ObjectMachine* boilerplate, const char* trans_dec, int cost, bool new_instance)
{
  int ins_id = i_desc->id;

  // Now we check the validity of this state
  // In case we might miss something
  ObjectState* cur_s = jump_to_state_with_map(i_desc, old_map_id, new_instance);

  // Build the target state
  temp_o.map_d = (map_id == -1 ? cur_s->get_map() : find_map(map_id));
  temp_o.machine = this;

  // Transfer state
  Transition* trans = cur_s->transfer( &temp_o, boilerplate );
  trans->insert_reason(trans_dec, cost);

  // Renew the position of this instance
  migrate_instance(ins_id, trans);

  // Record this action
  i_desc->add_operation(trans->last_);

  has_changed = true;
  return trans;
}


FunctionMachine::FunctionMachine()
{
  type = StateMachine::MFunction;
  allow_opt = true;
  opt_count = 0;
  deopt_count = 0;
  ic_miss_count = 0;
  ops_count = 0;
  last_op = NULL;
  
  // Initialize the first state
  start = new FunctionState(0);
  start->machine = this;
  states.insert(start);
}


State* FunctionMachine::search_state(Map* exp_map, Code* exp_code, bool create)
{
  temp_f.map_d = exp_map;
  temp_f.code_d = exp_code;
  temp_f.machine = this;
  return StateMachine::search_state(&temp_f, create);
}

 
void FunctionMachine::set_opt_state( bool allow, const char* msg )
{
  allow_opt = allow;
  optMsg.assign(msg);
}


Transition* 
FunctionMachine::evolve(InstanceDescriptor* i_desc, int map_id, int code_id, 
			const char* trans_dec, int cost, bool new_instance)
{ 
  int ins_id = i_desc->id;

  // Obtain current position of this instance
  FunctionState *cur_s = (FunctionState*)get_instance_pos(ins_id, new_instance);

  // Build the target state
  temp_f.map_d = (map_id == -1 ? cur_s->get_map() : find_map(map_id));
  temp_f.code_d = (code_id == -1 ? cur_s->get_code() : find_code(code_id));
  temp_f.machine = this;
  
  // Transfer state
  Transition* trans = cur_s->transfer( &temp_f, NULL );
  trans->insert_reason(trans_dec, cost);

  // Renew the position of this instance
  migrate_instance(ins_id, trans);

  //
  i_desc->add_operation(trans->last_);

  has_changed = true;
  return trans;
}


