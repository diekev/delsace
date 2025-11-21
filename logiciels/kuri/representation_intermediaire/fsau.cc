/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "fsau.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "utilitaires/algorithmes.hh"
#include "utilitaires/log.hh"
#include "utilitaires/type_opaque.hh"

#include "parsage/identifiant.hh"

#include "structures/enchaineuse.hh"
#include "structures/ensemble.hh"

#include "analyse.hh"
#include "bloc_basique.hh"
#include "constructrice_ri.hh"
#include "impression.hh"
#include "instructions.hh"

#define INSTRUCTION_NON_IMPLEMENTEE                                                               \
    assert_rappel(false, [&]() { dbg() << "Instruction non-gérée " << inst->genre; })

#define ATOME_NON_IMPLEMENTE                                                                      \
    assert_rappel(false, [&]() { dbg() << "Atome non-géré " << atome->genre_atome; })

// CONSTRUCTION FSAU https://pp.ipd.kit.edu/uploads/publikationen/braun13cc.pdf
// ARRAY FSAU https://dl.acm.org/doi/pdf/10.1145/268946.268956

namespace FSAU {

static void rièrevertis_en_ri(FonctionEtBlocs &fonction_et_blocs, ConstructriceRI &constructrice);

CREE_TYPE_OPAQUE(indice_table_utilisateur, int32_t);
static const indice_table_utilisateur indice_utilisateur_invalide = indice_table_utilisateur(-1);

CREE_TYPE_OPAQUE(indice_table_bloc, int32_t);
static const indice_table_bloc indice_bloc_invalide = indice_table_bloc(-1);

CREE_TYPE_OPAQUE(indice_table_relation, int32_t);
static const indice_table_relation indice_relation_invalide = indice_table_relation(-1);

struct TableDesRelations {
    struct Index {
        indice_table_utilisateur premier_utilisateur = indice_utilisateur_invalide;
        indice_table_bloc premier_bloc = indice_bloc_invalide;
    };
    kuri::tableau<Index, int32_t> m_index{};

    struct UtilisateurValeur {
        Valeur *utilisateur = nullptr;
        indice_table_utilisateur précédent = indice_utilisateur_invalide;
        indice_table_utilisateur suivant = indice_utilisateur_invalide;
    };
    kuri::tableau<UtilisateurValeur, int32_t> m_utilisateurs{};

  public:
    void remplace_ou_ajoute_utilisateur(Valeur *utilisée, Valeur *ancien, Valeur *par);
    void ajoute_utilisateur(Valeur *utilisée, Valeur *par);

    bool est_utilisée(const Valeur *valeur) const;

    void supprime(const Valeur *valeur);

    kuri::tablet<Valeur *, 6> donne_utilisateurs(Valeur const *valeur) const;

    void supprime_utilisateur(Valeur *utilisée, const Valeur *par);

  private:
    indice_table_relation donne_indice_pour_valeur(Valeur *valeur);

    void déconnecte(UtilisateurValeur *utilisateur);
    void supprime_utilisateur(indice_table_relation index, const Valeur *par);
};

#define ENUMERE_GENRE_VALEUR_SSA(O)                                                               \
    O(INDÉFINIE, ValeurIndéfinie, indéfinie)                                                      \
    O(CONSTANTE, ValeurConstante, constante)                                                      \
    O(FONCTION, ValeurFonction, fonction)                                                         \
    O(GLOBALE, ValeurGlobale, globale)                                                            \
    O(LOCALE, ValeurLocale, locale)                                                               \
    O(ADRESSE_DE, ValeurAdresseDe, adresse_de)                                                    \
    O(APPEL, ValeurAppel, appel)                                                                  \
    O(ACCÈS_RUBRIQUE, ValeurAccèsRubrique, accès_rubrique)                                        \
    O(ACCÈS_INDICE, ValeurAccèsIndice, accès_indice)                                               \
    O(ÉCRIS_INDICE, ValeurÉcrisIndice, écris_index)                                               \
    O(OPÉRATEUR_BINAIRE, ValeurOpérateurBinaire, opérateur_binaire)                               \
    O(OPÉRATEUR_UNAIRE, ValeurOpérateurUnaire, opérateur_unaire)                                  \
    O(BRANCHE, ValeurBranche, branche)                                                            \
    O(BRANCHE_COND, ValeurBrancheCond, branche_cond)                                              \
    O(RETOUR, ValeurRetour, retour)                                                               \
    O(PHI, NoeudPhi, phi)                                                                         \
    O(TRANSTYPAGE, ValeurTranstypage, transtypage)

enum class GenreValeur : uint8_t {
#define ENUMERE_GENRE_VALEUR_SSA_EX(genre, nom_classe, ident) genre,
    ENUMERE_GENRE_VALEUR_SSA(ENUMERE_GENRE_VALEUR_SSA_EX)
#undef ENUMERE_GENRE_VALEUR_SSA_EX
};

static std::ostream &operator<<(std::ostream &os, GenreValeur genre)
{
#define IMPRIME_GENRE_VALEUR(genre_, nom_classe, ident)                                           \
    case GenreValeur::genre_:                                                                     \
    {                                                                                             \
        os << #ident;                                                                             \
        break;                                                                                    \
    }

    switch (genre) {
        ENUMERE_GENRE_VALEUR_SSA(IMPRIME_GENRE_VALEUR)
    }

#undef IMPRIME_GENRE_VALEUR
    return os;
}

#define PRODECLARE_VALEURS(genre, nom_classe, ident) struct nom_classe;
ENUMERE_GENRE_VALEUR_SSA(PRODECLARE_VALEURS)
#undef PRODECLARE_VALEURS

#define RUBRIQUE_VALEUR(nom)                                                                      \
  private:                                                                                        \
    Valeur *nom = nullptr;                                                                        \
                                                                                                  \
  public:                                                                                         \
    void définis_##nom(TableDesRelations &table, Valeur *v)                                       \
    {                                                                                             \
        auto ancien_##nom = nom;                                                                  \
        nom = v;                                                                                  \
        table.remplace_ou_ajoute_utilisateur(v, ancien_##nom, this);                              \
    }                                                                                             \
    Valeur *donne_##nom() const                                                                   \
    {                                                                                             \
        return nom;                                                                               \
    }

#define DECLARE_FONCTIONS_DISCRIMINATION(genre_, nom_classe, ident)                               \
    inline bool est_##ident() const                                                               \
    {                                                                                             \
        return genre == GenreValeur::genre_;                                                      \
    }                                                                                             \
    nom_classe *comme_##ident();                                                                  \
    nom_classe const *comme_##ident() const;

#define DEFINIS_FONCTIONS_DISCRIMINATION(genre_, nom_classe, ident)                               \
    nom_classe *Valeur::comme_##ident()                                                           \
    {                                                                                             \
        assert_rappel(est_##ident(),                                                              \
                      [&]() { dbg() << "La valeur est de genre " << this->genre; });              \
        return static_cast<nom_classe *>(this);                                                   \
    }                                                                                             \
    nom_classe const *Valeur::comme_##ident() const                                               \
    {                                                                                             \
        assert_rappel(est_##ident(),                                                              \
                      [&]() { dbg() << "La valeur est de genre " << this->genre; });              \
        return static_cast<nom_classe const *>(this);                                             \
    }

#define CONSTRUCTEUR_VALEUR(nom_classe, nom_genre)                                                \
    nom_classe()                                                                                  \
    {                                                                                             \
        genre = GenreValeur::nom_genre;                                                           \
    }                                                                                             \
    EMPECHE_COPIE(nom_classe)

enum class DrapeauxValeur : uint8_t {
    ZÉRO,
    EST_UTILISÉE = (1u << 0),
    PARTICIPE_AU_FLOT_DU_PROGRAMME = (1u << 1),
    /* Pour les accès index ou rubrique qui ne crée pas de nouvelle valeur mais qui modifie
     * simplement leurs accédés. */
    NE_PRODUIS_PAS_DE_VALEUR = (1u << 2),
    ÉCRIS_INDICE_EST_SURÉCRIS = (1u << 3),
};
DEFINIS_OPERATEURS_DRAPEAU(DrapeauxValeur)

static std::ostream &operator<<(std::ostream &os, DrapeauxValeur drapeaux)
{
    if (drapeaux == DrapeauxValeur::ZÉRO) {
        os << "AUCUN";
        return os;
    }

#define SI_DRAPEAU_UTILISE(drapeau)                                                               \
    if ((drapeaux & DrapeauxValeur::drapeau) != DrapeauxValeur::ZÉRO) {                           \
        identifiants.ajoute(#drapeau);                                                            \
    }

    kuri::tablet<kuri::chaine_statique, 32> identifiants;

    SI_DRAPEAU_UTILISE(EST_UTILISÉE)
    SI_DRAPEAU_UTILISE(PARTICIPE_AU_FLOT_DU_PROGRAMME)
    SI_DRAPEAU_UTILISE(NE_PRODUIS_PAS_DE_VALEUR)

    auto virgule = "";

    POUR (identifiants) {
        os << virgule << it;
        virgule = " | ";
    }

#undef SI_DRAPEAU_UTILISE

    return os;
}

enum class DrapeauxRemplacement : uint32_t {
    AUCUN = 0,
    IGNORE_PHI = (1u << 0),
    IGNORE_ÉCRIS_INDICE = (1u << 1),
    IGNORE_ACCÈS_INDICE = (1u << 2),
};
DEFINIS_OPERATEURS_DRAPEAU(DrapeauxRemplacement)

static bool est_drapeau_actif(DrapeauxRemplacement const drapeaux,
                              DrapeauxRemplacement const drapeau)
{
    return (drapeaux & drapeau) != DrapeauxRemplacement::AUCUN;
}

static std::ostream &operator<<(std::ostream &os, DrapeauxRemplacement drapeaux)
{
    if (drapeaux == DrapeauxRemplacement::AUCUN) {
        os << "AUCUN";
        return os;
    }

#define SI_DRAPEAU_UTILISE(drapeau)                                                               \
    if ((drapeaux & DrapeauxRemplacement::drapeau) != DrapeauxRemplacement::AUCUN) {              \
        identifiants.ajoute(#drapeau);                                                            \
    }

    kuri::tablet<kuri::chaine_statique, 32> identifiants;

    SI_DRAPEAU_UTILISE(IGNORE_PHI)
    SI_DRAPEAU_UTILISE(IGNORE_ÉCRIS_INDICE)
    SI_DRAPEAU_UTILISE(IGNORE_ACCÈS_INDICE)

    auto virgule = "";

    POUR (identifiants) {
        os << virgule << it;
        virgule = " | ";
    }

#undef SI_DRAPEAU_UTILISE

    return os;
}

struct Valeur {
    GenreValeur genre{};
    DrapeauxValeur drapeaux = DrapeauxValeur::ZÉRO;
    uint32_t numéro = 0;
    indice_table_relation indice_relations = indice_relation_invalide;
    int32_t indice_bloc = -1;

    ENUMERE_GENRE_VALEUR_SSA(DECLARE_FONCTIONS_DISCRIMINATION)

    inline bool est_controle_de_flux() const
    {
        return this->est_branche() || this->est_branche_cond() || this->est_retour();
    }

    void remplace_par(TableDesRelations &table,
                      Valeur *valeur,
                      DrapeauxRemplacement const drapeaux_remplacement);

    inline bool possède_drapeau(DrapeauxValeur drapeau) const
    {
        return (drapeaux & drapeau) != DrapeauxValeur::ZÉRO;
    }

  private:
    void remplace_dans_utilisateur(TableDesRelations &table, Valeur *utilisateur, Valeur *par);
};

#undef DECLARE_FONCTIONS_DISCRIMINATION

struct NoeudPhi : public Valeur {
    Bloc *bloc = nullptr;
    kuri::tableau<Valeur *> opérandes{};

    CONSTRUCTEUR_VALEUR(NoeudPhi, PHI);

    void ajoute_opérande(TableDesRelations &table, Valeur *valeur);

    void définis_opérande(TableDesRelations &table, int index, Valeur *v);

    void supprime_opérande(Valeur *valeur);

    [[nodiscard]] kuri::tableau<Valeur *> supprime_utilisateur(TableDesRelations &table,
                                                               Valeur *utilisateur);
};

struct ValeurIndéfinie : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurIndéfinie, INDÉFINIE);
};

struct ValeurLocale : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurLocale, LOCALE);

    InstructionAllocation const *alloc = nullptr;
    RUBRIQUE_VALEUR(valeur)
};

struct ValeurAdresseDe : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurAdresseDe, ADRESSE_DE);

    Atome const *de = nullptr;
    RUBRIQUE_VALEUR(valeur);
};

struct ValeurBranche : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurBranche, BRANCHE);

    InstructionBranche const *inst = nullptr;
};

struct ValeurBrancheCond : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurBrancheCond, BRANCHE_COND);

    RUBRIQUE_VALEUR(condition)
    InstructionBrancheCondition const *inst = nullptr;
};

struct ValeurRetour : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurRetour, RETOUR);

    RUBRIQUE_VALEUR(valeur)
};

struct ValeurConstante : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurConstante, CONSTANTE);

    AtomeConstante const *atome = nullptr;
};

struct ValeurFonction : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurFonction, FONCTION);

    AtomeFonction const *atome = nullptr;
};

struct ValeurGlobale : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurGlobale, GLOBALE);

    AtomeGlobale const *atome = nullptr;
};

struct ValeurOpérateurBinaire : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurOpérateurBinaire, OPÉRATEUR_BINAIRE);

    RUBRIQUE_VALEUR(gauche)
    RUBRIQUE_VALEUR(droite)
    InstructionOpBinaire const *inst = nullptr;
};

struct ValeurOpérateurUnaire : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurOpérateurUnaire, OPÉRATEUR_UNAIRE);

    RUBRIQUE_VALEUR(droite)
};

struct ValeurAccèsRubrique : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurAccèsRubrique, ACCÈS_RUBRIQUE);

    RUBRIQUE_VALEUR(accédée)
    int32_t indice = 0;

    InstructionAccèsRubrique const *inst = nullptr;
};

struct ValeurAccèsIndice : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurAccèsIndice, ACCÈS_INDICE);

    RUBRIQUE_VALEUR(accédée)
    RUBRIQUE_VALEUR(indice)
};

struct ValeurÉcrisIndice : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurÉcrisIndice, ÉCRIS_INDICE);

    RUBRIQUE_VALEUR(accédée)
    RUBRIQUE_VALEUR(indice)
    RUBRIQUE_VALEUR(valeur);
};

struct ValeurTranstypage : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurTranstypage, TRANSTYPAGE);

    RUBRIQUE_VALEUR(valeur);
    InstructionTranstype const *inst = nullptr;
};

struct ValeurAppel : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurAppel, APPEL);

    RUBRIQUE_VALEUR(valeur_appelée)

  private:
    kuri::tableau<Valeur *> arguments{};

  public:
    void ajoute_argument(TableDesRelations &table, Valeur *v)
    {
        arguments.ajoute(v);
        table.ajoute_utilisateur(v, this);
    }

    void définis_argument(TableDesRelations &table, int index, Valeur *v)
    {
        auto ancien = arguments[index];
        arguments[index] = v;
        table.remplace_ou_ajoute_utilisateur(v, ancien, this);
    }

    kuri::tableau_statique<Valeur *> donne_arguments() const
    {
        return arguments;
    }
};

void NoeudPhi::ajoute_opérande(TableDesRelations &table, Valeur *valeur)
{
    opérandes.ajoute(valeur);
    table.ajoute_utilisateur(valeur, this);
}

void NoeudPhi::supprime_opérande(Valeur *valeur)
{
    auto partition = kuri::partition_stable(opérandes, [&](Valeur *v) { return v != valeur; });
    assert(partition.faux.taille() == 1);
    opérandes.redimensionne(partition.vrai.taille());
}

void NoeudPhi::définis_opérande(TableDesRelations &table, int index, Valeur *v)
{
    auto ancien = opérandes[index];
    opérandes[index] = v;
    table.remplace_ou_ajoute_utilisateur(v, ancien, this);
}

kuri::tableau<Valeur *> NoeudPhi::supprime_utilisateur(TableDesRelations &table,
                                                       Valeur *utilisateur)
{
    auto utilisateurs = table.donne_utilisateurs(this);

    // dbg() << "[" << __func__ << "] : utilisateurs " << utilisateurs.taille();
    kuri::tableau<Valeur *> résultat;
    POUR (utilisateurs) {
        if (it == utilisateur) {
            continue;
        }

        résultat.ajoute(it);
    }

    return résultat;
}

void Valeur::remplace_par(TableDesRelations &table,
                          Valeur *valeur,
                          DrapeauxRemplacement const drapeaux_remplacement)
{
    auto utilisateurs = table.donne_utilisateurs(this);
    // dbg() << "[" << __func__ << "] : utilisateurs " << utilisateurs.taille();

    auto nombre_utilisateurs = utilisateurs.taille();

    POUR (utilisateurs) {
        if (it == this) {
            nombre_utilisateurs -= 1;
            continue;
        }

        if (est_drapeau_actif(drapeaux_remplacement, DrapeauxRemplacement::IGNORE_PHI) &&
            it->est_phi()) {
            continue;
        }

        if (est_drapeau_actif(drapeaux_remplacement, DrapeauxRemplacement::IGNORE_ACCÈS_INDICE) &&
            it->est_accès_indice()) {
            continue;
        }

        if (est_drapeau_actif(drapeaux_remplacement, DrapeauxRemplacement::IGNORE_ÉCRIS_INDICE) &&
            it->est_écris_index()) {
            continue;
        }

        remplace_dans_utilisateur(table, it, valeur);
        nombre_utilisateurs -= 1;
    }

    if (nombre_utilisateurs == 0) {
        table.supprime(this);
    }
}

void Valeur::remplace_dans_utilisateur(TableDesRelations &table, Valeur *utilisateur, Valeur *par)
{
    switch (utilisateur->genre) {
        case GenreValeur::INDÉFINIE:
        case GenreValeur::FONCTION:
        case GenreValeur::GLOBALE:
        case GenreValeur::CONSTANTE:
        case GenreValeur::BRANCHE:
        {
            break;
        }
        case GenreValeur::BRANCHE_COND:
        {
            auto branche = utilisateur->comme_branche_cond();
            assert(branche->donne_condition() == this);
            branche->définis_condition(table, par);
            break;
        }
        case GenreValeur::RETOUR:
        {
            auto retour = utilisateur->comme_retour();
            assert(retour->donne_valeur() == this);
            retour->définis_valeur(table, par);
            break;
        }
        case GenreValeur::LOCALE:
        {
            auto locale = utilisateur->comme_locale();
            assert(locale->donne_valeur() == this);
            locale->définis_valeur(table, par);
            break;
        }
        case GenreValeur::ADRESSE_DE:
        {
            auto adresse_de = utilisateur->comme_adresse_de();
            assert(adresse_de->donne_valeur() == this);
            adresse_de->définis_valeur(table, par);
            break;
        }
        case GenreValeur::ACCÈS_RUBRIQUE:
        {
            auto accès_rubrique = utilisateur->comme_accès_rubrique();
            assert(accès_rubrique->donne_accédée() == this);
            accès_rubrique->définis_accédée(table, par);
            break;
        }
        case GenreValeur::ACCÈS_INDICE:
        {
            auto accès_indice = utilisateur->comme_accès_indice();
            if (accès_indice->donne_accédée() == this) {
                accès_indice->définis_accédée(table, par);
            }
            if (accès_indice->donne_indice() == this) {
                accès_indice->définis_indice(table, par);
            }
            break;
        }
        case GenreValeur::ÉCRIS_INDICE:
        {
            auto écris_index = utilisateur->comme_écris_index();
            if (écris_index->donne_accédée() == this) {
                écris_index->définis_accédée(table, par);
            }
            if (écris_index->donne_indice() == this) {
                écris_index->définis_indice(table, par);
            }
            if (écris_index->donne_valeur() == this) {
                écris_index->définis_valeur(table, par);
            }
            break;
        }
        case GenreValeur::APPEL:
        {
            auto appel = utilisateur->comme_appel();
            if (appel->donne_valeur_appelée() == this) {
                appel->définis_valeur_appelée(table, par);
            }
            POUR_INDICE (appel->donne_arguments()) {
                if (it == this) {
                    appel->définis_argument(table, indice_it, par);
                }
            }
            break;
        }
        case GenreValeur::OPÉRATEUR_BINAIRE:
        {
            auto op_binaire = utilisateur->comme_opérateur_binaire();
            if (op_binaire->donne_gauche() == this) {
                op_binaire->définis_gauche(table, par);
            }
            if (op_binaire->donne_droite() == this) {
                op_binaire->définis_droite(table, par);
            }
            break;
        }
        case GenreValeur::OPÉRATEUR_UNAIRE:
        {
            auto op_unaire = utilisateur->comme_opérateur_unaire();
            if (op_unaire->donne_droite() == this) {
                op_unaire->définis_droite(table, par);
            }
            break;
        }
        case GenreValeur::PHI:
        {
            auto phi = utilisateur->comme_phi();

            POUR_INDICE (phi->opérandes) {
                if (it == this) {
                    phi->définis_opérande(table, indice_it, par);
                }
            }
            break;
        }
        case GenreValeur::TRANSTYPAGE:
        {
            auto transtypage = utilisateur->comme_transtypage();
            assert(transtypage->donne_valeur() == this);
            transtypage->définis_valeur(table, par);
            break;
        }
    }
}

static void visite_valeur(Valeur *valeur,
                          kuri::ensemble<Valeur *> &visitées,
                          std::function<void(Valeur *)> const &rappel)
{
    if (visitées.possède(valeur)) {
        return;
    }

    rappel(valeur);

    visitées.insère(valeur);

    switch (valeur->genre) {
        case GenreValeur::INDÉFINIE:
        case GenreValeur::FONCTION:
        case GenreValeur::GLOBALE:
        case GenreValeur::CONSTANTE:
        case GenreValeur::BRANCHE:
        {
            break;
        }
        case GenreValeur::BRANCHE_COND:
        {
            auto branche = valeur->comme_branche_cond();
            visite_valeur(branche->donne_condition(), visitées, rappel);
            break;
        }
        case GenreValeur::RETOUR:
        {
            auto retour = valeur->comme_retour();
            if (retour->donne_valeur()) {
                visite_valeur(retour->donne_valeur(), visitées, rappel);
            }
            break;
        }
        case GenreValeur::LOCALE:
        {
            auto locale = valeur->comme_locale();
            visite_valeur(locale->donne_valeur(), visitées, rappel);
            break;
        }
        case GenreValeur::ADRESSE_DE:
        {
            auto adresse_de = valeur->comme_adresse_de();
            visite_valeur(adresse_de->donne_valeur(), visitées, rappel);
            break;
        }
        case GenreValeur::ACCÈS_RUBRIQUE:
        {
            auto accès_rubrique = valeur->comme_accès_rubrique();
            visite_valeur(accès_rubrique->donne_accédée(), visitées, rappel);
            break;
        }
        case GenreValeur::ACCÈS_INDICE:
        {
            auto accès_indice = valeur->comme_accès_indice();
            visite_valeur(accès_indice->donne_accédée(), visitées, rappel);
            visite_valeur(accès_indice->donne_indice(), visitées, rappel);
            break;
        }
        case GenreValeur::ÉCRIS_INDICE:
        {
            auto écris_index = valeur->comme_écris_index();
            visite_valeur(écris_index->donne_accédée(), visitées, rappel);
            visite_valeur(écris_index->donne_indice(), visitées, rappel);
            visite_valeur(écris_index->donne_valeur(), visitées, rappel);
            break;
        }
        case GenreValeur::APPEL:
        {
            auto appel = valeur->comme_appel();
            visite_valeur(appel->donne_valeur_appelée(), visitées, rappel);
            POUR_INDICE (appel->donne_arguments()) {
                visite_valeur(it, visitées, rappel);
            }
            break;
        }
        case GenreValeur::OPÉRATEUR_BINAIRE:
        {
            auto op_binaire = valeur->comme_opérateur_binaire();
            visite_valeur(op_binaire->donne_gauche(), visitées, rappel);
            visite_valeur(op_binaire->donne_droite(), visitées, rappel);
            break;
        }
        case GenreValeur::OPÉRATEUR_UNAIRE:
        {
            auto op_unaire = valeur->comme_opérateur_unaire();
            visite_valeur(op_unaire->donne_droite(), visitées, rappel);
            break;
        }
        case GenreValeur::PHI:
        {
            auto phi = valeur->comme_phi();
            POUR_INDICE (phi->opérandes) {
                visite_valeur(it, visitées, rappel);
            }
            break;
        }
        case GenreValeur::TRANSTYPAGE:
        {
            auto transtypage = valeur->comme_transtypage();
            visite_valeur(transtypage->donne_valeur(), visitées, rappel);
            break;
        }
    }
}

static void visite_opérande(Valeur *valeur, std::function<void(Valeur *)> const &rappel)
{
    switch (valeur->genre) {
        case GenreValeur::INDÉFINIE:
        case GenreValeur::FONCTION:
        case GenreValeur::GLOBALE:
        case GenreValeur::CONSTANTE:
        case GenreValeur::BRANCHE:
        {
            break;
        }
        case GenreValeur::BRANCHE_COND:
        {
            auto branche = valeur->comme_branche_cond();
            rappel(branche->donne_condition());
            break;
        }
        case GenreValeur::RETOUR:
        {
            auto retour = valeur->comme_retour();
            rappel(retour->donne_valeur());
            break;
        }
        case GenreValeur::LOCALE:
        {
            auto locale = valeur->comme_locale();
            rappel(locale->donne_valeur());
            break;
        }
        case GenreValeur::ADRESSE_DE:
        {
            auto adresse_de = valeur->comme_adresse_de();
            rappel(adresse_de->donne_valeur());
            break;
        }
        case GenreValeur::ACCÈS_RUBRIQUE:
        {
            auto accès_rubrique = valeur->comme_accès_rubrique();
            rappel(accès_rubrique->donne_accédée());
            break;
        }
        case GenreValeur::ACCÈS_INDICE:
        {
            auto accès_indice = valeur->comme_accès_indice();
            rappel(accès_indice->donne_accédée());
            rappel(accès_indice->donne_indice());
            break;
        }
        case GenreValeur::ÉCRIS_INDICE:
        {
            auto écris_index = valeur->comme_écris_index();
            rappel(écris_index->donne_accédée());
            rappel(écris_index->donne_indice());
            rappel(écris_index->donne_valeur());
            break;
        }
        case GenreValeur::APPEL:
        {
            auto appel = valeur->comme_appel();
            rappel(appel->donne_valeur_appelée());
            POUR_INDICE (appel->donne_arguments()) {
                rappel(it);
            }
            break;
        }
        case GenreValeur::OPÉRATEUR_BINAIRE:
        {
            auto op_binaire = valeur->comme_opérateur_binaire();
            rappel(op_binaire->donne_gauche());
            rappel(op_binaire->donne_droite());
            break;
        }
        case GenreValeur::OPÉRATEUR_UNAIRE:
        {
            auto op_unaire = valeur->comme_opérateur_unaire();
            rappel(op_unaire->donne_droite());
            break;
        }
        case GenreValeur::PHI:
        {
            auto phi = valeur->comme_phi();
            POUR_INDICE (phi->opérandes) {
                rappel(it);
            }
            break;
        }
        case GenreValeur::TRANSTYPAGE:
        {
            auto transtypage = valeur->comme_transtypage();
            rappel(transtypage->donne_valeur());
            break;
        }
    }
}

ENUMERE_GENRE_VALEUR_SSA(DEFINIS_FONCTIONS_DISCRIMINATION)
#undef DEFINIS_FONCTIONS_DISCRIMINATION

static void imprime_nom_valeur(Valeur const *valeur, Enchaineuse &sortie)
{
    sortie << "v" << valeur->numéro;

    if (valeur->numéro == 0) {
        sortie << " (" << valeur->genre << ")";
    }
}

static void imprime_tableau(kuri::tableau_statique<Valeur *> tableau,
                            kuri::chaine_statique caractère_début,
                            kuri::chaine_statique caractère_fin,
                            Enchaineuse &sortie)
{
    auto virgule = caractère_début;

    POUR (tableau) {
        sortie << virgule;
        imprime_nom_valeur(it, sortie);
        virgule = ", ";
    }

    if (tableau.taille() == 0) {
        sortie << caractère_début;
    }
    sortie << caractère_fin;
}

static void imprime_valeur(Valeur const *valeur, Enchaineuse &sortie)
{
    if (valeur->numéro != 0 &&
        !valeur->possède_drapeau(DrapeauxValeur::NE_PRODUIS_PAS_DE_VALEUR)) {
        sortie << "v" << valeur->numéro << " = ";
    }

    switch (valeur->genre) {
        case GenreValeur::INDÉFINIE:
        {
            sortie << "indéfinie";
            break;
        }
        case GenreValeur::FONCTION:
        {
            sortie << "fonction";
            break;
        }
        case GenreValeur::GLOBALE:
        {
            sortie << "globale";
            break;
        }
        case GenreValeur::CONSTANTE:
        {
            auto constante = valeur->comme_constante();
            sortie << imprime_atome(constante->atome);
            break;
        }
        case GenreValeur::LOCALE:
        {
            auto locale = valeur->comme_locale();
            auto alloc = locale->alloc;
            imprime_nom_valeur(locale->donne_valeur(), sortie);

            sortie << " (" << (alloc->ident ? alloc->ident->nom : "tmp") << ")";
            break;
        }
        case GenreValeur::ADRESSE_DE:
        {
            auto adresse_de = valeur->comme_adresse_de();
            sortie << "adresse_de ";
            imprime_nom_valeur(adresse_de->donne_valeur(), sortie);
            break;
        }
        case GenreValeur::BRANCHE:
        {
            auto branche = valeur->comme_branche();
            sortie << "br %" << branche->inst->label->id;
            break;
        }
        case GenreValeur::BRANCHE_COND:
        {
            auto branche = valeur->comme_branche_cond();
            sortie << "si ";
            imprime_nom_valeur(branche->donne_condition(), sortie);
            sortie << " alors %" << branche->inst->label_si_vrai->id << " sinon %"
                   << branche->inst->label_si_faux->id;
            break;
        }
        case GenreValeur::RETOUR:
        {
            auto retour = valeur->comme_retour();
            sortie << "ret";
            if (retour->donne_valeur()) {
                sortie << " ";
                imprime_nom_valeur(retour->donne_valeur(), sortie);
            }
            break;
        }
        case GenreValeur::ACCÈS_RUBRIQUE:
        {
            auto accès_rubrique = valeur->comme_accès_rubrique();
            sortie << "rubrique ";
            imprime_nom_valeur(accès_rubrique->donne_accédée(), sortie);
            break;
        }
        case GenreValeur::ACCÈS_INDICE:
        {
            auto accès_indice = valeur->comme_accès_indice();
            sortie << "index ";
            imprime_nom_valeur(accès_indice->donne_accédée(), sortie);
            sortie << "[";
            imprime_nom_valeur(accès_indice->donne_indice(), sortie);
            sortie << "]";
            break;
        }
        case GenreValeur::ÉCRIS_INDICE:
        {
            auto écris_index = valeur->comme_écris_index();
            sortie << "écris_index(";
            imprime_nom_valeur(écris_index->donne_accédée(), sortie);
            sortie << ", ";
            imprime_nom_valeur(écris_index->donne_indice(), sortie);
            sortie << ", ";
            imprime_nom_valeur(écris_index->donne_valeur(), sortie);
            sortie << ")";
            break;
        }
        case GenreValeur::APPEL:
        {
            auto appel = valeur->comme_appel();
            imprime_nom_valeur(appel->donne_valeur_appelée(), sortie);
            imprime_tableau(appel->donne_arguments(), "(", ")", sortie);
            break;
        }
        case GenreValeur::OPÉRATEUR_BINAIRE:
        {
            auto op_binaire = valeur->comme_opérateur_binaire();
            imprime_nom_valeur(op_binaire->donne_gauche(), sortie);
            sortie << " " << donne_chaine_lexème_pour_op_binaire(op_binaire->inst->op) << " ";
            imprime_nom_valeur(op_binaire->donne_droite(), sortie);
            break;
        }
        case GenreValeur::OPÉRATEUR_UNAIRE:
        {
            auto op_unaire = valeur->comme_opérateur_unaire();
            sortie << "op ";
            imprime_nom_valeur(op_unaire->donne_droite(), sortie);
            break;
        }
        case GenreValeur::PHI:
        {
            auto phi = valeur->comme_phi();
            sortie << "phi ";
            imprime_tableau(phi->opérandes, "<", ">", sortie);
            // imprime_tableau(phi->utilisateurs, " (", ")", sortie);
            break;
        }
        case GenreValeur::TRANSTYPAGE:
        {
            auto transtypage = valeur->comme_transtypage();
            sortie << "transtype ";
            imprime_nom_valeur(transtypage->donne_valeur(), sortie);
            break;
        }
    }
}

static kuri::chaine imprime_valeurs(kuri::tableau_statique<Valeur *> valeurs)
{
    Enchaineuse sortie;

    POUR (valeurs) {
        sortie << "  ";
        imprime_valeur(it, sortie);
        // sortie << " [" << it->drapeaux << "]";
        sortie << "\n";
    }

    return sortie.chaine();
}

}  // namespace FSAU

using namespace FSAU;

struct ConvertisseuseFSAU {
  private:
    struct DéfinitionVariableBloc {
        Atome const *variable = nullptr;
        Bloc *bloc = nullptr;
        Valeur *valeur = nullptr;
    };

    kuri::tableau<DéfinitionVariableBloc> m_définitions_courantes{};

    struct PhiIncomplet {
        Bloc *bloc = nullptr;
        Atome const *variable = nullptr;
        NoeudPhi *résultat = nullptr;
    };

    kuri::tableau<PhiIncomplet> phis_incomplets{};

    uint32_t nombre_valeurs = 0;

#define ENUMERE_GENRE_VALEUR_SSA_EX(genre, nom_classe, ident)                                     \
    kuri::tableau_page<FSAU::nom_classe> m_##ident{};
    ENUMERE_GENRE_VALEUR_SSA(ENUMERE_GENRE_VALEUR_SSA_EX)
#undef ENUMERE_GENRE_VALEUR_SSA_EX

    kuri::tableau_page<FSAU::ValeurConstante> m_constante_entières{};
    kuri::tableau_page<FSAU::ValeurConstante> m_constante_booléennes{};

    ConstructriceRI &m_constructrice;

    TableDesRelations m_table_relations{};

  public:
    ConvertisseuseFSAU(ConstructriceRI &constructrice_ri) : m_constructrice(constructrice_ri)
    {
    }

    [[nodiscard]] Valeur *génère_valeur_pour_instruction(Bloc *bloc,
                                                         Instruction const *inst,
                                                         const UtilisationAtome utilisation);

    TableDesRelations &donne_table_des_relations()
    {
        return m_table_relations;
    }

  public:
    // algorithme 1 : local value numbering

    void écris_variable(Atome const *variable, Bloc *bloc, Valeur *valeur)
    {
        POUR (m_définitions_courantes) {
            if (it.variable == variable && it.bloc == bloc) {
                it.valeur = valeur;
                return;
            }
        }

        auto définition = DéfinitionVariableBloc{variable, bloc, valeur};
        m_définitions_courantes.ajoute(définition);
    }

    Valeur *lis_variable(Atome const *variable, Bloc *bloc)
    {
        bool variable_rencontrée = false;
        /* Trouve un numérotage local de la valeur. */
        POUR (m_définitions_courantes) {
            if (it.variable != variable) {
                continue;
            }
            variable_rencontrée = true;
            if (it.bloc == bloc) {
                return it.valeur;
            }
        }
        assert(variable_rencontrée);

        /* Trouve un numérotage global de la valeur. */
        return lis_variable_récursif(variable, bloc);
    }

    // algorithme 2 : global value numbering
    Valeur *lis_variable_récursif(Atome const *variable, Bloc *bloc)
    {
        Valeur *résultat = nullptr;

        if (!bloc->possède_drapeau(DrapeauxBlocBasique::EST_SCELLÉ)) {
            // dbg() << "[" << __func__ << "] :
            // !bloc->possède_drapeau(DrapeauxBlocBasique::EST_SCELLÉ)";
            /* Graphe de controle incomplet. */
            auto phi = crée_noeud_phi(bloc);
            ajoute_phi_incomplet(bloc, variable, phi);
            résultat = phi;
        }
        else if (bloc->parents.taille() == 1) {
            // dbg() << "[" << __func__ << "] : bloc->parents.taille() == 1";
            /* Optimisation du cas commun d'un ancêtre : aucun Phi n'est nécessaire. */
            résultat = lis_variable(variable, bloc->parents[0]);
        }
        else {
            // dbg() << "[" << __func__ << "] : else";
            /* Brise les cycles potentiels avec des phis sans-opérandes. */
            auto phi = crée_noeud_phi(bloc);
            écris_variable(variable, bloc, phi);
            résultat = ajoute_opérande_phi(variable, phi);
        }

        écris_variable(variable, bloc, résultat);
        return résultat;
    }

    Valeur *ajoute_opérande_phi(Atome const *variable, NoeudPhi *phi)
    {
        // dbg() << __func__;
        POUR (phi->bloc->parents) {
            phi->ajoute_opérande(m_table_relations, lis_variable(variable, it));
        }

        return tente_suppression_phi_trivial(phi);
    }

    // algorithm 3 : détection et suppression récursive des phi triviaux

    Valeur *tente_suppression_phi_trivial(NoeudPhi *phi)
    {
        Valeur *same = nullptr;

        POUR_NOMME (op, phi->opérandes) {
            if (op == same || op == phi) {
                // dbg() << "[" << __func__ << "] : op == same || op == phi";
                /* Valeur unique ou auto-référence. */
                continue;
            }

            if (same != nullptr) {
                // dbg() << "[" << __func__ << "] : same != nullptr";
                /* Le phi fusionne au moins 2 valeurs : non-trivial. */
                return phi;
            }

            same = op;
        }

        if (same == nullptr) {
            /* Le phi est inatteignable ou dans le bloc de début. */
            same = new ValeurIndéfinie();
        }

        /* Remémore tous les utilisateurs sauf le phi lui-même. */
        auto users = phi->supprime_utilisateur(m_table_relations, phi);

        /* Dévie toutes les utilisations de phi vers same et supprime phi. */
        phi->remplace_par(m_table_relations, same, DrapeauxRemplacement::AUCUN);

        /* Essaie de supprimer tous les utilisateurs de phi, qui peuvent être devenus triviaux. */
        POUR_NOMME (use, users) {
            if (use->genre == FSAU::GenreValeur::PHI) {
                tente_suppression_phi_trivial(use->comme_phi());
            }
        }

        return same;
    }

    void scelle_bloc(Bloc *bloc)
    {
        if (bloc->possède_drapeau(DrapeauxBlocBasique::EST_SCELLÉ)) {
            return;
        }

        // dbg() << __func__ << " : phis_incomplets " << phis_incomplets.taille();
        POUR (phis_incomplets) {
            if (it.bloc != bloc) {
                continue;
            }

            ajoute_opérande_phi(it.variable, it.résultat);
        }

        bloc->drapeaux |= DrapeauxBlocBasique::EST_SCELLÉ;
    }

    // --------------------------------------------

  private:
    NoeudPhi *crée_noeud_phi(Bloc *bloc)
    {
        auto résultat = m_phi.ajoute_élément();
        résultat->bloc = bloc;
        return résultat;
    }

    void ajoute_phi_incomplet(Bloc *bloc, Atome const *variable, NoeudPhi *valeur)
    {
        phis_incomplets.ajoute({bloc, variable, valeur});
    }

    void ajoute_valeur_au_bloc(Valeur *v, Bloc *bloc)
    {
        assert(v->numéro == 0);
        v->numéro = ++nombre_valeurs;
        bloc->valeurs.ajoute(v);
    }

    Valeur *donne_valeur_pour_atome(Bloc *bloc,
                                    Atome const *atome,
                                    UtilisationAtome const utilisation)
    {
        switch (atome->genre_atome) {
            case Atome::Genre::GLOBALE:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::FONCTION:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::INSTRUCTION:
            {
                return génère_valeur_pour_instruction(
                    bloc, atome->comme_instruction(), utilisation);
            }
            case Atome::Genre::ACCÈS_INDICE_CONSTANT:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::TRANSTYPE_CONSTANT:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::NON_INITIALISATION:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::INITIALISATION_TABLEAU:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::CONSTANTE_TAILLE_DE:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::CONSTANTE_STRUCTURE:
            {
                auto résultat = m_constante.ajoute_élément();
                ajoute_valeur_au_bloc(résultat, bloc);
                return résultat;
            }
            case Atome::Genre::CONSTANTE_CARACTÈRE:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::CONSTANTE_NULLE:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::CONSTANTE_BOOLÉENNE:
            {
                auto constante_booléenne = atome->comme_constante_booléenne();

                POUR_TABLEAU_PAGE (m_constante_booléennes) {
                    auto existante = it.atome->comme_constante_booléenne();
                    if (existante->type == constante_booléenne->type &&
                        existante->valeur == constante_booléenne->valeur) {
                        return &it;
                    }
                }
                auto résultat = m_constante_booléennes.ajoute_élément();
                résultat->atome = constante_booléenne;
                ajoute_valeur_au_bloc(résultat, bloc);
                return résultat;
            }
            case Atome::Genre::CONSTANTE_RÉELLE:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::CONSTANTE_ENTIÈRE:
            {
                auto constante_entière = atome->comme_constante_entière();

                POUR_TABLEAU_PAGE (m_constante_entières) {
                    auto existante = it.atome->comme_constante_entière();
                    if (existante->type == constante_entière->type &&
                        existante->valeur == constante_entière->valeur) {
                        return &it;
                    }
                }
                auto résultat = m_constante_entières.ajoute_élément();
                résultat->atome = constante_entière;
                ajoute_valeur_au_bloc(résultat, bloc);
                return résultat;
            }
        }

        return nullptr;
    }

  private:
    Valeur *crée_opérateur_binaire();
};

#define DEBOGUE_UTILISATION_INSTRUCTION                                                           \
    if (inst->est_alloc()) {                                                                      \
        auto alloc = inst->comme_alloc();                                                         \
        dbg() << __func__ << " : " << inst->genre << " ("                                         \
              << (alloc->ident ? alloc->ident->nom : "tmp") << ")"                                \
              << ", " << utilisation;                                                             \
    }                                                                                             \
    else {                                                                                        \
        dbg() << __func__ << " : " << inst->genre << ", " << utilisation;                         \
    }

Valeur *ConvertisseuseFSAU::génère_valeur_pour_instruction(Bloc *bloc,
                                                           Instruction const *inst,
                                                           UtilisationAtome const utilisation)
{
    switch (inst->genre) {
        case GenreInstruction::LABEL:
        {
            return nullptr;
        }
        case GenreInstruction::BRANCHE:
        {
            auto branche = m_branche.ajoute_élément();
            branche->inst = inst->comme_branche();
            bloc->valeurs.ajoute(branche);
            return nullptr;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto inst_branche = inst->comme_branche_cond();
            auto branche = m_branche_cond.ajoute_élément();
            branche->inst = inst_branche;
            auto valeur = donne_valeur_pour_atome(
                bloc, inst_branche->condition, UtilisationAtome::POUR_BRANCHE_CONDITION);
            branche->définis_condition(m_table_relations, valeur);
            bloc->valeurs.ajoute(branche);
            return nullptr;
        }
        case GenreInstruction::APPEL:
        {
            INSTRUCTION_NON_IMPLEMENTEE;
            // écris_variable
            break;
        }
        case GenreInstruction::ALLOCATION:
        {
            DEBOGUE_UTILISATION_INSTRUCTION

            auto alloc = inst->comme_alloc();

            if (est_drapeau_actif(utilisation, UtilisationAtome::POUR_DESTINATION_ÉCRITURE)) {
                /* Nous devons créer une nouvelle version de la variable. */
                auto locale = m_locale.ajoute_élément();
                locale->alloc = alloc;
                ajoute_valeur_au_bloc(locale, bloc);
                écris_variable(alloc, bloc, locale);
                return locale;
            }

            if (est_drapeau_actif(utilisation, UtilisationAtome::RACINE)) {
                auto valeur = m_indéfinie.ajoute_élément();
                ajoute_valeur_au_bloc(valeur, bloc);

                /* Nous devons créer une nouvelle version de la variable. */
                auto locale = m_locale.ajoute_élément();
                locale->alloc = alloc;
                locale->définis_valeur(m_table_relations, valeur);
                ajoute_valeur_au_bloc(locale, bloc);
                écris_variable(alloc, bloc, locale);
                return locale;
            }

            /* Lis la valeur. */
            auto résultat = lis_variable(inst, bloc);
            if (résultat->est_phi() && résultat->numéro == 0) {
                ajoute_valeur_au_bloc(résultat, bloc);
            }

            return résultat;
        }
        case GenreInstruction::OPÉRATION_BINAIRE:
        {
            auto op_binaire = inst->comme_op_binaire();
            auto valeur_gauche = donne_valeur_pour_atome(
                bloc, op_binaire->valeur_gauche, UtilisationAtome::POUR_OPÉRATEUR);
            auto valeur_droite = donne_valeur_pour_atome(
                bloc, op_binaire->valeur_droite, UtilisationAtome::POUR_OPÉRATEUR);

            DEBOGUE_UTILISATION_INSTRUCTION

            POUR_TABLEAU_PAGE (m_opérateur_binaire) {
                if (it.donne_droite() != valeur_droite) {
                    continue;
                }

                if (it.donne_gauche() != valeur_gauche) {
                    continue;
                }

                if (it.inst->op != op_binaire->op) {
                    continue;
                }

                écris_variable(inst, bloc, &it);
                return &it;
            }

            /* Essaie de propager les constantes. */
            auto constante_gauche = valeur_gauche;
            if (constante_gauche->est_locale()) {
                constante_gauche = constante_gauche->comme_locale()->donne_valeur();
            }

            auto constante_droite = valeur_droite;
            if (constante_droite->est_locale()) {
                constante_droite = constante_droite->comme_locale()->donne_valeur();
            }

            if (constante_gauche->est_constante() && constante_droite->est_constante()) {
                auto const_gauche = constante_gauche->comme_constante();
                auto const_droite = constante_droite->comme_constante();

                InstructionOpBinaire tmp(inst->site);
                tmp.type = inst->type;
                tmp.op = op_binaire->op;
                tmp.valeur_gauche = const_cast<AtomeConstante *>(const_gauche->atome);
                tmp.valeur_droite = const_cast<AtomeConstante *>(const_droite->atome);

                auto résultat_possible = évalue_opérateur_binaire(&tmp, m_constructrice);
                if (résultat_possible) {
                    auto valeur = donne_valeur_pour_atome(
                        bloc, résultat_possible, UtilisationAtome::POUR_OPÉRATEUR);
                    écris_variable(inst, bloc, valeur);
                    return valeur;
                }
            }

            auto résultat = m_opérateur_binaire.ajoute_élément();
            résultat->définis_gauche(m_table_relations, valeur_gauche);
            résultat->définis_droite(m_table_relations, valeur_droite);
            résultat->inst = op_binaire;
            ajoute_valeur_au_bloc(résultat, bloc);

            écris_variable(inst, bloc, résultat);
            return résultat;
        }
        case GenreInstruction::OPÉRATION_UNAIRE:
        {
            INSTRUCTION_NON_IMPLEMENTEE;
            // écris_variable
            break;
        }
        case GenreInstruction::CHARGE_MÉMOIRE:
        {
            DEBOGUE_UTILISATION_INSTRUCTION
            auto charge = inst->comme_charge();
            auto valeur = donne_valeur_pour_atome(
                bloc, charge->chargée, UtilisationAtome::POUR_LECTURE);
            écris_variable(charge, bloc, valeur);
            return valeur;
        }
        case GenreInstruction::STOCKE_MÉMOIRE:
        {
            auto const stocke = inst->comme_stocke_mem();
            auto const source = stocke->source;
            auto const destination = stocke->destination;

            auto valeur_source = donne_valeur_pour_atome(
                bloc, source, UtilisationAtome::POUR_SOURCE_ÉCRITURE);
            auto valeur_destination = donne_valeur_pour_atome(
                bloc, destination, UtilisationAtome::POUR_DESTINATION_ÉCRITURE);

            DEBOGUE_UTILISATION_INSTRUCTION

            if (source->est_instruction()) {
                auto inst_stockée = source->comme_instruction();
                if (inst_stockée->est_alloc() || inst_stockée->est_accès_indice() ||
                    inst_stockée->est_accès_rubrique()) {
                    auto adresse_de = m_adresse_de.ajoute_élément();
                    adresse_de->définis_valeur(m_table_relations, valeur_source);
                    ajoute_valeur_au_bloc(adresse_de, bloc);
                    valeur_source = adresse_de;
                }
            }

            if (valeur_destination->est_locale()) {
                auto locale = valeur_destination->comme_locale();
                assert(locale->donne_valeur() == nullptr);
                locale->définis_valeur(m_table_relations, valeur_source);
            }
            else if (valeur_destination->est_écris_index()) {
                auto écris_index = valeur_destination->comme_écris_index();
                assert(écris_index->donne_valeur() == nullptr);
                écris_index->définis_valeur(m_table_relations, valeur_source);
            }

            écris_variable(destination, bloc, valeur_destination);
            return nullptr;
        }
        case GenreInstruction::RETOUR:
        {
            auto retour = inst->comme_retour();
            auto valeur = m_retour.ajoute_élément();
            if (retour->valeur) {
                auto nouvelle_valeur = donne_valeur_pour_atome(
                    bloc, retour->valeur, UtilisationAtome::POUR_LECTURE);
                valeur->définis_valeur(m_table_relations, nouvelle_valeur);
            }
            DEBOGUE_UTILISATION_INSTRUCTION
            bloc->valeurs.ajoute(valeur);
            return nullptr;
        }
        case GenreInstruction::ACCÈS_RUBRIQUE:
        {
            auto inst_accès = inst->comme_accès_rubrique();
            auto valeur_accédée = donne_valeur_pour_atome(bloc, inst_accès->accédé, utilisation);

            DEBOGUE_UTILISATION_INSTRUCTION

            auto valeur_accès = m_accès_rubrique.ajoute_élément();
            valeur_accès->définis_accédée(m_table_relations, valeur_accédée);
            valeur_accès->indice = inst_accès->indice;
            valeur_accès->inst = inst_accès;
            ajoute_valeur_au_bloc(valeur_accès, bloc);
            écris_variable(inst_accès, bloc, valeur_accès);
            return valeur_accès;
        }
        case GenreInstruction::ACCÈS_INDICE:
        {
            auto inst_accès = inst->comme_accès_indice();
            auto valeur_accédée = donne_valeur_pour_atome(
                bloc, inst_accès->accédé, UtilisationAtome::POUR_LECTURE);
            auto valeur_indice = donne_valeur_pour_atome(
                bloc, inst_accès->indice, UtilisationAtome::POUR_LECTURE);

            DEBOGUE_UTILISATION_INSTRUCTION

            if (est_drapeau_actif(utilisation, UtilisationAtome::POUR_DESTINATION_ÉCRITURE)) {
                if (!valeur_accédée->est_locale()) {
                    INSTRUCTION_NON_IMPLEMENTEE;
                }

                auto écris_index = m_écris_index.ajoute_élément();
                écris_index->définis_accédée(m_table_relations, valeur_accédée);
                écris_index->définis_indice(m_table_relations, valeur_indice);
                ajoute_valeur_au_bloc(écris_index, bloc);

                auto locale = valeur_accédée->comme_locale();
                /* À FAIRE : nous pourrions ne pas toujours créer de nouvelles locales. */
                auto nouvelle_locale = m_locale.ajoute_élément();
                nouvelle_locale->alloc = locale->alloc;
                nouvelle_locale->définis_valeur(m_table_relations, écris_index);
                ajoute_valeur_au_bloc(nouvelle_locale, bloc);

                écris_variable(locale->alloc, bloc, nouvelle_locale);
                return écris_index;
            }

            auto valeur_accès = m_accès_indice.ajoute_élément();
            valeur_accès->définis_accédée(m_table_relations, valeur_accédée);
            valeur_accès->définis_indice(m_table_relations, valeur_indice);
            ajoute_valeur_au_bloc(valeur_accès, bloc);
            écris_variable(inst_accès, bloc, valeur_accès);

            return valeur_accès;
        }
        case GenreInstruction::TRANSTYPE:
        {
            auto inst_transtype = inst->comme_transtype();
            auto valeur_transtypée = donne_valeur_pour_atome(
                bloc, inst_transtype->valeur, UtilisationAtome::POUR_LECTURE);

            DEBOGUE_UTILISATION_INSTRUCTION

            POUR_TABLEAU_PAGE (m_transtypage) {
                if (it.donne_valeur() != valeur_transtypée) {
                    continue;
                }

                if (it.inst->type != inst_transtype->type) {
                    continue;
                }

                écris_variable(inst_transtype, bloc, &it);
                return &it;
            }

            auto transtypage = m_transtypage.ajoute_élément();
            transtypage->inst = inst_transtype;
            transtypage->définis_valeur(m_table_relations, valeur_transtypée);
            ajoute_valeur_au_bloc(transtypage, bloc);
            écris_variable(inst_transtype, bloc, transtypage);
            return transtypage;
        }
        case GenreInstruction::SÉLECTION:
        {
            INSTRUCTION_NON_IMPLEMENTEE;
            break;
        }
        case GenreInstruction::INATTEIGNABLE:
        {
            INSTRUCTION_NON_IMPLEMENTEE;
            break;
        }
        case GenreInstruction::ARRÊT_DÉBUG:
        {
            INSTRUCTION_NON_IMPLEMENTEE;
            break;
        }
    }
    return nullptr;
}

static void imprime_bloc(Bloc *bloc)
{
    dbg() << "bloc: " << bloc->label->id;
    dbg() << imprime_valeurs(bloc->valeurs);
}

static void imprime_blocs(FonctionEtBlocs &fonction_et_blocs)
{
    dbg() << "------------------------------------------------";
    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        imprime_bloc(bloc);
    }
    dbg() << "------------------------------------------------";
}

static void numérote_valeurs(FonctionEtBlocs &fonction_et_blocs)
{
    auto numéro = 1u;
    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        POUR_NOMME (valeur, bloc->valeurs) {
            if (valeur->est_controle_de_flux() ||
                valeur->possède_drapeau(DrapeauxValeur::NE_PRODUIS_PAS_DE_VALEUR)) {
                valeur->numéro = 0;
                continue;
            }
            valeur->numéro = numéro++;
        }
    }
}

static bool supprime_branches_inutiles(FonctionEtBlocs &fonction_et_blocs,
                                       VisiteuseBlocs &visiteuse,
                                       TableDesRelations &table_des_relations)
{
    auto bloc_modifié = false;

    for (auto i = 0; i < fonction_et_blocs.blocs.taille(); ++i) {
        auto it = fonction_et_blocs.blocs[i];

        if (it->valeurs.est_vide()) {
            /* Le bloc fut fusionné ici. */
            continue;
        }

        auto di = it->valeurs.dernier_élément();

        if (di->est_branche_cond()) {
            auto valeur_branche = di->comme_branche_cond();
            auto branche = valeur_branche->inst;
            if (branche->label_si_faux == branche->label_si_vrai) {
                /* Remplace par une branche.
                 * À FAIRE : crée une instruction. */
                auto br = const_cast<InstructionBrancheCondition *>(branche);
                auto nouvelle_branche = reinterpret_cast<InstructionBranche *>(br);
                nouvelle_branche->genre = GenreInstruction::BRANCHE;
                nouvelle_branche->label = branche->label_si_faux;

                auto nv_br = reinterpret_cast<ValeurBranche *>(valeur_branche);
                nv_br->genre = FSAU::GenreValeur::BRANCHE;
                nv_br->inst = nouvelle_branche;

                bloc_modifié = true;
                i -= 1;
                continue;
            }

            if (valeur_branche->donne_condition()->est_constante()) {
                auto valeur_constante = valeur_branche->donne_condition()->comme_constante();
                InstructionLabel *label_cible;
                if (valeur_constante->atome->comme_constante_booléenne()->valeur) {
                    label_cible = branche->label_si_vrai;
                    it->enfants[1]->déconnecte_pour_branche_morte(it);
                }
                else {
                    label_cible = branche->label_si_faux;
                    it->enfants[0]->déconnecte_pour_branche_morte(it);
                }

                /* Remplace par une branche.
                 * À FAIRE : crée une instruction. */
                auto br = const_cast<InstructionBrancheCondition *>(branche);
                auto nouvelle_branche = reinterpret_cast<InstructionBranche *>(br);
                nouvelle_branche->genre = GenreInstruction::BRANCHE;
                nouvelle_branche->label = label_cible;

                auto nv_br = reinterpret_cast<ValeurBranche *>(valeur_branche);
                nv_br->genre = FSAU::GenreValeur::BRANCHE;
                nv_br->inst = nouvelle_branche;
                bloc_modifié = true;
                i -= 1;
                assert(it->enfants.taille() == 1);
            }

            continue;
        }

        if (!di->est_branche()) {
            continue;
        }

        if (it->enfants.taille() == 0) {
            // le bloc est orphelin car toutes les branches ont été optimisées ?
            imprime_bloc(it);
            continue;
        }

        auto bloc_enfant = it->enfants[0];
        if (bloc_enfant->parents.taille() != 1) {
            continue;
        }

        it->fusionne_enfant(bloc_enfant);
        bloc_enfant->instructions.efface();
        bloc_enfant->valeurs.efface();
        /* Regère ce bloc au cas où le nouvelle enfant serait également une branche. */
        if (it->valeurs.dernier_élément()->est_branche() ||
            it->valeurs.dernier_élément()->est_branche_cond()) {
            i -= 1;
        }
        bloc_modifié = true;
    }

    if (!bloc_modifié) {
        return false;
    }

    auto blocs_libérés = fonction_et_blocs.supprime_blocs_inatteignables(visiteuse);

    POUR_NOMME (bloc, blocs_libérés) {
        POUR_NOMME (valeur, bloc->valeurs) {
            auto utilisateurs = table_des_relations.donne_utilisateurs(valeur);
            POUR (utilisateurs) {
                if (!it->est_phi()) {
                    continue;
                }

                auto phi = it->comme_phi();
                phi->supprime_opérande(valeur);

                if (phi->opérandes.taille() > 1) {
                    continue;
                }

                phi->remplace_par(
                    table_des_relations, phi->opérandes[0], FSAU::DrapeauxRemplacement::AUCUN);
            }
        }
    }

    return true;
}

static void supprime_valeurs_inutilisées(FonctionEtBlocs &fonction_et_blocs,
                                         TableDesRelations &table_des_relations)
{
    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        POUR_NOMME (valeur, bloc->valeurs) {
            if (valeur->est_controle_de_flux()) {
                valeur->drapeaux |= DrapeauxValeur::EST_UTILISÉE;
                continue;
            }

            if (table_des_relations.est_utilisée(valeur)) {
                valeur->drapeaux |= DrapeauxValeur::EST_UTILISÉE;
            }
            else {
                table_des_relations.supprime(valeur);
            }
        }
    }

    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        auto partition = kuri::partition_stable(bloc->valeurs, [](Valeur const *v) {
            return (v->drapeaux & DrapeauxValeur::EST_UTILISÉE) != DrapeauxValeur::ZÉRO;
        });

        bloc->valeurs.redimensionne(partition.vrai.taille());
    }
}

struct PhiValeurIncrémentée {
    NoeudPhi *phi = nullptr;
    Valeur *valeur = nullptr;
    Valeur *incrément = nullptr;
};

static bool est_incrément_de_phi(NoeudPhi const *phi, Valeur const *valeur)
{
#if 1
    if (!valeur->est_locale()) {
        return false;
    }

    valeur = valeur->comme_locale()->donne_valeur();
#endif

    if (!valeur->est_opérateur_binaire()) {
        return false;
    }

    auto op_binaire = valeur->comme_opérateur_binaire();
    if (op_binaire->inst->op != OpérateurBinaire::Genre::Addition) {
        return false;
    }

    if (op_binaire->donne_gauche() != phi) {
        return false;
    }

    return true;
}

static bool détecte_expressions_communes(FonctionEtBlocs &fonction_et_blocs,
                                         TableDesRelations &table)
{
    kuri::tablet<PhiValeurIncrémentée, 6> phis_incréments;
    auto résultat = false;

    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        POUR_NOMME (valeur, bloc->valeurs) {
            if (!valeur->est_phi()) {
                continue;
            }

            auto phi = valeur->comme_phi();

            if (phi->opérandes.taille() != 2) {
                continue;
            }

            auto op1 = phi->opérandes[0]->comme_locale()->donne_valeur();
            auto op2 = phi->opérandes[1];

            if (est_incrément_de_phi(phi, op2)) {
                dbg() << "EST INCRÉMENT DE PHI";
                op2 = op2->comme_locale()->donne_valeur();
                auto incrément = op2->comme_opérateur_binaire()->donne_droite();

                auto remplacé = false;

                POUR_NOMME (inc_existant, phis_incréments) {
                    if (inc_existant.valeur == op1 && inc_existant.incrément == incrément) {
                        dbg() << "PEUT REMPLACER v" << valeur->numéro << " par v"
                              << inc_existant.phi->numéro;
                        remplacé = true;
                        résultat = true;

                        phi->remplace_par(table, inc_existant.phi, DrapeauxRemplacement::AUCUN);
                    }
                }

                if (!remplacé) {
                    phis_incréments.ajoute({phi, op1, incrément});
                }
            }
        }
    }

    return résultat;
}

static bool supprime_code_inutile(FonctionEtBlocs &fonction_et_blocs, TableDesRelations &table)
{
    auto résultat = false;

    int32_t indice_bloc = 0;
    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        POUR_NOMME (valeur, bloc->valeurs) {
            valeur->indice_bloc = indice_bloc;
            valeur->drapeaux &= ~DrapeauxValeur::PARTICIPE_AU_FLOT_DU_PROGRAMME;
        }
        indice_bloc++;
    }

    /* Remplace les phis par leurs dernières opérandes si toutes les opérandes sont dans le bloc du
     * phi. Ceci peut arriver lors de la fusion de blocs. Nous prenons la dernière opérande car
     * elle est sensée être la dernière valeur écrite chronologiquement. */
    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        POUR_NOMME (valeur, bloc->valeurs) {
            if (!valeur->est_phi()) {
                continue;
            }

            auto phi = valeur->comme_phi();
            auto bloc_phi = phi->indice_bloc;
            auto toutes_les_opérandes_sont_dans_le_bloc = true;
            POUR (phi->opérandes) {
                if (bloc_phi != it->indice_bloc) {
                    toutes_les_opérandes_sont_dans_le_bloc = false;
                    break;
                }
            }

            if (!toutes_les_opérandes_sont_dans_le_bloc) {
                continue;
            }

            phi->remplace_par(table,
                              phi->opérandes[phi->opérandes.taille() - 1],
                              FSAU::DrapeauxRemplacement::AUCUN);
        }
    }

    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        POUR_NOMME (valeur, bloc->valeurs) {
            if (!valeur->est_controle_de_flux()) {
                continue;
            }

            valeur->drapeaux |= DrapeauxValeur::PARTICIPE_AU_FLOT_DU_PROGRAMME;

            kuri::ensemble<Valeur *> visitées;
            visite_valeur(valeur, visitées, [](Valeur *opérande) {
                opérande->drapeaux |= DrapeauxValeur::PARTICIPE_AU_FLOT_DU_PROGRAMME;
            });
        }
    }

    /* Deuxième passe pour les index. */
    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        kuri::tablet<ValeurÉcrisIndice *, 16> valeurs_écris_index;
        POUR_NOMME (valeur, bloc->valeurs) {
            if (!valeur->est_écris_index()) {
                continue;
            }

            auto écris_index = valeur->comme_écris_index();
            auto accédée = écris_index->donne_accédée();
            if (!accédée->possède_drapeau(DrapeauxValeur::PARTICIPE_AU_FLOT_DU_PROGRAMME)) {
                continue;
            }

            POUR (valeurs_écris_index) {
                if (it->donne_accédée() != accédée) {
                    continue;
                }

                if (it->donne_indice() != écris_index->donne_indice()) {
                    continue;
                }

                it->drapeaux |= DrapeauxValeur::ÉCRIS_INDICE_EST_SURÉCRIS;
            }

            valeurs_écris_index.ajoute(écris_index);
        }

        POUR_NOMME (valeur, bloc->valeurs) {
            if (!valeur->est_écris_index() ||
                !valeur->possède_drapeau(DrapeauxValeur::NE_PRODUIS_PAS_DE_VALEUR) ||
                valeur->possède_drapeau(DrapeauxValeur::ÉCRIS_INDICE_EST_SURÉCRIS)) {
                continue;
            }

            auto écris_index = valeur->comme_écris_index();
            auto accédée = écris_index->donne_accédée();
            if (!accédée->possède_drapeau(DrapeauxValeur::PARTICIPE_AU_FLOT_DU_PROGRAMME)) {
                continue;
            }

            valeur->drapeaux |= DrapeauxValeur::PARTICIPE_AU_FLOT_DU_PROGRAMME;

            kuri::ensemble<Valeur *> visitées;
            visite_valeur(valeur, visitées, [](Valeur *opérande) {
                opérande->drapeaux |= DrapeauxValeur::PARTICIPE_AU_FLOT_DU_PROGRAMME;
            });
        }
    }

    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        auto partition = kuri::partition_stable(bloc->valeurs, [](Valeur const *v) {
            if (v->possède_drapeau(DrapeauxValeur::PARTICIPE_AU_FLOT_DU_PROGRAMME)) {
                return true;
            }

            return false;
        });

        bloc->valeurs.redimensionne(partition.vrai.taille());

        résultat |= partition.faux.taille() != 0;

        POUR (partition.faux) {
            table.supprime(it);
        }
    }

    return résultat;
}

static void simplifie_écris_index(FSAU::ValeurÉcrisIndice *écris_index, TableDesRelations &table)
{
    if (écris_index->donne_accédée()->est_locale()) {
        auto locale = écris_index->donne_accédée()->comme_locale();
        if (locale->donne_valeur()->est_écris_index()) {
            auto écris_indice_précédent = locale->donne_valeur()->comme_écris_index();
            auto ancêtre_accédé = écris_indice_précédent->donne_accédée();

            écris_index->définis_accédée(table, ancêtre_accédé);
        }

        /* Dans tous les cas nous ne produisons pas de valeurs. */
        écris_index->drapeaux |= DrapeauxValeur::NE_PRODUIS_PAS_DE_VALEUR;
    }
}

static void simplifie_locale(FSAU::ValeurLocale *locale, TableDesRelations &table)
{
    auto valeur_locale = locale->donne_valeur();
    if (valeur_locale->est_écris_index() &&
        valeur_locale->possède_drapeau(DrapeauxValeur::NE_PRODUIS_PAS_DE_VALEUR)) {
        auto écris_index = valeur_locale->comme_écris_index();
        locale->remplace_par(table, écris_index->donne_accédée(), DrapeauxRemplacement::AUCUN);
    }
}

static bool est_constante_zéro(FSAU::Valeur const *valeur)
{
    if (!valeur->est_constante()) {
        return false;
    }

    return est_constante_entière_zéro(valeur->comme_constante()->atome);
}

static void simplifie_accès_indice(FSAU::ValeurAccèsIndice *accès_indice, TableDesRelations &table)
{
    auto accédée = accès_indice->donne_accédée();

    if (accédée->est_adresse_de()) {
        auto adresse_de = accédée->comme_adresse_de();
        if (adresse_de->donne_valeur()->est_accès_indice()) {
            auto sous_accès_indice = adresse_de->donne_valeur()->comme_accès_indice();
            auto index = sous_accès_indice->donne_indice();
            if (est_constante_zéro(index)) {
                /* À FAIRE : généralise en remplaçant par une addition pour l'index. */
                /* (&v[0])[idx] -> v[idx] */
                accès_indice->définis_accédée(table, sous_accès_indice->donne_accédée());
            }
        }
    }
}

static bool simplifie_accès_indice(FonctionEtBlocs &fonction_et_blocs, TableDesRelations &table)
{
    /* À FAIRE : note si une valeur fut changée. */
    auto résultat = false;

    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        POUR_NOMME (valeur, bloc->valeurs) {
            if (valeur->est_écris_index()) {
                simplifie_écris_index(valeur->comme_écris_index(), table);
                continue;
            }

            if (valeur->est_locale()) {
                simplifie_locale(valeur->comme_locale(), table);
                continue;
            }

            if (valeur->est_accès_indice()) {
                simplifie_accès_indice(valeur->comme_accès_indice(), table);
                continue;
            }
        }
    }

    return résultat;
}

static bool propage_temporaires(FonctionEtBlocs &fonction_et_blocs, TableDesRelations &table)
{
    auto résultat = false;

    imprime_blocs(fonction_et_blocs);

    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        kuri::tablet<ValeurÉcrisIndice *, 16> valeurs_écris_index;

        POUR_NOMME (valeur, bloc->valeurs) {
            if (valeur->est_locale()) {
                auto const locale = valeur->comme_locale();
                auto const valeur_locale = locale->donne_valeur();
                auto drapeaux = DrapeauxRemplacement::IGNORE_PHI;

                if (valeur_locale->est_indéfinie() || valeur_locale->est_écris_index()) {
                    drapeaux |= DrapeauxRemplacement::IGNORE_ACCÈS_INDICE |
                                DrapeauxRemplacement::IGNORE_ÉCRIS_INDICE;
                }

                /* À FAIRE : note si une valeur fut remplacée. */
                locale->remplace_par(table, valeur_locale, drapeaux);
                continue;
            }

            if (valeur->est_écris_index()) {
                valeurs_écris_index.ajoute(valeur->comme_écris_index());
                continue;
            }

#if 0
            /* À FAIRE : les paramètres de fonctions devrait être modélisés correctement. */
            if (valeur->est_accès_indice()) {
                /* À FAIRE : cette optimisation doit être locale (ne remplace que dans le bloc). */
                auto accès_indice = valeur->comme_accès_indice();

                for (auto i = valeurs_écris_index.taille() - 1; i >= 0; i--) {
                    auto écris_index = valeurs_écris_index[i];

                    if (écris_index->donne_accédée() != accès_indice->donne_accédée()) {
                        continue;
                    }

                    if (écris_index->donne_indice() != accès_indice->donne_indice()) {
                        continue;
                    }

                    auto const valeur_indice = écris_index->donne_valeur();

                    auto drapeaux = DrapeauxRemplacement::IGNORE_PHI;

                    if (valeur_indice->est_indéfinie() || valeur_indice->est_écris_index()) {
                        drapeaux |= DrapeauxRemplacement::IGNORE_ACCÈS_INDICE |
                                    DrapeauxRemplacement::IGNORE_ÉCRIS_INDICE;
                    }

                    valeur->remplace_par(table, valeur_indice, drapeaux);
                }

                continue;
            }
#endif
        }
    }

    return résultat;
}

void convertis_fsau(EspaceDeTravail &espace,
                    AtomeFonction *fonction,
                    ConstructriceRI &constructrice)
{
    FonctionEtBlocs fonction_et_blocs;
    if (!fonction_et_blocs.convertis_en_blocs(espace, fonction)) {
        return;
    }

    auto convertisseuse_fsau = ConvertisseuseFSAU{constructrice};

    kuri::file<Bloc *> blocs;
    blocs.enfile(fonction_et_blocs.blocs[0]);

    POUR (fonction->params_entrée) {
        it->drapeaux |= DrapeauxAtome::EST_PARAMÈTRE_FONCTION;
        (void)convertisseuse_fsau.génère_valeur_pour_instruction(
            fonction_et_blocs.blocs[0], it, UtilisationAtome::RACINE);
    }

    /* Insère la valeur de retour dans le premier bloc. */
    (void)convertisseuse_fsau.génère_valeur_pour_instruction(
        fonction_et_blocs.blocs[0], fonction->param_sortie, UtilisationAtome::RACINE);

    while (!blocs.est_vide()) {
        auto bloc = blocs.defile();

        // dbg() << "dépile bloc " << bloc;

        if (bloc->tous_les_parents_furent_remplis()) {
            convertisseuse_fsau.scelle_bloc(bloc);
            if (bloc->possède_drapeau(DrapeauxBlocBasique::EST_REMPLIS)) {
                continue;
            }
        }

        if (!bloc->possède_drapeau(DrapeauxBlocBasique::EST_REMPLIS)) {
            dbg() << "Remplis bloc " << bloc->label->id;

            POUR_NOMME (inst, bloc->instructions) {
                if (!instruction_est_racine(inst) && !inst->est_alloc()) {
                    continue;
                }
                (void)convertisseuse_fsau.génère_valeur_pour_instruction(
                    bloc, inst, UtilisationAtome::RACINE);
            }
            bloc->drapeaux |= DrapeauxBlocBasique::EST_REMPLIS;
        }

        if (!bloc->tous_les_parents_furent_remplis()) {
            blocs.enfile(bloc);
            // dbg() << "empile bloc " << bloc << " car tous les parents ne furent pas remplis";
            POUR (bloc->parents) {
                if (it->possède_drapeau(DrapeauxBlocBasique::EST_REMPLIS)) {
                    continue;
                }

                // dbg() << "-- parent non remplis " << it;
            }
        }

        POUR (bloc->enfants) {
            blocs.enfile(it);
            // dbg() << "empile bloc " << it;
        }
    }

    TableDesRelations &table_des_relations = convertisseuse_fsau.donne_table_des_relations();

    imprime_blocs(fonction_et_blocs);

    auto visiteuse = VisiteuseBlocs(fonction_et_blocs);

    while (true) {
        auto chose_faite = false;

        chose_faite |= supprime_branches_inutiles(
            fonction_et_blocs, visiteuse, table_des_relations);

        chose_faite |= détecte_expressions_communes(fonction_et_blocs, table_des_relations);
        chose_faite |= propage_temporaires(fonction_et_blocs, table_des_relations);
        chose_faite |= simplifie_accès_indice(fonction_et_blocs, table_des_relations);

        // Redondant avec supprime_code_inutile, ne prends pas en compte les accès_indice
        // supprime_valeurs_inutilisées(fonction_et_blocs, table_des_relations);
        chose_faite |= supprime_code_inutile(fonction_et_blocs, table_des_relations);

        if (!chose_faite) {
            break;
        }
    }

    numérote_valeurs(fonction_et_blocs);
    imprime_blocs(fonction_et_blocs);

    //    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
    //        POUR_NOMME (valeur, bloc->valeurs) {
    //            visite_opérande(valeur, [&](Valeur *opérande) {
    //                table_des_relations.ajoute_utilisateur(opérande, valeur);
    //            });
    //        }
    //    }

    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        POUR_NOMME (valeur, bloc->valeurs) {
            if (valeur->est_controle_de_flux() ||
                valeur->possède_drapeau(DrapeauxValeur::NE_PRODUIS_PAS_DE_VALEUR)) {
                continue;
            }

            auto utilisateurs = table_des_relations.donne_utilisateurs(valeur);

            dbg() << "v" << valeur->numéro << ", utilisateurs : ";
            POUR (utilisateurs) {
                if (it->est_controle_de_flux() ||
                    it->possède_drapeau(DrapeauxValeur::NE_PRODUIS_PAS_DE_VALEUR)) {
                    dbg() << "-- " << it->genre;
                }
                else {
                    dbg() << "-- v" << it->numéro;
                }
            }
        }
    }

    rièrevertis_en_ri(fonction_et_blocs, constructrice);
}

/* ------------------------------------------------------------------------- */
/** \name Implémentation de la table des relations.
 * \{ */

void TableDesRelations::remplace_ou_ajoute_utilisateur(Valeur *utilisée,
                                                       Valeur *ancien,
                                                       Valeur *par)
{
    if (ancien) {
        supprime_utilisateur(ancien, par);
    }

    ajoute_utilisateur(utilisée, par);
}

void TableDesRelations::ajoute_utilisateur(Valeur *utilisée, Valeur *par)
{
    auto indice_données_utilisée = donne_indice_pour_valeur(utilisée);

    auto &index = m_index[int32_t(indice_données_utilisée)];

    auto info = UtilisateurValeur{par, indice_utilisateur_invalide, indice_utilisateur_invalide};

    if (index.premier_utilisateur == indice_utilisateur_invalide) {
        /* Insère un nouvelle utilisateur. */
        index.premier_utilisateur = indice_table_utilisateur(m_utilisateurs.taille());
    }
    else {
        /* Trouve le dernier utilisateur. */
        auto utilisateur = &m_utilisateurs[int32_t(index.premier_utilisateur)];
        info.précédent = index.premier_utilisateur;
        while (utilisateur->suivant != indice_utilisateur_invalide) {
            info.précédent = utilisateur->suivant;
            utilisateur = &m_utilisateurs[int32_t(utilisateur->suivant)];
        }

        utilisateur->suivant = indice_table_utilisateur(m_utilisateurs.taille());
    }

    // dbg() << __func__ << " : "
    //       << "précédent " << info.précédent << " suivant " << info.suivant;

    m_utilisateurs.ajoute(info);
}

bool TableDesRelations::est_utilisée(const Valeur *valeur) const
{
    auto const indice_données_utilisation = valeur->indice_relations;
    if (indice_données_utilisation == indice_relation_invalide) {
        return false;
    }
    return m_index[int32_t(indice_données_utilisation)].premier_utilisateur !=
           indice_utilisateur_invalide;
}

void TableDesRelations::supprime(const Valeur *valeur)
{
    /* Supprime la valeur de la liste des utilisateurs des valeurs qu'elle utilise. */
    for (int i = 0; i < m_index.taille(); i++) {
        if (m_index[i].premier_utilisateur == indice_utilisateur_invalide) {
            continue;
        }

        supprime_utilisateur(indice_table_relation(i), valeur);
    }
}

kuri::tablet<Valeur *, 6> TableDesRelations::donne_utilisateurs(const Valeur *valeur) const
{
    kuri::tablet<Valeur *, 6> résultat;

    auto const indice_données_utilisation = valeur->indice_relations;
    // assert(indice_données_utilisation != -1);
    if (indice_données_utilisation == indice_relation_invalide) {
        return résultat;
    }

    auto &index = m_index[int32_t(indice_données_utilisation)];

    auto indice_utilisateur = index.premier_utilisateur;
    while (indice_utilisateur != indice_utilisateur_invalide) {
        auto utilisateur = m_utilisateurs[int32_t(indice_utilisateur)].utilisateur;
        résultat.ajoute(utilisateur);
        indice_utilisateur = m_utilisateurs[int32_t(indice_utilisateur)].suivant;
    }

    return résultat;
}

void TableDesRelations::supprime_utilisateur(Valeur *utilisée, Valeur const *par)
{
    auto indice_données_utilisée = utilisée->indice_relations;
    assert(indice_données_utilisée != indice_relation_invalide);
    supprime_utilisateur(indice_données_utilisée, par);
}

void TableDesRelations::supprime_utilisateur(indice_table_relation indice_données_utilisée,
                                             Valeur const *par)
{
    auto &index = m_index[int32_t(indice_données_utilisée)];

    kuri::tablet<indice_table_utilisateur, 6> indice_libres{};

    auto indice_utilisateur = index.premier_utilisateur;
    assert(indice_utilisateur != indice_utilisateur_invalide);
    while (indice_utilisateur != indice_utilisateur_invalide) {
        auto utilisateur = &m_utilisateurs[int32_t(indice_utilisateur)];
        auto sauvegarde = indice_utilisateur;
        indice_utilisateur = utilisateur->suivant;

        if (utilisateur->utilisateur == par) {
            déconnecte(utilisateur);

            if (sauvegarde == index.premier_utilisateur) {
                index.premier_utilisateur = indice_utilisateur;
            }

            indice_libres.ajoute(sauvegarde);
        }
    }

    // if (index.premier_utilisateur == -1) {
    //     dbg() << __func__ << " : n'est plus utilisée";
    // }
    // dbg() << __func__ << " : index libres " << indice_libres.taille();
}

void TableDesRelations::déconnecte(UtilisateurValeur *utilisateur)
{
    if (utilisateur->précédent != indice_utilisateur_invalide) {
        auto précédent = &m_utilisateurs[int32_t(utilisateur->précédent)];
        précédent->suivant = utilisateur->suivant;
    }

    if (utilisateur->suivant != indice_utilisateur_invalide) {
        auto suivant = &m_utilisateurs[int32_t(utilisateur->suivant)];
        suivant->précédent = utilisateur->précédent;
    }

    utilisateur->utilisateur = nullptr;
    utilisateur->précédent = indice_utilisateur_invalide;
    utilisateur->suivant = indice_utilisateur_invalide;
}

indice_table_relation TableDesRelations::donne_indice_pour_valeur(Valeur *valeur)
{
    if (valeur->indice_relations != indice_relation_invalide) {
        return valeur->indice_relations;
    }

    valeur->indice_relations = indice_table_relation(m_index.taille());
    m_index.ajoute({});
    return valeur->indice_relations;
}

/** \} */

namespace FSAU {

class Rièrevertisseuse {
  public:
    Atome *rièrevertis_en_ri(Valeur *valeur, ConstructriceRI &constructrice, bool pour_opérande);
};

Atome *Rièrevertisseuse::rièrevertis_en_ri(Valeur *valeur,
                                           ConstructriceRI &constructrice,
                                           bool pour_opérande)
{
    switch (valeur->genre) {
        case GenreValeur::INDÉFINIE:
        case GenreValeur::FONCTION:
        {
            break;
        }
        case GenreValeur::GLOBALE:
        {
            break;
        }
        case GenreValeur::CONSTANTE:
        {
            auto constante = valeur->comme_constante();
            return const_cast<AtomeConstante *>(constante->atome);
        }
        case GenreValeur::BRANCHE:
        {
            auto branche = valeur->comme_branche();
            return constructrice.crée_branche(nullptr, branche->inst->label);
        }
        case GenreValeur::BRANCHE_COND:
        {
            auto branche = valeur->comme_branche_cond();
            auto prédicat = rièrevertis_en_ri(branche->donne_condition(), constructrice, true);
            return constructrice.crée_branche_condition(
                nullptr, prédicat, branche->inst->label_si_vrai, branche->inst->label_si_faux);
        }
        case GenreValeur::RETOUR:
        {
            auto retour = valeur->comme_retour();
            auto valeur_retournée = retour->donne_valeur();
            if (valeur_retournée) {
                return constructrice.crée_retour(
                    nullptr, rièrevertis_en_ri(valeur_retournée, constructrice, true));
            }
            return constructrice.crée_retour(nullptr, nullptr);
        }
        case GenreValeur::LOCALE:
        {
            auto locale = valeur->comme_locale();
            auto alloc = const_cast<InstructionAllocation *>(locale->alloc);
            assert(alloc);

            if (pour_opérande) {
                return constructrice.crée_charge_mem(nullptr, alloc);
            }

            if (!alloc->possède_drapeau(DrapeauxAtome::FUT_RÉINSÉRÉ_APRÈS_FSAU) &&
                !alloc->possède_drapeau(DrapeauxAtome::EST_PARAMÈTRE_FONCTION)) {
                constructrice.insère(alloc);
                alloc->drapeaux |= DrapeauxAtome::FUT_RÉINSÉRÉ_APRÈS_FSAU;
            }

            if (locale->donne_valeur()->genre != GenreValeur::INDÉFINIE) {
                auto opérande = rièrevertis_en_ri(locale->donne_valeur(), constructrice, true);
                constructrice.crée_stocke_mem(nullptr, alloc, opérande);
            }

            return alloc;
        }
        case GenreValeur::ADRESSE_DE:
        {
            // auto adresse_de = valeur->comme_adresse_de();
            break;
        }
        case GenreValeur::ACCÈS_RUBRIQUE:
        {
            // auto accès_rubrique = valeur->comme_accès_rubrique();
            break;
        }
        case GenreValeur::ACCÈS_INDICE:
        {
            if (!pour_opérande) {
                return nullptr;
            }

            auto const accès_indice = valeur->comme_accès_indice();
            auto const valeur_accédée = accès_indice->donne_accédée();
            auto const valeur_indice = accès_indice->donne_indice();
            auto atome_accédée = rièrevertis_en_ri(valeur_accédée, constructrice, false);
            assert_rappel(atome_accédée,
                          [&]() { dbg() << "Genre valeur accédée : " << valeur_accédée->genre; });
            auto atome_index = rièrevertis_en_ri(valeur_indice, constructrice, true);
            assert_rappel(atome_index,
                          [&]() { dbg() << "Genre valeur index : " << valeur_indice->genre; });
            auto résultat = constructrice.crée_accès_indice(nullptr, atome_accédée, atome_index);
            return constructrice.crée_charge_mem(nullptr, résultat);
        }
        case GenreValeur::ÉCRIS_INDICE:
        {
            auto écris_index = valeur->comme_écris_index();
            auto const valeur_accédée = écris_index->donne_accédée();
            auto const valeur_indice = écris_index->donne_indice();
            auto const valeur_valeur = écris_index->donne_valeur();
            auto atome_accédée = rièrevertis_en_ri(valeur_accédée, constructrice, false);
            assert_rappel(atome_accédée,
                          [&]() { dbg() << "Genre valeur accédée : " << valeur_accédée->genre; });
            auto atome_index = rièrevertis_en_ri(valeur_indice, constructrice, true);
            assert_rappel(atome_index,
                          [&]() { dbg() << "Genre valeur index : " << valeur_indice->genre; });
            auto atome_valeur = rièrevertis_en_ri(valeur_valeur, constructrice, true);
            assert_rappel(atome_valeur,
                          [&]() { dbg() << "Genre valeur valeur : " << valeur_valeur->genre; });
            auto accès_indice = constructrice.crée_accès_indice(nullptr, atome_accédée, atome_index);
            return constructrice.crée_stocke_mem(nullptr, accès_indice, atome_valeur);
        }
        case GenreValeur::APPEL:
        {
            // auto appel = valeur->comme_appel();
            break;
        }
        case GenreValeur::OPÉRATEUR_BINAIRE:
        {
            if (!pour_opérande) {
                return nullptr;
            }

            auto op_binaire = valeur->comme_opérateur_binaire();
            auto opérande_gauche = rièrevertis_en_ri(
                op_binaire->donne_gauche(), constructrice, true);
            assert_rappel(opérande_gauche, [&]() {
                dbg() << "Genre opérande gauche : " << op_binaire->donne_gauche()->genre;
            });
            auto opérande_droite = rièrevertis_en_ri(
                op_binaire->donne_droite(), constructrice, true);
            assert_rappel(opérande_droite, [&]() {
                dbg() << "Genre opérande droite : " << op_binaire->donne_droite()->genre;
            });

            return constructrice.crée_op_binaire(nullptr,
                                                 op_binaire->inst->type,
                                                 op_binaire->inst->op,
                                                 opérande_gauche,
                                                 opérande_droite);
        }
        case GenreValeur::OPÉRATEUR_UNAIRE:
        {
            // auto op_unaire = valeur->comme_opérateur_unaire();
            break;
        }
        case GenreValeur::PHI:
        {
            auto phi = valeur->comme_phi();
            auto alloc = const_cast<InstructionAllocation *>(
                phi->opérandes[0]->comme_locale()->alloc);

            if (pour_opérande) {
                return constructrice.crée_charge_mem(nullptr, alloc);
            }
            break;
        }
        case GenreValeur::TRANSTYPAGE:
        {
            // auto transtypage = valeur->comme_transtypage();
            break;
        }
    }

    return nullptr;
}

static void rièrevertis_en_ri(FonctionEtBlocs &fonction_et_blocs, ConstructriceRI &constructrice)
{
    dbg() << "--------------------------";
    auto fonction = fonction_et_blocs.fonction;
    fonction->instructions.efface();

    constructrice.définis_fonction_courante(fonction);

    Rièrevertisseuse rièrevertisseuse;

    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        constructrice.insère_label(bloc->label);
        POUR_NOMME (valeur, bloc->valeurs) {
            rièrevertisseuse.rièrevertis_en_ri(valeur, constructrice, false);
        }
    }

    constructrice.définis_fonction_courante(nullptr);

    dbg() << imprime_fonction(fonction);
}

}  // namespace FSAU
