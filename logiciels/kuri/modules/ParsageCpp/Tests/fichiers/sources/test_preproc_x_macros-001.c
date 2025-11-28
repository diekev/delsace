#define LIST_OF_VARIABLES \
    X(value1) \
    X(value2) \
    X(value3)

#define X(name) int name;
LIST_OF_VARIABLES
#undef X

void print_variables(void)
{
#define X(name) printf("%s = %d\n", #name, name);
    LIST_OF_VARIABLES
#undef X
}