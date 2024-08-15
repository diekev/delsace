// utilisation d'une entête pour ajouter un niveau d'indirection car GCC ne supprime undef avec le
// pragam donc nous utilisons pragma GCC system_header
// https://stackoverflow.com/questions/38831058/wundef-is-not-being-ignored-with-pragma-in-g

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wdeprecated-copy"
#    pragma GCC diagnostic ignored "-Wdouble-promotion"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wextra"
#    pragma GCC diagnostic ignored "-Wfloat-conversion"
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#    pragma GCC diagnostic ignored "-Wshadow"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#    pragma GCC diagnostic ignored "-Wundef"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC system_header
#endif
#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/imageio.h>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
