// List of events and their handlers

#ifndef EVENTS_H
#define EVENTS_H


#define OBJECT_EVENTS_LIST(V)						\
  V(CreateObjBoilerplate,   create_obj_boilerplate,  "NewObj")		\
  V(CreateArrayBoilerplate, create_array_boilerplate,  "NewArray")	\
  V(CreateObjectLiteral,    create_object_literal, "NewObjLit")		\
  V(CreateArrayLiteral,     create_array_literal,  "NewArrayLit")	\
  V(CreateNewObject,        create_new_object, "NewObj")		\
  V(CreateNewArray,         create_new_array, "NewArray")		\
  V(CreateFunction,       create_function, "NewFunc")			\
  V(CopyObject,           copy_object, "CopyObj")			\
  V(ChangeFuncPrototype,      change_func_prototype, "ChgFuncProto")	\
  V(ChangeObjPrototype,      change_obj_prototype, "ChgObjProto")	\
  V(SetMap,               set_map,  "SetMap")				\
  V(MigrateToMap,         migrate_to_map, "MigrateMap")			\
  V(NewField,             new_field,  "NewField")			\
  V(DelField,             del_field,  "DelField")			\
  V(UpdateField,          update_field,  "UpdateField")			\
  V(ElemTransition,       elem_transition,  "ElmTran")			\
  V(CowCopy,              cow_copy,  "CowCpy")				\
  V(ExpandArray,          expand_array,  "AryExp")			\
  V(ElemToSlowMode,       elem_to_slow,  "ElemSlow")			\
  V(PropertyToSlowMode,   prop_to_slow,  "PropSlow")			\
  V(ElemToFastMode,       elem_to_fast,  "ElemFast")			\
  V(PropertyToFastMode,   prop_to_fast, "PropFast")				


#define FUNCTION_EVENTS_LIST(V)						\
  V(GenFullCode,          gen_full_code,  "Full")			\
  V(GenOptCode,           gen_opt_code,  "Opt")				\
  V(GenOsrCode,           gen_osr_code,  "Osr")				\
  V(SetCode,              set_code,  "SetCode")				\
  V(DisableOpt,           disable_opt,  "DisableOpt")			\
  V(ReenableOpt,          reenable_opt,  "ReenableOpt")			\
  V(OptFailed,            opt_failed,  "OptFailed")			\
  V(RegularDeopt,         regular_deopt,  "Deopt")			\
  V(DeoptAsInline,        deopt_as_inline,  "DeoptInl")			\
  V(ForceDeopt,           force_deopt,  "ForceDeopt")


#define MAP_EVENTS_LIST(V)						\
  V(BeginDeoptOnMap,      begin_deopt_on_map,  "BeginForceDeopt")		


#define SYS_EVENTS_LIST(V)						\
  V(GCMoveObject,         gc_move_object,  "GCMoveObj")			\
  V(GCMoveCode,           gc_move_code,  "GCMoveCode")			\
  V(GCMoveShared,         gc_move_shared, "GCMoveShared")		\
  V(GCMoveMap,            gc_move_map,  "GCMoveMap")			\
  V(NotifyStackDeoptAll, notify_stack_deopt_all,  "StackDeoptAll")	\
  V(SetCheckpoint,        set_checkpoint, "SetCheckpoint")

#endif
