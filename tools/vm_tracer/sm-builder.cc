// Handling events, modeling the events as state machine
// By richardxx, 2013.10

#include <cstring>
#include <cassert>
#include "events.h"
#include "sm-builder.hh"
#include "miner.hh"

using namespace std;


// Events types
enum InternalEvent {
#define GetEventName(name, handler) name,
  
  OBJECT_EVENTS_LIST(GetEventName)
  FUNCTION_EVENTS_LIST(GetEventName)
  MAP_EVENTS_LIST(GetEventName)
  SYS_EVENTS_LIST(GetEventName)

  events_count
  
#undef GetEventname
};


// Define the events handler prototype
typedef void (*EventHandler)(FILE*);


// Handler declarations
#define DeclEventHandler(name, handler)		\
  static void handler(FILE*);

OBJECT_EVENTS_LIST(DeclEventHandler)
FUNCTION_EVENTS_LIST(DeclEventHandler)
MAP_EVENTS_LIST(DeclEventHandler)
SYS_EVENTS_LIST(DeclEventHandler)

static void
null_handler(FILE* file) { }

#undef DeclEventHandler


// Fill in the hanlder list
EventHandler handlers[] = {
#define GetEventHandler(name, handler) handler,

  OBJECT_EVENTS_LIST(GetEventHandler)
  FUNCTION_EVENTS_LIST(GetEventHandler)
  MAP_EVENTS_LIST(GetEventHandler)
  SYS_EVENTS_LIST(GetEventHandler)

  null_handler
#undef GetEventHandler
};


// From allocation signature to state machine descriptor
static map<int, StateMachine*> machines[StateMachine::MCount];

// From function/object/map instance to internal descriptor
static map<int, InstanceDescriptor*> instances[StateMachine::MCount];

// Internal id counters
static int id_counter[StateMachine::MCount];


// --------------Helper functions
static InstanceDescriptor*
install_context(int context, Transition* trans)
{
  StateMachine* sm = NULL;
  InstanceDescriptor* i_desc = NULL;

  if ( context != 0 ) {
    i_desc = find_instance(context, StateMachine::MFunction, true, true);
    sm = i_desc->sm;
  }

  // Fix: We directly replace the last context, this might be a problem
  TransPacket* tp = trans->last_;
  tp->context = sm;

  return i_desc;
}


// ---------------Events Handlers------------------
static void
create_boilerplate_common(FILE* file, const char* msg)
{
  int def_function;
  int o_addr;
  int map_id;
  int index;

  fscanf( file, "%p %p %p %d",
	  &def_function,
	  &o_addr, &map_id, &index );

  StateMachine::Mtype type = StateMachine::MBoilerplate;

  InstanceDescriptor* i_desc = find_instance(o_addr, type, true, false);
  StateMachine* sm = find_signature(o_addr, type, true);
  
  if ( i_desc->sm != sm ) {
    i_desc->sm = sm;
  }
  
  // Update transitions
  ObjectMachine* osm = (ObjectMachine*)sm;
  Transition* trans = osm->evolve(i_desc, -1, map_id, NULL, msg, 0, true);
  
  // Update context
  i_desc = install_context(def_function, trans);

  // If this is a new machine
  if ( !sm->has_name() ) {
    char buf[256];
    sprintf( buf, "%s->%d", i_desc->sm->m_name.c_str(), index ); 
    sm->set_machine_name(buf);
    // We use instance id as allocation signature
    machines[type][o_addr] = sm;
  }
}


static void 
create_obj_boilerplate(FILE* file)
{
  create_boilerplate_common(file, "NewObj");
}


static void 
create_array_boilerplate(FILE* file)
{
  create_boilerplate_common(file, "NewAry");
}


static void
create_obj_common(FILE* file, StateMachine::Mtype type, const char* msg)
{
  int def_function;
  int o_addr;
  int alloc_sig;
  int map_id, code;
  char name_buf[256];

  fscanf( file, "%p %p %p %p",
	  &def_function, &o_addr, 
	  &alloc_sig, &map_id);
  
  if ( type == StateMachine::MFunction )
    fscanf( file, "%p", &code );
  
  fscanf( file, " %[^\t\n]", name_buf );
  
  // We lookup the instance first, because some operations may already use this object
  InstanceDescriptor* i_desc = find_instance(o_addr, type, true, false);
  // We looup signature for machine
  StateMachine* sm = find_signature(alloc_sig, type, true);

  if ( i_desc->sm != sm ) {
    // In case they are different, this instance might be reclaimed by GC
    // We directly reset the machine to new machine
    i_desc->sm = sm;
  }

  // If this is a new machine
  if ( !sm->has_name() ) {
    sm->set_machine_name( name_buf );
  }
  
  // We look up if this object is created by cloning a boilerplate
  ObjectMachine* boilerplate = NULL;
  if ( type == StateMachine::MObject ) {
    boilerplate = (ObjectMachine*)find_signature(alloc_sig, StateMachine::MBoilerplate);
  }

  // Update transitions
  Transition* trans = NULL;

  switch (type) {
  case StateMachine::MObject:
    {
      ObjectMachine* osm = (ObjectMachine*)sm;
      trans = osm->evolve(i_desc, -1, map_id, boilerplate, msg, 0, true);
    }
    break;

  case StateMachine::MFunction:
    {
      FunctionMachine* fsm = (FunctionMachine*)sm;
      trans = fsm->evolve(i_desc, map_id, code, msg, 0, true);
    }
    break;

  default:
    break;
  }

  // Install context
  install_context(def_function, trans);
}


static void 
create_object_literal(FILE* file)
{
  create_obj_common(file, StateMachine::MObject, "NewObjLit");
}


static void 
create_array_literal(FILE* file)
{
  create_obj_common(file, StateMachine::MObject, "NewArrayLit");
}


static void 
create_new_object(FILE* file)
{
  create_obj_common(file, StateMachine::MObject, "NewObj");
}


static void 
create_new_array(FILE* file)
{
  create_obj_common(file, StateMachine::MObject, "NewArray");
}


static void
create_function(FILE* file)
{
  create_obj_common(file, StateMachine::MFunction, "NewFunc");
}


static void
copy_object(FILE* file)
{
  int def_function;
  int dst, src;

  fscanf( file, 
	  "%p %p %p", 
	  &def_function, &dst, &src);

  InstanceDescriptor* src_desc = find_instance(src, StateMachine::MObject);
  if ( src_desc == NULL ) return;
  
  InstanceDescriptor* dst_desc = find_instance(dst, StateMachine::MObject, true);
  StateMachine* sm = src_desc->sm;
  dst_desc->sm = sm;
  
  // Directly put the new object to a state
  State* s = sm->get_instance_pos(src_desc->id);
  sm->add_instance(dst_desc->id, s);  
}


static InstanceDescriptor*
lookup_object(int o_addr)
{
  InstanceDescriptor* i_desc = find_instance(o_addr, StateMachine::MBoilerplate);
  if ( i_desc == NULL )
    i_desc = find_instance(o_addr, StateMachine::MObject, true, true);
  return i_desc;
}


// Just make a transition without additional semantics
static Transition*
SimpleObjectTransition( int context, int o_addr, int old_map_id, int map_id, const char* msg, int cost = 0 )
{
  InstanceDescriptor* i_desc = lookup_object(o_addr);
  ObjectMachine* osm = (ObjectMachine*)i_desc->sm;
  Transition* trans = osm->evolve(i_desc, old_map_id, map_id, NULL, msg, cost);

  // Install context
  install_context(context, trans);
  return trans;
}


// If the last transition is a set map operation, just replace it
// Otherwise call SimpleObjectTransition
static Transition*
ReplaceSetMapTransition( int context, int o_addr, int old_map_id, int map_id, const char* msg, int cost = 0 )
{
  InstanceDescriptor* i_desc = lookup_object(o_addr);
  Transition* trans;
  
  if ( i_desc->last_raw_transition != NULL ) {
    // We just fill up the last transition
    StateMachine* sm = i_desc->sm;
    trans = i_desc->last_raw_transition;
    trans->insert_reason(msg, cost);
    sm->migrate_instance(i_desc->id, trans);
    i_desc->last_raw_transition = NULL;
    // Install context
    install_context(context, trans);
  }
  else {
    trans = SimpleObjectTransition( context, o_addr, old_map_id, map_id, msg, cost );
  }
  
  return trans;
}


static void
change_prototype(FILE* file)
{
  int def_function;
  int o_addr;
  int map_id, proto;
  char msg[128];

  fscanf( file, "%p %p %p %p",
	  &def_function,
	  &o_addr, &map_id, &proto );
  
  sprintf(msg, "ChgProto: %p", proto);
  ReplaceSetMapTransition( def_function, o_addr, -1, map_id, msg );
}


static void
set_map(FILE* file)
{
  int def_function;
  int o_addr;
  int map_id;

  fscanf( file, "%p %p %p",
	  &def_function,
	  &o_addr, &map_id );
  
  InstanceDescriptor* i_desc = lookup_object(o_addr);
  ObjectMachine* osm = (ObjectMachine*)i_desc->sm;
  i_desc->last_raw_transition = osm->set_instance_map(i_desc, map_id);
}


static void
field_update_common(FILE* file, char* msg)
{
  int def_function;
  int o_addr;
  int old_map_id, map_id;

  fscanf( file, "%p %p %p %p %[^\t\n]",
	  &def_function,
	  &o_addr, 
	  &old_map_id, &map_id, 
	  msg + strlen(msg) );
  
  ReplaceSetMapTransition( def_function, o_addr, old_map_id, map_id, msg ); 
}


static void
new_field(FILE* file)
{
  char msg[256];
  sprintf( msg, "NewFld: ");
  field_update_common(file, msg);
}


static void
del_field(FILE* file)
{
  char msg[256];
  sprintf( msg, "DelFld: ");
  field_update_common(file, msg);
}


static void
write_field_transition(FILE* file)
{
  char msg[256];
  sprintf( msg, "ChgFld: ");
  field_update_common(file, msg);
}


static void
elem_transition(FILE* file)
{
  int def_function;
  int o_addr;
  int old_map_id, map_id;
  int bytes;

  fscanf( file, "%p %p %p %p %d",
	  &def_function,
	  &o_addr, 
	  &old_map_id, &map_id, 
	  &bytes );

  ReplaceSetMapTransition( def_function, o_addr, old_map_id, map_id, "ElmTran", bytes );
}


static void
cow_copy(FILE* file)
{
  int def_function;
  int o_addr;
  int bytes;
  
  fscanf( file, "%p %p %d",
	  &def_function,
	  &o_addr, &bytes );

  ReplaceSetMapTransition( def_function, o_addr, -1, -1, "CowCpy", bytes );
}


static void
elem_to_slow(FILE* file)
{
  int def_function;
  int o_addr;
  int old_map_id, map_id;

  fscanf( file, "%p %p %p %p",
	  &def_function,
	  &o_addr, 
	  &old_map_id, &map_id );

  ReplaceSetMapTransition( def_function, o_addr, old_map_id, map_id, "ElemSlow" );
}


static void
prop_to_slow(FILE* file)
{
  int def_function;
  int o_addr;
  int old_map_id, map_id;
    
  fscanf( file, "%p %p %p %p",
	  &def_function,
	  &o_addr, 
	  &old_map_id, &map_id);

  ReplaceSetMapTransition( def_function, o_addr, old_map_id, map_id, "PropSlow");
}


static void
elem_to_fast(FILE* file)
{
  int def_function;
  int o_addr;
  int old_map_id, map_id;

  fscanf( file, "%p %p %p %p",
	  &def_function,
	  &o_addr, 
	  &old_map_id, &map_id );

  ReplaceSetMapTransition( def_function, o_addr, old_map_id, map_id, "ElemFast" );
}


static void
prop_to_fast(FILE* file)
{
  int def_function;
  int o_addr;
  int old_map_id, map_id;
    
  fscanf( file, "%p %p %p %p",
	  &def_function,
	  &o_addr, 
	  &old_map_id, &map_id);

  ReplaceSetMapTransition( def_function, o_addr, old_map_id, map_id, "PropFast");
}


static void
expand_array(FILE* file)
{
  int def_function;
  int o_addr;
  int bytes;
  
  fscanf( file, "%p %p %d",
	  &def_function,
	  &o_addr, &bytes );

  ReplaceSetMapTransition( def_function, o_addr, -1, -1, "AryExp", bytes );
}


static void
array_ops_copy(FILE* file)
{
  // To-do
}


static Transition*
SimpleFunctionTransition( int f_addr, int code, const char* msg, int cost = 0 )
{
  InstanceDescriptor* i_desc = find_instance(f_addr, StateMachine::MFunction, true, true);
  FunctionMachine* fsm = (FunctionMachine*)i_desc->sm;
  return fsm->evolve(i_desc, -1, code, msg, cost);
}


static void
gen_full_code(FILE* file)
{
  int f_addr, code;
  fscanf( file, "%p %p", 
	  &f_addr, &code );
  SimpleFunctionTransition( f_addr, code, "Full" );
}


static void
gen_opt_code(FILE* file)
{
  int f_addr, code;
  char opt_buf[256];
  
  sprintf( opt_buf, "Opt: " );
  fscanf( file, "%p %p %[^\t\n]", 
	  &f_addr, &code, 
	  opt_buf + strlen(opt_buf) );
  
  Transition* trans = SimpleFunctionTransition( f_addr, code, opt_buf );
  State* s = trans->source;
  FunctionMachine* fm = (FunctionMachine*)s->machine;
  fm->been_optimized = true;
}


static void
gen_osr_code(FILE* file)
{
  int f_addr, code;
  char opt_buf[256];

  sprintf( opt_buf, "Osr: " );
  fscanf( file, "%p %p %[^\t\n]",
          &f_addr, &code, opt_buf + strlen(opt_buf) );

  Transition* trans = SimpleFunctionTransition( f_addr, code, opt_buf );
  State* s = trans->source;
  FunctionMachine* fm = (FunctionMachine*)s->machine;
  fm->been_optimized = true;
}


static void
set_code(FILE* file)
{
  int f_addr, code;

  fscanf( file, "%p %p", &f_addr, &code );
}


static void
disable_opt(FILE* file)
{
  int shared, f_addr;
  char opt_buf[256];

  fscanf( file, "%p %p %[^\t\n]",
          &f_addr, &shared, opt_buf );

  StateMachine* sm = find_signature(shared, StateMachine::MFunction);
  if ( sm == NULL ) return;
  FunctionMachine* fsm = (FunctionMachine*)sm;
  fsm->set_opt_state( false, opt_buf );
}


static void
reenable_opt(FILE* file)
{
  int shared, f_addr;
  char opt_buf[256];

  fscanf( file, "%p %p %[^\t\n]",
          &f_addr, &shared, opt_buf );

  StateMachine* sm = find_signature(shared, StateMachine::MFunction);
  if ( sm == NULL ) return;
  FunctionMachine* fsm = (FunctionMachine*)sm;
  fsm->set_opt_state( true, opt_buf );
}


static void
gen_opt_failed(FILE* file)
{
  int f_addr, new_code;
  char opt_buf[256];

  sprintf( opt_buf, "OptFailed: " );
  int last_pos = strlen(opt_buf);
  
  fscanf( file, "%p %p %[^\t\n]",
          &f_addr, 
	  &new_code, opt_buf + last_pos );

  InstanceDescriptor* i_desc = find_instance(f_addr, StateMachine::MFunction);
  if ( i_desc == NULL ) return;
  FunctionMachine* fsm = (FunctionMachine*)i_desc->sm;
    
  if ( opt_buf[last_pos] == '-' &&
       opt_buf[last_pos+1] == '\0' ) {
    // Reuse the disable message
    sprintf( opt_buf+last_pos, "%s", fsm->optMsg.c_str() );
  }
  
  fsm->evolve( i_desc, -1, new_code, opt_buf );
}


static Transition*
do_deopt_common(int f_addr, int old_code, int new_code, const char* msg)
{
  InstanceDescriptor* i_func = find_instance(f_addr, StateMachine::MFunction);
  if ( i_func == NULL ) return NULL;
  FunctionMachine* fsm = (FunctionMachine*)i_func->sm;
  
  // We first decide if the old_code is used currently
  FunctionState* cur_s = (FunctionState*)fsm->get_instance_pos(i_func->id);
  if ( cur_s->code_d->id() != old_code ) {
    //This might be missing sites in V8 engine to capture all VM actions.
    //A quick fix, we directly make a transition.    
    fsm->evolve(i_func, -1, old_code, "Opt");
  }
  
  // Then, we transfer to the new_code
  return fsm->evolve(i_func, -1, new_code, msg);
}


static void
regular_deopt(FILE* file)
{
  int f_addr, old_code, new_code;
  int failed_obj, exp_map_id;
  char deopt_buf[256];

  sprintf( deopt_buf, "Deopt: " );
  fscanf( file, "%p %p %p %p %p %[^\t\n]",
	  &f_addr,
	  &old_code,
	  &new_code,
	  &failed_obj, &exp_map_id,
	  deopt_buf + strlen(deopt_buf) );

  // We first model this transition
  Transition* trans = do_deopt_common(f_addr, old_code, new_code, deopt_buf);
  if ( trans == NULL ) return;

  // Infer
  printf( "[%s] deoptimized, possibly because:\n", trans->source->machine->toString().c_str() );

  if ( exp_map_id != 0 ) {
    Map* exp_map = find_map(exp_map_id, false);
    if ( exp_map == null_map ) return;
    infer_deoptimization_reason(failed_obj, exp_map);
  }
  else if ( failed_obj != 0 ) {
    InstanceDescriptor* i_obj = find_instance(failed_obj, StateMachine::MObject);
    printf( "\tPlease consider the recent actions on [%s]\n", i_obj->sm->toString().c_str() ); 
  }
  else {
    printf( "\tSorry, I don't know, T__T\n" );
  }
  
  printf( "\n" );
}


static void
deopt_as_inline(FILE* file)
{
  int f_addr;
  int old_code, new_code;
  int real_deopt_func;

  fscanf( file, "%p %p %p %p",
	  &f_addr, &old_code, &new_code,
	  &real_deopt_func );

  Transition* trans = do_deopt_common(f_addr, old_code, new_code, "Deopt: inline");
}


static void
force_deopt(FILE* file)
{
  int f_addr;
  int old_code, new_code;

  fscanf( file, "%p %p %p",
	  &f_addr, &old_code, &new_code );

  Transition* trans = do_deopt_common(f_addr, old_code, new_code, "ForceDeopt");
  if ( trans == NULL ) return;
  FunctionMachine* fsm = (FunctionMachine*)trans->source->machine;

  record_deopt_function(fsm);
}


static void
begin_deopt_on_map(FILE* file)
{
  int map_id;
  fscanf( file, "%p", &map_id );

  // Map change can result in code deoptimization
  Map* map_d = find_map(map_id);
  register_map_notifier(map_d);
}


static void
gc_move_object(FILE* file)
{
  int from, to;
  fscanf( file, "%p %p", &from, &to );
  return;
  // Update the specific instance
  for ( int i = StateMachine::MBoilerplate; i <= StateMachine::MFunction; ++i ) {
    StateMachine::Mtype type = (StateMachine::Mtype)i;
    InstanceDescriptor* i_desc = find_instance(from, type);
    if ( i_desc != NULL ) {
      // First update the mapping for global descriptors
      instances[type].erase(from);
      instances[type][to] = i_desc;

      // Second update the mapping for local record
      StateMachine* sm = i_desc->sm;
      sm->rename_instance(from, to);
      break;
    }
  }

  // Some objects are used as signatures for other objects
  for ( int i = StateMachine::MBoilerplate; i <= StateMachine::MObject; ++i ) {
    StateMachine::Mtype type = (StateMachine::Mtype)i;
    StateMachine* sm = find_signature(from, type);
    if ( sm != NULL ) {
      machines[type].erase(from);
      machines[type][to] = sm;
    }
  }
}


static void
gc_move_map(FILE* file)
{
  int old_id, new_id;
  fscanf( file, "%p %p", &old_id, &new_id );

  Map* map_d = find_map(old_id);
  if ( map_d == null_map ) return;
  map_d->update_map(new_id);
}


static void
gc_move_shared(FILE* file)
{
  int from, to;
  fscanf( file, "%p %p", &from, &to );

  // Update functioin machine
  StateMachine::Mtype type = StateMachine::MFunction;
  StateMachine* sm = find_signature(from, type);
  if ( sm != NULL ) {
    machines[type].erase(from);
    machines[type][to] = sm;
  }
  
  // Update the object machine that uses sharedinfo as signature
  type = StateMachine::MObject;
  sm = find_signature(from, type);
  if ( sm != NULL ) {
    machines[type].erase(from);
    machines[type][to] = sm;
  }
}


static void
gc_move_code(FILE* file)
{
  int old_code, new_code;
  fscanf( file, "%p %p", 
	  &old_code, &new_code );

  Code* code_d = find_code(old_code);
  if ( code_d == null_code ) return;

  code_d->update_code(new_code);
  // We make a transition to all the instances owning this code
  // set<State*, State::state_ptr_cmp> it, end;
  // for ( it = code_d->used_by.begin(),
  // 	  end = code_d->used_by.end(); it != end; ++it ) {
    
  //   State* fs = *it;
  //   StateMachine* fsm = fs->machine;
  // }
}


static void
notify_stack_deopt_all(FILE* file)
{
  
}


// ---------------Public interfaces-------------------

StateMachine* 
find_signature(int m_sig, StateMachine::Mtype type, bool create)
{
  StateMachine* sm = NULL;
  map<int, StateMachine*> &t_mac = machines[type];
  map<int, StateMachine*>::iterator i_sm = t_mac.find(m_sig);
  
  if ( i_sm != t_mac.end() ) {
    // found
    sm = i_sm->second;
  }
  else if ( create == true ) {
    sm = StateMachine::NewMachine(type);
    t_mac[m_sig] = sm;
  }
  
  return sm;
}


InstanceDescriptor*
find_instance( int ins_addr, StateMachine::Mtype type, bool create_descriptor, bool create_sm)
{
  map<int, InstanceDescriptor*> &t_ins_map = instances[type];
  map<int, InstanceDescriptor*>::iterator it;

  it = t_ins_map.find(ins_addr);

  if ( it == t_ins_map.end() ) {
    if ( !create_descriptor ) return NULL;

    // Create a new instance descriptor
    InstanceDescriptor* i_desc = new InstanceDescriptor;
    if ( create_sm ) i_desc -> sm = StateMachine::NewMachine(type);
    i_desc -> id = id_counter[type]++;
    t_ins_map[ins_addr] = i_desc;

    return i_desc;
  }
  
  return it->second;
}


void 
prepare_machines()
{
  for ( int i = 0; i < StateMachine::MCount; ++i ) {
    id_counter[i] = 0;
  }
}


void 
clean_machines()
{
  
}


void
visualize_machines(const char* file_name)
{
  FILE* file;
  extern int states_count_limit;

  file = fopen(file_name, "w");
  if ( file == NULL ) {
    fprintf(stderr, "Cannot create visualization file %s\n", file_name);
    return;
  }
  
  // Do BFS to draw graphs
  for ( int i = StateMachine::MObject; i < StateMachine::MCount; ++i ) {
    map<int, StateMachine*> &t_mac = machines[i];

    for ( map<int, StateMachine*>::iterator it = t_mac.begin();
	  it != t_mac.end();
	  ++it ) {
      StateMachine* sm = it->second;
      if ( sm->size() < states_count_limit ) continue;
      if ( !sm->has_name() ) continue;
      if ( i == StateMachine::MFunction &&
	   ((FunctionMachine*)sm)->been_optimized == false ) continue;

      sm->draw_graphviz( file );
    }
  }
    
  fclose(file);
}


static void
sanity_check()
{
#define ASSERT_HANDLER(name, handler)		\
  assert(handlers[name] == handler);

  OBJECT_EVENTS_LIST(ASSERT_HANDLER)
  FUNCTION_EVENTS_LIST(ASSERT_HANDLER)
  MAP_EVENTS_LIST(ASSERT_HANDLER)
  SYS_EVENTS_LIST(ASSERT_HANDLER)
}


bool 
build_automata(const char* log_file)
{
  FILE* file;
  int event_type;
  
  file = fopen( log_file, "r" );
  if ( file == NULL ) return false;
  
  prepare_machines();
  int i = 0;
  while ( fscanf(file, "%d", &event_type) == 1 ) {
    //printf("before %d, ", i);
    handlers[event_type](file);
    //sanity_check();
    //printf("after %d\n", i);
    //fflush(stdout);
    ++i;
  }
  
  register_map_notifier(NULL);
  clean_machines();
  fclose(file);
  return true;
}

