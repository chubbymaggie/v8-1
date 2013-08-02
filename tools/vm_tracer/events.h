// List of events and their handlers

#ifndef EVENTS_H
#define EVENTS_H


#define EVENTS_LIST(V)							\
  V(CreateObject,         create_object)				\
  V(CreateArray,          create_array)					\
  V(CreateFunction,       create_function)				\
  V(GCMoveFunction,       gc_move_function)				\
  V(GCMoveObject,         gc_move_object)				\
  V(GCMoveShared,         gc_move_shared)				\
  V(GCMoveCode,           gc_move_code)					\
  V(ChangeType,           change_type)					\
  V(ExpandArray,          expand_array)					\
  V(MakeHole,             make_hole)					\
  V(ToSlowMode,           to_slow_mode)					\
  V(ArrayOpsStoreChange,  array_ops_store_change)			\
  V(ArrayOpsPure,         array_ops_pure)				\
  V(GenFullCode,          gen_full_code)				\
  V(GenFullWithDeopt,     gen_full_deopt)				\
  V(GenOptCode,           gen_opt_code)					\
  V(GenOsrCode,           gen_osr_code)					\
  V(DisableOpt,           disable_opt)					\
  V(ReenableOpt,          reenable_opt)					\
  V(OptFailed,            gen_opt_failed)				\
  V(DeoptCode,            deopt_code)			


typedef void (*EventHandler)(FILE*);


#endif
