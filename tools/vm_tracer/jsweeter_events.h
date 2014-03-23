// List of events and their handlers

#ifndef EVENTS_H
#define EVENTS_H


#define OBJECT_EVENTS_LIST(V)						\
  V(CreateObjBoilerplate,   create_obj_boilerplate,  "+ObjBp")		\
  V(CreateArrayBoilerplate, create_array_boilerplate,  "+AryBp")	\
  V(CreateObjectLiteral,    create_object_literal, "+ObjLit")		\
  V(CreateArrayLiteral,     create_array_literal,  "+AryLit")	\
  V(CreateNewObject,        create_new_object, "+Obj")		\
  V(CreateNewArray,         create_new_array, "+Array")		\
  V(CreateFunction,       create_function, "+Func")			\
  V(CopyObject,           copy_object, "CpyObj")			\
  V(ChangeFuncPrototype,      change_func_prototype, "!FProto")	\
  V(ChangeObjPrototype,      change_obj_prototype, "!OProto")	\
  V(SetMap,               set_map,  "!Map")				\
  V(MigrateToMap,         migrate_to_map, "Map->*")			\
  V(NewField,             new_field,  "+Field")			\
  V(DelField,             del_field,  "+Field")			\
  V(UpdateField,          update_field,  "!Field")			\
  V(ElemTransition,       elem_transition,  "ElmTy->*")			\
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
