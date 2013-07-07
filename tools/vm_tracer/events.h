// List of events and their handlers

#ifndef EVENTS_H
#define EVENTS_H


#define EVENTS_LIST(V)					\
  V(CreateObject,         create_object)		\
  V(CreateFunction,       create_function)		\
  V(ChangeType,           change_type)			\
  V(ExpandArray,          expand_array)			\
  V(MakeHole,             make_hole)			\
  V(ToSlowMode,           to_slow)			\
  V(ArrayOpsStoreChange,  array_ops_store_change)	\
  V(ArrayOpsPure,         array_ops_pure)		\
  V(GenUnoptCode,         gen_unopt_code)		\
  V(GenOptCode,           gen_opt_code)			\
  V(GenOsrCode,           gen_osr_code)			


typedef void (*EVENT_HANDLER)(FILE*);


#endif
