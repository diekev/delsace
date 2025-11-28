#define FOR_LIST_OF_VARIABLES(DO) \
    DO(id1, name1) \
    DO(id2, name2) \
    DO(id3, name3)

#define DEFINE_NAME_VAR(id, name, ...) int name;
FOR_LIST_OF_VARIABLES( DEFINE_NAME_VAR )

void print_variables(void)
{
    #define PRINT_NAME_AND_VALUE(id, name, ...) printf(#name " = %d\n", name);
    FOR_LIST_OF_VARIABLES( PRINT_NAME_AND_VALUE )
}