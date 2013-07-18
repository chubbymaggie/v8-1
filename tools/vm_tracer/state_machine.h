// Description of state machine

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <vector>
#include <map>
#include <set>

using namespace std;

class State;

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

class StateMachine;


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
  TransMap edges;
  
 public:
  State() 
    {
      edges.clear();
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
};


class FunctionState : public State
{
 public:
  int code;
  
 public:
  FunctionState( int my_id )
    {
      code = 0;
      id = my_id;
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
  StatesPool states;
  map<int, State*> ptrs_now;
  
  // This class cannot be instantiated directly
  StateMachine()
    {
      
    }

public:
  State* start;
  string machine_name;

public:
  enum Mtype {
    Function,
    Object
  };

  State* search_state(State* s)
  {
    StatesPool::iterator it = states.find(s);

    if ( it == states.end() ) {
      s = s->clone();
      // Update the id
      s->id = states.size();
      states.insert(s);
    }
    else
      s = *it;

    return s;
  }

  void set_machine_name(const char* name)
  {
    if ( machine_name.size() == 0 )
      machine_name.append(name);
  }

  int size() 
  { 
    return states.size(); 
  }

  State* get_instance_pos(int d)
  {
    map<int, State*>::iterator it = ptrs_now.find(d);
    if ( it == ptrs_now.end() ) {
      // Add this instance
      ptrs_now[d] = start;
      return start;
    }
    
    return it->second;
  }

  void instance_walk_to(int d, State* s)
  {
    ptrs_now[d] = s;
  }
};

class FunctionMachine : public StateMachine
{
 public:
  bool allow_opt;
  string optMsg;
  
 public:
  FunctionMachine()
    {
      allow_opt = true;
      start = new FunctionState(0);
      states.insert(start);
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
};


#endif
