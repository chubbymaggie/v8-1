#include <cstdio>
#include "miner.hh"
#include "sm-builder.hh"

using namespace std;


// Globals
static vector<FunctionMachine*> force_deopt_functions;


void
print_machine_path(deque<Transition*>& path)
{
  bool f_first = true;
  string reason;
      
  for ( deque<Transition*>::iterator it = path.begin(), end = path.end(); 
	it != end; ++it ) {
    Transition* trans = *it;
    State* src = trans->source;
    State* tgt = trans->target;
    
    if ( f_first == true ) {
      printf( "{%s}", src->toString().c_str() );
      f_first = false;
    }
    
    reason.clear();
    trans->merge_reasons(reason);    
    printf( "--[%s]--{%s}", reason.c_str(), tgt->toString().c_str() );
  }
  
  printf("\n");
}


void
print_instance_path(vector<TransPacket*> &history, int i)
{
  bool f_first = true;
  int size = history.size();

  for ( ; i < size; ++i ) {
    TransPacket* tp = history[i];
    StateMachine* context = tp->context;
    Transition* trans = tp->trans;
    State* src = trans->source;
    State* tgt = trans->target;
    
    if ( f_first == true ) {
      printf( "{%s}", src->toString().c_str() );
      f_first = false;
    }
    
    printf( "--[%s, %s]-{%s}",
	    context->m_name.c_str(),
	    tp->reason.c_str(),
	    tgt->toString().c_str() );
  }
  
  printf("\n");
}


static int
backward_slice(InstanceDescriptor* i_desc, Map* exp_map)
{
  vector<TransPacket*> &history = i_desc->history;
  int size = history.size();
  int i ;

  for ( i = size - 1; i > -1; --i ) {
    TransPacket* tp = history[i];
    Map* cur_map = tp->trans->source->get_map();
    if ( cur_map == exp_map ) break;
  }

  // Not found
  if ( i == -1 ) return 0;

  StateMachine* sm = history[i]->trans->source->machine;
  printf( "\t%s: ", sm->m_name.c_str() );
  print_instance_path(history, i);
  printf( "\n" );

  return 1;
}


static int 
dfs_backward_search_path(State* cur_s, State* end_s, deque<Transition*>& path, bool reachability_test)
{
  if ( cur_s == end_s ) {
    // We found a path
    if ( !reachability_test ) {
      printf( "\t" );
      print_machine_path(path);
      printf( "\n" );
    }
    
    return 1;
  }
  
  int ans = 0;
  State::TransMap edges = cur_s->in_edges;
  for ( State::TransMap::iterator it = edges.begin(), end = edges.end();
	it != end; ++it ) {
    State* prev_s = it->first;
    Transition* trans = it->second;
    if ( prev_s == cur_s ) continue;
    path.push_back(trans);
    ans += dfs_backward_search_path(prev_s, end_s, path, reachability_test);
    path.pop_back();
    if ( ans > 0 && reachability_test == true ) break;
  }
  
  return ans;
}


/*
 * We search a path from cur_s to end_s.
 * For each state on the path, we search the history to see if this a past state for failed_obj.
 */
static int
dfs_backward_search_fork(State* cur_s, State* end_s, deque<Transition*>& path, vector<TransPacket*> &history)
{
  if ( cur_s == end_s ) return 0;

  Map* cur_map = cur_s->get_map();

  // We first juedge if this node is branch point
  // If it is, we do not go back further
  int size = history.size();
  int i;
  for ( i = size - 1; i > -1; --i ) {
    TransPacket* tp = history[i];
    Map* past_map = tp->trans->source->get_map();
    if ( past_map == cur_map ) break;
  }

  if ( i > -1 ) {
    // We output suggestion to programmer
    printf( "\tBranch paths found:\n" );
    
    printf( "\tRequired: " );
    print_machine_path(path);

    printf( "\tActually: ");
    print_instance_path(history, i);
    
    printf( "\tPlease try to eliminate two different evolution paths.\n" );
    return 1;
  }
  
  int ans = 0;
  State::TransMap edges = cur_s->in_edges;
  for ( State::TransMap::iterator it = edges.begin(), end = edges.end();
	it != end; ++it ) {
    State* prev_s = it->first;
    Transition* trans = it->second;
    if ( prev_s == cur_s ) continue;
    path.push_back(trans);
    ans += dfs_backward_search_fork(prev_s, end_s, path, history);
    path.pop_back();
  }
  
  return ans;
}


static int 
dfs_forward_search_path(State* cur_s, State* end_s, deque<Transition*>& path, bool reachability_test)
{
  if ( cur_s == end_s ) {
    // We found a path
    if ( !reachability_test ) {
      printf( "\tThe expected map is a potential map for this object. Try following operations on this object:\n" );
      printf( "\t" );
      print_machine_path(path);
      printf( "\n" );
    }
    
    return 1;
  }
  
  int ans = 0;
  int i = 0;
  State::TransMap edges = cur_s->out_edges;
  for ( State::TransMap::iterator it = edges.begin(), end = edges.end();
	it != end; ++it ) {
    State* next_s = it->first;
    Transition* trans = it->second;
    if ( next_s == cur_s ) continue;
    path.push_back(trans);
    ans += dfs_forward_search_path(next_s, end_s, path, reachability_test);
    path.pop_back();
    if ( ans > 0 && reachability_test == true ) break;
    ++i;
  }
  
  return ans;
}


static void 
output_force_deopt_functions()
{
  vector<FunctionMachine*>::iterator it_funcs;

  // Iterate the deoptimized functions
  for ( it_funcs = force_deopt_functions.begin();
	it_funcs != force_deopt_functions.end(); ++it_funcs ) {
    FunctionMachine* fm = *it_funcs;
    string fm_name = fm->toString();
    
    printf( "\t [%s]\n", fm_name.c_str() );
  }
  
  printf( "\n" );
  
  force_deopt_functions.clear();
}


// ------------------------------ Interface Implementation ---------------------------
void
record_force_deopt_function(FunctionMachine* f)
{
  force_deopt_functions.push_back(f);
}


void
ops_result_in_force_deopt(Transition* trans)
{
  if ( force_deopt_functions.size() == 0 ) return;
  
  printf( "Force deoptimization detected:\n" );

  if ( trans != NULL ) {
    TransPacket* tp = trans->last_;
    StateMachine* trigger_obj = trans->source->machine;
    
    string def_func_name;
    if ( tp->context != NULL )
      def_func_name = tp->context->toString();
    else
      def_func_name = "global";
    
    string &action = tp->reason;
    string obj_name = trigger_obj->toString();
    
    printf( "\tIn [%s], operation <%s> on object [%s]\n",
	    def_func_name.c_str(),
	    action.c_str(),
	    obj_name.c_str() );
  }
  else {
    printf( "\t(?)\n" );
  }

  printf( "\t===>\n" );
  output_force_deopt_functions();
}


void
sys_result_in_force_deopt(const char* reason)
{
  if ( force_deopt_functions.size() == 0 ) return;
  
  printf( "Force deoptimization detected:\n" );
  printf( "\t%s\n", reason);
  printf( "\t===>\n" );
  output_force_deopt_functions();
}


bool
infer_deoptimization_reason(int failed_obj, Map* exp_map, FunctionMachine* fsm)
{
  // Lookup if we have seen this instance
  InstanceDescriptor* i_obj = find_instance(failed_obj, StateMachine::MObject);
  if ( i_obj == NULL ) {
    printf( "\tOops, never seen this instance: %p\n", failed_obj );
    return false;
  }


  // Lookup if other instances from this same allocation site have the desired map
  ObjectMachine* osm = (ObjectMachine*)i_obj->sm;
  State* cur_state = osm->get_instance_pos(i_obj->id, false);
  State* exp_state = osm->search_state(exp_map, false);

  if ( exp_state == NULL ) {
    // This optimized code is trained by objects allocated from other places
    CoreInfo::RefSet &ref_states = exp_map->used_by;
    if ( ref_states.size() >  0 ) {
      printf( "\tThe optimized code is not trained by [%s], please check the following objects:\n", osm->toString().c_str() );

      CoreInfo::RefSet::iterator it, end;
      for ( it = ref_states.begin(), end = ref_states.end();
	    it != end; ++it ) {
	StateMachine* sm = (*it)->machine;
	if ( sm->has_name() )
	  printf( "\t\t[%s]\n", sm->toString().c_str() );
      }
     
      // Perhaps increasing the inline cache slots can solve the problem
      fsm->ic_miss_count++;
      if ( fsm->ic_miss_count >= 2 )
	printf( "\tWe observed twice this function is de-optimized by unknown type. Consider increasing IC slots.\n" );
      return true;
    }
    else {
      printf( "\tOops, no instance has the desired map: %p. It might be a bug of this program.\n", exp_map->id() );
      return false;
    }
  }

  int ans = 0;
  deque<Transition*> path;
  
  if ( dfs_backward_search_path(cur_state, exp_state, path, true ) > 0 ) {
    /*
     * Case 1:
     * failed_obj or its group owned exp_map in the past.
     *
     */
    
    // We first test if failed_obj itself owned exp_map in the past
    ans = backward_slice(i_obj, exp_map);
    if ( ans > 0 ) return true;
    
    // We second check if brother objects have this map ever
    ans = dfs_backward_search_path(cur_state, exp_state, path, false);
    if ( ans > 0 ) return true;
  }
  else if ( (ans = dfs_forward_search_path(cur_state, exp_state, path, false)) > 0 ) {
    /*
     * Case 2: Expected map is older than current map
     */
    return true;
  }
  else {
    /*
     * Case 3:
     * There is a branch point that leads to the desired map.
     */
    ans = dfs_backward_search_fork(exp_state, cur_state->machine->start, path, i_obj->history);
    if ( ans > 0 ) return true;
  }
    
  // Nothing found
  // We output all evolution paths for debugging
  //dfs_search_update_path(osm->start, cur_state, path, false);
  //printf( "\tSorry, I don't know, T__T\n" );
  return false;
}



