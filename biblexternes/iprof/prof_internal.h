#ifndef Prof_INC_PROF_INTERNAL_H
#define Prof_INC_PROF_INTERNAL_H

#include "prof.h"

// number of unique zones allowed in the entire application
// @TODO: remove MAX_PROFILING_ZONES and make it dynamic
#define MAX_PROFILING_ZONES                512

// report functions

#define NUM_VALUES 4
#define NUM_TITLE 2
#define NUM_HEADER (NUM_VALUES+1)


// really internal functions

extern void Prof_graph(int num_frames, 
                       void (*callback)(int id, int x0, int x1, float *values, void *data),
                       void *data);

extern void Prof_init_highlevel();

extern double Prof_get_time(void);

extern int        Prof_num_zones;
extern Prof_Zone *Prof_zones[];

extern Prof_Declare(_global);



#endif
