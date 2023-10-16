/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "machine_virtuelle.hh"

#include <fstream>
#include <iostream>

#include "biblinternes/chrono/chronometrage.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/broyage.hh"
#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"
#include "compilation/ipa.hh"
#include "compilation/metaprogramme.hh"

#include "parsage/identifiant.hh"

#include "structures/ensemble.hh"
#include "structures/table_hachage.hh"

#include "instructions.hh"

#undef DEBOGUE_INTERPRETEUSE
#undef CHRONOMETRE_INTERPRETATION
#undef DEBOGUE_VALEURS_ENTREE_SORTIE
#undef DEBOGUE_LOCALES

/* ------------------------------------------------------------------------- */
/** \name Fuites de mémoire.
 * \{ */

static void rapporte_avertissement_pour_fuite_de_mémoire(MetaProgramme *métaprogramme,
                                                         size_t taille_bloc,
                                                         kuri::tableau<FrameAppel> const &frames)
{
    auto espace = métaprogramme->unite->espace;
    Enchaineuse enchaineuse;

    enchaineuse << "Fuite de mémoire dans l'exécution du métaprogramme : " << taille_bloc
                << " octets non libérés.\n\n";

    for (int f = int(frames.taille()) - 1; f >= 0; f--) {
        erreur::imprime_site(enchaineuse, *espace, frames[f].site);
    }

    espace->rapporte_avertissement(métaprogramme->directive, enchaineuse.chaine());
}

void DétectriceFuiteDeMémoire::ajoute_bloc(void *ptr,
                                           size_t taille,
                                           const kuri::tableau<FrameAppel> &frame)
{
    auto info_bloc = InformationsBloc{taille, frame};

#ifdef UTILISE_NOTRE_TABLE
    table_allocations.insere(ptr, info_bloc);
#else
    table_allocations.insert({ptr, info_bloc});
#endif
}

bool DétectriceFuiteDeMémoire::supprime_bloc(void *ptr)
{
    /* Permet de passer des pointeurs nuls (puisque free et delete le permettent). */
    if (!ptr) {
        return true;
    }

#ifdef UTILISE_NOTRE_TABLE
    if (table_allocations.possede(ptr)) {
        table_allocations.efface(ptr);
        return true;
    }
#else
    if (table_allocations.find(ptr) != table_allocations.end()) {
        table_allocations.erase(ptr);
        return true;
    }
#endif

    // À FAIRE : erreur
    return false;
}

void imprime_fuites_de_mémoire(MetaProgramme *métaprogramme)
{
    auto données = métaprogramme->donnees_execution;

#ifdef UTILISE_NOTRE_TABLE
    données->détectrice_fuite_de_mémoire.table_allocations.pour_chaque_élément(
        [&](DétectriceFuiteDeMémoire::InformationsBloc const &info) {
            rapporte_avertissement_pour_fuite_de_mémoire(métaprogramme, info.taille, info.frame);
        });
#else
    POUR (données->détectrice_fuite_de_mémoire.table_allocations) {
        rapporte_avertissement_pour_fuite_de_mémoire(
            métaprogramme, it.second.taille, it.second.frame);
    }
#endif
}

/** \} */

#define EST_FONCTION_COMPILATRICE(fonction)                                                       \
    ptr_fonction->données_exécution->donnees_externe.ptr_fonction ==                              \
        reinterpret_cast<Symbole::type_fonction>(fonction)

inline bool adresse_est_nulle(const void *adresse)
{
    /* 0xbebebebebebebebe peut être utilisé par les débogueurs.
     * Vérification si adresse est dans la première page pour également détecter les
     * déréférencement d'adresses nulles. */
    return adresse == nullptr || adresse == reinterpret_cast<void *>(0xbebebebebebebebe) ||
           reinterpret_cast<uint64_t>(adresse) < 4096;
}

static std::ostream &operator<<(std::ostream &os, MachineVirtuelle::ResultatInterpretation res)
{
#define CASE(NOM)                                                                                 \
    case MachineVirtuelle::ResultatInterpretation::NOM:                                           \
        os << #NOM;                                                                               \
        break
    switch (res) {
        CASE(OK);
        CASE(ERREUR);
        CASE(COMPILATION_ARRETEE);
        CASE(TERMINE);
        CASE(PASSE_AU_SUIVANT);
    }
#undef CASE
    return os;
}

namespace oper {

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#endif

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

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

}  // namespace oper

#define LIS_OCTET() (*frame->pointeur++)

#define LIS_4_OCTETS()                                                                            \
    *reinterpret_cast<int *>(frame->pointeur);                                                    \
    (frame->pointeur += 4)

#define LIS_8_OCTETS()                                                                            \
    *reinterpret_cast<int64_t *>(frame->pointeur);                                                \
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
        op, int) else OP_UNAIRE_POUR_TYPE(op, int64_t)

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
        op, int) else OP_BINAIRE_POUR_TYPE(op, int64_t)

#define OP_BINAIRE_NATUREL(op)                                                                    \
    auto taille = LIS_4_OCTETS();                                                                 \
    OP_BINAIRE_POUR_TYPE(op, unsigned char)                                                       \
    else OP_BINAIRE_POUR_TYPE(op, unsigned short) else OP_BINAIRE_POUR_TYPE(                      \
        op, uint32_t) else OP_BINAIRE_POUR_TYPE(op, uint64_t)

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
                os << *reinterpret_cast<int64_t *>(pointeur);
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
                os << *reinterpret_cast<uint32_t *>(pointeur);
            }
            else if (type->taille_octet == 8) {
                os << *reinterpret_cast<uint64_t *>(pointeur);
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
            auto type_structure = type->comme_type_structure();

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
            auto valeur_chaine = *reinterpret_cast<int64_t *>(pointeur + 8);

            auto chaine = kuri::chaine_statique(*reinterpret_cast<char **>(valeur_pointeur),
                                                valeur_chaine);
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
        std::cerr << "-- paramètre " << index_sortie << " (" << chaine_type(it) << ") : ";
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
        if (it->est_type_rien()) {
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
        lis_valeur(pointeur_locale, it.type->comme_type_pointeur()->type_pointe, std::cerr);
        os << '\n';
    }
}
#endif

/* ************************************************************************** */

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
inline T depile(NoeudExpression *site, octet_t *&pointeur_pile)
{
    pointeur_pile -= static_cast<int64_t>(sizeof(T));
    return *reinterpret_cast<T *>(pointeur_pile);
}

void MachineVirtuelle::depile(NoeudExpression *site, int64_t n)
{
    pointeur_pile -= n;
    // std::cerr << "Dépile " << n << " octet(s), décalage : " << static_cast<int>(pointeur_pile -
    // pile) << '\n';
#ifndef NDEBUG
    if (pointeur_pile < pile) {
        rapporte_erreur_execution(site, "Erreur interne : sous-tamponnage de la pile de données");
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
    frame->pointeur = fonction->données_exécution->chunk.code;
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
    imprime_valeurs_entrees(pointeur_pile,
                            ptr_fonction->type->comme_type_fonction(),
                            ptr_fonction->nom,
                            profondeur_appel);
#endif

    if (!appel(ptr_fonction, site)) {
        return false;
    }

    frame = &frames[profondeur_appel - 1];
    return true;
}

#define RAPPORTE_ERREUR_SI_NUL(pointeur, message)                                                 \
    if (!pointeur) {                                                                              \
        rapporte_erreur_execution(site, message);                                                 \
    }

void MachineVirtuelle::appel_fonction_compilatrice(AtomeFonction *ptr_fonction,
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
        RAPPORTE_ERREUR_SI_NUL(espace_recu, "Reçu un espace de travail nul");

        auto &messagere = compilatrice.messagere;
        messagere->commence_interception(espace_recu);

        espace_recu->metaprogramme = m_metaprogramme;
        static_cast<void>(espace_recu);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_termine_interception)) {
        auto espace_recu = depile<EspaceDeTravail *>(site);
        RAPPORTE_ERREUR_SI_NUL(espace_recu, "Reçu un espace de travail nul");

        if (espace_recu->metaprogramme != m_metaprogramme) {
            /* L'espace du « site » est celui de métaprogramme, et non
             * l'espace reçu en paramètre. */
            m_metaprogramme->unite->espace->rapporte_erreur(
                site,
                "Le métaprogramme terminant l'interception n'est pas celui l'ayant commancé !");
        }

        espace_recu->metaprogramme = nullptr;

        /* Ne passons pas par la messagère car il est possible que le GestionnaireCode soit
         * vérrouiller par quelqu'un, et par le passé la Messagère prévenait le GestionnaireCode,
         * causant un verrou mort. */
        auto &gestionnaire = compilatrice.gestionnaire_code;
        gestionnaire->interception_message_terminee(espace_recu);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_lexe_fichier)) {
        auto chemin_recu = depile<kuri::chaine_statique>(site);
        auto espace = m_metaprogramme->unite->espace;
        auto lexemes = compilatrice.lexe_fichier(espace, chemin_recu, site);
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
        RAPPORTE_ERREUR_SI_NUL(options, "Reçu des options de compilation nulles");
        compilatrice.ajourne_options_compilation(options);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(ajoute_chaine_a_la_compilation)) {
        auto chaine = depile<kuri::chaine_statique>(site);
        auto espace = depile<EspaceDeTravail *>(site);
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        compilatrice.ajoute_chaine_compilation(espace, site, chaine);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(ajoute_fichier_a_la_compilation)) {
        auto chaine = depile<kuri::chaine_statique>(site);
        auto espace = depile<EspaceDeTravail *>(site);
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        compilatrice.ajoute_fichier_compilation(espace, chaine, site);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(ajoute_chaine_au_module)) {
        auto chaine = depile<kuri::chaine_statique>(site);
        auto module = depile<Module *>(site);
        auto espace = depile<EspaceDeTravail *>(site);
        RAPPORTE_ERREUR_SI_NUL(module, "Reçu un module nul");
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        compilatrice.ajoute_chaine_au_module(espace, site, module, chaine);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(demarre_un_espace_de_travail)) {
        auto options = depile<OptionsDeCompilation *>(site);
        auto nom = depile<kuri::chaine_statique>(site);
        RAPPORTE_ERREUR_SI_NUL(options, "Reçu des options nulles");
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
        /* Dans les noeuds codes, les lignes commencent à 1. */
        auto ligne = depile<int>(site) - 1;
        auto fichier = depile<kuri::chaine_statique>(site);
        auto espace = depile<EspaceDeTravail *>(site);
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        espace->rapporte_erreur(fichier, ligne, message);
        m_metaprogramme->a_rapporté_une_erreur = true;
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_rapporte_avertissement)) {
        auto message = depile<kuri::chaine_statique>(site);
        /* Dans les noeuds codes, les lignes commencent à 1. */
        auto ligne = depile<int>(site) - 1;
        auto fichier = depile<kuri::chaine_statique>(site);
        auto espace = depile<EspaceDeTravail *>(site);
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        espace->rapporte_avertissement(fichier, ligne, message);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_possede_erreur)) {
        auto espace = depile<EspaceDeTravail *>(site);
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        empile(site, compilatrice.possede_erreur(espace));
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_module_courant)) {
        auto fichier = compilatrice.fichier(site->lexeme->fichier);
        auto module = fichier->module;
        empile(site, module);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_message_recu)) {
        auto message = depile<Message *>(site);
        RAPPORTE_ERREUR_SI_NUL(message, "Reçu un message nul");
        compilatrice.gestionnaire_code->message_recu(message);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_fonctions_parsees)) {
        auto espace = m_metaprogramme->unite->espace;
        auto fonctions = compilatrice.fonctions_parsees(espace);
        empile(site, fonctions);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_module_pour_code)) {
        auto code = depile<NoeudCode *>(site);
        RAPPORTE_ERREUR_SI_NUL(code, "Reçu un noeud code nul");
        const auto fichier = compilatrice.fichier(code->chemin_fichier);
        RAPPORTE_ERREUR_SI_NUL(fichier, "Aucun fichier correspond au noeud code");
        const auto module = fichier->module;
        empile(site, module);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_module_pour_type)) {
        auto info_type = depile<InfoType *>(site);
        RAPPORTE_ERREUR_SI_NUL(info_type, "Reçu un InfoType nul");
        const auto decl = compilatrice.typeuse.decl_pour_info_type(info_type);
        if (!decl) {
            empile(site, nullptr);
            return;
        }
        const auto fichier = compilatrice.fichier(decl->lexeme->fichier);
        if (!fichier) {
            empile(site, nullptr);
            return;
        }
        const auto module = fichier->module;
        empile(site, module);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_nom_module)) {
        auto module = depile<Module *>(site);
        RAPPORTE_ERREUR_SI_NUL(module, "Reçu un Module nul");
        empile(site, module->nom_->nom);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_chemin_module)) {
        auto module = depile<Module *>(site);
        RAPPORTE_ERREUR_SI_NUL(module, "Reçu un Module nul");
        empile(site, module->chemin());
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_module_racine_compilation)) {
        auto module = compilatrice.module_racine_compilation;
        empile(site, module);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_racine_installation_kuri)) {
        kuri::chaine_statique racine_kuri = compilatrice.racine_kuri;
        empile(site, racine_kuri);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_donne_arguments_ligne_de_commande)) {
        kuri::tableau_statique<kuri::chaine_statique> arguments =
            compilatrice.arguments.arguments_pour_métaprogrammes;
        empile(site, arguments);
        return;
    }
}

void MachineVirtuelle::appel_fonction_externe(AtomeFonction *ptr_fonction,
                                              int taille_argument,
                                              InstructionAppel *inst_appel,
                                              NoeudExpression *site)
{
    if (EST_FONCTION_COMPILATRICE(notre_malloc)) {
        auto taille = depile<size_t>(site);
        auto résultat = notre_malloc(taille);
        empile(site, résultat);

        auto données = m_metaprogramme->donnees_execution;
        données->détectrice_fuite_de_mémoire.ajoute_bloc(
            résultat, taille, donne_tableau_frame_appel());
        return;
    }

    if (EST_FONCTION_COMPILATRICE(notre_realloc)) {
        auto taille = depile<size_t>(site);
        auto ptr = depile<void *>(site);

        auto données = m_metaprogramme->donnees_execution;
        données->détectrice_fuite_de_mémoire.supprime_bloc(ptr);

        auto résultat = notre_realloc(ptr, taille);
        empile(site, résultat);

        données->détectrice_fuite_de_mémoire.ajoute_bloc(
            résultat, taille, donne_tableau_frame_appel());
        return;
    }

    if (EST_FONCTION_COMPILATRICE(notre_free)) {
        auto ptr = depile<void *>(site);

        auto données = m_metaprogramme->donnees_execution;
        données->détectrice_fuite_de_mémoire.supprime_bloc(ptr);

        notre_free(ptr);
        return;
    }

    auto type_fonction = ptr_fonction->decl->type->comme_type_fonction();
    auto &donnees_externe = ptr_fonction->données_exécution->donnees_externe;

    auto pointeur_arguments = pointeur_pile - taille_argument;

#ifdef DEBOGUE_VALEURS_ENTREE_SORTIE
    imprime_valeurs_entrees(
        pointeur_arguments, type_fonction, ptr_fonction->nom, profondeur_appel);
#endif

    auto pointeurs_arguments = kuri::tablet<void *, 12>();
    auto decalage_argument = 0u;

    if (ptr_fonction->decl->possede_drapeau(DrapeauxNoeudFonction::EST_VARIADIQUE)) {
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

            if (it->type->est_type_entier_constant()) {
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
        if (taille_type_retour <= static_cast<uint32_t>(taille_argument)) {
            memcpy(pointeur_arguments, pointeur_pile, taille_type_retour);
        }
        else {
            memcpy(pointeur_arguments, pointeur_pile, static_cast<uint32_t>(taille_argument));
            memcpy(pointeur_arguments + static_cast<uint32_t>(taille_argument),
                   pointeur_pile + static_cast<uint32_t>(taille_argument),
                   taille_type_retour - static_cast<uint32_t>(taille_argument));
        }
    }

    // écrase la liste d'arguments
    pointeur_pile = pointeur_arguments + taille_type_retour;
}

inline void MachineVirtuelle::empile_constante(NoeudExpression *site, FrameAppel *frame)
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
            EMPILE_CONSTANTE(int64_t)
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
            EMPILE_CONSTANTE(uint32_t)
        }
        case CONSTANTE_ENTIER_NATUREL | BITS_64:
        {
            EMPILE_CONSTANTE(uint64_t)
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
    ptr_donnees_constantes = metaprogramme->données_constantes.donnees();
    ptr_donnees_globales = metaprogramme->données_globales.donnees();
    donnees_constantes = &compilatrice.données_constantes_exécutions;

    intervalle_adresses_globales.min = ptr_donnees_globales;
    intervalle_adresses_globales.max = ptr_donnees_globales +
                                       metaprogramme->données_globales.taille();

    intervalle_adresses_pile_execution.min = pile;
    intervalle_adresses_pile_execution.max = pile + TAILLE_PILE;

    assert(pile);
    assert(pointeur_pile);

    m_metaprogramme = metaprogramme;
}

void MachineVirtuelle::desinstalle_metaprogramme(MetaProgramme *metaprogramme,
                                                 int compte_executees)
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

    de->instructions_executees += compte_executees;
    if (compilatrice.arguments.profile_metaprogrammes) {
        profileuse.ajoute_echantillon(metaprogramme, compte_executees);
    }
}

#define INSTRUCTIONS_PAR_BATCH 1000

MachineVirtuelle::ResultatInterpretation MachineVirtuelle::execute_instructions(
    int &compte_executees)
{
    auto frame = &frames[profondeur_appel - 1];

    for (auto i = 0; i < INSTRUCTIONS_PAR_BATCH; ++i) {
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

        NoeudExpression *site = site_courant;
        if (site_courant) {
            m_metaprogramme->donnees_execution->site = site_courant;
        }
        else {
            site = m_metaprogramme->donnees_execution->site;
        }

#ifdef STATS_OP_CODES
        m_metaprogramme->donnees_execution->compte_instructions[instruction] += 1;
#endif

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
                frame->pointeur = frame->fonction->données_exécution->chunk.code +
                                  *reinterpret_cast<int *>(frame->pointeur);
                break;
            }
            case OP_BRANCHE_CONDITION:
            {
                auto decalage_si_vrai = LIS_4_OCTETS();
                auto decalage_si_faux = LIS_4_OCTETS();
                auto condition = depile<bool>(site);

                if (condition) {
                    frame->pointeur = frame->fonction->données_exécution->chunk.code +
                                      decalage_si_vrai;
                }
                else {
                    frame->pointeur = frame->fonction->données_exécution->chunk.code +
                                      decalage_si_faux;
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
                FAIS_TRANSTYPE_AUGMENTE(unsigned char, unsigned short, uint32_t, uint64_t)
                break;
            }
            case OP_DIMINUE_NATUREL:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                FAIS_TRANSTYPE_DIMINUE(unsigned char, unsigned short, uint32_t, uint64_t)
                break;
            }
            case OP_AUGMENTE_RELATIF:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                FAIS_TRANSTYPE_AUGMENTE(char, short, int, int64_t)
                break;
            }
            case OP_DIMINUE_RELATIF:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                FAIS_TRANSTYPE_DIMINUE(char, short, int, int64_t)
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
                TRANSTYPE_EVR(int64_t)

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
            empile(site, static_cast<int64_t>(v));                                                \
        }                                                                                         \
    }

                TRANSTYPE_RVE(float)
                TRANSTYPE_RVE(double)

#undef TRANSTYPE_RVE
                break;
            }
            case OP_RETOURNE:
            {
                auto type_fonction = frame->fonction->type->comme_type_fonction();
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

                    compte_executees = i + 1;
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
            case OP_VERIFIE_CIBLE_APPEL:
            {
                auto est_pointeur = LIS_OCTET();
                AtomeFonction *ptr_fonction = nullptr;
                if (est_pointeur) {
                    /* Ne perturbe pas notre pile. */
                    auto ptr = this->pointeur_pile;
                    auto adresse = ::depile<void *>(site, ptr);
                    ptr_fonction = reinterpret_cast<AtomeFonction *>(adresse);
                }
                else {
                    ptr_fonction = LIS_POINTEUR(AtomeFonction);
                }
                if (verifie_cible_appel(ptr_fonction, site) != ResultatInterpretation::OK) {
                    compte_executees = i + 1;
                    return ResultatInterpretation::ERREUR;
                }
                break;
            }
            case OP_APPEL:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);
                auto taille_argument = LIS_4_OCTETS();
                // saute l'instruction d'appel
                frame->pointeur += 8;

#ifdef DEBOGUE_INTERPRETEUSE
                std::cerr << "-- appel : " << ptr_fonction->nom << " ("
                          << chaine_type(ptr_fonction->type) << ')' << '\n';
#endif

                if (!appel_fonction_interne(ptr_fonction, taille_argument, frame, site)) {
                    compte_executees = i + 1;
                    return ResultatInterpretation::ERREUR;
                }

                break;
            }
            case OP_APPEL_EXTERNE:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);
                auto taille_argument = LIS_4_OCTETS();
                auto ptr_inst_appel = LIS_POINTEUR(InstructionAppel);
                appel_fonction_externe(ptr_fonction, taille_argument, ptr_inst_appel, site);
                break;
            }
            case OP_APPEL_COMPILATRICE:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);

                auto resultat = ResultatInterpretation::OK;
                appel_fonction_compilatrice(ptr_fonction, site, resultat);

                if (resultat == ResultatInterpretation::PASSE_AU_SUIVANT) {
                    frame->pointeur = pointeur_debut;
                    return resultat;
                }

                break;
            }
            case OP_APPEL_INTRINSÈQUE:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);
                appel_fonction_intrinsèque(ptr_fonction, site);
                break;
            }
            case OP_APPEL_POINTEUR:
            {
                auto taille_argument = LIS_4_OCTETS();
                auto valeur_inst = LIS_8_OCTETS();
                auto adresse = depile<void *>(site);
                auto ptr_fonction = reinterpret_cast<AtomeFonction *>(adresse);
                auto ptr_inst_appel = reinterpret_cast<InstructionAppel *>(valeur_inst);

                if (ptr_fonction->decl && ptr_fonction->decl->possede_drapeau(
                                              DrapeauxNoeudFonction::EST_IPA_COMPILATRICE)) {
                    auto resultat = ResultatInterpretation::OK;
                    appel_fonction_compilatrice(ptr_fonction, site, resultat);

                    if (resultat == ResultatInterpretation::PASSE_AU_SUIVANT) {
                        frame->pointeur = pointeur_debut;
                        compte_executees = i + 1;
                        return resultat;
                    }
                }
                else if (ptr_fonction->est_externe) {
                    appel_fonction_externe(ptr_fonction, taille_argument, ptr_inst_appel, site);
                }
                else {
                    if (!appel_fonction_interne(ptr_fonction, taille_argument, frame, site)) {
                        compte_executees = i + 1;
                        return ResultatInterpretation::ERREUR;
                    }
                }

                break;
            }
            case OP_VERIFIE_ADRESSAGE_ASSIGNE:
            {
                auto taille = LIS_4_OCTETS();

                /* Ne perturbe pas notre pile. */
                auto ptr = this->pointeur_pile;
                auto adresse_ou = ::depile<void *>(site, ptr);
                auto adresse_de = static_cast<void *>(ptr - taille);

                if (!adressage_est_possible(site, adresse_ou, adresse_de, taille, true)) {
                    compte_executees = i + 1;
                    return ResultatInterpretation::ERREUR;
                }

                break;
            }
            case OP_ASSIGNE:
            {
                auto taille = LIS_4_OCTETS();
                auto adresse_ou = depile<void *>(site);
                auto adresse_de = static_cast<void *>(this->pointeur_pile - taille);
                memcpy(adresse_ou, adresse_de, static_cast<size_t>(taille));
                depile(site, taille);
                break;
            }
            case OP_ALLOUE:
            {
                auto type = LIS_POINTEUR(Type);
                // saute l'identifiant
                frame->pointeur += 8;
                this->pointeur_pile += type->taille_octet;

                if (type->taille_octet == 0) {
                    m_metaprogramme->unite->espace
                        ->rapporte_erreur(
                            site, "Erreur interne : allocation d'un type de taille 0 dans la MV !")
                        .ajoute_message("La type est : ", chaine_type(type), ".\n");
                    compte_executees = i + 1;
                    return ResultatInterpretation::ERREUR;
                }

                break;
            }
            case OP_VERIFIE_ADRESSAGE_CHARGE:
            {
                auto taille = LIS_4_OCTETS();

                /* Ne perturbe pas notre pile. */
                auto ptr = this->pointeur_pile;
                auto adresse_de = ::depile<void *>(site, ptr);
                auto adresse_ou = static_cast<void *>(ptr);

                if (!adressage_est_possible(site, adresse_ou, adresse_de, taille, false)) {
                    compte_executees = i + 1;
                    return ResultatInterpretation::ERREUR;
                }

                break;
            }
            case OP_CHARGE:
            {
                auto taille = LIS_4_OCTETS();
                auto adresse_de = depile<void *>(site);
                auto adresse_ou = static_cast<void *>(this->pointeur_pile);
                memcpy(adresse_ou, adresse_de, static_cast<size_t>(taille));
                this->pointeur_pile += taille;
                break;
            }
            case OP_REFERENCE_VARIABLE:
            {
                auto index = LIS_4_OCTETS();
                auto const &locale = frame->fonction->données_exécution->chunk.locales[index];
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
                auto index = depile<int64_t>(site);
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
                rapporte_erreur_execution(m_metaprogramme->donnees_execution->dernier_site,
                                          "Erreur interne : Opération inconnue dans la MV !");
                compte_executees = i + 1;
                return ResultatInterpretation::ERREUR;
            }
        }

        m_metaprogramme->donnees_execution->dernier_site = site;
    }

    compte_executees = INSTRUCTIONS_PAR_BATCH;
    return ResultatInterpretation::OK;
}

void MachineVirtuelle::imprime_trace_appel(NoeudExpression *site)
{
    erreur::imprime_site(*m_metaprogramme->unite->espace, site);
    for (int i = profondeur_appel - 1; i >= 0; --i) {
        erreur::imprime_site(*m_metaprogramme->unite->espace, frames[i].site);
    }
}

void MachineVirtuelle::rapporte_erreur_execution(NoeudExpression *site,
                                                 kuri::chaine_statique message)
{
    auto e = m_metaprogramme->unite->espace->rapporte_erreur(site, message);

    if (site) {
        e.ajoute_message("Le type du site est « ", chaine_type(site->type), " »\n\n");
    }

    e.ajoute_message("Trace d'appel :\n\n");
    ajoute_trace_appel(e);
}

void MachineVirtuelle::ajoute_trace_appel(Erreur &e)
{
    /* La première frame d'appel possède le même lexème que la directive d'exécution du
     * métaprogramme, donc ignorons-là également. */
    for (int i = profondeur_appel - 1; i >= 1; --i) {
        e.ajoute_site(frames[i].site);
    }
}

kuri::tableau<FrameAppel> MachineVirtuelle::donne_tableau_frame_appel() const
{
    kuri::tableau<FrameAppel> résultat;
    résultat.reserve(profondeur_appel);

    for (int i = 0; i < profondeur_appel; i++) {
        résultat.ajoute(frames[i]);
    }

    return résultat;
}

bool MachineVirtuelle::adressage_est_possible(NoeudExpression *site,
                                              const void *adresse_ou,
                                              const void *adresse_de,
                                              const int64_t taille,
                                              bool assignation)
{
    auto const taille_disponible = std::abs(static_cast<const char *>(adresse_de) -
                                            static_cast<const char *>(adresse_ou));
    if (taille_disponible < taille) {
        auto message = assignation ? "Erreur interne : superposition de la copie dans la "
                                     "machine virtuelle lors d'une assignation !" :
                                     "Erreur interne : superposition de la copie dans la "
                                     "machine virtuelle lors d'un chargement !";
        m_metaprogramme->unite->espace->rapporte_erreur(site, message)
            .ajoute_message("La taille à copier est de    : ", taille, ".\n")
            .ajoute_message("L'adresse d'origine est      : ", adresse_de, ".\n")
            .ajoute_message("L'adresse de destination est : ", adresse_ou, ".\n")
            .ajoute_message("Le type du site est          : ", chaine_type(site->type), "\n")
            .ajoute_message("Le taille du type est        : ", site->type->taille_octet, "\n")
            .ajoute_message("Le taille disponible est     : ", taille_disponible, "\n");
        return false;
    }

    if (adresse_est_nulle(adresse_de)) {
        auto message = assignation ? "Assignation depuis une adresse nulle !" :
                                     "Chargement depuis une adresse nulle !";
        rapporte_erreur_execution(site, message);
        return false;
    }

    if (adresse_est_nulle(adresse_ou)) {
        auto message = assignation ? "Assignation vers une adresse nulle !" :
                                     "Chargement vers une adresse nulle !";
        rapporte_erreur_execution(site, message);
        return false;
    }

    if (!assignation) {
        if (!adresse_est_assignable(adresse_ou)) {
            m_metaprogramme->unite->espace
                ->rapporte_erreur(site, "Copie vers une adresse non-assignable !")
                .ajoute_message("L'adresse est : ", adresse_ou, "\n");
            return false;
        }

#if 0
        // À FAIRE : il nous faudrait les adresses des messages, des noeuds codes, etc.
        if (!adresse_est_assignable(adresse_de)) {
            m_metaprogramme->unite->espace
                ->rapporte_erreur(site, "Copie depuis une adresse non-chargeable !")
                .ajoute_message("L'adresse est : ", adresse_de, "\n");
            return false;
        }
#endif
    }

    return true;
}

bool MachineVirtuelle::adresse_est_assignable(const void *adresse)
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
                            "\n")
            .ajoute_donnees([&](Erreur &e) { ajoute_trace_appel(e); });
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
    desinstalle_metaprogramme(metaprogramme, 0);
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
        auto métaprogramme = m_metaprogrammes[i];

#ifdef DEBOGUE_INTERPRETEUSE
        std::cerr << "== exécution " << it->fonction->nom_broye(it->unite->espace) << " ==\n";
#endif

        assert(métaprogramme->donnees_execution->profondeur_appel >= 1);

        installe_metaprogramme(métaprogramme);

        int compte_executees = 0;
        auto res = execute_instructions(compte_executees);

        if (res == ResultatInterpretation::PASSE_AU_SUIVANT) {
            // RÀF
        }
        else if (res == ResultatInterpretation::ERREUR) {
            métaprogramme->resultat = MetaProgramme::RésultatExécution::ERREUR;
            m_metaprogrammes_termines.ajoute(métaprogramme);
            std::swap(m_metaprogrammes[i], m_metaprogrammes[nombre_metaprogrammes - 1]);
            nombre_metaprogrammes -= 1;
            i -= 1;
        }
        else if (res == ResultatInterpretation::TERMINE) {
            métaprogramme->resultat = MetaProgramme::RésultatExécution::SUCCÈS;
            m_metaprogrammes_termines.ajoute(métaprogramme);
            std::swap(m_metaprogrammes[i], m_metaprogrammes[nombre_metaprogrammes - 1]);
            nombre_metaprogrammes -= 1;
            i -= 1;

#ifdef STATS_OP_CODES
            std::cerr << "------------------------------------ Instructions :\n";
            for (auto j = 0; j < NOMBRE_OP_CODE; j++) {
                std::cerr << chaine_code_operation(octet_t(j)) << " : "
                          << it->donnees_execution->compte_instructions[j] << '\n';
            }
#endif
        }

        desinstalle_metaprogramme(métaprogramme, compte_executees);

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

    instructions_executees += donnees->instructions_executees;

    // À FAIRE : récupère la mémoire
    memoire::deloge_tableau("MachineVirtuelle::pile", donnees->pile, TAILLE_PILE);
    donnees = nullptr;
}

void MachineVirtuelle::rassemble_statistiques(Statistiques &stats)
{
    stats.memoire_mv += donnees_execution.memoire_utilisee();
    stats.nombre_metaprogrammes_executes += nombre_de_metaprogrammes_executes;
    stats.temps_metaprogrammes += temps_execution_metaprogammes;
    stats.instructions_executees += instructions_executees;

    if (compilatrice.arguments.profile_metaprogrammes) {
        profileuse.cree_rapports(compilatrice.arguments.format_rapport_profilage);
    }
}

std::ostream &operator<<(std::ostream &os, PatchDonnéesConstantes const &patch)
{
    os << "Patch données constantes :\n";

    os << "-- destination         : ";
    if (patch.destination.type == DONNÉES_CONSTANTES) {
        os << "données constantes\n";
    }
    else {
        os << "données globales\n";
    }

    os << "-- source              : ";
    if (patch.source.type == ADRESSE_CONSTANTE) {
        os << "adresse constante\n";
    }
    else {
        os << "adresse globale\n";
    }

    os << "-- adresse source      : " << patch.source.décalage << '\n';
    os << "-- adresse destination : " << patch.destination.décalage << '\n';

    return os;
}

InformationProfilage &Profileuse::informations_pour(MetaProgramme *metaprogramme)
{
    POUR (informations_pour_metaprogrammes) {
        if (it.metaprogramme == metaprogramme) {
            return it;
        }
    }

    auto informations = InformationProfilage();
    informations.metaprogramme = metaprogramme;
    informations_pour_metaprogrammes.ajoute(informations);
    return informations_pour_metaprogrammes.derniere();
}

void Profileuse::ajoute_echantillon(MetaProgramme *metaprogramme, int poids)
{
    if (poids == 0) {
        return;
    }

    auto &informations = informations_pour(metaprogramme);

    auto echantillon = EchantillonProfilage();
    echantillon.profondeur_frame_appel = metaprogramme->donnees_execution->profondeur_appel;
    echantillon.poids = poids;

    for (int i = 0; i < echantillon.profondeur_frame_appel; i++) {
        echantillon.frames[i] = metaprogramme->donnees_execution->frames[i];
    }

    informations.echantillons.ajoute(echantillon);
}

void Profileuse::cree_rapports(FormatRapportProfilage format)
{
    POUR (informations_pour_metaprogrammes) {
        cree_rapport(it, format);
    }
}

static void imprime_nom_fonction(AtomeFonction const *fonction, std::ostream &os)
{
    if (fonction->decl) {
        os << nom_humainement_lisible(fonction->decl);
    }
    else {
        os << fonction->nom;
    }
}

static void cree_rapport_format_echantillons_total_plus_fonction(
    const InformationProfilage &informations, std::ostream &os)
{
    auto table = kuri::table_hachage<AtomeFonction *, int>("Échantillons profilage");
    auto fonctions = kuri::ensemble<AtomeFonction *>();

    POUR (informations.echantillons) {
        for (int i = 0; i < it.profondeur_frame_appel; i++) {
            auto &frame = it.frames[i];
            auto valeur = table.valeur_ou(frame.fonction, 0);
            table.insere(frame.fonction, valeur + 1);

            fonctions.insere(frame.fonction);
        }
    }

    auto fonctions_et_echantillons = kuri::tableau<PaireEnchantillonFonction>();

    fonctions.pour_chaque_element([&](AtomeFonction *fonction) {
        auto nombre_echantillons = table.valeur_ou(fonction, 0);
        auto paire = PaireEnchantillonFonction{fonction, nombre_echantillons};
        fonctions_et_echantillons.ajoute(paire);
    });

    std::sort(fonctions_et_echantillons.begin(),
              fonctions_et_echantillons.end(),
              [](auto &a, auto &b) { return a.nombre_echantillons > b.nombre_echantillons; });

    POUR (fonctions_et_echantillons) {
        os << it.nombre_echantillons << " : ";
        imprime_nom_fonction(it.fonction, os);
        os << '\n';
    }
}

static void cree_rapport_format_brendan_gregg(const InformationProfilage &informations,
                                              std::ostream &os)
{
    POUR (informations.echantillons) {
        if (it.profondeur_frame_appel == 0) {
            continue;
        }

        for (int i = 0; i < it.profondeur_frame_appel; i++) {
            auto &frame = it.frames[i];
            imprime_nom_fonction(frame.fonction, os);

            if (i < it.profondeur_frame_appel - 1) {
                os << ";";
            }
        }

        os << " " << it.poids << '\n';
    }
}

void Profileuse::cree_rapport(const InformationProfilage &informations,
                              FormatRapportProfilage format)
{
    auto nom_base_fichier = enchaine("métaprogramme", informations.metaprogramme, ".txt");
    auto chemin_fichier = kuri::chemin_systeme::chemin_temporaire(nom_base_fichier);

    std::ofstream os(vers_std_path(chemin_fichier));

    switch (format) {
        case FormatRapportProfilage::ECHANTILLONS_TOTAL_POUR_FONCTION:
        {
            cree_rapport_format_echantillons_total_plus_fonction(informations, os);
            break;
        }
        case FormatRapportProfilage::BRENDAN_GREGG:
        {
            cree_rapport_format_brendan_gregg(informations, os);
            break;
        }
    }
}
