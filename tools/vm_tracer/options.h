// Options

#ifndef OPTIONS_H
#define OPTIONS_H

#include <unistd.h>

static bool visualize = false;
static char* intput_file = NULL;

static int parse_options(int argc, char** argv)
{
  int c;

  while ( (c = getopt(argc, argv, "h") ) != -1 ) {
    switch(c) {
    case 'v':
      visualize = true;
      break;

    case 'h':
      printf( "Usage: %s [options] input_file\n", argv[0] );
      printf( "Options:\n");
      printf( "-v        : Output graphviz file for visualization\n" );
      printf( "-h        : Print this help.\n" );
      break;
    }
  }

  if ( optind == argc ) {
    printf( "Missing input file. Aborted.\n" );
    return 0;
  }

  input_file = argv[optind];
  return 1;
}


#endif

