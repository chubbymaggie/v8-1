#include <cstdio>
#include "miner.hh"
#include "sm-builder.hh"

using namespace std;


static void
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
      printf( "%s", src->toString().c_str() );
      f_first = false;
    }
    
    reason.clear();
    trans->merge_reasons(reason);    
    printf( "-<%s>-%s", reason.c_str(), tgt->toString().c_str() );
  }
  
  printf("\n");
}


static void
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
      printf( "%s", src->toString().c_str() );
      f_first = false;
    }
    
    printf( "-<%s, %s>-%s",
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

  return 1;
}


static int 
dfs_search_update_path(State* cur_s, State* end_s, deque<Transition*>& path, bool reachability_test)
{
  if ( cur_s == end_s ) {
    // We found a path
    if ( !reachability_test ) {
      printf( "\t" );
      print_machine_path(path);
    }
    
    return 1;
  }
  
  int ans = 0;
  State::TransMap edges = cur_s->out_edges;
  for ( State::TransMap::iterator it = edges.begin(), end = edges.end();
	it != end; ++it ) {
    State* next_s = it->first;
    Transition* trans = it->second;
    path.push_back(trans);
    ans += dfs_search_update_path(next_s, end_s, path, reachability_test);
    path.pop_back();
    if ( ans > 0 && reachability_test == true ) break;
  }
  
  return ans;
}


static int
dfs_search_fork(State* cur_s, State* end_s, deque<Transition*>& path, Map* exp_map, vector<TransPacket*> &history)
{
  if ( cur_s == end_s ) return 0;

  // We first juedge if this node is branch point
  // If it is, we do not go back further
  int size = history.size();
  int i;
  for ( i = size - 1; i > -1; --i ) {
    TransPacket* tp = history[i];
    Map* past_map = tp->trans->source->get_map();
    if ( past_map == exp_map ) break;
  }

  if ( i > -1 ) {
    // We output suggestion to programmer
    printf( "\tFork evolution paths found:\n" );
    
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
    State* next_s = it->first;
    Transition* trans = it->second;
    path.push_back(trans);
    ans += dfs_search_fork(next_s, end_s, path, exp_map, history);
    path.pop_back();
  }
  
  return ans;
}



void
infer_deoptimization_reason(int failed_obj, Map* exp_map)
{
  InstanceDescriptor* i_obj = find_instance(failed_obj, StateMachine::MObject);
  if ( i_obj == NULL ) {
    printf( "Oops, never seen this instance: %p\n", failed_obj );
    return;
  }

  ObjectMachine* osm = (ObjectMachine*)i_obj->sm;
  State* inst_state = osm->get_instance_pos(i_obj->id, false); 
  State* exp_state = osm->search_state(exp_map, false);
  if ( exp_state == NULL ) {
    printf( "Oops, all instances from the same allocation site do not have this map: %p\n", exp_map->id() );
    return;
  }
  
  int ans = 0;
  deque<Transition*> path;

  
  if ( dfs_search_update_path(exp_state, inst_state, path, true ) > 0 ) {
    /*
   * Case 1:
   * failed_obj or its group owned exp_map in the past.
   *
   */

    // We first test this instance
    ans = backward_slice(i_obj, exp_map);
    if ( ans > 0 ) return;

    // We second check the group behavior of this allocation site
    ans = dfs_search_update_path(exp_state, inst_state, path, false);
    if ( ans > 0 ) return;
  }
  else {
    /*
     * Case 2:
     * The map evolution path of failed_obj is different to exp_map.
     * Perhaps this is a branch point.
     */
    ans = dfs_search_fork(exp_state, osm->start, path, exp_map, i_obj->history);
    if ( ans > 0 ) return;

    /*
     * This failed_obj never owned exp_map.
     * We check if other instance from the same allocation site owned exp_map.
     */
  }
  
  // Nothing found
  // We output all evolution paths for debugging
  dfs_search_update_path(osm->start, inst_state, path, false);
}



