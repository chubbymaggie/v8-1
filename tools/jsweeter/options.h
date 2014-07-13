// Options

#ifndef OPTIONS_H
#define OPTIONS_H

extern const char* input_file;
extern const char* visual_file;
extern const char* slice_sig;

extern bool debug_mode;
extern int draw_mode;
extern int states_count_limit;
extern bool do_analyze;


enum DrawMode {
  DRAW_OBJECTS_ONLY,
  DRAW_CLOSURES_ONLY,
  DRAW_BOTH
};

#endif

