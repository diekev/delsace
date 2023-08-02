#ifndef INC_PROFILER_LOWLEVEL_H
#define INC_PROFILER_LOWLEVEL_H

#ifdef __cplusplus
  #define Prof_C               "C"
  #define Prof_extern_C        extern "C"
  #define Prof_dummy_declare
#else
  #define Prof_C
  #define Prof_extern_C
  #define Prof_dummy_declare  int Prof_dummy_dec =
#endif

#ifdef WIN32
  #include "prof_win32.h"
#else
 // #error "need to define Prof_get_timestamp() and Prof_Int64"
	typedef long long Prof_Int64;

	inline void Prof_get_timestamp(Prof_Int64 *result)
	{
		unsigned int lo, hi;
		__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
		*result = ((Prof_Int64)(hi) << 32) | (Prof_Int64)(lo);
	}
#endif


typedef struct
{
   const char * name;
   void * highlevel;
   char   initialized;
   char   visited;
   char   pad0,pad1;
} Prof_Zone;

typedef struct Prof_Zone_Stack
{
   Prof_Int64               t_self_start;

   Prof_Int64               total_self_ticks;
   Prof_Int64               total_hier_ticks;

   unsigned int             total_entry_count;

   struct Prof_Zone_Stack * parent;
   Prof_Zone              * zone;
   int                      recursion_depth;

   void                   * highlevel;
} Prof_Zone_Stack;


	// number of unique zones allowed in the entire application
	// @TODO: remove MAX_PROFILING_ZONES and make it dynamic
#define MAX_PROFILING_ZONES                512

	// report functions

#define NUM_VALUES 4
#define NUM_TITLE 2
#define NUM_HEADER (NUM_VALUES+1)

typedef struct {
	int indent;
	const char *name;
	int number;
	char prefix;
	int value_flag;
	double values[NUM_VALUES];
	double heat;

	// used internally
	void *zone;
} Prof_Report_Record;

typedef struct
{
	char *title[NUM_TITLE];
	char *header[NUM_HEADER];
	int num_record;
	int hilight;
	Prof_Report_Record *record;
} Prof_Report;

void         Prof_free_report(Prof_Report *z);
Prof_Report *Prof_create_report(void);

extern Prof_C Prof_Zone_Stack * Prof_stack; // current Zone stack
extern Prof_C Prof_Zone_Stack   Prof_dummy; // parent never matches

extern Prof_C Prof_Zone_Stack * Prof_StackAppend(Prof_Zone *zone);
// return the zone stack created by pushing 'zone' on the current

#ifdef Prof_ENABLED

static Prof_Int64 Prof_time;

#define Prof_Begin_Cache(z)                                     \
      /* declare a static cache of the zone stack */            \
   static Prof_Zone_Stack *Prof_cache = &Prof_dummy

#define Prof_Begin_Raw(z)                                       \
   Prof_Begin_Cache(z);                                         \
   Prof_Begin_Code(z)

#define Prof_Begin_Code(z)                                      \
   Prof_dummy_declare (                                         \
                                                                \
      /* check the cached Zone_Stack and update if needed */    \
    (Prof_cache->parent != Prof_stack                           \
        ? Prof_cache = Prof_StackAppend(&z)                     \
        : 0),                                                   \
                                                                \
    ++Prof_cache->total_entry_count,                            \
    Prof_get_timestamp(&Prof_time),                             \
                                                                \
      /* stop the timer on the parent zone stack */             \
    (Prof_stack->total_self_ticks +=                            \
       Prof_time - Prof_stack->t_self_start),                   \
                                                                \
      /* make cached stack current */                           \
    Prof_stack = Prof_cache,                                    \
                                                                \
      /* start the timer on this stack */                       \
    Prof_stack->t_self_start = Prof_time,                       \
    0)

#define Prof_End_Raw()                          \
                                                \
   (Prof_get_timestamp(&Prof_time),             \
                                                \
      /* stop timer for current zone stack */   \
    Prof_stack->total_self_ticks +=             \
       Prof_time - Prof_stack->t_self_start,    \
                                                \
      /* make parent chain current */           \
    Prof_stack = Prof_stack->parent,            \
                                                \
      /* start timer for parent zone stack */   \
    Prof_stack->t_self_start = Prof_time)


#define Prof_Declare(z)  Prof_Zone Prof_region_##z
#define Prof_Define(z)   Prof_Declare(z) = { #z }
#define Prof_Region(z)   Prof_Begin_Raw(Prof_region_##z);
#define Prof_End         Prof_End_Raw();

#define Prof_Begin(z)    static Prof_Define(z); Prof_Region(z)
#define Prof_Counter(z)  Prof_Begin(z) Prof_End

#ifdef __cplusplus

   #define Prof(x)        static Prof_Define(x); Prof_Scope(x)
 
   #define Prof_Scope(x)   \
      Prof_Begin_Cache(x); \
      Prof_Scope_Var Prof_scope_var(Prof_region_ ## x, Prof_cache)

   struct Prof_Scope_Var {
      inline Prof_Scope_Var(Prof_Zone &zone, Prof_Zone_Stack * &Prof_cache);
      inline ~Prof_Scope_Var();
   };

   inline Prof_Scope_Var::Prof_Scope_Var(Prof_Zone &zone, Prof_Zone_Stack * &Prof_cache) {
      Prof_Begin_Code(zone);
   }

   inline Prof_Scope_Var::~Prof_Scope_Var() {
      Prof_End_Raw();
   }

#endif



#else  // ifdef Prof_ENABLED

#ifdef __cplusplus
#define Prof(x)
#define Prof_Scope(x)
#endif

#define Prof_Define(name)
#define Prof_Begin(z)
#define Prof_End
#define Prof_Region(z)
#define Prof_Counter(z)

#endif

#endif // INC_PROFILER_LOWLEVEL_H
