// Description of state machine

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>

using std::string;
using std::vector;
using std::set;
using std::map;
using std::stringstream;

class Transition;
class State;
class StateMachine;
class ObjectMachine;
class FunctionMachine;
class InstanceDescriptor;


class CoreInfo
{
 public:
  // States coming here must be stored in any other places before
  // Hence, we directly compre the pointer values to distinguish them
  typedef std::vector<State*> RefSet;
  
 public:
  RefSet used_by;
  
 public:
  CoreInfo();
  // Record which states use this map
  void add_usage(State* user_s);
};


class Map : public CoreInfo
{
public:
  int map_id;

public:
  Map(int);
  int id() const { return map_id; }
  int& operator*() { return map_id; }
  void update_map(int);

public:
  //
  void correlate(Transition* trans);
  
public:
  vector<FunctionMachine*> deopt_functions;
};


class Code : public CoreInfo
{
 public:
  int code_id;

 public:
  Code(int);
  int id() const { return code_id; }
  int& operator*() { return code_id; }

  //
  void update_code(int);
};


extern Map* null_map;
extern Code* null_code;


// Find or create a map structure from given map_id
Map* 
find_map(int new_map, bool create=true);

// Find or create a code structure
Code* 
find_code(int new_code, bool create=true);


//
void 
register_map_notifier(Map*);


//
void
record_deopt_function(FunctionMachine*);


// Describe a transition
class TransPacket
{
 public:
  struct ptr_cmp
  {
    bool operator()(TransPacket* lhs, TransPacket* rhs)
    {
      return (lhs->reason) < (rhs->reason);
    }
  };

 public:
  // The transition that holds this packet
  Transition* trans;
  // Why this transition happened?
  string reason;
  // Cost of this transition
  int cost;
  // For an object event, context is the function where this operation happens
  StateMachine* context;
  //
  int count;

 public:
  TransPacket();
  TransPacket(const char* desc);
  TransPacket(const char* desc, int c_);

  bool has_reason();

  void describe(std::stringstream& ss) const;
  
  bool operator<(const TransPacket& rhs) const;

  TransPacket& operator=(const TransPacket& rhs);
};


// Normal state transition edge
class Transition
{
 public:
  enum TransType
  {
    TNormal,
    TSummary
  };

 public:
  // Transition target
  State *source, *target;
  // Transision triggering operations and their cost
  typedef std::set<TransPacket*, TransPacket::ptr_cmp> TpSet; 
  TpSet triggers;
  // Last triggering operation
  TransPacket* last_;
  
 public:  
  Transition();
   
  Transition(State* s_, State* t_);

  virtual TransType type() { return TNormal; }

  void insert_reason(const char* r, int cost = 0);

  void insert_reason(TransPacket* tp);

  void merge_reasons(string& final);

  // Tell the visualizer how to draw this transition
  virtual const char* graphviz_style();
};


// This transition describes a conceptual walk on another state machine
// An example usage is cloning a boilerplate
class SummaryTransition : public Transition
{
 public:
  // From which state on another machine, it exits and returns back
  State *exit;

 public:
  SummaryTransition();

  SummaryTransition(State* s_, State* t_, State* exit_ );

  TransType type() { return TSummary; }

  const char* graphviz_style();
};



// Base class for all different states
class State 
{
 public:
  struct ptr_cmp
  {
    bool operator()(State* lhs, State* rhs)
    {
      return lhs->less_than( rhs );
    }
  };
  
  typedef std::map<State*, Transition*, ptr_cmp> TransMap;
  typedef TransMap::iterator TransIterator;

  enum Stype
  {
    SObject,
    SFunction
  };

 public:
  // ID for this state
  int id;
  // Outgoing transition edges
  TransMap out_edges;
  // Incoming transition edges
  TransMap in_edges;
  // The state machine that contains this state
  StateMachine* machine;

 public:
  // Interfaces

  // Used for state search in set
  virtual bool less_than( const State* ) const = 0;
  // Make a clone of this state
  virtual State* clone() const = 0;
  // Generate a text description for this state
  virtual string toString() const = 0;
  // Generate graphviz style descriptor
  virtual const char* graphviz_style() const = 0;
  //
  virtual Stype type() const = 0;
  //
  virtual Map* get_map() const = 0;
  //
  virtual void set_map(Map*) = 0;


 public:
  State();

  // Return the number of transitions starting from this state
  int size();
  
  Transition* find_transition( State* next_s, bool by_boilerplate = false );
  
  Transition* add_transition( State* next_s );

  Transition* add_summary_transition( State* next_s, State* exit_s );

  Transition* transfer(State* next_s, ObjectMachine* boilerplate);
};


// Describe an object
class ObjectState : public State
{
 public:
  //
  Map* map_d;
  
 public:
  ObjectState();
  ObjectState( int my_id );

  // Implement virtual functions
  bool less_than(const State* other) const;   
  State* clone() const;
  string toString() const;
  const char* graphviz_style() const;
  Stype type() const { return SObject; }
  Map* get_map() const;
  void set_map(Map*);
};


// Describe a function
// Function is also an object
class FunctionState : public ObjectState
{
 public:
  Code* code_d;
  
 public:
  FunctionState();
  FunctionState( int my_id ); 
  void set_code(Code*);
  Code* get_code();

  // Implement virtual functions
  bool less_than(const State* other) const;
  State* clone() const;
  string toString() const;
  Stype type() const { return SFunction; }
};


// A state machine maintains a collection of states
class StateMachine
{
 public:
  typedef std::set<State*, State::ptr_cmp> StatesPool;

  enum Mtype {
    MBoilerplate,
    MObject,
    MFunction,
    MMap,
    MCount       // Record how many different machines
  };

  // The only way to create an instance of state machine is calling this static function
  static StateMachine* NewMachine(Mtype);

 public:
  //
  int id;
  // Record all the states belonging to this machine
  StatesPool states;
  // Map object/function instances to states
  map<int, State*> inst_at;
   // Start state of this machine
  State* start;
  // Name of this machine
  string m_name;
  // Record the type of this machine
  Mtype type;
  
 public:
  
  void set_machine_name(const char* name);
  
  bool has_name();

  string toString();

  int size();

  // Lookup if the given state has been created for this machine
  // If not, we copy the content of input state to create a new state
  State* search_state(State*, bool create=true);
  // Directly add the input state to states pool
  void add_state(State*);
  // Directly delete the input state from states pool
  void delete_state(State*);
  // Lookup the state for a particular instance
  State* get_instance_pos(int d, bool new_instance=false);
  //
  void add_instance(int, State*);
  // Replace an instance name to with a given name
  void rename_instance(int,int);
  // Move an instance to another state
  void migrate_instance(int, Transition*);

  // Output graphviz instructions to draw this machine
  // m_id is used as the id of this machine
  void draw_graphviz(FILE* file);

 protected:
  // Not available for instantialization
  StateMachine();

 private:
  static int id_counter; 
};


// Specialized machine for object states
class ObjectMachine : public StateMachine
{
 public:
  // Is this objct used as a boilerplate
  bool is_boilerplate;

 public:
  ObjectMachine();
  // We have one more constructor because object machine can support other types
  ObjectMachine(StateMachine::Mtype);

  // A specialized version of searching only object states
  State* search_state(Map*, bool=true);

  // Get exit state
  // If all instances are in the same state, we return this state as exit state
  // Otherwise it returns null
  State* exit_state();

  //
  ObjectState* jump_to_state_with_map(InstanceDescriptor*, int, bool);

  // Set transition that just sets map of an instance
  Transition* set_instance_map(InstanceDescriptor*, int map_d);

  //
  Transition* evolve(InstanceDescriptor*, int, int, ObjectMachine*, const char*, int = 0, bool = false );

 public:
  static ObjectState temp_o;
};


// Specialized state machine storing function states only
class FunctionMachine : public ObjectMachine
{
 public:
  //
  bool been_optimized;
  // Is this function approved for optimization?
  bool allow_opt;
  // Optimization/deoptimization message
  std::string optMsg;
   
 public:
  FunctionMachine();

  // A specialized version of searching only function states
  State* search_state(Map*, Code*, bool=true);
  
  //
  void set_opt_state( bool allow, const char* msg );

  // Evolve to the next state
  Transition* evolve(InstanceDescriptor*, int, int, const char*, int = 0, bool = false);

 public:
  static FunctionState temp_f;
};


class InstanceDescriptor
{
 public:
  // Internal id
  int id;
  // State machine that contains this instance
  StateMachine* sm;
  // Last transition for this intance
  Transition* last_raw_transition;
  // The operation history for this instance
  std::vector<TransPacket*> history;

 public:
  InstanceDescriptor() { 
    id = -1; 
    sm = NULL;
    last_raw_transition = NULL;
  }

  void add_operation(TransPacket* tp) { history.push_back(tp); }
};

#endif
