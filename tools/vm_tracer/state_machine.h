// Description of state machine

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>

using namespace std;

class State;
class StateMachine;

// Describe a transition
struct TransPacket
{
  string* reason;
  int extra;

  TransPacket()
  {
    reason = NULL;
    extra = 0;
  }

  TransPacket(const char* desc)
  {
    reason = new string(desc);
    extra = 0;
  }

  void clear()
  {
    if ( reason != NULL ) {
      delete reason;
      reason = NULL;
    }
  }
  
  void describe(stringstream& ss) const
  {
    ss << *reason;
    if ( extra != 0 )
      ss << ": " << extra;
  }
  
  bool operator<(const TransPacket& rhs) const
  {
    return (*reason) < *(rhs.reason);
  }
};

// Describes state transition edge
class Transition
{
 public:
  // Transition target
  State *source, *target;
  // Number of times this transition are repeated
  int count;
  // Why did this transition happen?
  set<TransPacket> triggers;


 public:  
  Transition()
    {
      source = NULL;
      target = NULL;
      count = 0;
    }
  
  Transition(State* s_, State* t_)
    {
      source = s_;
      target = t_;
      count = 0;
    }

  void insert_reason(const char* r, int extra = 0)
  {
    if ( r == NULL ) return;
    TransPacket tp(r);

    set<TransPacket>::iterator it = triggers.find(tp);
    if ( it != triggers.end() ) {
      tp.clear();
      tp = *it;
      triggers.erase(it);
    }

    tp.extra += extra;
    triggers.insert(tp);
  }

  void merge_reasons(string& final)
  {
    int i = 0;
    stringstream ss;
    
    for ( set<TransPacket>::iterator it = triggers.begin();
	  it != triggers.end();
	  ++it ) {
      if ( i > 0 )
	ss << '+';
      it->describe(ss);
      ++i;
    }
    
    final = ss.str();
  }
};



// Base class for all different states
class State 
{
 public:
  struct state_ptr_cmp
  {
    bool operator()(State* lhs, State* rhs)
    {
      return lhs->less_than( rhs );
    }
  };
  
  typedef map<State*, Transition*, state_ptr_cmp> TransMap;
  typedef TransMap::iterator TransIterator;
  
 public:
  // ID for this state
  int id;
  // Outgoing transition edges
  TransMap edges;
  // The state machine that contains this state
  StateMachine* machine;
  // Instances that own this state
  set<int> instances;

 public:
  State() 
    {
      edges.clear();
      machine = NULL;
      instances.clear();
    }
  
  // Interfaces
  virtual bool less_than( const State* ) const = 0;
  virtual State* clone() const = 0;
  virtual string descriptor() const = 0;
  virtual void set_core_data(int) = 0;
  virtual int get_core_data() = 0;
  
  // Accessors for transition edges
  inline TransIterator begin()
  {
    return edges.begin();
  }

  inline TransIterator end()
  {
    return edges.end();
  }

  inline int size() { return edges.size(); }

  Transition* find_transition( State* next_s )
  {
    TransMap::iterator it = edges.find(next_s);
    if ( it == edges.end() ) return NULL;
    return it->second;
  }

  Transition* add_transition( State* next_s )
  {
    Transition* trans = new Transition(this, next_s);
    edges[next_s] = trans;
    return trans;
  }

  void add_instance(int d)
  {
    instances.insert(d);
  }

  void remove_instance(int d)
  {
    instances.erase(d);
  }
};


// Describe function
class FunctionState : public State
{
 public:
  int code;
  
 public:
  FunctionState() 
    { 
      id = 0;
      machine = NULL;
      code = 0;
    }
  
  FunctionState( int my_id )
    {
      id = my_id;
      machine = NULL;
      code = 0;
    }
  
  FunctionState( int my_id, StateMachine* m )
    {
      id = my_id;
      machine = m;
      code = 0;
    }
  
  // Implement virtual functions
  bool less_than(const State* other) const
  {
    return code < ((const FunctionState*)other)->code;
  }
    
  State* clone() const
  {
    FunctionState* new_s = new FunctionState(id);
    new_s->code = code;
    new_s->machine = machine;
    return new_s;
  }

  string descriptor() const 
  {
    char buf[32];
    sprintf(buf, "%p", code);
    return string(buf);
  }

  void set_core_data(int new_code)
  {
    code = new_code;
  }
  
  int get_core_data()
  {
    return code;
  }
    
  // Own functions
  bool operator==(const FunctionState* fs_o) const
  {
    return fs_o->code == code;
  }
};



// Describe object
class ObjectState : public State
{
 public:
  // Describe the internal structure of object
  int struct_d;

 public:
  ObjectState() 
    { 
      id = 0;
      machine = NULL;
      struct_d= 0; 
    }
  
  ObjectState( int my_id )
    {
      id = my_id;
      machine = NULL;
      struct_d = 0;
    }

  // Implement virtual functions
  bool less_than(const State* other) const
  {
    return struct_d < ((const ObjectState*)other)->struct_d;
  }
   
  State* clone() const
  {
    ObjectState* new_s = new ObjectState;
    new_s->struct_d = struct_d;
    new_s->machine = machine;
    return new_s;
  }

  string descriptor() const 
  {
    char buf[32];
    sprintf(buf, "%p", struct_d);
    return string(buf);
  }

  void set_core_data(int new_struct_d)
  {
    struct_d = new_struct_d;
  }

  int get_core_data()
  {
    return struct_d;
  }

  // Own functions
  bool operator==(const ObjectState* os_o) const
  {
    return os_o->struct_d == struct_d;
  }
};


// A state machine maintains a collection of states
class StateMachine
{
 private:
  typedef set<State*, State::state_ptr_cmp> StatesPool;
  
 public:
  // Record all the states belonging to this machine
  StatesPool states;
  // Map object/function instances to state
  map<int, State*> inst_at;
  
  StateMachine()
    {
      states.clear();
      inst_at.clear();
      m_name.clear();
    }

 public:
  // Start state of this machine
  State* start;
  // Name of this machine
  string m_name;
  
 public:
  enum Mtype {
    TFunction,
    TObject
  };
  
  // Different machines may have different additional actions
  virtual State* insert_new_state(State* s) = 0;
  virtual Mtype type() = 0;

  void set_machine_name(const char* name)
  {
      m_name.assign(name);
  }

  bool has_no_name()
  {
    return m_name.size() == 0;
  }

  int size() 
  { 
    return states.size(); 
  }

  // Has the given state been included?
  State* search_state(State* s)
  {
    StatesPool::iterator it = states.find(s);
    
    if ( it == states.end() ) {
      s = insert_new_state(s);
    }
    else
      s = *it;
    
    return s;
  }

  State* get_instance_pos(int d)
  {
    State* s = NULL;

    map<int, State*>::iterator it = inst_at.find(d);
    if ( it == inst_at.end() ) {
      // Add this instance
      s = start;
      inst_at[d] = s;
      s->add_instance(d);
    }
    else {
      s = it->second;
    }

    return s;
  }

  void instance_walk_to(int d, State* new_s)
  {
    // Update instance <-> state maps
    State* prev_s = inst_at[d];
    prev_s->remove_instance(d);

    inst_at[d] = new_s;
    new_s->add_instance(d);
  }
};


// Specialized state machine storing function states only
class FunctionMachine : public StateMachine
{
 public:
  // Code -> states map
  // States are in different machines
  typedef map<int, vector<FunctionState*>* > CodeStatesMap;
  static CodeStatesMap* code_in;

 public:
  // Is this function approved for optimization?
  bool allow_opt;
  // Optimization/deoptimization message
  string optMsg;
  
 public:
  FunctionMachine()
    {
      allow_opt = true;
      start = new FunctionState(0, this);
      states.insert(start);
    }

  State* insert_new_state(State* s)
  {
    // Create a new state
    FunctionState* fs = (FunctionState*)s->clone();
    fs->id = states.size();
    states.insert(fs);
    
    // Update code -> states map
    int code = fs->code;
    CodeStatesMap::iterator it = code_in->find(code);
    vector<FunctionState*> *vec = NULL;

    if ( it == code_in->end() ) {
      vec = new vector<FunctionState*>;
      (*code_in)[code] = vec;
    }
    else
      vec = it->second;
    
    vec->push_back(fs);
    
    return fs;
  }

  Mtype type() { return TFunction; }

  void set_opt_state( bool allow, const char* msg )
  {
    allow_opt = allow;
    optMsg.assign(msg);
  }
};


// Specialized machine for object states
class ObjectMachine : public StateMachine
{
 public:
  ObjectMachine()
    {
      start = new ObjectState(0);
      states.insert(start);
    }

  State* insert_new_state(State* s)
  {
    // Duplicate
    ObjectState* os = (ObjectState*)s->clone();
    os->id = states.size();
    states.insert(os);

    return os;
  }

  Mtype type() { return TObject; }
};


#endif
