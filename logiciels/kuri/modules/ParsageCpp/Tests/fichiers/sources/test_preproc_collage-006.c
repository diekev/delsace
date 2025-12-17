#define CHAINE_MULTILIGNE(...) #__VA_ARGS__

const char *nuanceur = CHAINE_MULTILIGNE(
    \x23version core\n
    void main()
    {
        foo(a, b, c);
    }
);