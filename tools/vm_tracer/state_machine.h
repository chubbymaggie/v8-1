// Description of state machine

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <vector>
#include <map>
#include <set>

using namespace std;

class State;
class StateMachine;

// Describes state transition edge
class Transition
{
private:
  struct str_ptr_cmp
  {
    bool operator()(const string* lhs, const string* rhs) const
    {
      return (*lhs) < (*rhs);
    }
  };

public:
  State* target;
  int count;
  set<string*, str_ptr_cmp> reasons;
  
  Transition()
  {
    target = NULL;
    count = 0;
  }
  
  Transition(State* t_)
  {
    target = t_;
    count = 0;
  }

  void insert_reason(const char* r)
  {
    string *rs = new string(r);

    if ( reasons.find(rs) == reasons.end() )
      reasons.insert( rs );
    else
      delete rs;
  }

  string* merge_reasons()
  {
    int i = 0;
    string *final = new string;
    
    for ( set<string*, str_ptr_cmp>::iterator it = reasons.begin();
	  it != reasons.end();
	  ++it ) {
      if ( i > 0 )
	final->push_back('+');
      final->append( **it );
      ++i;
    }
    
    return final;
  }
};


// Base class for all different states
class State 
{
 public:
  struct state_ptr_cmp
  {
    bool operator()(const State* lhs, 
		    const State* rhs) const
    {
      return lhs->less_than( rhs );
    }
  };
  
  typedef map<State*, Transition*, state_ptr_cmp> TransMap;
  typedef TransMap::iterator TransIterator;
  
 public:
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
  virtual bool equals( const State* ) const = 0;
  virtual bool less_than( const State* ) const = 0;
  virtual State* clone() const = 0;
  virtual string descriptor() const = 0;
  
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
    Transition* trans = new Transition(next_s);
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


class FunctionState : public State
{
 public:
  int code;
  
 public:
  FunctionState() { code = 0; }
  
  FunctionState( int my_id )
    {
      code = 0;
      id = my_id;
    }
  
  FunctionState( int my_id, StateMachine* m )
    {
      code = 0;
      id = my_id;
      machine = m;
    }
  
  // Virtual functions
  bool equals(const State* other) const
  {
    return operator==((const FunctionState*)other);
  }
  
  bool less_than( const State* other ) const
  {
    const FunctionState* fs_o = (const FunctionState*)other;
    return code < fs_o->code;
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
  
  bool operator==(const FunctionState* fs_o) const
  {
    return fs_o->code == code;
  }
};

class StateMachine
{
 private:
  typedef set<State*, State::state_ptr_cmp> StatesPool;
  
 protected:
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
    Function,
    Object
  };
  
  // Different state machine may have different additional actions
  virtual State* insert_new_state(State* s) = 0;
  
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
    FunctionState* fs = (FunctionState*)s->clone();
    // Update the id
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

  void set_opt_state( bool allow, const char* msg )
  {
    allow_opt = allow;
    optMsg.assign(msg);
  }
};

class ObjectMachine : public StateMachine
{
 public:
  ObjectMachine()
    {
      // To fix.
      start = new FunctionState(0);
      states.insert(start);
    }

  State* insert_new_state(State* s)
  {
    return s;
  }
};


#endif
