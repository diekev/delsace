/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "machine_virtuelle.hh"

#include "biblinternes/chrono/chronometrage.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/broyage.hh"
#include "compilation/compilatrice.hh"
#include "compilation/erreur.h"
#include "compilation/espace_de_travail.hh"
#include "compilation/ipa.hh"
#include "compilation/metaprogramme.hh"

#include "parsage/identifiant.hh"

#include "instructions.hh"

#undef DEBOGUE_INTERPRETEUSE
#undef CHRONOMETRE_INTERPRETATION
#undef DEBOGUE_VALEURS_ENTREE_SORTIE
#undef DEBOGUE_LOCALES

#define EST_FONCTION_COMPILATRICE(fonction)                                                       \
    ptr_fonction->donnees_externe.ptr_fonction ==                                                 \
        reinterpret_cast<Symbole::type_fonction>(fonction)

namespace oper {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

/* Ces structures nous servent à faire en sorte que le résultat des opérations
 * soient du bon type, pour éviter les problèmes liés à la promotion de nombre
 * entier :
 *
 * en C++, char + char = int, donc quand nous empilons le résultat, nous empilons
 * un int, alors qu'après, nous dépilerons un char...
 */
#define DEFINIS_OPERATEUR(__nom, __op, __type_operande, __type_resultat)                          \
    template <typename __type_operande>                                                           \
    struct __nom {                                                                                \
        using type_resultat = __type_resultat;                                                    \
        type_resultat operator()(__type_operande v1, __type_operande v2)                          \
        {                                                                                         \
            return v1 __op v2;                                                                    \
        }                                                                                         \
    };

DEFINIS_OPERATEUR(ajoute, +, T, T)
DEFINIS_OPERATEUR(soustrait, -, T, T)
DEFINIS_OPERATEUR(multiplie, *, T, T)
DEFINIS_OPERATEUR(divise, /, T, T)
DEFINIS_OPERATEUR(modulo, %, T, T)
DEFINIS_OPERATEUR(egal, ==, T, bool)
DEFINIS_OPERATEUR(different, !=, T, bool)
DEFINIS_OPERATEUR(inferieur, <, T, bool)
DEFINIS_OPERATEUR(inferieur_egal, <=, T, bool)
DEFINIS_OPERATEUR(superieur, >, T, bool)
DEFINIS_OPERATEUR(superieur_egal, >=, T, bool)
DEFINIS_OPERATEUR(et_binaire, &, T, T)
DEFINIS_OPERATEUR(ou_binaire, |, T, T)
DEFINIS_OPERATEUR(oux_binaire, ^, T, T)
DEFINIS_OPERATEUR(dec_gauche, <<, T, T)
DEFINIS_OPERATEUR(dec_droite, >>, T, T)

#pragma GCC diagnostic pop

}  // namespace oper

#define LIS_OCTET() (*frame->pointeur++)

#define LIS_4_OCTETS()                                                                            \
    *reinterpret_cast<int *>(frame->pointeur);                                                    \
    (frame->pointeur += 4)

#define LIS_8_OCTETS()                                                                            \
    *reinterpret_cast<long *>(frame->pointeur);                                                   \
    (frame->pointeur += 8)

#define LIS_POINTEUR(type)                                                                        \
    *reinterpret_cast<type **>(frame->pointeur);                                                  \
    (frame->pointeur += 8)

#define OP_UNAIRE_POUR_TYPE(op, type)                                                             \
    if (taille == static_cast<int>(sizeof(type))) {                                               \
        auto a = depile<type>(site);                                                              \
        empile(site, op a);                                                                       \
    }

#define OP_UNAIRE(op)                                                                             \
    auto taille = LIS_4_OCTETS();                                                                 \
    OP_UNAIRE_POUR_TYPE(op, char)                                                                 \
    else OP_UNAIRE_POUR_TYPE(op, short) else OP_UNAIRE_POUR_TYPE(                                 \
        op, int) else OP_UNAIRE_POUR_TYPE(op, long)

#define OP_UNAIRE_REEL(op)                                                                        \
    auto taille = LIS_4_OCTETS();                                                                 \
    OP_UNAIRE_POUR_TYPE(op, float)                                                                \
    else OP_UNAIRE_POUR_TYPE(op, double)

#define OP_BINAIRE_POUR_TYPE(op, type)                                                            \
    if (taille == static_cast<int>(sizeof(type))) {                                               \
        auto b = depile<type>(site);                                                              \
        auto a = depile<type>(site);                                                              \
        auto r = op<type>()(a, b);                                                                \
        empile(site, r);                                                                          \
    }

#define OP_BINAIRE(op)                                                                            \
    auto taille = LIS_4_OCTETS();                                                                 \
    OP_BINAIRE_POUR_TYPE(op, char)                                                                \
    else OP_BINAIRE_POUR_TYPE(op, short) else OP_BINAIRE_POUR_TYPE(                               \
        op, int) else OP_BINAIRE_POUR_TYPE(op, long)

#define OP_BINAIRE_NATUREL(op)                                                                    \
    auto taille = LIS_4_OCTETS();                                                                 \
    OP_BINAIRE_POUR_TYPE(op, unsigned char)                                                       \
    else OP_BINAIRE_POUR_TYPE(op, unsigned short) else OP_BINAIRE_POUR_TYPE(                      \
        op, unsigned int) else OP_BINAIRE_POUR_TYPE(op, unsigned long)

#define OP_BINAIRE_REEL(op)                                                                       \
    auto taille = LIS_4_OCTETS();                                                                 \
    OP_BINAIRE_POUR_TYPE(op, float)                                                               \
    else OP_BINAIRE_POUR_TYPE(op, double)

#define FAIS_TRANSTYPE(type_de, type_vers)                                                        \
    if (taille_vers == static_cast<int>(sizeof(type_vers))) {                                     \
        auto v = depile<type_de>(site);                                                           \
        empile(site, static_cast<type_vers>(v));                                                  \
    }

#define FAIS_TRANSTYPE_AUGMENTE(type1, type2, type3, type4)                                       \
    if (taille_de == 1) {                                                                         \
        FAIS_TRANSTYPE(type1, type2)                                                              \
        else FAIS_TRANSTYPE(type1, type3) else FAIS_TRANSTYPE(type1, type4)                       \
    }                                                                                             \
    else if (taille_de == 2) {                                                                    \
        FAIS_TRANSTYPE(type2, type3)                                                              \
        else FAIS_TRANSTYPE(type2, type4)                                                         \
    }                                                                                             \
    else if (taille_de == 4) {                                                                    \
        FAIS_TRANSTYPE(type3, type4)                                                              \
    }

#define FAIS_TRANSTYPE_DIMINUE(type1, type2, type3, type4)                                        \
    if (taille_de == 8) {                                                                         \
        FAIS_TRANSTYPE(type4, type3)                                                              \
        else FAIS_TRANSTYPE(type4, type2) else FAIS_TRANSTYPE(type4, type1)                       \
    }                                                                                             \
    else if (taille_de == 4) {                                                                    \
        FAIS_TRANSTYPE(type3, type2)                                                              \
        else FAIS_TRANSTYPE(type3, type1)                                                         \
    }                                                                                             \
    else if (taille_de == 2) {                                                                    \
        FAIS_TRANSTYPE(type2, type1)                                                              \
    }

/* ************************************************************************** */

#if defined(DEBOGUE_VALEURS_ENTREE_SORTIE) || defined(DEBOGUE_LOCALES) ||                         \
    defined(DEBOGUE_INTERPRETEUSE)
static auto imprime_tab(std::ostream &os, int n)
{
    for (auto i = 0; i < n - 1; ++i) {
        os << ' ';
    }
}
#endif

#if defined(DEBOGUE_VALEURS_ENTREE_SORTIE) || defined(DEBOGUE_LOCALES)
static void lis_valeur(octet_t *pointeur, Type *type, std::ostream &os)
{
    switch (type->genre) {
        default:
        {
            os << "valeur non prise en charge";
            break;
        }
        case GenreType::ENTIER_RELATIF:
        {
            if (type->taille_octet == 1) {
                os << *reinterpret_cast<char *>(pointeur);
            }
            else if (type->taille_octet == 2) {
                os << *reinterpret_cast<short *>(pointeur);
            }
            else if (type->taille_octet == 4) {
                os << *reinterpret_cast<int *>(pointeur);
            }
            else if (type->taille_octet == 8) {
                os << *reinterpret_cast<long *>(pointeur);
            }

            break;
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        case GenreType::ENTIER_NATUREL:
        {
            if (type->taille_octet == 1) {
                os << *pointeur;
            }
            else if (type->taille_octet == 2) {
                os << *reinterpret_cast<unsigned short *>(pointeur);
            }
            else if (type->taille_octet == 4) {
                os << *reinterpret_cast<unsigned int *>(pointeur);
            }
            else if (type->taille_octet == 8) {
                os << *reinterpret_cast<unsigned long *>(pointeur);
            }

            break;
        }
        case GenreType::BOOL:
        {
            os << (*reinterpret_cast<bool *>(pointeur) ? "vrai" : "faux");
            break;
        }
        case GenreType::REEL:
        {
            if (type->taille_octet == 4) {
                os << *reinterpret_cast<float *>(pointeur);
            }
            else if (type->taille_octet == 8) {
                os << *reinterpret_cast<double *>(pointeur);
            }

            break;
        }
        case GenreType::FONCTION:
        case GenreType::POINTEUR:
        {
            os << *reinterpret_cast<void **>(pointeur);
            break;
        }
        case GenreType::STRUCTURE:
        {
            auto type_structure = type->comme_structure();

            auto virgule = "{ ";

            POUR (type_structure->membres) {
                os << virgule;
                os << it.nom << " = ";

                auto pointeur_membre = pointeur + it.decalage;
                lis_valeur(pointeur_membre, it.type, os);

                virgule = ", ";
            }

            os << " }";

            break;
        }
        case GenreType::CHAINE:
        {
            auto valeur_pointeur = pointeur;
            auto valeur_chaine = *reinterpret_cast<long *>(pointeur + 8);

            kuri::chaine chaine;
            chaine.pointeur = *reinterpret_cast<char **>(valeur_pointeur);
            chaine.taille = valeur_chaine;

            os << '"' << chaine << '"';

            break;
        }
    }
}
#endif

#ifdef DEBOGUE_VALEURS_ENTREE_SORTIE
static auto imprime_valeurs_entrees(octet_t *pointeur_debut_entree,
                                    TypeFonction *type_fonction,
                                    kuri::chaine const &nom,
                                    int profondeur_appel)
{
    imprime_tab(std::cerr, profondeur_appel);

    std::cerr << "Appel de " << nom << '\n';

    auto index_sortie = 0;
    auto pointeur_lecture_retour = pointeur_debut_entree;
    POUR (type_fonction->types_entrees) {
        imprime_tab(std::cerr, profondeur_appel);
        std::cerr << "-- paramètre " << index_sortie << " : ";
        lis_valeur(pointeur_lecture_retour, it, std::cerr);
        std::cerr << '\n';

        pointeur_lecture_retour += it->taille_octet;
        index_sortie += 1;
    }
}

static auto imprime_valeurs_sorties(octet_t *pointeur_debut_retour,
                                    TypeFonction *type_fonction,
                                    kuri::chaine const &nom,
                                    int profondeur_appel)
{
    imprime_tab(std::cerr, profondeur_appel);

    std::cerr << "Retour de " << nom << '\n';

    auto index_entree = 0;
    auto pointeur_lecture_retour = pointeur_debut_retour;
    POUR (type_fonction->types_sorties) {
        if (it->genre == GenreType::RIEN) {
            continue;
        }

        imprime_tab(std::cerr, profondeur_appel);
        std::cerr << "-- résultat " << index_entree << " : ";
        lis_valeur(pointeur_lecture_retour, it, std::cerr);
        std::cerr << '\n';

        pointeur_lecture_retour += it->taille_octet;
        index_entree += 1;
    }
}
#endif

#ifdef DEBOGUE_LOCALES
static auto imprime_valeurs_locales(FrameAppel *frame, int profondeur_appel, std::ostream &os)
{
    imprime_tab(os, profondeur_appel);
    os << frame->fonction->nom << " :\n";

    POUR (frame->fonction->chunk.locales) {
        auto pointeur_locale = &frame->pointeur_pile[it.adresse];
        imprime_tab(std::cerr, profondeur_appel);
        os << "Locale (" << static_cast<void *>(pointeur_locale) << ") : ";

        if (it.ident) {
            os << it.ident->nom;
        }
        else {
            os << "temporaire";
        }

        os << " = ";
        lis_valeur(pointeur_locale, it.type->comme_pointeur()->type_pointe, std::cerr);
        os << '\n';
    }
}
#endif

/* ************************************************************************** */

static constexpr auto TAILLE_PILE = 1024 * 1024;

MachineVirtuelle::MachineVirtuelle(Compilatrice &compilatrice_) : compilatrice(compilatrice_)
{
}

MachineVirtuelle::~MachineVirtuelle()
{
    POUR (m_metaprogrammes) {
        deloge_donnees_execution(it->donnees_execution);
    }

    POUR (m_metaprogrammes_termines) {
        deloge_donnees_execution(it->donnees_execution);
    }
}

template <typename T>
void MachineVirtuelle::empile(NoeudExpression *site, T valeur)
{
    *reinterpret_cast<T *>(this->pointeur_pile) = valeur;
#ifndef NDEBUG
    if (pointeur_pile > (pile + TAILLE_PILE)) {
        m_metaprogramme->unite->espace->rapporte_erreur(
            site, "Erreur interne : surrentamponnage de la pile de données");
    }
#else
    static_cast<void>(site);
#endif
    this->pointeur_pile += static_cast<long>(sizeof(T));
    // std::cerr << "Empile " << sizeof(T) << " octet(s), décalage : " <<
    // static_cast<int>(pointeur_pile - pile) << '\n';
}

template <typename T>
T MachineVirtuelle::depile(NoeudExpression *site)
{
    this->pointeur_pile -= static_cast<long>(sizeof(T));
    // std::cerr << "Dépile " << sizeof(T) << " octet(s), décalage : " <<
    // static_cast<int>(pointeur_pile - pile) << '\n';
#ifndef NDEBUG
    if (pointeur_pile < pile) {
        m_metaprogramme->unite->espace
            ->rapporte_erreur(site, "Erreur interne : sousentamponnage de la pile de données")
            .ajoute_message("Le type du site est « ", chaine_type(site->type), " »");
    }
#else
    static_cast<void>(site);
#endif
    return *reinterpret_cast<T *>(this->pointeur_pile);
}

void MachineVirtuelle::depile(NoeudExpression *site, long n)
{
    pointeur_pile -= n;
    // std::cerr << "Dépile " << n << " octet(s), décalage : " << static_cast<int>(pointeur_pile -
    // pile) << '\n';
#ifndef NDEBUG
    if (pointeur_pile < pile) {
        m_metaprogramme->unite->espace->rapporte_erreur(
            site, "Erreur interne : sous-tamponnage de la pile de données");
    }
#else
    static_cast<void>(site);
#endif
}

bool MachineVirtuelle::appel(AtomeFonction *fonction, NoeudExpression *site)
{
    auto frame = &frames[profondeur_appel++];
    frame->fonction = fonction;
    frame->site = site;
    frame->pointeur = fonction->chunk.code;
    frame->pointeur_pile = pointeur_pile;
    return true;
}

bool MachineVirtuelle::appel_fonction_interne(AtomeFonction *ptr_fonction,
                                              int taille_argument,
                                              FrameAppel *&frame,
                                              NoeudExpression *site)
{
    // puisque les arguments utilisent des instructions d'allocations retire la taille des
    // arguments du pointeur de la pile pour ne pas que les allocations ne l'augmente
    pointeur_pile -= taille_argument;

#ifdef DEBOGUE_VALEURS_ENTREE_SORTIE
    imprime_valeurs_entrees(
        pointeur_pile, ptr_fonction->type->comme_fonction(), ptr_fonction->nom, profondeur_appel);
#endif

    if (!appel(ptr_fonction, site)) {
        return false;
    }

    frame = &frames[profondeur_appel - 1];
    return true;
}

void MachineVirtuelle::appel_fonction_externe(AtomeFonction *ptr_fonction,
                                              int taille_argument,
                                              InstructionAppel *inst_appel,
                                              NoeudExpression *site,
                                              ResultatInterpretation &resultat)
{
    /* Détermine ici si nous avons une fonction de l'IPA pour prendre en compte les appels via des
     * pointeurs de fonctions. */
    if (EST_FONCTION_COMPILATRICE(compilatrice_espace_courant)) {
        empile(site, m_metaprogramme->unite->espace);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_attend_message)) {
        auto message = compilatrice.attend_message();

        if (!message) {
            resultat = ResultatInterpretation::PASSE_AU_SUIVANT;
            return;
        }

        empile(site, message);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_commence_interception)) {
        auto espace_recu = depile<EspaceDeTravail *>(site);

        auto &messagere = compilatrice.messagere;
        messagere->commence_interception(espace_recu);

        espace_recu->metaprogramme = m_metaprogramme;
        static_cast<void>(espace_recu);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_termine_interception)) {
        auto espace_recu = depile<EspaceDeTravail *>(site);

        if (espace_recu->metaprogramme != m_metaprogramme) {
            /* L'espace du « site » est celui de métaprogramme, et non
             * l'espace reçu en paramètre. */
            rapporte_erreur(
                m_metaprogramme->unite->espace,
                site,
                "Le métaprogramme terminant l'interception n'est pas celui l'ayant commancé !");
        }

        espace_recu->metaprogramme = nullptr;

        auto &messagere = compilatrice.messagere;
        messagere->termine_interception(espace_recu);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_lexe_fichier)) {
        auto chemin_recu = depile<kuri::chaine_statique>(site);
        auto lexemes = compilatrice.lexe_fichier(chemin_recu, site);
        empile(site, lexemes);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_obtiens_options)) {
        auto options = compilatrice.options_compilation();
        empile(site, options);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_ajourne_options)) {
        auto options = depile<OptionsDeCompilation *>(site);
        compilatrice.ajourne_options_compilation(options);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(ajoute_chaine_a_la_compilation)) {
        auto chaine = depile<kuri::chaine_statique>(site);
        auto espace = depile<EspaceDeTravail *>(site);
        compilatrice.ajoute_chaine_compilation(espace, chaine);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(ajoute_fichier_a_la_compilation)) {
        auto chaine = depile<kuri::chaine_statique>(site);
        auto espace = depile<EspaceDeTravail *>(site);
        compilatrice.ajoute_fichier_compilation(espace, chaine);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(ajoute_chaine_au_module)) {
        auto chaine = depile<kuri::chaine_statique>(site);
        auto module = depile<Module *>(site);
        auto espace = depile<EspaceDeTravail *>(site);
        compilatrice.ajoute_chaine_au_module(espace, module, chaine);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(demarre_un_espace_de_travail)) {
        auto options = depile<OptionsDeCompilation *>(site);
        auto nom = depile<kuri::chaine_statique>(site);
        auto espace = compilatrice.demarre_un_espace_de_travail(*options, nom);
        empile(site, espace);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(espace_defaut_compilation)) {
        auto espace = compilatrice.espace_defaut_compilation();
        empile(site, espace);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_rapporte_erreur)) {
        auto message = depile<kuri::chaine_statique>(site);
        auto ligne = depile<int>(site);
        auto fichier = depile<kuri::chaine_statique>(site);
        auto espace = depile<EspaceDeTravail *>(site);
        espace->rapporte_erreur(fichier, ligne, message);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_rapporte_avertissement)) {
        auto message = depile<kuri::chaine_statique>(site);
        auto ligne = depile<int>(site);
        auto fichier = depile<kuri::chaine_statique>(site);
        auto espace = depile<EspaceDeTravail *>(site);
        espace->rapporte_avertissement(fichier, ligne, message);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_possede_erreur)) {
        auto espace = depile<EspaceDeTravail *>(site);
        empile(site, compilatrice.possede_erreur(espace));
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_module_courant)) {
        auto espace = depile<EspaceDeTravail *>(site);
        auto fichier = espace->fichier(site->lexeme->fichier);
        auto module = fichier->module;
        empile(site, module);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_message_recu)) {
        auto message = depile<Message *>(site);
        compilatrice.gestionnaire_code->message_recu(message);
        return;
    }

    auto type_fonction = ptr_fonction->decl->type->comme_fonction();
    auto &donnees_externe = ptr_fonction->donnees_externe;

    auto pointeur_arguments = pointeur_pile - taille_argument;

#ifdef DEBOGUE_VALEURS_ENTREE_SORTIE
    imprime_valeurs_entrees(
        pointeur_arguments, type_fonction, ptr_fonction->nom, profondeur_appel);
#endif

    auto pointeurs_arguments = dls::tablet<void *, 12>();
    auto decalage_argument = 0u;

    if (ptr_fonction->decl->est_variadique) {
        auto nombre_arguments_fixes = static_cast<unsigned>(type_fonction->types_entrees.taille() -
                                                            1);
        auto nombre_arguments_totaux = static_cast<unsigned>(inst_appel->args.taille());

        donnees_externe.types_entrees.efface();
        donnees_externe.types_entrees.reserve(nombre_arguments_totaux);

        POUR (inst_appel->args) {
            auto type = converti_type_ffi(it->type);
            donnees_externe.types_entrees.ajoute(type);

            auto ptr = &pointeur_arguments[decalage_argument];
            pointeurs_arguments.ajoute(ptr);

            if (it->type->genre == GenreType::ENTIER_CONSTANT) {
                decalage_argument += 4;
            }
            else {
                decalage_argument += it->type->taille_octet;
            }
        }

        donnees_externe.types_entrees.ajoute(nullptr);

        auto type_ffi_sortie = converti_type_ffi(type_fonction->type_sortie);
        auto ptr_types_entrees = donnees_externe.types_entrees.donnees();

        auto status = ffi_prep_cif_var(&donnees_externe.cif,
                                       FFI_DEFAULT_ABI,
                                       nombre_arguments_fixes,
                                       nombre_arguments_totaux,
                                       type_ffi_sortie,
                                       ptr_types_entrees);

        if (status != FFI_OK) {
            std::cerr << "Impossible de préparer la fonction variadique externe !\n";
            return;
        }
    }
    else {
        POUR (type_fonction->types_entrees) {
            auto ptr = &pointeur_arguments[decalage_argument];
            pointeurs_arguments.ajoute(ptr);
            decalage_argument += it->taille_octet;
        }
    }

    ffi_call(&donnees_externe.cif,
             donnees_externe.ptr_fonction,
             pointeur_pile,
             pointeurs_arguments.donnees());

    auto taille_type_retour = type_fonction->type_sortie->taille_octet;

    if (taille_type_retour != 0) {
        if (taille_type_retour <= static_cast<unsigned int>(taille_argument)) {
            memcpy(pointeur_arguments, pointeur_pile, taille_type_retour);
        }
        else {
            memcpy(pointeur_arguments, pointeur_pile, static_cast<unsigned int>(taille_argument));
            memcpy(pointeur_arguments + static_cast<unsigned int>(taille_argument),
                   pointeur_pile + static_cast<unsigned int>(taille_argument),
                   taille_type_retour - static_cast<unsigned int>(taille_argument));
        }
    }

    // écrase la liste d'arguments
    pointeur_pile = pointeur_arguments + taille_type_retour;
}

void MachineVirtuelle::empile_constante(NoeudExpression *site, FrameAppel *frame)
{
    auto drapeaux = LIS_OCTET();

#define EMPILE_CONSTANTE(type)                                                                    \
    type v = *(reinterpret_cast<type *>(frame->pointeur));                                        \
    empile(site, v);                                                                              \
    frame->pointeur += (drapeaux >> 3);                                                           \
    break;

    switch (drapeaux) {
        case CONSTANTE_ENTIER_RELATIF | BITS_8:
        {
            EMPILE_CONSTANTE(char)
        }
        case CONSTANTE_ENTIER_RELATIF | BITS_16:
        {
            EMPILE_CONSTANTE(short)
        }
        case CONSTANTE_ENTIER_RELATIF | BITS_32:
        {
            EMPILE_CONSTANTE(int)
        }
        case CONSTANTE_ENTIER_RELATIF | BITS_64:
        {
            EMPILE_CONSTANTE(long)
        }
        case CONSTANTE_ENTIER_NATUREL | BITS_8:
        {
            // erreur de compilation pour transtype inutile avec drapeaux stricts
            empile(site, LIS_OCTET());
            break;
        }
        case CONSTANTE_ENTIER_NATUREL | BITS_16:
        {
            EMPILE_CONSTANTE(unsigned short)
        }
        case CONSTANTE_ENTIER_NATUREL | BITS_32:
        {
            EMPILE_CONSTANTE(unsigned int)
        }
        case CONSTANTE_ENTIER_NATUREL | BITS_64:
        {
            EMPILE_CONSTANTE(unsigned long)
        }
        case CONSTANTE_NOMBRE_REEL | BITS_32:
        {
            EMPILE_CONSTANTE(float)
        }
        case CONSTANTE_NOMBRE_REEL | BITS_64:
        {
            EMPILE_CONSTANTE(double)
        }
    }

#undef EMPILE_CONSTANTE
}

void MachineVirtuelle::installe_metaprogramme(MetaProgramme *metaprogramme)
{
    auto de = metaprogramme->donnees_execution;
    profondeur_appel = de->profondeur_appel;
    pile = de->pile;
    pointeur_pile = de->pointeur_pile;
    frames = de->frames;
    ptr_donnees_constantes = metaprogramme->donnees_constantes.donnees();
    ptr_donnees_globales = metaprogramme->donnees_globales.donnees();
    donnees_constantes = &metaprogramme->unite->espace->donnees_constantes_executions;

    intervalle_adresses_globales.min = ptr_donnees_globales;
    intervalle_adresses_globales.max = ptr_donnees_globales +
                                       metaprogramme->donnees_globales.taille();

    intervalle_adresses_pile_execution.min = pile;
    intervalle_adresses_pile_execution.max = pile + TAILLE_PILE;

    assert(pile);
    assert(pointeur_pile);

    m_metaprogramme = metaprogramme;
}

void MachineVirtuelle::desinstalle_metaprogramme(MetaProgramme *metaprogramme)
{
    auto de = metaprogramme->donnees_execution;
    de->profondeur_appel = profondeur_appel;
    de->pointeur_pile = pointeur_pile;

    assert(intervalle_adresses_pile_execution.possede_inclusif(de->pointeur_pile));

    profondeur_appel = 0;
    pile = nullptr;
    pointeur_pile = nullptr;
    frames = nullptr;
    ptr_donnees_constantes = nullptr;
    ptr_donnees_globales = nullptr;
    donnees_constantes = nullptr;
    intervalle_adresses_globales = {};
    intervalle_adresses_pile_execution = {};

    m_metaprogramme = nullptr;
}

MachineVirtuelle::ResultatInterpretation MachineVirtuelle::execute_instructions()
{
    auto frame = &frames[profondeur_appel - 1];

    for (auto i = 0; i < 1000; ++i) {
#ifdef DEBOGUE_INTERPRETEUSE
        auto &sortie = std::cerr;
        imprime_tab(sortie, profondeur_appel);
        desassemble_instruction(
            frame->fonction->chunk, (frame->pointeur - frame->fonction->chunk.code), sortie);
#endif
        /* sauvegarde le pointeur si compilatrice_attend_message n'a pas encore de messages */
        auto pointeur_debut = frame->pointeur;
        auto instruction = LIS_OCTET();
        auto site_courant = LIS_POINTEUR(NoeudExpression);

        /* Fais en sorte d'avoir toujours un site valide. */
        static NoeudExpression *site = nullptr;
        if (site_courant) {
            site = site_courant;
        }

        /* Ce site est utilisé pour déterminer où se trouve le dernier site valide
         * au cas où le pointeur est désynchroniser et une instruction est invalide. */
        static NoeudExpression *dernier_site = nullptr;

        switch (instruction) {
            case OP_LABEL:
            {
                // saute le label
                frame->pointeur += 4;
                break;
            }
            case OP_BRANCHE:
            {
                /* frame->pointeur contient le décalage relatif à l'adresse du début de la
                 * fonction, leur addition nous donne donc le nouveau pointeur. */
                frame->pointeur = frame->fonction->chunk.code +
                                  *reinterpret_cast<int *>(frame->pointeur);
                break;
            }
            case OP_BRANCHE_CONDITION:
            {
                auto decalage_si_vrai = LIS_4_OCTETS();
                auto decalage_si_faux = LIS_4_OCTETS();
                auto condition = depile<bool>(site);

                if (condition) {
                    frame->pointeur = frame->fonction->chunk.code + decalage_si_vrai;
                }
                else {
                    frame->pointeur = frame->fonction->chunk.code + decalage_si_faux;
                }

                break;
            }
            case OP_CONSTANTE:
            {
                empile_constante(site, frame);
                break;
            }
            case OP_CHAINE_CONSTANTE:
            {
                auto pointeur_chaine = LIS_8_OCTETS();
                auto taille_chaine = LIS_8_OCTETS();
                empile(site, pointeur_chaine);
                empile(site, taille_chaine);
                break;
            }
            case OP_COMPLEMENT_ENTIER:
            {
                OP_UNAIRE(-)
                break;
            }
            case OP_COMPLEMENT_REEL:
            {
                OP_UNAIRE_REEL(-)
                break;
            }
            case OP_NON_BINAIRE:
            {
                OP_UNAIRE(~)
                break;
            }
            case OP_AJOUTE:
            {
                OP_BINAIRE(oper::ajoute)
                break;
            }
            case OP_SOUSTRAIT:
            {
                OP_BINAIRE(oper::soustrait)
                break;
            }
            case OP_MULTIPLIE:
            {
                OP_BINAIRE(oper::multiplie)
                break;
            }
            case OP_DIVISE:
            {
                OP_BINAIRE_NATUREL(oper::divise)
                break;
            }
            case OP_DIVISE_RELATIF:
            {
                OP_BINAIRE(oper::divise)
                break;
            }
            case OP_AJOUTE_REEL:
            {
                OP_BINAIRE_REEL(oper::ajoute)
                break;
            }
            case OP_SOUSTRAIT_REEL:
            {
                OP_BINAIRE_REEL(oper::soustrait)
                break;
            }
            case OP_MULTIPLIE_REEL:
            {
                OP_BINAIRE_REEL(oper::multiplie)
                break;
            }
            case OP_DIVISE_REEL:
            {
                OP_BINAIRE_REEL(oper::divise)
                break;
            }
            case OP_RESTE_NATUREL:
            {
                OP_BINAIRE_NATUREL(oper::modulo)
                break;
            }
            case OP_RESTE_RELATIF:
            {
                OP_BINAIRE(oper::modulo)
                break;
            }
            case OP_COMP_EGAL:
            {
                OP_BINAIRE(oper::egal)
                break;
            }
            case OP_COMP_INEGAL:
            {
                OP_BINAIRE(oper::different)
                break;
            }
            case OP_COMP_INF:
            {
                OP_BINAIRE(oper::inferieur)
                break;
            }
            case OP_COMP_INF_EGAL:
            {
                OP_BINAIRE(oper::inferieur_egal)
                break;
            }
            case OP_COMP_SUP:
            {
                OP_BINAIRE(oper::superieur)
                break;
            }
            case OP_COMP_SUP_EGAL:
            {
                OP_BINAIRE(oper::superieur_egal)
                break;
            }
            case OP_COMP_INF_NATUREL:
            {
                OP_BINAIRE_NATUREL(oper::inferieur)
                break;
            }
            case OP_COMP_INF_EGAL_NATUREL:
            {
                OP_BINAIRE_NATUREL(oper::inferieur_egal)
                break;
            }
            case OP_COMP_SUP_NATUREL:
            {
                OP_BINAIRE_NATUREL(oper::superieur)
                break;
            }
            case OP_COMP_SUP_EGAL_NATUREL:
            {
                OP_BINAIRE_NATUREL(oper::superieur_egal)
                break;
            }
            case OP_COMP_EGAL_REEL:
            {
                OP_BINAIRE_REEL(oper::egal)
                break;
            }
            case OP_COMP_INEGAL_REEL:
            {
                OP_BINAIRE_REEL(oper::different)
                break;
            }
            case OP_COMP_INF_REEL:
            {
                OP_BINAIRE_REEL(oper::inferieur)
                break;
            }
            case OP_COMP_INF_EGAL_REEL:
            {
                OP_BINAIRE_REEL(oper::inferieur_egal)
                break;
            }
            case OP_COMP_SUP_REEL:
            {
                OP_BINAIRE_REEL(oper::superieur)
                break;
            }
            case OP_COMP_SUP_EGAL_REEL:
            {
                OP_BINAIRE_REEL(oper::superieur_egal)
                break;
            }
            case OP_ET_BINAIRE:
            {
                OP_BINAIRE(oper::et_binaire)
                break;
            }
            case OP_OU_BINAIRE:
            {
                OP_BINAIRE(oper::ou_binaire)
                break;
            }
            case OP_OU_EXCLUSIF:
            {
                OP_BINAIRE(oper::oux_binaire)
                break;
            }
            case OP_DEC_GAUCHE:
            {
                OP_BINAIRE(oper::dec_gauche)
                break;
            }
            case OP_DEC_DROITE_ARITHM:
            {
                OP_BINAIRE(oper::dec_droite)
                break;
            }
            case OP_DEC_DROITE_LOGIQUE:
            {
                OP_BINAIRE_NATUREL(oper::dec_droite)
                break;
            }
            case OP_AUGMENTE_NATUREL:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                FAIS_TRANSTYPE_AUGMENTE(unsigned char, unsigned short, unsigned int, unsigned long)
                break;
            }
            case OP_DIMINUE_NATUREL:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                FAIS_TRANSTYPE_DIMINUE(unsigned char, unsigned short, unsigned int, unsigned long)
                break;
            }
            case OP_AUGMENTE_RELATIF:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                FAIS_TRANSTYPE_AUGMENTE(char, short, int, long)
                break;
            }
            case OP_DIMINUE_RELATIF:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                FAIS_TRANSTYPE_DIMINUE(char, short, int, long)
                break;
            }
            case OP_AUGMENTE_REEL:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();

                if (taille_de == 4 && taille_vers == 8) {
                    auto v = depile<float>(site);
                    empile(site, static_cast<double>(v));
                }

                break;
            }
            case OP_DIMINUE_REEL:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();

                if (taille_de == 8 && taille_vers == 4) {
                    auto v = depile<double>(site);
                    empile(site, static_cast<float>(v));
                }

                break;
            }
            case OP_ENTIER_VERS_REEL:
            {
                // @Incomplet : on perd l'information du signe dans le nombre entier
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();

#define TRANSTYPE_EVR(type_)                                                                      \
    if (taille_de == static_cast<int>(taille_de(type_))) {                                        \
        auto v = depile<type_>(site);                                                             \
        if (taille_vers == 2) {                                                                   \
            /* @Incomplet : r16 */                                                                \
        }                                                                                         \
        else if (taille_vers == 4) {                                                              \
            empile(site, static_cast<float>(v));                                                  \
        }                                                                                         \
        else if (taille_vers == 8) {                                                              \
            empile(site, static_cast<double>(v));                                                 \
        }                                                                                         \
    }

                TRANSTYPE_EVR(char)
                TRANSTYPE_EVR(short)
                TRANSTYPE_EVR(int)
                TRANSTYPE_EVR(long)

#undef TRANSTYPE_EVR
                break;
            }
            case OP_REEL_VERS_ENTIER:
            {
                // @Incomplet : on perd l'information du signe dans le nombre entier
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();

#define TRANSTYPE_RVE(type_)                                                                      \
    if (taille_de == static_cast<int>(taille_de(type_))) {                                        \
        auto v = depile<type_>(site);                                                             \
        if (taille_vers == 1) {                                                                   \
            empile(site, static_cast<char>(v));                                                   \
        }                                                                                         \
        else if (taille_vers == 2) {                                                              \
            empile(site, static_cast<short>(v));                                                  \
        }                                                                                         \
        else if (taille_vers == 4) {                                                              \
            empile(site, static_cast<int>(v));                                                    \
        }                                                                                         \
        else if (taille_vers == 8) {                                                              \
            empile(site, static_cast<long>(v));                                                   \
        }                                                                                         \
    }

                TRANSTYPE_RVE(float)
                TRANSTYPE_RVE(double)

#undef TRANSTYPE_RVE
                break;
            }
            case OP_RETOURNE:
            {
                auto type_fonction = frame->fonction->type->comme_fonction();
                auto taille_retour = static_cast<int>(type_fonction->type_sortie->taille_octet);
                auto pointeur_debut_retour = pointeur_pile - taille_retour;

#ifdef DEBOGUE_LOCALES
                imprime_valeurs_locales(frame, profondeur_appel, std::cerr);
#endif

#ifdef DEBOGUE_VALEURS_ENTREE_SORTIE
                imprime_valeurs_sorties(
                    pointeur_debut_retour, type_fonction, frame->fonction->nom, profondeur_appel);
#endif

                profondeur_appel--;

                if (profondeur_appel == 0) {
                    if (pointeur_pile != pile) {
                        pointeur_pile = pointeur_debut_retour;
                    }

                    return ResultatInterpretation::TERMINE;
                }

                pointeur_pile = frame->pointeur_pile;
                // std::cerr << "Retourne, décalage : " << static_cast<int>(pointeur_pile - pile)
                // << '\n';

                if (taille_retour != 0 && pointeur_pile != pointeur_debut_retour) {
                    memcpy(pointeur_pile,
                           pointeur_debut_retour,
                           static_cast<unsigned>(taille_retour));
                }

                pointeur_pile += taille_retour;
                // std::cerr << "Empile " << taille_retour << " octet(s), décalage : " <<
                // static_cast<int>(pointeur_pile - pile) << '\n';

                frame = &frames[profondeur_appel - 1];
                break;
            }
            case OP_APPEL:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);
                if (verifie_cible_appel(ptr_fonction, site) != ResultatInterpretation::OK) {
                    return ResultatInterpretation::ERREUR;
                }

                auto taille_argument = LIS_4_OCTETS();
                // saute l'instruction d'appel
                frame->pointeur += 8;

#ifdef DEBOGUE_INTERPRETEUSE
                std::cerr << "-- appel : " << ptr_fonction->nom << " ("
                          << chaine_type(ptr_fonction->type) << ')' << '\n';
#endif

                if (!appel_fonction_interne(ptr_fonction, taille_argument, frame, site)) {
                    return ResultatInterpretation::ERREUR;
                }

                break;
            }
            case OP_APPEL_EXTERNE:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);
                if (verifie_cible_appel(ptr_fonction, site) != ResultatInterpretation::OK) {
                    return ResultatInterpretation::ERREUR;
                }

                auto taille_argument = LIS_4_OCTETS();
                auto ptr_inst_appel = LIS_POINTEUR(InstructionAppel);

                auto resultat = ResultatInterpretation::OK;
                appel_fonction_externe(
                    ptr_fonction, taille_argument, ptr_inst_appel, site, resultat);

                if (resultat == ResultatInterpretation::PASSE_AU_SUIVANT) {
                    frame->pointeur = pointeur_debut;
                    return resultat;
                }

                break;
            }
            case OP_APPEL_POINTEUR:
            {
                auto taille_argument = LIS_4_OCTETS();
                auto valeur_inst = LIS_8_OCTETS();
                auto adresse = depile<void *>(site);
                auto ptr_fonction = reinterpret_cast<AtomeFonction *>(adresse);
                if (verifie_cible_appel(ptr_fonction, site) != ResultatInterpretation::OK) {
                    return ResultatInterpretation::ERREUR;
                }

                auto ptr_inst_appel = reinterpret_cast<InstructionAppel *>(valeur_inst);

                if (ptr_fonction->est_externe) {
                    auto resultat = ResultatInterpretation::OK;
                    appel_fonction_externe(
                        ptr_fonction, taille_argument, ptr_inst_appel, site, resultat);

                    if (resultat == ResultatInterpretation::PASSE_AU_SUIVANT) {
                        frame->pointeur = pointeur_debut;
                        return resultat;
                    }
                }
                else {
                    if (!appel_fonction_interne(ptr_fonction, taille_argument, frame, site)) {
                        return ResultatInterpretation::ERREUR;
                    }
                }

                break;
            }
            case OP_ASSIGNE:
            {
                auto taille = LIS_4_OCTETS();
                auto adresse_ou = depile<void *>(site);
                auto adresse_de = static_cast<void *>(this->pointeur_pile - taille);
                //				std::cerr << "----------------\n";
                //				std::cerr << "adresse_ou : " << adresse_ou << '\n';
                //				std::cerr << "adresse_de : " << adresse_de << '\n';
                //				std::cerr << "taille     : " << taille << '\n';

                if (std::abs(static_cast<char *>(adresse_de) - static_cast<char *>(adresse_ou)) <
                    taille) {
                    rapporte_erreur(m_metaprogramme->unite->espace,
                                    site,
                                    "Erreur interne : superposition de la copie dans la machine "
                                    "virtuelle lors d'une assignation !")
                        .ajoute_message("La taille à copier est de    : ", taille, ".\n")
                        .ajoute_message("L'adresse d'origine est      : ", adresse_de, ".\n")
                        .ajoute_message("L'adresse de destination est : ", adresse_ou, ".\n")
                        .ajoute_message(
                            "Le type du site  est         : ", chaine_type(site->type), "\n");
                }

                memcpy(adresse_ou, adresse_de, static_cast<size_t>(taille));
                depile(site, taille);
                break;
            }
            case OP_ALLOUE:
            {
                auto type = LIS_POINTEUR(Type);
                // saute l'identifiant
                frame->pointeur += 8;
                //				std::cerr << "----------------\n";
                //				std::cerr << "alloue : " << type->taille_octet << '\n';
                this->pointeur_pile += type->taille_octet;
                // std::cerr << "Empile " << type->taille_octet << " octet(s), décalage : " <<
                // static_cast<int>(pointeur_pile - pile) << '\n';

                if (type->taille_octet == 0) {
                    rapporte_erreur(
                        m_metaprogramme->unite->espace,
                        site,
                        "Erreur interne : allocation d'un type de taille 0 dans la MV !")
                        .ajoute_message("La type est : ", chaine_type(type), ".\n");
                }

                break;
            }
            case OP_CHARGE:
            {
                auto taille = LIS_4_OCTETS();
                auto adresse_de = depile<void *>(site);
                auto adresse_ou = static_cast<void *>(this->pointeur_pile);

                if (std::abs(static_cast<char *>(adresse_de) - static_cast<char *>(adresse_ou)) <
                    taille) {
                    rapporte_erreur(m_metaprogramme->unite->espace,
                                    site,
                                    "Erreur interne : superposition de la copie dans la machine "
                                    "virtuelle lors d'un chargement !")
                        .ajoute_message("La taille à copier est de    : ", taille, ".\n")
                        .ajoute_message("L'adresse d'origine est      : ", adresse_de, ".\n")
                        .ajoute_message("L'adresse de destination est : ", adresse_ou, ".\n")
                        .ajoute_message(
                            "Le type du site  est         : ", chaine_type(site->type), "\n");
                    return ResultatInterpretation::ERREUR;
                }

                if (adresse_de == nullptr) {
                    rapporte_erreur(
                        m_metaprogramme->unite->espace, site, "Copie depuis une adresse nulle !");
                    return ResultatInterpretation::ERREUR;
                }

                if (adresse_ou == nullptr) {
                    rapporte_erreur(
                        m_metaprogramme->unite->espace, site, "Copie vers une adresse nulle !");
                    return ResultatInterpretation::ERREUR;
                }

#if 0
                // À FAIRE : il nous faudrait les adresses des messages, des noeuds codes, etc.
                if (!adresse_est_assignable(adresse_de)) {
                    rapporte_erreur(m_metaprogramme->unite->espace,
                                    site,
                                    "Copie depuis une adresse non-chargeable !")
                        .ajoute_message("L'adresse est : ", adresse_de, "\n");
                    return ResultatInterpretation::ERREUR;
                }
#endif

                if (!adresse_est_assignable(adresse_ou)) {
                    rapporte_erreur(m_metaprogramme->unite->espace,
                                    site,
                                    "Copie vers une adresse non-assignable !")
                        .ajoute_message("L'adresse est : ", adresse_ou, "\n");
                    return ResultatInterpretation::ERREUR;
                }

                memcpy(adresse_ou, adresse_de, static_cast<size_t>(taille));
                this->pointeur_pile += taille;
                // std::cerr << "Empile " << taille << " octet(s), décalage : " <<
                // static_cast<int>(pointeur_pile - pile) << '\n';
                break;
            }
            case OP_REFERENCE_VARIABLE:
            {
                auto index = LIS_4_OCTETS();
                auto const &locale = frame->fonction->chunk.locales[index];
                empile(site, &frame->pointeur_pile[locale.adresse]);
                break;
            }
            case OP_REFERENCE_GLOBALE:
            {
                auto index = LIS_4_OCTETS();
                auto const &globale = donnees_constantes->globales[index];
                if (globale.adresse_pour_execution) {
                    empile(site, globale.adresse_pour_execution);
                }
                else {
                    empile(site, &ptr_donnees_globales[globale.adresse]);
                }
                break;
            }
            case OP_REFERENCE_MEMBRE:
            {
                auto decalage = LIS_4_OCTETS();
                auto adresse_de = depile<char *>(site);
                empile(site, adresse_de + decalage);
                // std::cerr << "adresse_de : " << static_cast<void *>(adresse_de) << '\n';
                break;
            }
            case OP_ACCEDE_INDEX:
            {
                auto taille_donnees = LIS_4_OCTETS();
                auto adresse = depile<char *>(site);
                auto index = depile<long>(site);
                auto nouvelle_adresse = adresse + index * taille_donnees;
                empile(site, nouvelle_adresse);
                //				std::cerr << "nouvelle_adresse : " << static_cast<void
                //*>(nouvelle_adresse) << '\n'; 				std::cerr << "index            : "
                //<< index
                //<<
                //'\n'; 				std::cerr << "taille_donnees   : " << taille_donnees <<
                //'\n';
                break;
            }
            default:
            {
                rapporte_erreur(m_metaprogramme->unite->espace,
                                dernier_site,
                                "Erreur interne : Opération inconnue dans la MV !");
                return ResultatInterpretation::ERREUR;
            }
        }

        dernier_site = site;
    }

    return ResultatInterpretation::OK;
}

void MachineVirtuelle::imprime_trace_appel(NoeudExpression *site)
{
    erreur::imprime_site(*m_metaprogramme->unite->espace, site);
    for (int i = profondeur_appel - 1; i >= 0; --i) {
        erreur::imprime_site(*m_metaprogramme->unite->espace, frames[i].site);
    }
}

bool MachineVirtuelle::adresse_est_assignable(void *adresse)
{
    return intervalle_adresses_globales.possede_inclusif(adresse) ||
           intervalle_adresses_pile_execution.possede_inclusif(adresse);
}

MachineVirtuelle::ResultatInterpretation MachineVirtuelle::verifie_cible_appel(
    AtomeFonction *ptr_fonction, NoeudExpression *site)
{
    if (!m_metaprogramme->cibles_appels.possede(ptr_fonction)) {
        auto espace = m_metaprogramme->unite->espace;
        espace
            ->rapporte_erreur(site,
                              "Alors que j'exécute un métaprogramme, je rencontre une instruction "
                              "d'appel vers une fonction ne faisant pas partie du métaprogramme.")
            .ajoute_message("Il est possible que l'adresse de la fonction soit invalide : ",
                            static_cast<void *>(ptr_fonction),
                            "\n");
        return ResultatInterpretation::ERREUR;
    }

    return ResultatInterpretation::OK;
}

void MachineVirtuelle::ajoute_metaprogramme(MetaProgramme *metaprogramme)
{
    /* Appel le métaprogramme pour initialiser sa frame d'appels, l'installation et la
     * désinstallation ajournement les données d'exécution. */
    installe_metaprogramme(metaprogramme);
    appel(static_cast<AtomeFonction *>(metaprogramme->fonction->atome), metaprogramme->directive);
    desinstalle_metaprogramme(metaprogramme);
    m_metaprogrammes.ajoute(metaprogramme);
}

void MachineVirtuelle::execute_metaprogrammes_courants()
{
    /* efface la liste de métaprogrammes dont l'exécution est finie */
    if (m_metaprogrammes_termines_lu) {
        m_metaprogrammes_termines.efface();
        m_metaprogrammes_termines_lu = false;
    }

    if (terminee()) {
        return;
    }

    auto nombre_metaprogrammes = m_metaprogrammes.taille();

    dls::chrono::compte_seconde chrono_exec;

    for (auto i = 0; i < nombre_metaprogrammes; ++i) {
        auto it = m_metaprogrammes[i];

#ifdef DEBOGUE_INTERPRETEUSE
        std::cerr << "== exécution " << it->fonction->nom_broye(it->unite->espace) << " ==\n";
#endif

        assert(it->donnees_execution->profondeur_appel >= 1);

        installe_metaprogramme(it);

        auto res = execute_instructions();

        if (res == ResultatInterpretation::PASSE_AU_SUIVANT) {
            // RÀF
        }
        else if (res == ResultatInterpretation::ERREUR) {
            it->resultat = MetaProgramme::ResultatExecution::ERREUR;
            m_metaprogrammes_termines.ajoute(it);
            std::swap(m_metaprogrammes[i], m_metaprogrammes[nombre_metaprogrammes - 1]);
            nombre_metaprogrammes -= 1;
            i -= 1;
        }
        else if (res == ResultatInterpretation::TERMINE) {
            it->resultat = MetaProgramme::ResultatExecution::SUCCES;
            m_metaprogrammes_termines.ajoute(it);
            std::swap(m_metaprogrammes[i], m_metaprogrammes[nombre_metaprogrammes - 1]);
            nombre_metaprogrammes -= 1;
            i -= 1;
        }

        desinstalle_metaprogramme(it);

        if (stop || compilatrice.possede_erreur()) {
            break;
        }
    }

    m_metaprogrammes.redimensionne(nombre_metaprogrammes);

    temps_execution_metaprogammes += chrono_exec.temps();

    nombre_de_metaprogrammes_executes += m_metaprogrammes_termines.taille();
}

DonneesExecution *MachineVirtuelle::loge_donnees_execution()
{
    auto donnees = donnees_execution.ajoute_element();
    donnees->pile = memoire::loge_tableau<octet_t>("MachineVirtuelle::pile", TAILLE_PILE);
    donnees->pointeur_pile = donnees->pile;
    return donnees;
}

void MachineVirtuelle::deloge_donnees_execution(DonneesExecution *&donnees)
{
    if (!donnees) {
        return;
    }

    // À FAIRE : récupère la mémoire
    memoire::deloge_tableau("MachineVirtuelle::pile", donnees->pile, TAILLE_PILE);
    donnees = nullptr;
}

void MachineVirtuelle::rassemble_statistiques(Statistiques &stats)
{
    stats.memoire_mv += donnees_execution.memoire_utilisee();
    stats.nombre_metaprogrammes_executes += nombre_de_metaprogrammes_executes;
    stats.temps_metaprogrammes += temps_execution_metaprogammes;
}

std::ostream &operator<<(std::ostream &os, PatchDonneesConstantes const &patch)
{
    os << "Patch données constantes :\n";

    os << "-- où           : ";
    if (patch.ou == DONNEES_CONSTANTES) {
        os << "données constantes\n";
    }
    else {
        os << "données globales\n";
    }

    os << "-- quoi         : ";
    if (patch.quoi == ADRESSE_CONSTANTE) {
        os << "adresse constante\n";
    }
    else {
        os << "adresse globale\n";
    }

    os << "-- adresse quoi : " << patch.decalage_quoi << '\n';
    os << "-- adresse où   : " << patch.decalage_ou << '\n';

    return os;
}
