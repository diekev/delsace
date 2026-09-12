/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020-2026 Kévin Dietrich. */

#include "alembic.h"
#include "alembic_ipa_c.h"

using namespace Alembic;

struct Abc_MetaData_Iterator;
struct ContexteKuri;

/* ------------------------------------------------------------------------- */
/** \nom MetaData
 * \{ */

struct Abc_MetaData {
    ContexteKuri *ctx_kuri = nullptr;
    Abc::MetaData metadata{};
    Abc_MetaData_Iterator *iterators = nullptr;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Array_Sample
 * \{ */

struct Array_Sample_Data {
    std::vector<std::string> strings{};
    std::vector<Abc_String> input_strings{};
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Array_Sample
 * \{ */

struct Abc_Camera_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::CameraSample sample{};
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Xform_Sample
 * \{ */

struct Abc_Xform_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::XformSample sample{};
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Utilitaires
 * \{ */

template <typename T>
void liste_ajoute(T **tête, T *élément)
{
    élément->next = *tête;
    *tête = élément;
}

template <typename T>
void kuri_deloge_liste(ContexteKuri *ctx_kuri, T *liste)
{
    while (liste != nullptr) {
        auto next = liste->next;
        kuri_deloge(ctx_kuri, liste);
        liste = next;
    }
}

inline void vers_abc_string(Abc_String *result, const std::string &name)
{
    if (result) {
        result->characters = name.c_str();
        result->size = name.size();
    }
}

inline std::string vers_std_string(struct Abc_String string)
{
    if (string.characters == nullptr) {
        return "";
    }
    return std::string(string.characters, string.size);
}

inline std::string vers_std_string_ou_défaut(struct Abc_String string, std::string_view défaut)
{
    if (string.characters == nullptr) {
        return std::string(défaut.data(), défaut.size());
    }
    return std::string(string.characters, string.size);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom value_converter
 * \{ */

template <typename Type_IPA>
struct value_converter {
    using Type_Abc = Type_IPA;

    static Type_Abc convert_value(Type_IPA *ptr)
    {
        return *ptr;
    }
};

template <>
struct value_converter<Abc_String> {
    using Type_Abc = std::string;

    static Type_Abc convert_value(Abc_String *ptr)
    {
        return *ptr;
    }
};

#define DECLARE_VALUE_CONVERTER(type_geom, type_abc, type_c, nom_court)                           \
    template <>                                                                                   \
    struct value_converter<type_c> {                                                              \
        using Type_Abc = type_abc;                                                                \
        static Type_Abc convert_value(type_c *ptr)                                                \
        {                                                                                         \
            return *reinterpret_cast<type_abc *>(ptr);                                            \
        }                                                                                         \
    };

ENUMERATE_ABC_ATTRIBUTE_SPECIAL_UNIQUE(DECLARE_VALUE_CONVERTER)

#undef DECLARE_VALUE_CONVERTER

template <typename Type_Abc>
struct import_value_converter {
    using Type_IPA = Type_Abc;

    static Type_IPA convert_value(Type_Abc *ptr)
    {
        return *ptr;
    }
};

template <>
struct import_value_converter<std::string> {
    using Type_IPA = Abc_String;

    static Type_IPA convert_value(std::string *ptr)
    {
        Abc_String résultat;
        vers_abc_string(&résultat, *ptr);
        return résultat;
    }
};

#define DECLARE_VALUE_CONVERTER(type_geom, type_abc, type_c, nom_court)                           \
    template <>                                                                                   \
    struct import_value_converter<type_abc> {                                                     \
        using Type_IPA = type_c;                                                                  \
        static Type_IPA convert_value(type_abc *ptr)                                              \
        {                                                                                         \
            return *reinterpret_cast<Type_IPA *>(ptr);                                            \
        }                                                                                         \
    };

ENUMERATE_ABC_ATTRIBUTE_SPECIAL_UNIQUE(DECLARE_VALUE_CONVERTER)

#undef DECLARE_VALUE_CONVERTER

/** \} */
