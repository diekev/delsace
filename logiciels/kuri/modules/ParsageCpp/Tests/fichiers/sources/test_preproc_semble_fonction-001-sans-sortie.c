#define SEMBLE_FONCTION() 1

#if !SEMBLE_FONCTION()
#error "La fonction devait retourner 1"
#endif