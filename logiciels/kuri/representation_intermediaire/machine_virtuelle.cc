/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "machine_virtuelle.hh"

#include <iostream>

#ifndef _MSC_VER
#    include <x86intrin.h>
#endif

#include "arbre_syntaxique/noeud_code.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"
#include "compilation/ipa.hh"
#include "compilation/metaprogramme.hh"

#include "parsage/identifiant.hh"

#include "structures/ensemble.hh"
#include "structures/table_hachage.hh"

#include "utilitaires/calcul.hh"
#include "utilitaires/chrono.hh"
#include "utilitaires/log.hh"

#include "instructions.hh"

/* ------------------------------------------------------------------------- */
/** \name Fuites de mémoire.
 * \{ */

static void imprime_entête_fuite_de_mémoire(Enchaineuse &enchaineuse, size_t taille_bloc)
{
    enchaineuse << "Fuite de mémoire dans l'exécution du métaprogramme : " << taille_bloc
                << " octets non libérés.\n\n";
}

static void rapporte_avertissement_pour_fuite_de_mémoire(MetaProgramme *métaprogramme,
                                                         size_t taille_bloc,
                                                         kuri::tableau<FrameAppel> const &frames)
{
    auto espace = métaprogramme->unité->espace;

    auto &logueuse = métaprogramme->donne_logueuse(TypeLogMétaprogramme::FUITES_DE_MÉMOIRE);
    logueuse << "-----------------------------------------------------------------------------\n";

    imprime_entête_fuite_de_mémoire(logueuse, taille_bloc);

    for (int f = int(frames.taille()) - 1; f >= 0; f--) {
        erreur::imprime_site(logueuse, *espace, frames[f].site);
    }
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
    if (table_allocations.possède(ptr)) {
        table_allocations.efface(ptr);
        return true;
    }
#else
    if (table_allocations.find(ptr) != table_allocations.end()) {
        table_allocations.erase(ptr);
        return true;
    }
#endif

    return false;
}

void DétectriceFuiteDeMémoire::réinitialise()
{
#ifdef UTILISE_NOTRE_TABLE
    table_allocations.reinitialise();
#else
    table_allocations = {};
#endif
}

void imprime_fuites_de_mémoire(MetaProgramme *métaprogramme)
{
    auto données = métaprogramme->données_exécution;
    auto taille_non_libérée = size_t(0);

#ifdef UTILISE_NOTRE_TABLE
    données->détectrice_fuite_de_mémoire.table_allocations.pour_chaque_élément(
        [&](DétectriceFuiteDeMémoire::InformationsBloc const &info) {
            taille_non_libérée += info.taille;
            rapporte_avertissement_pour_fuite_de_mémoire(métaprogramme, info.taille, info.frame);
        });
#else
    POUR (données->détectrice_fuite_de_mémoire.table_allocations) {
        taille_non_libérée += it.second.taille;
        rapporte_avertissement_pour_fuite_de_mémoire(
            métaprogramme, it.second.taille, it.second.frame);
    }
#endif

    if (taille_non_libérée == 0) {
        return;
    }

    Enchaineuse enchaineuse;
    imprime_entête_fuite_de_mémoire(enchaineuse, taille_non_libérée);
    enchaineuse
        << "Veuillez-vous référer au fichier de log du métaprogramme pour plus de détails.";

    auto espace = métaprogramme->unité->espace;
    espace->rapporte_avertissement(métaprogramme->directive, enchaineuse.chaine());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Fuites de mémoire.
 * \{ */

void DonnéesExécution::réinitialise()
{
    this->pointeur_pile = this->pile;
    this->profondeur_appel = 0;
    this->instructions_exécutées = 0;
    this->détectrice_fuite_de_mémoire.réinitialise();
    this->tailles_empilées.efface();
    this->profileuse.réinitialise();
}

void DonnéesExécution::imprime_stats_instructions(Enchaineuse &os)
{
    using TypeEntréeStat = std::pair<octet_t, int>;
    kuri::tableau<TypeEntréeStat> entrées;

    auto nombre_instructions = 0;
    for (auto i = 0; i < NOMBRE_OP_CODE; i++) {
        if (i == OP_STAT_INSTRUCTION) {
            nombre_instructions = compte_instructions[i];
            continue;
        }

        if (compte_instructions[i] == 0) {
            continue;
        }

        entrées.ajoute({octet_t(i), compte_instructions[i]});
    }

    std::sort(
        entrées.begin(), entrées.end(), [](auto &a, auto &b) { return a.second > b.second; });

    auto taille_max_chaine = int64_t(0);
    POUR (entrées) {
        auto chaine_code = chaine_code_operation(it.first);
        taille_max_chaine = std::max(taille_max_chaine, chaine_code.taille());
    }

    os << "------------------------------------ Instructions :\n";
    os << "Instructions exécutées : " << nombre_instructions << "\n";

    POUR (entrées) {
        auto chaine_code = chaine_code_operation(it.first);
        os << "-- " << chaine_code_operation(it.first);

        for (int i = 0; i < (taille_max_chaine - chaine_code.taille()); i++) {
            os << ' ';
        }

        os << " : " << it.second;
        os << " (" << (double(it.second) * 100.0 / double(nombre_instructions)) << "%)";
        os << '\n';
    }
}

/** \} */

void logue_stats_instructions(MetaProgramme *métaprogramme)
{
    auto &logueuse = métaprogramme->donne_logueuse(TypeLogMétaprogramme::STAT_INSTRUCTION);
    métaprogramme->données_exécution->imprime_stats_instructions(logueuse);
}

#define EST_FONCTION_COMPILATRICE(fonction)                                                       \
    ptr_fonction->données_exécution->données_externe.ptr_fonction ==                              \
        reinterpret_cast<Symbole::type_adresse_fonction>(fonction)

inline bool adresse_est_nulle(const void *adresse)
{
    /* 0xbebebebebebebebe peut être utilisé par les débogueurs.
     * Vérification si adresse est dans la première page pour également détecter les
     * déréférencement d'adresses nulles. */
    return adresse == nullptr || adresse == reinterpret_cast<void *>(0xbebebebebebebebe) ||
           reinterpret_cast<uint64_t>(adresse) < 4096;
}

static std::ostream &operator<<(std::ostream &os, MachineVirtuelle::RésultatInterprétation res)
{
#define CASE(NOM)                                                                                 \
    case MachineVirtuelle::RésultatInterprétation::NOM:                                           \
        os << #NOM;                                                                               \
        break
    switch (res) {
        CASE(OK);
        CASE(ERREUR);
        CASE(COMPILATION_ARRÊTÉE);
        CASE(TERMINÉ);
        CASE(PASSE_AU_SUIVANT);
    }
#undef CASE
    return os;
}

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
        auto a = dépile<type>();                                                                  \
        empile(op a);                                                                             \
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
        auto b = dépile<type>();                                                                  \
        auto a = dépile<type>();                                                                  \
        auto r = op::applique_opération<type>(a, b);                                              \
        empile(r);                                                                                \
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
        auto v = dépile<type_de>();                                                               \
        empile(static_cast<type_vers>(v));                                                        \
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

#define TRANSTYPE_EVR(type_)                                                                      \
    if (taille_de == static_cast<int>(taille_de(type_))) {                                        \
        auto v = dépile<type_>();                                                                 \
        if (taille_vers == 2) {                                                                   \
            /* @Incomplet : r16 */                                                                \
        }                                                                                         \
        else if (taille_vers == 4) {                                                              \
            empile(static_cast<float>(v));                                                        \
        }                                                                                         \
        else if (taille_vers == 8) {                                                              \
            empile(static_cast<double>(v));                                                       \
        }                                                                                         \
    }

#define TRANSTYPE_RVE(type_, type1, type2, type4, type8)                                          \
    if (taille_de == static_cast<int>(taille_de(type_))) {                                        \
        auto v = dépile<type_>();                                                                 \
        if (taille_vers == 1) {                                                                   \
            empile(static_cast<type1>(v));                                                        \
        }                                                                                         \
        else if (taille_vers == 2) {                                                              \
            empile(static_cast<type2>(v));                                                        \
        }                                                                                         \
        else if (taille_vers == 4) {                                                              \
            empile(static_cast<type4>(v));                                                        \
        }                                                                                         \
        else if (taille_vers == 8) {                                                              \
            empile(static_cast<type8>(v));                                                        \
        }                                                                                         \
    }

/* ************************************************************************** */

static void lis_valeur(octet_t *pointeur, Type const *type, Enchaineuse &os)
{
    switch (type->genre) {
        default:
        {
            os << "valeur de type " << chaine_type(type) << " non prise en charge";
            break;
        }
        case GenreNoeud::TUPLE:
        {
            auto type_tuple = type->comme_type_tuple();
            POUR (type_tuple->rubriques) {
                lis_valeur(pointeur + it.decalage, it.type, os);
            }
            break;
        }
        case GenreNoeud::ENTIER_RELATIF:
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
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ENTIER_NATUREL:
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
        case GenreNoeud::BOOL:
        {
            os << (*reinterpret_cast<bool *>(pointeur) ? "vrai" : "faux");
            break;
        }
        case GenreNoeud::RÉEL:
        {
            if (type->taille_octet == 4) {
                os << *reinterpret_cast<float *>(pointeur);
            }
            else if (type->taille_octet == 8) {
                os << *reinterpret_cast<double *>(pointeur);
            }

            break;
        }
        case GenreNoeud::FONCTION:
        case GenreNoeud::POINTEUR:
        {
            os << *reinterpret_cast<void **>(pointeur);
            break;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        {
            auto type_structure = type->comme_type_structure();

            auto virgule = "{ ";

            POUR (type_structure->rubriques) {
                os << virgule;
                os << it.nom << " = ";

                auto pointeur_rubrique = pointeur + it.decalage;
                lis_valeur(pointeur_rubrique, it.type, os);

                virgule = ", ";
            }

            os << " }";

            break;
        }
        case GenreNoeud::CHAINE:
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

static auto imprime_valeurs_entrées(octet_t *pointeur_debut_entree,
                                    AtomeFonction const *fonction,
                                    int profondeur_appel,
                                    Enchaineuse &logueuse)
{
    logueuse << chaine_indentations(profondeur_appel) << "Appel de " << fonction->nom << '\n';

    auto type_fonction = fonction->type->comme_type_fonction();
    auto pointeur_lecture_retour = pointeur_debut_entree;
    POUR_INDICE (type_fonction->types_entrées) {
        logueuse << chaine_indentations(profondeur_appel) << "-- paramètre " << indice_it << " ("
                 << chaine_type(it) << ") : ";
        lis_valeur(pointeur_lecture_retour, it, logueuse);
        logueuse << '\n';

        pointeur_lecture_retour += it->taille_octet;
    }
}

static auto imprime_valeurs_sorties(octet_t *pointeur_debut_retour,
                                    AtomeFonction const *fonction,
                                    int profondeur_appel,
                                    Enchaineuse &logueuse)
{
    logueuse << chaine_indentations(profondeur_appel) << "Retour de " << fonction->nom << '\n';

    auto type_fonction = fonction->type->comme_type_fonction();
    auto type_sortie = type_fonction->type_sortie;
    if (type_sortie->est_type_rien()) {
        return;
    }

    logueuse << chaine_indentations(profondeur_appel) << "-- résultat : ";
    lis_valeur(pointeur_debut_retour, type_sortie, logueuse);
    logueuse << '\n';
}

static auto imprime_valeurs_locales(FrameAppel *frame, int profondeur_appel, Enchaineuse &os)
{
    os << chaine_indentations(profondeur_appel) << frame->fonction->nom << " :\n";

    POUR (frame->fonction->données_exécution->chunk.locales) {
        auto pointeur_locale = &frame->pointeur_pile[it.adresse];
        os << chaine_indentations(profondeur_appel) << "Locale ("
           << static_cast<void *>(pointeur_locale) << ") : ";

        if (it.ident) {
            os << it.ident->nom;
        }
        else {
            os << "temporaire";
        }

        os << " = ";
        lis_valeur(pointeur_locale, it.type, os);
        os << '\n';
    }
}

static inline void *donne_adresse_locale(FrameAppel *frame, int index)
{
    auto const &locale = frame->fonction->données_exécution->chunk.locales[index];
    return &frame->pointeur_pile[locale.adresse];
}

struct LimitesCodeFrame {
    octet_t const *adresse_début = nullptr;
    octet_t const *adresse_fin = nullptr;
};

static LimitesCodeFrame donne_limite_code_frame(FrameAppel const *frame)
{
    auto adresse_base = frame->fonction->données_exécution->chunk.code;
    auto taille = frame->fonction->données_exécution->chunk.compte;
    return {adresse_base, adresse_base + taille};
}

static bool est_hors_limites(LimitesCodeFrame const limites, octet_t const *adresse)
{
    if (adresse < limites.adresse_début || adresse >= limites.adresse_fin) {
        return true;
    }

    return false;
}

/* ************************************************************************** */

MachineVirtuelle::MachineVirtuelle(Compilatrice &compilatrice_) : compilatrice(compilatrice_)
{
}

MachineVirtuelle::~MachineVirtuelle()
{
    POUR (m_métaprogrammes) {
        déloge_données_exécution(it->données_exécution);
    }

    POUR (m_métaprogrammes_terminés) {
        déloge_données_exécution(it->données_exécution);
    }

    POUR (m_données_exécution_libres) {
        mémoire::deloge_tableau("MachineVirtuelle::pile", it->pile, TAILLE_PILE);
    }
}

template <typename T>
inline T dépile(octet_t *&pointeur_pile)
{
    pointeur_pile -= static_cast<int64_t>(sizeof(T));
    return *reinterpret_cast<T *>(pointeur_pile);
}

void MachineVirtuelle::incrémente_pointeur_de_pile(int64_t taille)
{
    this->pointeur_pile += taille;
    if (pointeur_pile >= (pile + TAILLE_PILE)) {
        rapporte_erreur_exécution("sur-entamponnage de la pile de données");
    }
}

void MachineVirtuelle::décrémente_pointeur_de_pile(int64_t taille)
{
    pointeur_pile -= taille;
    if (pointeur_pile < pile) {
        rapporte_erreur_exécution("sous-entamponnage de la pile de données");
    }
}

bool MachineVirtuelle::appel(AtomeFonction *fonction, NoeudExpression const *site)
{
    if (profondeur_appel == TAILLE_FRAMES_APPEL) {
        rapporte_erreur_exécution("Dépassement de la profondeur d'appels possibles.");
        return false;
    }

    auto frame = &frames[profondeur_appel++];
    frame->fonction = fonction;
    frame->site = site;
    frame->pointeur = fonction->données_exécution->chunk.code;
    frame->pointeur_pile = pointeur_pile;
    /* Réserve de l'espace sur la pile pour nos locales. */
    incrémente_pointeur_de_pile(fonction->données_exécution->chunk.taille_allouée);
    return true;
}

bool MachineVirtuelle::appel_fonction_interne(AtomeFonction *ptr_fonction,
                                              int taille_argument,
                                              FrameAppel *&frame)
{
    // puisque les arguments utilisent des instructions d'allocations retire la taille des
    // arguments du pointeur de la pile pour ne pas que les allocations ne l'augmente
    décrémente_pointeur_de_pile(taille_argument);

    auto const site = donne_site_adresse_courante();
    if (!appel(ptr_fonction, site)) {
        return false;
    }

    frame = &frames[profondeur_appel - 1];
    return true;
}

#define RAPPORTE_ERREUR_SI_NUL(pointeur, message)                                                 \
    if (!pointeur) {                                                                              \
        rapporte_erreur_exécution(message);                                                       \
    }

void MachineVirtuelle::appel_fonction_compilatrice(AtomeFonction *ptr_fonction,
                                                   RésultatInterprétation &résultat)
{
    /* Détermine ici si nous avons une fonction de l'IPA pour prendre en compte les appels via des
     * pointeurs de fonctions. */
    if (EST_FONCTION_COMPILATRICE(compilatrice_espace_courant)) {
        empile(m_métaprogramme->unité->espace);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_attend_message)) {
        auto message = compilatrice.attend_message();

        if (!message) {
            résultat = RésultatInterprétation::PASSE_AU_SUIVANT;
            return;
        }

        empile(message);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_commence_interception)) {
        auto espace_recu = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace_recu, "Reçu un espace de travail nul");

        auto &messagère = compilatrice.messagère;
        messagère->commence_interception(espace_recu);

        espace_recu->metaprogramme = m_métaprogramme;
        static_cast<void>(espace_recu);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_termine_interception)) {
        auto espace_recu = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace_recu, "Reçu un espace de travail nul");

        if (espace_recu->metaprogramme != m_métaprogramme) {
            auto const site = donne_site_adresse_courante();
            /* L'espace du « site » est celui de métaprogramme, et non
             * l'espace reçu en paramètre. */
            m_métaprogramme->unité->espace->rapporte_erreur(
                site,
                "Le métaprogramme terminant l'interception n'est pas celui l'ayant commancé !");
        }

        espace_recu->metaprogramme = nullptr;

        /* Ne passons pas par la messagère car il est possible que le GestionnaireCode soit
         * vérrouiller par quelqu'un, et par le passé la Messagère prévenait le GestionnaireCode,
         * causant un verrou mort. */
        auto &gestionnaire = compilatrice.gestionnaire_code;
        gestionnaire->interception_message_terminée(espace_recu);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_lèxe_fichier)) {
        auto const site = donne_site_adresse_courante();
        auto chemin_recu = dépile<kuri::chaine_statique>();
        auto espace = m_métaprogramme->unité->espace;
        auto lexemes = compilatrice.lexe_fichier(espace, chemin_recu, site);
        empile(lexemes);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_donne_options)) {
        auto options = compilatrice.options_compilation();
        empile(options);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_ajourne_options)) {
        auto options = dépile<OptionsDeCompilation *>();
        RAPPORTE_ERREUR_SI_NUL(options, "Reçu des options de compilation nulles");
        compilatrice.ajourne_options_compilation(options);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(ajoute_chaine_à_la_compilation)) {
        auto const site = donne_site_adresse_courante();
        auto chaine = dépile<kuri::chaine_statique>();
        auto espace = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        auto espace_pour_site = m_métaprogramme->unité->espace;
        compilatrice.ajoute_chaine_compilation(espace, espace_pour_site, site, chaine);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(ajoute_fichier_à_la_compilation)) {
        auto const site = donne_site_adresse_courante();
        auto chaine = dépile<kuri::chaine_statique>();
        auto espace = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        compilatrice.ajoute_fichier_compilation(espace, chaine, site);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(ajoute_chaine_au_module)) {
        auto const site = donne_site_adresse_courante();
        auto chaine = dépile<kuri::chaine_statique>();
        auto module = dépile<Module *>();
        auto espace = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(module, "Reçu un module nul");
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        auto espace_pour_site = m_métaprogramme->unité->espace;
        compilatrice.ajoute_chaine_au_module(espace, espace_pour_site, site, module, chaine);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(démarre_un_espace_de_travail)) {
        auto options = dépile<OptionsDeCompilation *>();
        auto nom = dépile<kuri::chaine_statique>();
        RAPPORTE_ERREUR_SI_NUL(options, "Reçu des options nulles");
        auto dossier = compilatrice.espace_de_travail_défaut->module->chemin();
        auto espace = compilatrice.démarre_un_espace_de_travail(*options, nom, dossier);
        empile(espace);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(espace_défaut_compilation)) {
        auto espace = compilatrice.espace_défaut_compilation();
        empile(espace);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_rapporte_erreur)) {
        auto message = dépile<kuri::chaine_statique>();
        /* Dans les noeuds codes, les lignes commencent à 1. */
        auto ligne = dépile<int>() - 1;
        auto fichier = dépile<kuri::chaine_statique>();
        auto espace = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        espace->rapporte_erreur(fichier, ligne, message);
        m_métaprogramme->a_rapporté_une_erreur = true;
        /* À FAIRE : devrions nous avoir un résultat plus spécifique ? */
        résultat = RésultatInterprétation::TERMINÉ;
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_rapporte_erreur_externe)) {
        auto params = ParamètresErreurExterne{};
        params.indice_colonne_fin = dépile<int>();
        params.indice_colonne_début = dépile<int>();
        params.indice_colonne = dépile<int>();
        params.numéro_ligne = dépile<int>();
        params.texte_ligne = dépile<kuri::chaine_statique>();
        params.chemin_fichier = dépile<kuri::chaine_statique>();
        params.message = dépile<kuri::chaine_statique>();
        auto espace = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        espace->rapporte_erreur_externe(params);
        m_métaprogramme->a_rapporté_une_erreur = true;
        /* À FAIRE : devrions nous avoir un résultat plus spécifique ? */
        résultat = RésultatInterprétation::TERMINÉ;
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_rapporte_avertissement)) {
        auto message = dépile<kuri::chaine_statique>();
        /* Dans les noeuds codes, les lignes commencent à 1. */
        auto ligne = dépile<int>() - 1;
        auto fichier = dépile<kuri::chaine_statique>();
        auto espace = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        espace->rapporte_avertissement(fichier, ligne, message);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_rapporte_avertissement_externe)) {
        auto params = ParamètresErreurExterne{};
        params.indice_colonne_fin = dépile<int>();
        params.indice_colonne_début = dépile<int>();
        params.indice_colonne = dépile<int>();
        params.numéro_ligne = dépile<int>();
        params.texte_ligne = dépile<kuri::chaine_statique>();
        params.chemin_fichier = dépile<kuri::chaine_statique>();
        params.message = dépile<kuri::chaine_statique>();
        auto espace = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        espace->rapporte_avertissement_externe(params);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_rapporte_info)) {
        auto message = dépile<kuri::chaine_statique>();
        /* Dans les noeuds codes, les lignes commencent à 1. */
        auto ligne = dépile<int>() - 1;
        auto fichier = dépile<kuri::chaine_statique>();
        auto espace = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        espace->rapporte_info(fichier, ligne, message);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_rapporte_info_externe)) {
        auto params = ParamètresErreurExterne{};
        params.indice_colonne_fin = dépile<int>();
        params.indice_colonne_début = dépile<int>();
        params.indice_colonne = dépile<int>();
        params.numéro_ligne = dépile<int>();
        params.texte_ligne = dépile<kuri::chaine_statique>();
        params.chemin_fichier = dépile<kuri::chaine_statique>();
        params.message = dépile<kuri::chaine_statique>();
        auto espace = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        espace->rapporte_info_externe(params);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_possède_erreur)) {
        auto espace = dépile<EspaceDeTravail *>();
        RAPPORTE_ERREUR_SI_NUL(espace, "Reçu un espace de travail nul");
        empile(compilatrice.possède_erreur(espace));
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_module_courant)) {
        auto const site = donne_site_adresse_courante();
        auto espace = m_métaprogramme->unité->espace;
        auto fichier = espace->fichier(site->lexème->fichier);
        auto module = fichier->module;
        empile(module);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_message_reçu)) {
        auto message = dépile<Message *>();
        RAPPORTE_ERREUR_SI_NUL(message, "Reçu un message nul");
        compilatrice.gestionnaire_code->message_reçu(message);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_fonctions_parsées)) {
        auto espace = m_métaprogramme->unité->espace;
        auto fonctions = compilatrice.fonctions_parsees(espace);
        empile(fonctions);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_module_pour_code)) {
        auto code = dépile<NoeudCode *>();
        RAPPORTE_ERREUR_SI_NUL(code, "Reçu un noeud code nul");
        auto espace = m_métaprogramme->unité->espace;
        const auto fichier = espace->fichier(code->chemin_fichier);
        RAPPORTE_ERREUR_SI_NUL(fichier, "Aucun fichier correspond au noeud code");
        const auto module = fichier->module;
        empile(module);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_module_pour_type)) {
        auto info_type = dépile<InfoType *>();
        RAPPORTE_ERREUR_SI_NUL(info_type, "Reçu un InfoType nul");
        auto espace = m_métaprogramme->unité->espace;
        const auto decl = espace->typeuse.decl_pour_info_type(info_type);
        if (!decl) {
            empile(nullptr);
            return;
        }
        const auto fichier = espace->fichier(decl->lexème->fichier);
        if (!fichier) {
            empile(nullptr);
            return;
        }
        const auto module = fichier->module;
        empile(module);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_nom_module)) {
        auto module = dépile<Module *>();
        RAPPORTE_ERREUR_SI_NUL(module, "Reçu un Module nul");
        empile(module->nom_->nom);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_chemin_module)) {
        auto module = dépile<Module *>();
        RAPPORTE_ERREUR_SI_NUL(module, "Reçu un Module nul");
        empile(module->chemin());
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_module_racine_compilation)) {
        auto module = compilatrice.espace_de_travail_défaut->module;
        empile(module);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_racine_installation_kuri)) {
        kuri::chaine_statique racine_kuri = compilatrice.racine_kuri;
        empile(racine_kuri);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_donne_arguments_ligne_de_commande)) {
        kuri::tableau_statique<kuri::chaine_statique> arguments =
            compilatrice.arguments.arguments_pour_métaprogrammes;
        empile(arguments);
        return;
    }

    if (EST_FONCTION_COMPILATRICE(compilatrice_donne_fichier_entrée_compilation)) {
        empile(compilatrice.arguments.fichier_entrée_compilation);
        return;
    }
}

void MachineVirtuelle::empile_fonction_non_interne(AtomeFonction *ptr_fonction)
{
    profondeur_appel++;
    frames[profondeur_appel - 1].fonction = ptr_fonction;
}

void MachineVirtuelle::dépile_fonction_non_interne(AtomeFonction * /*ptr_fonction*/)
{
    profondeur_appel--;
}

void MachineVirtuelle::appel_fonction_externe(AtomeFonction *ptr_fonction,
                                              int taille_argument,
                                              InstructionAppel *inst_appel,
                                              RésultatInterprétation &résultat_interp)
{
    if (EST_FONCTION_COMPILATRICE(notre_malloc)) {
        auto taille = dépile<size_t>();
        auto résultat = notre_malloc(taille);
        empile(résultat);

        auto données = m_métaprogramme->données_exécution;
        données->détectrice_fuite_de_mémoire.ajoute_bloc(
            résultat, taille, donne_tableau_frame_appel());
        return;
    }

    if (EST_FONCTION_COMPILATRICE(notre_realloc)) {
        auto taille = dépile<size_t>();
        auto ptr = dépile<void *>();

        auto données = m_métaprogramme->données_exécution;
        if (!données->détectrice_fuite_de_mémoire.supprime_bloc(ptr)) {
            rapporte_erreur_exécution(
                "Réallocation d'un objet logé à une adresse qui ne fut pas allouée.");
            résultat_interp = RésultatInterprétation::ERREUR;
            return;
        }

        auto résultat = notre_realloc(ptr, taille);
        empile(résultat);

        données->détectrice_fuite_de_mémoire.ajoute_bloc(
            résultat, taille, donne_tableau_frame_appel());
        return;
    }

    if (EST_FONCTION_COMPILATRICE(notre_free)) {
        auto ptr = dépile<void *>();

        auto données = m_métaprogramme->données_exécution;
        if (!données->détectrice_fuite_de_mémoire.supprime_bloc(ptr)) {
            rapporte_erreur_exécution(
                "Délogement d'un objet logé à une adresse qui ne fut pas allouée.");
            résultat_interp = RésultatInterprétation::ERREUR;
            return;
        }

        notre_free(ptr);
        return;
    }

    auto type_fonction = ptr_fonction->decl->type->comme_type_fonction();
    auto &données_externe = ptr_fonction->données_exécution->données_externe;

    auto pointeur_arguments = pointeur_pile - taille_argument;

    auto pointeurs_arguments = kuri::tablet<void *, 12>();
    auto decalage_argument = 0u;

    if (ptr_fonction->decl->possède_drapeau(DrapeauxNoeudFonction::EST_VARIADIQUE)) {
        auto nombre_arguments_fixes = static_cast<unsigned>(type_fonction->types_entrées.taille() -
                                                            1);
        auto nombre_arguments_totaux = static_cast<unsigned>(inst_appel->args.taille());

        données_externe.types_entrées.efface();
        données_externe.types_entrées.réserve(nombre_arguments_totaux);

        POUR (inst_appel->args) {
            auto type_primitif = donne_type_primitif(it->type);
            auto type_ffi = converti_type_ffi(type_primitif);
            données_externe.types_entrées.ajoute(type_ffi);

            auto ptr = &pointeur_arguments[decalage_argument];
            pointeurs_arguments.ajoute(ptr);

            decalage_argument += type_primitif->taille_octet;
        }

        données_externe.types_entrées.ajoute(nullptr);

        auto type_ffi_sortie = converti_type_ffi(type_fonction->type_sortie);
        auto ptr_types_entrees = données_externe.types_entrées.données();

        auto status = ffi_prep_cif_var(&données_externe.cif,
                                       FFI_DEFAULT_ABI,
                                       nombre_arguments_fixes,
                                       nombre_arguments_totaux,
                                       type_ffi_sortie,
                                       ptr_types_entrees);

        if (status != FFI_OK) {
            rapporte_erreur_exécution("Erreur interne : impossible de préparer les arguments FFI "
                                      "pour la fonction variadique externe.");
            résultat_interp = RésultatInterprétation::ERREUR;
            return;
        }
    }
    else {
        POUR (type_fonction->types_entrées) {
            auto ptr = &pointeur_arguments[decalage_argument];
            pointeurs_arguments.ajoute(ptr);
            decalage_argument += it->taille_octet;
        }
    }

    ffi_call(&données_externe.cif,
             données_externe.ptr_fonction,
             pointeur_pile,
             pointeurs_arguments.données());

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

inline void MachineVirtuelle::empile_constante(FrameAppel *frame)
{
    auto drapeaux = LIS_OCTET();

#define EMPILE_CONSTANTE(type)                                                                    \
    type v = *(reinterpret_cast<type *>(frame->pointeur));                                        \
    empile(v);                                                                                    \
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
            empile(LIS_OCTET());
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

void MachineVirtuelle::installe_métaprogramme(MetaProgramme *métaprogramme)
{
    auto de = métaprogramme->données_exécution;
    profondeur_appel = de->profondeur_appel;
    pile = de->pile;
    pointeur_pile = de->pointeur_pile;
    frames = de->frames;
    ptr_données_constantes = métaprogramme->données_constantes.données();
    ptr_données_globales = métaprogramme->données_globales.données();
    données_constantes = &métaprogramme->unité->espace->données_constantes_exécutions;

    intervalle_adresses_globales.min = ptr_données_globales;
    intervalle_adresses_globales.max = ptr_données_globales +
                                       métaprogramme->données_globales.taille();

    intervalle_adresses_pile_exécution.min = pile;
    intervalle_adresses_pile_exécution.max = pile + TAILLE_PILE;

    assert(pile);
    assert(pointeur_pile);

    m_métaprogramme = métaprogramme;
    if (compilatrice.arguments.profile_metaprogrammes) {
        de->profileuse.prépare_pour_profilage();
    }
}

void MachineVirtuelle::désinstalle_métaprogramme(MetaProgramme *métaprogramme,
                                                 int compte_exécutées)
{
    auto de = métaprogramme->données_exécution;
    de->profondeur_appel = profondeur_appel;
    de->pointeur_pile = pointeur_pile;

    assert(intervalle_adresses_pile_exécution.possède_inclusif(de->pointeur_pile));

    profondeur_appel = 0;
    pile = nullptr;
    pointeur_pile = nullptr;
    frames = nullptr;
    ptr_données_constantes = nullptr;
    ptr_données_globales = nullptr;
    données_constantes = nullptr;
    intervalle_adresses_globales = {};
    intervalle_adresses_pile_exécution = {};

    m_métaprogramme = nullptr;

    de->instructions_exécutées += compte_exécutées;
    if (compilatrice.arguments.profile_metaprogrammes) {
        de->profileuse.ajoute_échantillon(métaprogramme, compte_exécutées);
    }
}

#define INSTRUCTIONS_PAR_LOT 1000

MachineVirtuelle::RésultatInterprétation MachineVirtuelle::exécute_instructions(
    int &compte_exécutées)
{
    auto frame = &frames[profondeur_appel - 1];

    for (auto i = 0; i < INSTRUCTIONS_PAR_LOT; ++i) {
        /* sauvegarde le pointeur si compilatrice_attend_message n'a pas encore de messages */
        auto pointeur_debut = frame->pointeur;
        auto instruction = LIS_OCTET();

        switch (instruction) {
            case OP_VÉRIFIE_CIBLE_BRANCHE:
            {
                auto const décalage = LIS_4_OCTETS();
                auto const limites = donne_limite_code_frame(frame);
                auto const adresse_finale = limites.adresse_début + décalage;
                if (est_hors_limites(limites, adresse_finale)) {
                    rapporte_erreur_exécution(
                        "Branche vers une destination hors des limites de la fonction");
                    return RésultatInterprétation::ERREUR;
                }
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
            case OP_VÉRIFIE_CIBLE_BRANCHE_CONDITION:
            {
                auto const décalage_si_vrai = LIS_4_OCTETS();
                auto const limites = donne_limite_code_frame(frame);
                auto const adresse_finale_si_vrai = limites.adresse_début + décalage_si_vrai;
                if (est_hors_limites(limites, adresse_finale_si_vrai)) {
                    rapporte_erreur_exécution(
                        "Branche vers une destination hors des limites de la fonction");
                    return RésultatInterprétation::ERREUR;
                }
                auto const décalage_si_faux = LIS_4_OCTETS();
                auto const adresse_finale_si_faux = limites.adresse_début + décalage_si_faux;
                if (est_hors_limites(limites, adresse_finale_si_faux)) {
                    rapporte_erreur_exécution(
                        "Branche vers une destination hors des limites de la fonction");
                    return RésultatInterprétation::ERREUR;
                }
                break;
            }
            case OP_BRANCHE_CONDITION:
            {
                auto decalage_si_vrai = LIS_4_OCTETS();
                auto decalage_si_faux = LIS_4_OCTETS();
                auto condition = dépile<bool>();

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
            case OP_BRANCHE_SI_ZÉRO:
            {
                auto taille = LIS_4_OCTETS();
                auto decalage_si_vrai = LIS_4_OCTETS();
                auto decalage_si_faux = LIS_4_OCTETS();

                auto condition = false;
                switch (taille) {
                    case 1:
                    {
                        auto valeur = dépile<int8_t>();
                        condition = valeur == 0;
                        break;
                    }
                    case 2:
                    {
                        auto valeur = dépile<int16_t>();
                        condition = valeur == 0;
                        break;
                    }
                    case 4:
                    {
                        auto valeur = dépile<int32_t>();
                        condition = valeur == 0;
                        break;
                    }
                    case 8:
                    {
                        auto valeur = dépile<int64_t>();
                        condition = valeur == 0;
                        break;
                    }
                    default:
                    {
                        assert(false);
                        break;
                    }
                }
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
                empile_constante(frame);
                break;
            }
            case OP_STRUCTURE_CONSTANTE:
            {
                auto taille_structure = LIS_4_OCTETS();

                auto source = frame->pointeur;
                auto destination = this->pointeur_pile;
                memcpy(destination, source, size_t(taille_structure));

                frame->pointeur += taille_structure;
                incrémente_pointeur_de_pile(taille_structure);
                break;
            }
            case OP_REMBOURRAGE:
            {
                auto rembourrage = LIS_4_OCTETS();
                incrémente_pointeur_de_pile(rembourrage);
                break;
            }
            case OP_INCRÉMENTE:
            {
                auto taille = LIS_4_OCTETS();

                if (taille == 1) {
                    auto valeur = dépile<uint8_t>();
                    empile(valeur + 1);
                }
                else if (taille == 2) {
                    auto valeur = dépile<uint16_t>();
                    empile(valeur + 1);
                }
                else if (taille == 4) {
                    auto valeur = dépile<uint32_t>();
                    empile(valeur + 1);
                }
                else {
                    auto valeur = dépile<uint64_t>();
                    empile(valeur + 1);
                }

                break;
            }
            case OP_INCRÉMENTE_LOCALE:
            {
                auto taille = LIS_4_OCTETS();
                auto index = LIS_4_OCTETS();
                auto adresse_variable = donne_adresse_locale(frame, index);
                if (taille == 1) {
                    *reinterpret_cast<uint8_t *>(adresse_variable) += 1;
                }
                else if (taille == 2) {
                    *reinterpret_cast<uint16_t *>(adresse_variable) += 1;
                }
                else if (taille == 4) {
                    *reinterpret_cast<uint32_t *>(adresse_variable) += 1;
                }
                else {
                    *reinterpret_cast<uint64_t *>(adresse_variable) += 1;
                }
                break;
            }
            case OP_DÉCRÉMENTE:
            {
                auto taille = LIS_4_OCTETS();

                if (taille == 1) {
                    auto valeur = dépile<uint8_t>();
                    empile(valeur - 1);
                }
                else if (taille == 2) {
                    auto valeur = dépile<uint16_t>();
                    empile(valeur - 1);
                }
                else if (taille == 4) {
                    auto valeur = dépile<uint32_t>();
                    empile(valeur - 1);
                }
                else {
                    auto valeur = dépile<uint64_t>();
                    empile(valeur - 1);
                }

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
                OP_BINAIRE(Addition)
                break;
            }
            case OP_SOUSTRAIT:
            {
                OP_BINAIRE(Soustraction)
                break;
            }
            case OP_MULTIPLIE:
            {
                OP_BINAIRE(Multiplication)
                break;
            }
            case OP_DIVISE:
            {
                OP_BINAIRE_NATUREL(Division)
                break;
            }
            case OP_DIVISE_RELATIF:
            {
                OP_BINAIRE(Division)
                break;
            }
            case OP_AJOUTE_REEL:
            {
                OP_BINAIRE_REEL(Addition)
                break;
            }
            case OP_SOUSTRAIT_REEL:
            {
                OP_BINAIRE_REEL(Soustraction)
                break;
            }
            case OP_MULTIPLIE_REEL:
            {
                OP_BINAIRE_REEL(Multiplication)
                break;
            }
            case OP_DIVISE_REEL:
            {
                OP_BINAIRE_REEL(Division)
                break;
            }
            case OP_RESTE_NATUREL:
            {
                OP_BINAIRE_NATUREL(Modulo)
                break;
            }
            case OP_RESTE_RELATIF:
            {
                OP_BINAIRE(Modulo)
                break;
            }
            case OP_COMP_EGAL:
            {
                OP_BINAIRE(Égal)
                break;
            }
            case OP_COMP_INEGAL:
            {
                OP_BINAIRE(Différent)
                break;
            }
            case OP_COMP_INF:
            {
                OP_BINAIRE(Inférieur)
                break;
            }
            case OP_COMP_INF_EGAL:
            {
                OP_BINAIRE(InférieurÉgal)
                break;
            }
            case OP_COMP_SUP:
            {
                OP_BINAIRE(Supérieur)
                break;
            }
            case OP_COMP_SUP_EGAL:
            {
                OP_BINAIRE(SupérieurÉgal)
                break;
            }
            case OP_COMP_INF_NATUREL:
            {
                OP_BINAIRE_NATUREL(Inférieur)
                break;
            }
            case OP_COMP_INF_EGAL_NATUREL:
            {
                OP_BINAIRE_NATUREL(InférieurÉgal)
                break;
            }
            case OP_COMP_SUP_NATUREL:
            {
                OP_BINAIRE_NATUREL(Supérieur)
                break;
            }
            case OP_COMP_SUP_EGAL_NATUREL:
            {
                OP_BINAIRE_NATUREL(SupérieurÉgal)
                break;
            }
            case OP_COMP_EGAL_REEL:
            {
                OP_BINAIRE_REEL(Égal)
                break;
            }
            case OP_COMP_INEGAL_REEL:
            {
                OP_BINAIRE_REEL(Différent)
                break;
            }
            case OP_COMP_INF_REEL:
            {
                OP_BINAIRE_REEL(Inférieur)
                break;
            }
            case OP_COMP_INF_EGAL_REEL:
            {
                OP_BINAIRE_REEL(InférieurÉgal)
                break;
            }
            case OP_COMP_SUP_REEL:
            {
                OP_BINAIRE_REEL(Supérieur)
                break;
            }
            case OP_COMP_SUP_EGAL_REEL:
            {
                OP_BINAIRE_REEL(SupérieurÉgal)
                break;
            }
            case OP_ET_BINAIRE:
            {
                OP_BINAIRE(ConjonctionBinaire)
                break;
            }
            case OP_OU_BINAIRE:
            {
                OP_BINAIRE(DisjonctionBinaire)
                break;
            }
            case OP_OU_EXCLUSIF:
            {
                OP_BINAIRE(DisjonctionBinaireExclusive)
                break;
            }
            case OP_DEC_GAUCHE:
            {
                OP_BINAIRE(DécalageGauche)
                break;
            }
            case OP_DEC_DROITE_ARITHM:
            {
                OP_BINAIRE(DécalageDroite)
                break;
            }
            case OP_DEC_DROITE_LOGIQUE:
            {
                OP_BINAIRE_NATUREL(DécalageDroite)
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
                    auto v = dépile<float>();
                    empile(static_cast<double>(v));
                }

                break;
            }
            case OP_DIMINUE_REEL:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();

                if (taille_de == 8 && taille_vers == 4) {
                    auto v = dépile<double>();
                    empile(static_cast<float>(v));
                }

                break;
            }
            case OP_NATUREL_VERS_REEL:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                TRANSTYPE_EVR(uint8_t)
                TRANSTYPE_EVR(uint16_t)
                TRANSTYPE_EVR(uint32_t)
                TRANSTYPE_EVR(int64_t)
                break;
            }
            case OP_RELATIF_VERS_REEL:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                TRANSTYPE_EVR(int8_t)
                TRANSTYPE_EVR(int16_t)
                TRANSTYPE_EVR(int32_t)
                TRANSTYPE_EVR(int64_t)
                break;
            }
            case OP_REEL_VERS_NATUREL:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                TRANSTYPE_RVE(float, uint8_t, uint16_t, uint32_t, uint64_t)
                TRANSTYPE_RVE(double, uint8_t, uint16_t, uint32_t, uint64_t)
                break;
            }
            case OP_REEL_VERS_RELATIF:
            {
                auto taille_de = LIS_4_OCTETS();
                auto taille_vers = LIS_4_OCTETS();
                TRANSTYPE_RVE(float, int8_t, int16_t, int32_t, int64_t)
                TRANSTYPE_RVE(double, int8_t, int16_t, int32_t, int64_t)
                break;
            }
            case OP_RETOURNE:
            {
                /* ATTENTION : si ceci change il faudra ajourner OP_LOGUE_SORTIES et
                 * OP_LOGUE_RETOUR. */
                auto type_fonction = frame->fonction->type->comme_type_fonction();
                auto taille_retour = static_cast<int>(type_fonction->type_sortie->taille_octet);
                auto pointeur_debut_retour = pointeur_pile - taille_retour;

                profondeur_appel--;

                if (profondeur_appel == 0) {
                    /* Nous retournons de la fonction principale du métaprogramme. */
                    if (pointeur_pile != pile) {
                        pointeur_pile = pointeur_debut_retour;
                    }

                    compte_exécutées = i + 1;
                    return RésultatInterprétation::TERMINÉ;
                }

                /* Restaure le pointeur_pile pour la frame précédente. */
                pointeur_pile = frame->pointeur_pile;

                /* Empile le résultat de la fonction. */
                if (taille_retour != 0 && pointeur_pile != pointeur_debut_retour) {
                    memcpy(pointeur_pile,
                           pointeur_debut_retour,
                           static_cast<unsigned>(taille_retour));
                }

                incrémente_pointeur_de_pile(taille_retour);

                /* Reprend l'exécution à la frame précédente. */
                frame = &frames[profondeur_appel - 1];
                break;
            }
            case OP_VÉRIFIE_CIBLE_APPEL:
            {
                auto est_pointeur = LIS_OCTET();
                AtomeFonction *ptr_fonction = nullptr;
                if (est_pointeur) {
                    /* Ne perturbe pas notre pile. */
                    auto ptr = this->pointeur_pile;
                    auto adresse = ::dépile<void *>(ptr);
                    ptr_fonction = reinterpret_cast<AtomeFonction *>(adresse);
                }
                else {
                    ptr_fonction = LIS_POINTEUR(AtomeFonction);
                }
                if (vérifie_cible_appel(ptr_fonction) != RésultatInterprétation::OK) {
                    compte_exécutées = i + 1;
                    return RésultatInterprétation::ERREUR;
                }
                break;
            }
            case OP_APPEL:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);
                auto taille_argument = LIS_4_OCTETS();

                if (!appel_fonction_interne(ptr_fonction, taille_argument, frame)) {
                    compte_exécutées = i + 1;
                    return RésultatInterprétation::ERREUR;
                }

                break;
            }
            case OP_APPEL_EXTERNE:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);
                auto taille_argument = LIS_4_OCTETS();
                auto ptr_inst_appel = LIS_POINTEUR(InstructionAppel);
                auto résultat = RésultatInterprétation::OK;
                empile_fonction_non_interne(ptr_fonction);
                appel_fonction_externe(ptr_fonction, taille_argument, ptr_inst_appel, résultat);
                if (résultat == RésultatInterprétation::ERREUR) {
                    return résultat;
                }
                dépile_fonction_non_interne(ptr_fonction);
                break;
            }
            case OP_APPEL_COMPILATRICE:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);

                auto résultat = RésultatInterprétation::OK;
                empile_fonction_non_interne(ptr_fonction);
                appel_fonction_compilatrice(ptr_fonction, résultat);
                dépile_fonction_non_interne(ptr_fonction);

                if (résultat == RésultatInterprétation::PASSE_AU_SUIVANT ||
                    résultat == RésultatInterprétation::TERMINÉ) {
                    frame->pointeur = pointeur_debut;
                    return résultat;
                }

                break;
            }
            case OP_APPEL_INTRINSÈQUE:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);
                empile_fonction_non_interne(ptr_fonction);
                appel_fonction_intrinsèque(ptr_fonction);
                dépile_fonction_non_interne(ptr_fonction);
                break;
            }
            case OP_APPEL_POINTEUR:
            {
                auto taille_argument = LIS_4_OCTETS();
                auto valeur_inst = LIS_8_OCTETS();
                auto adresse = dépile<void *>();
                auto ptr_fonction = reinterpret_cast<AtomeFonction *>(adresse);
                auto ptr_inst_appel = reinterpret_cast<InstructionAppel *>(valeur_inst);

                if (ptr_fonction->decl && ptr_fonction->decl->possède_drapeau(
                                              DrapeauxNoeudFonction::EST_IPA_COMPILATRICE)) {
                    auto résultat = RésultatInterprétation::OK;
                    empile_fonction_non_interne(ptr_fonction);
                    appel_fonction_compilatrice(ptr_fonction, résultat);
                    dépile_fonction_non_interne(ptr_fonction);

                    if (résultat == RésultatInterprétation::PASSE_AU_SUIVANT ||
                        résultat == RésultatInterprétation::TERMINÉ) {
                        frame->pointeur = pointeur_debut;
                        compte_exécutées = i + 1;
                        return résultat;
                    }
                }
                else if (ptr_fonction->est_externe) {
                    auto résultat = RésultatInterprétation::OK;
                    empile_fonction_non_interne(ptr_fonction);
                    appel_fonction_externe(
                        ptr_fonction, taille_argument, ptr_inst_appel, résultat);
                    dépile_fonction_non_interne(ptr_fonction);
                    if (résultat == RésultatInterprétation::ERREUR) {
                        return résultat;
                    }
                }
                else {
                    if (!appel_fonction_interne(ptr_fonction, taille_argument, frame)) {
                        compte_exécutées = i + 1;
                        return RésultatInterprétation::ERREUR;
                    }
                }

                break;
            }
            case OP_VÉRIFIE_ADRESSAGE_ASSIGNE:
            {
                auto taille = LIS_4_OCTETS();

                /* Ne perturbe pas notre pile. */
                auto ptr = this->pointeur_pile;
                auto adresse_ou = ::dépile<void *>(ptr);
                auto adresse_de = static_cast<void *>(ptr - taille);

                if (!adressage_est_possible(adresse_ou, adresse_de, taille, true)) {
                    compte_exécutées = i + 1;
                    return RésultatInterprétation::ERREUR;
                }

                break;
            }
            case OP_ASSIGNE:
            {
                auto taille = LIS_4_OCTETS();
                auto adresse_ou = dépile<void *>();
                auto adresse_de = static_cast<void *>(this->pointeur_pile - taille);
                memcpy(adresse_ou, adresse_de, static_cast<size_t>(taille));
                décrémente_pointeur_de_pile(taille);
                break;
            }
            case OP_ASSIGNE_LOCALE:
            {
                auto index = LIS_4_OCTETS();
                auto taille = LIS_4_OCTETS();

                auto adresse_ou = donne_adresse_locale(frame, index);
                auto adresse_de = static_cast<void *>(this->pointeur_pile - taille);
                memcpy(adresse_ou, adresse_de, static_cast<size_t>(taille));

                décrémente_pointeur_de_pile(taille);
                break;
            }
            case OP_INIT_LOCALE_ZÉRO:
            {
                auto index = LIS_4_OCTETS();
                auto taille = LIS_4_OCTETS();
                auto adresse_ou = donne_adresse_locale(frame, index);

                switch (taille) {
                    case 1:
                        *reinterpret_cast<int8_t *>(adresse_ou) = 0;
                        break;
                    case 2:
                        *reinterpret_cast<int16_t *>(adresse_ou) = 0;
                        break;
                    case 4:
                        *reinterpret_cast<int32_t *>(adresse_ou) = 0;
                        break;
                    case 8:
                        *reinterpret_cast<int64_t *>(adresse_ou) = 0;
                        break;
                    default:
                    {
                        assert(false);
                        break;
                    }
                }

                break;
            }
            case OP_COPIE_LOCALE:
            {
                auto taille = LIS_4_OCTETS();
                auto indice_source = LIS_4_OCTETS();
                auto indice_destination = LIS_4_OCTETS();

                auto adresse_source = donne_adresse_locale(frame, indice_source);
                auto adresse_destination = donne_adresse_locale(frame, indice_destination);

                memcpy(adresse_destination, adresse_source, static_cast<size_t>(taille));
                break;
            }
            case OP_VÉRIFIE_ADRESSAGE_CHARGE:
            {
                auto taille = LIS_4_OCTETS();

                /* Ne perturbe pas notre pile. */
                auto ptr = this->pointeur_pile;
                auto adresse_de = ::dépile<void *>(ptr);
                auto adresse_ou = static_cast<void *>(ptr);

                if (!adressage_est_possible(adresse_ou, adresse_de, taille, false)) {
                    compte_exécutées = i + 1;
                    return RésultatInterprétation::ERREUR;
                }

                break;
            }
            case OP_CHARGE:
            {
                auto taille = LIS_4_OCTETS();
                auto adresse_de = dépile<void *>();
                auto adresse_ou = static_cast<void *>(this->pointeur_pile);
                memcpy(adresse_ou, adresse_de, static_cast<size_t>(taille));
                incrémente_pointeur_de_pile(taille);
                break;
            }
            case OP_CHARGE_LOCALE:
            {
                auto index = LIS_4_OCTETS();
                auto taille = LIS_4_OCTETS();

                auto adresse_de = donne_adresse_locale(frame, index);
                auto adresse_ou = static_cast<void *>(this->pointeur_pile);
                memcpy(adresse_ou, adresse_de, static_cast<size_t>(taille));

                incrémente_pointeur_de_pile(taille);
                break;
            }
            case OP_RÉFÉRENCE_LOCALE:
            {
                auto index = LIS_4_OCTETS();
                empile(donne_adresse_locale(frame, index));
                break;
            }
            case OP_REFERENCE_GLOBALE:
            {
                auto index = LIS_4_OCTETS();
                auto const &globale = données_constantes->globales[index];
                empile(&ptr_données_globales[globale.adresse]);
                break;
            }
            case OP_REFERENCE_GLOBALE_EXTERNE:
            {
                auto adresse = LIS_8_OCTETS();
                empile(adresse);
                break;
            }
            case OP_REFERENCE_RUBRIQUE:
            {
                auto decalage = LIS_4_OCTETS();
                auto adresse_de = dépile<char *>();
                empile(adresse_de + decalage);
                // dbg() << "adresse_de : " << static_cast<void *>(adresse_de);
                break;
            }
            case OP_RÉFÉRENCE_RUBRIQUE_LOCALE:
            {
                auto pointeur = LIS_4_OCTETS();
                auto décalage = LIS_4_OCTETS();
                auto adresse_base = donne_adresse_locale(frame, pointeur);
                auto adresse_rubrique = static_cast<char *>(adresse_base) + décalage;
                empile(adresse_rubrique);
                break;
            }
            case OP_ACCÈDE_INDICE:
            {
                auto taille_données = LIS_4_OCTETS();
                auto adresse = dépile<char *>();
                auto index = dépile<int64_t>();
                auto nouvelle_adresse = adresse + index * taille_données;
                empile(nouvelle_adresse);
                // dbg() << "nouvelle_adresse : " << static_cast<void *>(nouvelle_adresse) << '\n'
                //       << "index            : " << index << '\n'
                //       << "taille_données   : " << taille_données;
                break;
            }
            case OP_STAT_INSTRUCTION:
            {
                /* L'opération est directement après nous. Il ne faut pas incémenter le pointeur.
                 */
                auto op = *frame->pointeur;
                m_métaprogramme->données_exécution->compte_instructions[op] += 1;
                m_métaprogramme->données_exécution->compte_instructions[OP_STAT_INSTRUCTION] += 1;
                break;
            }
            case OP_LOGUE_INSTRUCTION:
            {
                auto décalage = LIS_4_OCTETS();
                auto &chunk = frame->fonction->données_exécution->chunk;
                auto &logueuse = m_métaprogramme->donne_logueuse(
                    TypeLogMétaprogramme::INSTRUCTION);
                logueuse << chaine_indentations(profondeur_appel);
                désassemble_instruction(chunk, décalage, logueuse);
                break;
            }
            case OP_LOGUE_VALEURS_LOCALES:
            {
                auto &logueuse = m_métaprogramme->donne_logueuse(TypeLogMétaprogramme::APPEL);
                imprime_valeurs_locales(frame, profondeur_appel, logueuse);
                break;
            }
            case OP_LOGUE_APPEL:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);
                auto &logueuse = m_métaprogramme->donne_logueuse(TypeLogMétaprogramme::APPEL);
                logueuse << "-- appel : " << ptr_fonction->nom << " ("
                         << chaine_type(ptr_fonction->type) << ')' << '\n';
                break;
            }
            case OP_LOGUE_ENTRÉES:
            {
                auto ptr_fonction = LIS_POINTEUR(AtomeFonction);
                auto taille_arguments = LIS_4_OCTETS();
                auto pointeur_arguments = pointeur_pile - taille_arguments;
                auto &logueuse = m_métaprogramme->donne_logueuse(TypeLogMétaprogramme::APPEL);
                imprime_valeurs_entrées(
                    pointeur_arguments, ptr_fonction, profondeur_appel, logueuse);
                break;
            }
            case OP_LOGUE_SORTIES:
            {
                auto type_fonction = frame->fonction->type->comme_type_fonction();
                auto taille_retour = static_cast<int>(type_fonction->type_sortie->taille_octet);
                auto pointeur_debut_retour = pointeur_pile - taille_retour;
                auto &logueuse = m_métaprogramme->donne_logueuse(TypeLogMétaprogramme::APPEL);
                imprime_valeurs_sorties(
                    pointeur_debut_retour, frame->fonction, profondeur_appel, logueuse);
                break;
            }
            case OP_LOGUE_RETOUR:
            {
                auto type_fonction = frame->fonction->type->comme_type_fonction();
                auto taille_retour = static_cast<int>(type_fonction->type_sortie->taille_octet);
                auto &logueuse = m_métaprogramme->donne_logueuse(TypeLogMétaprogramme::APPEL);
                logueuse << "Retourne, décalage : "
                         << static_cast<int>(frame->pointeur_pile - pile) << '\n';
                logueuse << "Empile " << taille_retour << " octet(s), décalage : "
                         << static_cast<int>(frame->pointeur_pile + taille_retour - pile) << '\n';
                break;
            }
            case OP_NOTIFIE_DÉPILAGE_VALEUR:
            {
                auto taille_données = LIS_4_OCTETS();
                auto résultat = notifie_dépile(frame, frame->pointeur, uint32_t(taille_données));
                if (résultat != RésultatInterprétation::OK) {
                    return résultat;
                }
                break;
            }
            case OP_NOTIFIE_EMPILAGE_VALEUR:
            {
                auto taille_données = LIS_4_OCTETS();
                notifie_empile(frame, frame->pointeur, uint32_t(taille_données));
                break;
            }
            case OP_PROFILE_DÉBUTE_APPEL:
            {
                auto de = m_métaprogramme->données_exécution;
                de->profondeur_appel = profondeur_appel;
                de->profileuse.ajoute_échantillon(m_métaprogramme, 1);
                break;
            }
            case OP_PROFILE_TERMINE_APPEL:
            {
                auto de = m_métaprogramme->données_exécution;
                /* La profondeur d'appel fut modifiée par dépile_fonction_non_interne ou par les
                 * retours, donc nous devons l'ajuster ici. */
                de->profondeur_appel = profondeur_appel + 1;
                de->profileuse.ajoute_échantillon(m_métaprogramme, 1);
                de->profondeur_appel = profondeur_appel;
                break;
            }
            case OP_INATTEIGNABLE:
            {
                rapporte_erreur_exécution("Erreur : l'exécution du métaprogramme atteint une "
                                          "instruction inatteignable.");
                compte_exécutées = i + 1;
                return RésultatInterprétation::ERREUR;
            }
            case OP_SÉLECTION:
            {
                auto taille_arguments = LIS_4_OCTETS();
                auto valeur_condition = dépile<bool>();
                auto pointeur_si_vrai = pointeur_pile - taille_arguments;
                auto pointeur_si_faux = pointeur_pile - taille_arguments * 2;

                if (valeur_condition) {
                    /* Copie le vrai sur le faux. */
                    memcpy(
                        pointeur_si_faux, pointeur_si_vrai, static_cast<size_t>(taille_arguments));
                }

                /* Nous ne supprimons qu'un seul argument. */
                décrémente_pointeur_de_pile(taille_arguments);
                break;
            }
            default:
            {
                rapporte_erreur_exécution("Erreur interne : Opération inconnue dans la MV !");
                compte_exécutées = i + 1;
                return RésultatInterprétation::ERREUR;
            }
        }
    }

    compte_exécutées = INSTRUCTIONS_PAR_LOT;
    return RésultatInterprétation::OK;
}

void MachineVirtuelle::imprime_trace_appel(NoeudExpression const *site)
{
    dbg() << erreur::imprime_site(*m_métaprogramme->unité->espace, site);
    for (int i = profondeur_appel - 1; i >= 0; --i) {
        dbg() << erreur::imprime_site(*m_métaprogramme->unité->espace, frames[i].site);
    }
}

void MachineVirtuelle::rapporte_erreur_exécution(kuri::chaine_statique message)
{
    auto const site = donne_site_adresse_courante();
    auto e = m_métaprogramme->unité->espace->rapporte_erreur(site, message);

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
    résultat.réserve(profondeur_appel);

    for (int i = 0; i < profondeur_appel; i++) {
        résultat.ajoute(frames[i]);
    }

    return résultat;
}

void MachineVirtuelle::notifie_empile(FrameAppel *frame, octet_t *adresse_op, uint32_t taille)
{
    m_métaprogramme->données_exécution->tailles_empilées.empile({frame, adresse_op, taille});
    auto &logueuse = m_métaprogramme->donne_logueuse(TypeLogMétaprogramme::PILE_DE_VALEURS);
    logueuse << profondeur_appel << 'e' << taille << '\n';
}

MachineVirtuelle::RésultatInterprétation MachineVirtuelle::notifie_dépile(FrameAppel *frame,
                                                                          octet_t *adresse_op,
                                                                          uint32_t taille)
{
    auto données_exécution_ = m_métaprogramme->données_exécution;

    auto &logueuse = m_métaprogramme->donne_logueuse(TypeLogMétaprogramme::PILE_DE_VALEURS);
    logueuse << profondeur_appel << 'd' << taille << '\n';

    if (données_exécution_->tailles_empilées.est_vide()) {
        m_métaprogramme->préserve_log_empilage();

        auto message = enchaine("Dans l'exécution du métaprogramme \"",
                                m_métaprogramme->donne_nom_pour_fichier_log(),
                                "\" : tentative de dépiler ",
                                taille,
                                " octets alors que la pile d'exécution est vide.");
        rapporte_erreur_exécution(message);
        return RésultatInterprétation::ERREUR;
    }

    auto données = m_métaprogramme->données_exécution->tailles_empilées.depile();

    if (données.taille != taille) {
        m_métaprogramme->préserve_log_empilage();

        auto site_empilage = données.frame->fonction->données_exécution->chunk
                                 .donne_site_pour_adresse(données.adresse);
        auto chaine_site_empilage = erreur::imprime_site(*m_métaprogramme->unité->espace,
                                                         site_empilage);

        auto message = enchaine("Dans l'exécution du métaprogramme \"",
                                m_métaprogramme->donne_nom_pour_fichier_log(),
                                "\" : tentative de dépiler ",
                                taille,
                                " octets alors que la dernière taille empilée était de ",
                                données.taille,
                                "\nNOTE : l'empilage fut fait ici : ",
                                chaine_site_empilage,
                                "\n",
                                "\nNOTE : le pointeur programme est au déalage : ",
                                frame->pointeur - frame->fonction->données_exécution->chunk.code,
                                "\n");
        rapporte_erreur_exécution(message);
        return RésultatInterprétation::ERREUR;
    }

    return RésultatInterprétation::OK;
}

NoeudExpression const *MachineVirtuelle::donne_site_adresse_courante() const
{
    auto frame = &frames[profondeur_appel - 1];
    /* Fonction externe. */
    if (frame->fonction->est_externe) {
        frame--;
    }
    return frame->fonction->données_exécution->chunk.donne_site_pour_adresse(frame->pointeur);
}

bool MachineVirtuelle::adressage_est_possible(const void *adresse_ou,
                                              const void *adresse_de,
                                              const int64_t taille,
                                              bool assignation)
{
    auto const site = donne_site_adresse_courante();
    auto const taille_disponible = std::abs(static_cast<const char *>(adresse_de) -
                                            static_cast<const char *>(adresse_ou));
    if (taille_disponible < taille) {
        auto message = assignation ? "Erreur interne : superposition de la copie dans la "
                                     "machine virtuelle lors d'une assignation !" :
                                     "Erreur interne : superposition de la copie dans la "
                                     "machine virtuelle lors d'un chargement !";
        m_métaprogramme->unité->espace->rapporte_erreur(site, message)
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
        rapporte_erreur_exécution(message);
        return false;
    }

    if (adresse_est_nulle(adresse_ou)) {
        auto message = assignation ? "Assignation vers une adresse nulle !" :
                                     "Chargement vers une adresse nulle !";
        rapporte_erreur_exécution(message);
        return false;
    }

    if (!assignation) {
        if (!adresse_est_assignable(adresse_ou)) {
            m_métaprogramme->unité->espace
                ->rapporte_erreur(site, "Copie vers une adresse non-assignable !")
                .ajoute_message("L'adresse est : ", adresse_ou, "\n");
            return false;
        }

#if 0
        // À FAIRE : il nous faudrait les adresses des messages, des noeuds codes, etc.
        if (!adresse_est_assignable(adresse_de)) {
            m_métaprogramme->unite->espace
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
    return intervalle_adresses_globales.possède_inclusif(adresse) ||
           intervalle_adresses_pile_exécution.possède_inclusif(adresse);
}

MachineVirtuelle::RésultatInterprétation MachineVirtuelle::vérifie_cible_appel(
    AtomeFonction *ptr_fonction)
{
    if (!m_métaprogramme->cibles_appels.possède(ptr_fonction)) {
        auto const site = donne_site_adresse_courante();
        auto espace = m_métaprogramme->unité->espace;
        espace
            ->rapporte_erreur(site,
                              "Alors que j'exécute un métaprogramme, je rencontre une instruction "
                              "d'appel vers une fonction ne faisant pas partie du métaprogramme.")
            .ajoute_message("Il est possible que l'adresse de la fonction soit invalide : ",
                            static_cast<void *>(ptr_fonction),
                            "\n")
            .ajoute_donnees([&](Erreur &e) { ajoute_trace_appel(e); });
        return RésultatInterprétation::ERREUR;
    }

    return RésultatInterprétation::OK;
}

void MachineVirtuelle::ajoute_métaprogramme(MetaProgramme *métaprogramme)
{
    métaprogramme->état = ÉtatMétaprogramme::EN_EXÉCUTION;
    /* Appel le métaprogramme pour initialiser sa frame d'appels, l'installation et la
     * désinstallation ajournement les données d'exécution. */
    installe_métaprogramme(métaprogramme);
    appel(métaprogramme->fonction->atome->comme_fonction(), métaprogramme->directive);
    désinstalle_métaprogramme(métaprogramme, 0);
    m_métaprogrammes.ajoute(métaprogramme);
}

void MachineVirtuelle::exécute_métaprogrammes_courants()
{
    /* efface la liste de métaprogrammes dont l'exécution est finie */
    if (m_métaprogrammes_terminés_lu) {
        m_métaprogrammes_terminés.efface();
        m_métaprogrammes_terminés_lu = false;
    }

    if (terminee()) {
        return;
    }

    auto nombre_métaprogrammes = m_métaprogrammes.taille();

    kuri::chrono::compte_seconde chrono_exec;

    for (auto i = 0; i < nombre_métaprogrammes; ++i) {
        auto métaprogramme = m_métaprogrammes[i];

        assert(métaprogramme->données_exécution->profondeur_appel >= 1);

        installe_métaprogramme(métaprogramme);

        int compte_exécutées = 0;
        auto res = exécute_instructions(compte_exécutées);

        désinstalle_métaprogramme(métaprogramme, compte_exécutées);

        if (res == RésultatInterprétation::PASSE_AU_SUIVANT) {
            // RÀF
        }
        else if (res == RésultatInterprétation::ERREUR) {
            métaprogramme->résultat = MetaProgramme::RésultatExécution::ERREUR;
            m_métaprogrammes_terminés.ajoute(métaprogramme);
            std::swap(m_métaprogrammes[i], m_métaprogrammes[nombre_métaprogrammes - 1]);
            nombre_métaprogrammes -= 1;
            i -= 1;
        }
        else if (res == RésultatInterprétation::TERMINÉ) {
            métaprogramme->résultat = MetaProgramme::RésultatExécution::SUCCÈS;
            m_métaprogrammes_terminés.ajoute(métaprogramme);
            std::swap(m_métaprogrammes[i], m_métaprogrammes[nombre_métaprogrammes - 1]);
            nombre_métaprogrammes -= 1;
            i -= 1;

            if (compilatrice.arguments.émets_stats_ops_exécution) {
                logue_stats_instructions(métaprogramme);
            }

            if (compilatrice.arguments.profile_metaprogrammes) {
                auto &profileuse = métaprogramme->données_exécution->profileuse;
                profileuse.crée_rapport(métaprogramme,
                                        compilatrice.arguments.format_rapport_profilage);
            }
        }

        if (stop || compilatrice.possède_erreur()) {
            break;
        }
    }

    m_métaprogrammes.redimensionne(nombre_métaprogrammes);

    temps_exécution_métaprogammes += chrono_exec.temps();

    nombre_de_métaprogrammes_exécutés += m_métaprogrammes_terminés.taille();
}

DonnéesExécution *MachineVirtuelle::loge_données_exécution()
{
    if (!m_données_exécution_libres.est_vide()) {
        auto résultat = m_données_exécution_libres.dernier_élément();
        m_données_exécution_libres.supprime_dernier();
        résultat->réinitialise();
        return résultat;
    }

    auto données = données_exécution.ajoute_élément();
    données->pile = mémoire::loge_tableau<octet_t>("MachineVirtuelle::pile", TAILLE_PILE);
    données->pointeur_pile = données->pile;
    return données;
}

void MachineVirtuelle::déloge_données_exécution(DonnéesExécution *&données)
{
    if (!données) {
        return;
    }

    instructions_exécutées += données->instructions_exécutées;

    m_données_exécution_libres.ajoute(données);

    données = nullptr;
}

void MachineVirtuelle::rassemble_statistiques(Statistiques &stats)
{
    stats.ajoute_mémoire_utilisée("Machine Virtuelle", données_exécution.mémoire_utilisée());
    stats.nombre_métaprogrammes_exécutés += nombre_de_métaprogrammes_exécutés;
    stats.temps_métaprogrammes += temps_exécution_métaprogammes;
    stats.instructions_exécutées += instructions_exécutées;
}

std::ostream &operator<<(std::ostream &os, PatchDonnéesConstantes const &patch)
{
    os << "Patch données constantes :\n";

    os << "-- destination         : ";
    if (patch.destination.type == DONNÉES_CONSTANTES) {
        os << "données constantes\n";
    }
    else if (patch.destination.type == DONNÉES_GLOBALES) {
        os << "données globales\n";
    }
    else {
        os << "code fonction " << patch.destination.fonction->nom << '\n';
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

void Profileuse::réinitialise()
{
    échantillons.efface();
}

void Profileuse::prépare_pour_profilage()
{
    ajourne_ticks();
}

void Profileuse::ajourne_ticks()
{
    ticks_de_bases = __rdtsc();
}

uint64_t Profileuse::donne_ticks()
{
    return __rdtsc() - ticks_de_bases;
}

static bool les_frames_sont_les_mêmes(FrameAppel const *frame1,
                                      int taille1,
                                      FrameAppel const *frame2,
                                      int taille2)
{
    if (taille1 != taille2) {
        return false;
    }

    for (int i = 0; i < taille1; ++i) {
        if (frame1[i].fonction != frame2[i].fonction) {
            return false;
        }
    }

    return true;
}

void Profileuse::ajoute_échantillon(MetaProgramme *métaprogramme, int poids)
{
    if (poids == 0) {
        return;
    }

    auto ticks = donne_ticks();

    auto de = métaprogramme->données_exécution;
    if (!échantillons.est_vide()) {
        auto &dernier_échantillon = échantillons.dernier_élément();
        auto profondeur_échantillon = dernier_échantillon.profondeur_frame_appel;

        if (les_frames_sont_les_mêmes(dernier_échantillon.frames,
                                      profondeur_échantillon,
                                      de->frames,
                                      de->profondeur_appel)) {
            dernier_échantillon.poids += ticks;
            return;
        }
    }

    auto echantillon = ÉchantillonProfilage();
    echantillon.profondeur_frame_appel = de->profondeur_appel;
    echantillon.poids = ticks;

    for (int i = 0; i < echantillon.profondeur_frame_appel; i++) {
        echantillon.frames[i] = de->frames[i];
    }

    échantillons.ajoute(echantillon);

    ajourne_ticks();
}

static void imprime_nom_fonction(AtomeFonction const *fonction, Enchaineuse &os)
{
    if (fonction->decl) {
        os << nom_humainement_lisible(fonction->decl);
    }
    else {
        os << fonction->nom;
    }
}

static void crée_rapport_format_echantillons_total_plus_fonction(
    kuri::tableau_statique<ÉchantillonProfilage> échantillons, Enchaineuse &os)
{
    auto table = kuri::table_hachage<AtomeFonction *, int>("Échantillons profilage");
    auto fonctions = kuri::ensemble<AtomeFonction *>();

    POUR (échantillons) {
        for (int i = 0; i < it.profondeur_frame_appel; i++) {
            auto &frame = it.frames[i];
            auto valeur = table.valeur_ou(frame.fonction, 0);
            table.insère(frame.fonction, valeur + 1);

            fonctions.insère(frame.fonction);
        }
    }

    auto fonctions_et_echantillons = kuri::tableau<PaireÉchantillonFonction>();

    fonctions.pour_chaque_element([&](AtomeFonction *fonction) {
        auto nombre_échantillons = table.valeur_ou(fonction, 0);
        auto paire = PaireÉchantillonFonction{fonction, nombre_échantillons};
        fonctions_et_echantillons.ajoute(paire);
    });

    std::sort(fonctions_et_echantillons.begin(),
              fonctions_et_echantillons.end(),
              [](auto &a, auto &b) { return a.nombre_échantillons > b.nombre_échantillons; });

    POUR (fonctions_et_echantillons) {
        os << it.nombre_échantillons << " : ";
        imprime_nom_fonction(it.fonction, os);
        os << '\n';
    }
}

static void crée_rapport_format_brendan_gregg(
    kuri::tableau_statique<ÉchantillonProfilage> échantillons, Enchaineuse &os)
{
    POUR (échantillons) {
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

void Profileuse::crée_rapport(MetaProgramme *métaprogramme, FormatRapportProfilage format)
{
    auto &logueuse = métaprogramme->donne_logueuse(TypeLogMétaprogramme::PROFILAGE);

    switch (format) {
        case FormatRapportProfilage::ECHANTILLONS_TOTAL_POUR_FONCTION:
        {
            crée_rapport_format_echantillons_total_plus_fonction(échantillons, logueuse);
            break;
        }
        case FormatRapportProfilage::BRENDAN_GREGG:
        {
            crée_rapport_format_brendan_gregg(échantillons, logueuse);
            break;
        }
    }
}
