/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "ssa.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "utilitaires/algorithmes.hh"
#include "utilitaires/log.hh"
#include "utilitaires/type_opaque.hh"

#include "parsage/identifiant.hh"

#include "structures/enchaineuse.hh"
#include "structures/ensemble.hh"
#include "structures/pile.hh"

#include "analyse.hh"
#include "bloc_basique.hh"
#include "impression.hh"
#include "instructions.hh"

#define INSTRUCTION_NON_IMPLEMENTEE                                                               \
    assert_rappel(false, [&]() { dbg() << "Instruction non-gérée " << inst->genre; })

#define ATOME_NON_IMPLEMENTE                                                                      \
    assert_rappel(false, [&]() { dbg() << "Atome non-géré " << atome->genre_atome; })

// CONSTRUCTION SSA https://pp.ipd.kit.edu/uploads/publikationen/braun13cc.pdf
// ARRAY SSA https://dl.acm.org/doi/pdf/10.1145/268946.268956

namespace SSA {

CREE_TYPE_OPAQUE(index_table_utilisateur, int32_t);
static const index_table_utilisateur index_utilisateur_invalide = index_table_utilisateur(-1);

CREE_TYPE_OPAQUE(index_table_bloc, int32_t);
static const index_table_bloc index_bloc_invalide = index_table_bloc(-1);

CREE_TYPE_OPAQUE(index_table_relation, int32_t);
static const index_table_relation index_relation_invalide = index_table_relation(-1);

struct TablesDesRelations {
    struct Index {
        index_table_utilisateur premier_utilisateur = index_utilisateur_invalide;
        index_table_bloc premier_bloc = index_bloc_invalide;
    };
    kuri::tableau<Index, int32_t> m_index{};

    struct UtilisateurValeur {
        Valeur *utilisateur = nullptr;
        index_table_utilisateur précédent = index_utilisateur_invalide;
        index_table_utilisateur suivant = index_utilisateur_invalide;
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
    index_table_relation donne_index_pour_valeur(Valeur *valeur);

    void déconnecte(UtilisateurValeur *utilisateur);
    void supprime_utilisateur(index_table_relation index, const Valeur *par);
};

#define ENUMERE_GENRE_VALEUR_SSA(O)                                                               \
    O(INDÉFINIE, ValeurIndéfinie, indéfinie)                                                      \
    O(CONSTANTE, ValeurConstante, constante)                                                      \
    O(FONCTION, ValeurFonction, fonction)                                                         \
    O(GLOBALE, ValeurGlobale, globale)                                                            \
    O(LOCALE, ValeurLocale, locale)                                                               \
    O(APPEL, ValeurAppel, appel)                                                                  \
    O(ACCÈS_MEMBRE, ValeurAccèdeMembre, accès_membre)                                             \
    O(ACCÈS_INDEX, ValeurAccèdeIndex, accès_index)                                                \
    O(OPÉRATEUR_BINAIRE, ValeurOpérateurBinaire, opérateur_binaire)                               \
    O(OPÉRATEUR_UNAIRE, ValeurOpérateurUnaire, opérateur_unaire)                                  \
    O(BRANCHE, ValeurBranche, branche)                                                            \
    O(BRANCHE_COND, ValeurBrancheCond, branche_cond)                                              \
    O(RETOUR, ValeurRetour, retour) O(PHI, NoeudPhi, phi)

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

#define MEMBRE_VALEUR(nom)                                                                        \
  private:                                                                                        \
    Valeur *nom = nullptr;                                                                        \
                                                                                                  \
  public:                                                                                         \
    void définis_##nom(TablesDesRelations &table, Valeur *v)                                      \
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

#define DEFINIS_FONCTIONS_DISCRIMINATION(genre, nom_classe, ident)                                \
    nom_classe *Valeur::comme_##ident()                                                           \
    {                                                                                             \
        assert(est_##ident());                                                                    \
        return static_cast<nom_classe *>(this);                                                   \
    }                                                                                             \
    nom_classe const *Valeur::comme_##ident() const                                               \
    {                                                                                             \
        assert(est_##ident());                                                                    \
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
};
DEFINIS_OPERATEURS_DRAPEAU(DrapeauxValeur)

struct Valeur {
    GenreValeur genre{};
    DrapeauxValeur drapeaux = DrapeauxValeur::ZÉRO;
    uint32_t numéro = 0;
    index_table_relation index_relations = index_relation_invalide;

    ENUMERE_GENRE_VALEUR_SSA(DECLARE_FONCTIONS_DISCRIMINATION)

    inline bool est_controle_de_flux() const
    {
        return this->est_branche() || this->est_branche_cond() || this->est_retour();
    }
};

#undef DECLARE_FONCTIONS_DISCRIMINATION

struct NoeudPhi : public Valeur {
    Bloc *bloc = nullptr;
    kuri::tableau<Valeur *> opérandes{};

    CONSTRUCTEUR_VALEUR(NoeudPhi, PHI);

    void ajoute_opérande(TablesDesRelations &table, Valeur *valeur);

    void définis_opérande(TablesDesRelations &table, int index, Valeur *v);

    [[nodiscard]] kuri::tableau<Valeur *> supprime_utilisateur(TablesDesRelations &table,
                                                               Valeur *utilisateur);

    void replaceBy(TablesDesRelations &table, Valeur *valeur);

  private:
    void remplace_dans_utisateur(TablesDesRelations &table, Valeur *utilisateur, Valeur *par);
};

struct ValeurIndéfinie : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurIndéfinie, INDÉFINIE);
};

struct ValeurLocale : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurLocale, LOCALE);

    InstructionAllocation const *alloc = nullptr;
    MEMBRE_VALEUR(valeur)
};

struct ValeurBranche : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurBranche, BRANCHE);

    InstructionBranche const *inst = nullptr;
};

struct ValeurBrancheCond : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurBrancheCond, BRANCHE_COND);

    MEMBRE_VALEUR(condition)
    InstructionBrancheCondition const *inst = nullptr;
};

struct ValeurRetour : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurRetour, RETOUR);

    MEMBRE_VALEUR(valeur)
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

    MEMBRE_VALEUR(gauche)
    MEMBRE_VALEUR(droite)
    InstructionOpBinaire const *inst = nullptr;
};

struct ValeurOpérateurUnaire : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurOpérateurUnaire, OPÉRATEUR_UNAIRE);

    MEMBRE_VALEUR(droite)
};

struct ValeurAccèdeMembre : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurAccèdeMembre, ACCÈS_MEMBRE);

    MEMBRE_VALEUR(accédée)
};

struct ValeurAccèdeIndex : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurAccèdeIndex, ACCÈS_INDEX);

    MEMBRE_VALEUR(accédée)
    MEMBRE_VALEUR(index)
};

struct ValeurAppel : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurAppel, APPEL);

    MEMBRE_VALEUR(valeur_appelée)

  private:
    kuri::tableau<Valeur *> arguments{};

  public:
    void ajoute_argument(TablesDesRelations &table, Valeur *v)
    {
        arguments.ajoute(v);
        table.ajoute_utilisateur(v, this);
    }

    void définis_argument(TablesDesRelations &table, int index, Valeur *v)
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

void NoeudPhi::ajoute_opérande(TablesDesRelations &table, Valeur *valeur)
{
    opérandes.ajoute(valeur);
    table.ajoute_utilisateur(valeur, this);
}

void NoeudPhi::définis_opérande(TablesDesRelations &table, int index, Valeur *v)
{
    auto ancien = opérandes[index];
    opérandes[index] = v;
    table.remplace_ou_ajoute_utilisateur(v, ancien, this);
}

kuri::tableau<Valeur *> NoeudPhi::supprime_utilisateur(TablesDesRelations &table,
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

void NoeudPhi::replaceBy(TablesDesRelations &table, Valeur *valeur)
{
    auto utilisateurs = table.donne_utilisateurs(this);
    // dbg() << "[" << __func__ << "] : utilisateurs " << utilisateurs.taille();
    POUR (utilisateurs) {
        if (it == this) {
            continue;
        }

        remplace_dans_utisateur(table, it, valeur);
    }

    table.supprime(this);
}

void NoeudPhi::remplace_dans_utisateur(TablesDesRelations &table, Valeur *utilisateur, Valeur *par)
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
        case GenreValeur::ACCÈS_MEMBRE:
        {
            auto accès_membre = utilisateur->comme_accès_membre();
            assert(accès_membre->donne_accédée() == this);
            accès_membre->définis_accédée(table, par);
            break;
        }
        case GenreValeur::ACCÈS_INDEX:
        {
            auto accès_index = utilisateur->comme_accès_index();
            if (accès_index->donne_accédée() == this) {
                accès_index->définis_accédée(table, par);
            }
            if (accès_index->donne_index() == this) {
                accès_index->définis_index(table, par);
            }
            break;
        }
        case GenreValeur::APPEL:
        {
            auto appel = utilisateur->comme_appel();
            if (appel->donne_valeur_appelée() == this) {
                appel->définis_valeur_appelée(table, par);
            }
            POUR_INDEX (appel->donne_arguments()) {
                if (it == this) {
                    appel->définis_argument(table, index_it, par);
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
            POUR_INDEX (phi->opérandes) {
                if (it == this) {
                    phi->définis_opérande(table, index_it, it);
                }
            }
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
            visite_valeur(retour->donne_valeur(), visitées, rappel);
            break;
        }
        case GenreValeur::LOCALE:
        {
            auto locale = valeur->comme_locale();
            visite_valeur(locale->donne_valeur(), visitées, rappel);
            break;
        }
        case GenreValeur::ACCÈS_MEMBRE:
        {
            auto accès_membre = valeur->comme_accès_membre();
            visite_valeur(accès_membre->donne_accédée(), visitées, rappel);
            break;
        }
        case GenreValeur::ACCÈS_INDEX:
        {
            auto accès_index = valeur->comme_accès_index();
            visite_valeur(accès_index->donne_accédée(), visitées, rappel);
            visite_valeur(accès_index->donne_index(), visitées, rappel);
            break;
        }
        case GenreValeur::APPEL:
        {
            auto appel = valeur->comme_appel();
            visite_valeur(appel->donne_valeur_appelée(), visitées, rappel);
            POUR_INDEX (appel->donne_arguments()) {
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
            POUR_INDEX (phi->opérandes) {
                visite_valeur(it, visitées, rappel);
            }
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
        case GenreValeur::ACCÈS_MEMBRE:
        {
            auto accès_membre = valeur->comme_accès_membre();
            rappel(accès_membre->donne_accédée());
            break;
        }
        case GenreValeur::ACCÈS_INDEX:
        {
            auto accès_index = valeur->comme_accès_index();
            rappel(accès_index->donne_accédée());
            rappel(accès_index->donne_index());
            break;
        }
        case GenreValeur::APPEL:
        {
            auto appel = valeur->comme_appel();
            rappel(appel->donne_valeur_appelée());
            POUR_INDEX (appel->donne_arguments()) {
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
            POUR_INDEX (phi->opérandes) {
                rappel(it);
            }
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
    if (valeur->numéro != 0) {
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
            sortie << " %" << branche->inst->label_si_vrai->id << " sinon %"
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
        case GenreValeur::ACCÈS_MEMBRE:
        {
            auto accès_membre = valeur->comme_accès_membre();
            sortie << "membre ";
            imprime_nom_valeur(accès_membre->donne_accédée(), sortie);
            break;
        }
        case GenreValeur::ACCÈS_INDEX:
        {
            auto accès_index = valeur->comme_accès_index();
            sortie << "index ";
            imprime_nom_valeur(accès_index->donne_accédée(), sortie);
            sortie << "[";
            imprime_nom_valeur(accès_index->donne_index(), sortie);
            sortie << "]";
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
    }
}

static kuri::chaine imprime_valeurs(kuri::tableau_statique<Valeur *> valeurs)
{
    Enchaineuse sortie;

    POUR (valeurs) {
        sortie << "  ";
        imprime_valeur(it, sortie);
        sortie << "\n";
    }

    return sortie.chaine();
}

}  // namespace SSA

using namespace SSA;

struct ConvertisseuseSSA {
  private:
    // À FAIRE : utilise drapeau
    kuri::ensemble<Bloc *> m_sealed_blocks{};

    struct DéfinitionVariableBloc {
        Atome const *variable = nullptr;
        Bloc *bloc = nullptr;
        Valeur *valeur = nullptr;
    };

    kuri::tableau<DéfinitionVariableBloc> currentDef{};

    struct PhiIncomplet {
        Bloc *bloc = nullptr;
        Atome const *variable = nullptr;
        NoeudPhi *résultat = nullptr;
    };

    kuri::tableau<PhiIncomplet> phis_incomplets{};

    uint32_t nombre_valeurs = 0;

    tableau_page<SSA::ValeurGlobale> m_globales{};
    tableau_page<SSA::ValeurConstante> m_constantes{};
    tableau_page<SSA::ValeurConstante> m_constantes_entières{};
    tableau_page<SSA::ValeurConstante> m_constantes_booléennes{};
    tableau_page<SSA::ValeurOpérateurBinaire> m_opérateurs_binaires{};
    tableau_page<SSA::ValeurRetour> m_retours{};
    tableau_page<SSA::ValeurBranche> m_branches{};
    tableau_page<SSA::ValeurBrancheCond> m_branches_cond{};
    tableau_page<SSA::NoeudPhi> m_noeuds_phi{};
    tableau_page<SSA::ValeurLocale> m_locales{};

    ConstructriceRI &m_constructrice;

    TablesDesRelations m_table_relations{};

  public:
    ConvertisseuseSSA(ConstructriceRI &constructrice_ri) : m_constructrice(constructrice_ri)
    {
    }

    void crée_valeurs_depuis_instruction(Bloc *bloc, Instruction const *inst);

    TablesDesRelations &donne_table_des_relations()
    {
        return m_table_relations;
    }

  public:
    // algorithme 1 : local value numbering

    void writeVariable(Atome const *variable, Bloc *bloc, Valeur *valeur)
    {
        POUR (currentDef) {
            if (it.variable == variable && it.bloc == bloc) {
                it.valeur = valeur;
                return;
            }
        }

        auto définition = DéfinitionVariableBloc{variable, bloc, valeur};
        currentDef.ajoute(définition);
    }

    Valeur *readVariable(Atome const *variable, Bloc *bloc)
    {
        bool variable_rencontrée = false;
        /* Trouve un numérotage local de la valeur. */
        POUR (currentDef) {
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
        return readVariableRecursive(variable, bloc);
    }

    // algorithme 2 : global value numbering
    Valeur *readVariableRecursive(Atome const *variable, Bloc *bloc)
    {
        Valeur *résultat = nullptr;

        if (!m_sealed_blocks.possède(bloc)) {
            // dbg() << "[" << __func__ << "] : !m_sealed_blocks.possède(bloc)";
            /* Graphe de controle incomplet. */
            auto phi = crée_noeud_phi(bloc);
            ajoute_phi_incomplet(bloc, variable, phi);
            résultat = phi;
        }
        else if (bloc->parents.taille() == 1) {
            // dbg() << "[" << __func__ << "] : bloc->parents.taille() == 1";
            /* Optimisation du cas commun d'un ancêtre : aucun Phi n'est nécessaire. */
            résultat = readVariable(variable, bloc->parents[0]);
        }
        else {
            // dbg() << "[" << __func__ << "] : else";
            /* Brise les cycles potentiels avec des phis sans-opérandes. */
            auto phi = crée_noeud_phi(bloc);
            writeVariable(variable, bloc, phi);
            résultat = addPhiOperands(variable, phi);
        }

        writeVariable(variable, bloc, résultat);
        return résultat;
    }

    Valeur *addPhiOperands(Atome const *variable, NoeudPhi *phi)
    {
        // dbg() << __func__;
        POUR (phi->bloc->parents) {
            phi->ajoute_opérande(m_table_relations, readVariable(variable, it));
        }

        return tryRemoveTrivialPhi(phi);
    }

    // algorithm 3 : détection et suppression récursive des phi triviaux

    Valeur *tryRemoveTrivialPhi(NoeudPhi *phi)
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
        phi->replaceBy(m_table_relations, same);

        /* Essaie de supprimer tous les utilisateurs de phi, qui peuvent être devenus triviaux. */
        POUR_NOMME (use, users) {
            if (use->genre == SSA::GenreValeur::PHI) {
                tryRemoveTrivialPhi(use->comme_phi());
            }
        }

        return same;
    }

    void sealBlock(Bloc *bloc)
    {
        if (m_sealed_blocks.possède(bloc)) {
            return;
        }

        // dbg() << __func__ << " : phis_incomplets " << phis_incomplets.taille();
        POUR (phis_incomplets) {
            if (it.bloc != bloc) {
                continue;
            }

            addPhiOperands(it.variable, it.résultat);
        }

        m_sealed_blocks.insère(bloc);
    }

    // --------------------------------------------

  private:
    NoeudPhi *crée_noeud_phi(Bloc *bloc)
    {
        auto résultat = m_noeuds_phi.ajoute_element();
        résultat->bloc = bloc;
        return résultat;
    }

    void ajoute_phi_incomplet(Bloc *bloc, Atome const *variable, NoeudPhi *valeur)
    {
        phis_incomplets.ajoute({bloc, variable, valeur});
    }

    void ajoute_valeur_au_bloc(Valeur *v, Bloc *bloc)
    {
        v->numéro = ++nombre_valeurs;
        bloc->valeurs.ajoute(v);
    }

    Valeur *donne_valeur_pour_atome(Bloc *bloc, Atome const *atome)
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
                auto résultat = readVariable(atome, bloc);
                if (résultat->genre == SSA::GenreValeur::PHI && résultat->numéro == 0) {
                    ajoute_valeur_au_bloc(résultat, bloc);
                }
                else if (résultat->est_locale()) {
                    return résultat->comme_locale()->donne_valeur();
                }
                return résultat;
            }
            case Atome::Genre::ACCÈS_INDEX_CONSTANT:
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
            case Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE:
            {
                ATOME_NON_IMPLEMENTE;
                return nullptr;
            }
            case Atome::Genre::CONSTANTE_TYPE:
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
                auto résultat = m_constantes.ajoute_element();
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

                POUR_TABLEAU_PAGE (m_constantes_booléennes) {
                    auto existante = it.atome->comme_constante_booléenne();
                    if (existante->type == constante_booléenne->type &&
                        existante->valeur == constante_booléenne->valeur) {
                        return &it;
                    }
                }
                auto résultat = m_constantes_booléennes.ajoute_element();
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

                POUR_TABLEAU_PAGE (m_constantes_entières) {
                    auto existante = it.atome->comme_constante_entière();
                    if (existante->type == constante_entière->type &&
                        existante->valeur == constante_entière->valeur) {
                        return &it;
                    }
                }
                auto résultat = m_constantes_entières.ajoute_element();
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

void ConvertisseuseSSA::crée_valeurs_depuis_instruction(Bloc *bloc, Instruction const *inst)
{
    switch (inst->genre) {
        case GenreInstruction::LABEL:
        {
            break;
        }
        case GenreInstruction::BRANCHE:
        {
            auto branche = m_branches.ajoute_element();
            branche->inst = inst->comme_branche();
            bloc->valeurs.ajoute(branche);
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto inst_branche = inst->comme_branche_cond();
            auto branche = m_branches_cond.ajoute_element();
            branche->inst = inst_branche;
            auto valeur = donne_valeur_pour_atome(bloc, inst_branche->condition);
            branche->définis_condition(m_table_relations, valeur);
            bloc->valeurs.ajoute(branche);
            break;
        }
        case GenreInstruction::APPEL:
        {
            INSTRUCTION_NON_IMPLEMENTEE;
            // writeVariable
            break;
        }
        case GenreInstruction::ALLOCATION:
        {
            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto op_binaire = inst->comme_op_binaire();
            auto valeur_gauche = donne_valeur_pour_atome(bloc, op_binaire->valeur_gauche);
            auto valeur_droite = donne_valeur_pour_atome(bloc, op_binaire->valeur_droite);

            POUR_TABLEAU_PAGE (m_opérateurs_binaires) {
                if (it.donne_droite() != valeur_droite) {
                    continue;
                }

                if (it.donne_gauche() != valeur_gauche) {
                    continue;
                }

                if (it.inst->op != op_binaire->op) {
                    continue;
                }

                writeVariable(inst, bloc, &it);
                return;
            }

            if (valeur_gauche->est_constante() && valeur_droite->est_constante()) {
                auto const_gauche = valeur_gauche->comme_constante();
                auto const_droite = valeur_droite->comme_constante();

                InstructionOpBinaire tmp(inst->site);
                tmp.type = inst->type;
                tmp.op = op_binaire->op;
                tmp.valeur_gauche = const_cast<AtomeConstante *>(const_gauche->atome);
                tmp.valeur_droite = const_cast<AtomeConstante *>(const_droite->atome);

                auto résultat_possible = évalue_opérateur_binaire(&tmp, m_constructrice);
                if (résultat_possible) {
                    auto valeur = donne_valeur_pour_atome(bloc, résultat_possible);
                    writeVariable(inst, bloc, valeur);
                    return;
                }
            }

            auto résultat = m_opérateurs_binaires.ajoute_element();
            résultat->définis_gauche(m_table_relations, valeur_gauche);
            résultat->définis_droite(m_table_relations, valeur_droite);
            résultat->inst = op_binaire;
            ajoute_valeur_au_bloc(résultat, bloc);

            writeVariable(inst, bloc, résultat);
            break;
        }
        case GenreInstruction::OPERATION_UNAIRE:
        {
            INSTRUCTION_NON_IMPLEMENTEE;
            // writeVariable
            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            auto charge = inst->comme_charge();
            auto valeur = donne_valeur_pour_atome(bloc, charge->chargee);
            writeVariable(charge, bloc, valeur);
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto stocke = inst->comme_stocke_mem();
            auto destination = stocke->ou;
            auto valeur_stockée = donne_valeur_pour_atome(bloc, stocke->valeur);

            if (destination->est_instruction()) {
                auto inst_dest = destination->comme_instruction();
                if (inst_dest->est_alloc()) {
                    auto alloc = inst_dest->comme_alloc();
                    dbg() << "Nouvelle version de " << (alloc->ident ? alloc->ident->nom : "tmp");

                    auto locale = m_locales.ajoute_element();
                    locale->alloc = alloc;
                    locale->définis_valeur(m_table_relations, valeur_stockée);
                    ajoute_valeur_au_bloc(locale, bloc);
                    valeur_stockée = locale;
                }
            }

            writeVariable(destination, bloc, valeur_stockée);
            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto retour = inst->comme_retour();
            auto valeur = m_retours.ajoute_element();
            if (retour->valeur) {
                valeur->définis_valeur(m_table_relations,
                                       donne_valeur_pour_atome(bloc, retour->valeur));
            }
            bloc->valeurs.ajoute(valeur);
            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            INSTRUCTION_NON_IMPLEMENTEE;
            break;
        }
        case GenreInstruction::ACCEDE_INDEX:
        {
            INSTRUCTION_NON_IMPLEMENTEE;
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            INSTRUCTION_NON_IMPLEMENTEE;
            break;
        }
    }
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
            if (valeur->est_controle_de_flux()) {
                continue;
            }
            valeur->numéro = numéro++;
        }
    }
}

static void supprime_branches_inutiles(FonctionEtBlocs &fonction_et_blocs,
                                       VisiteuseBlocs &visiteuse)
{
    auto bloc_modifié = false;

    for (auto i = 0; i < fonction_et_blocs.blocs.taille(); ++i) {
        auto it = fonction_et_blocs.blocs[i];

        if (it->valeurs.est_vide()) {
            /* Le bloc fut fusionné ici. */
            continue;
        }

        auto di = it->valeurs.dernière();

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
                nv_br->genre = SSA::GenreValeur::BRANCHE;
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
                nv_br->genre = SSA::GenreValeur::BRANCHE;
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
        if (it->valeurs.dernière()->est_branche() || it->valeurs.dernière()->est_branche_cond()) {
            i -= 1;
        }
        bloc_modifié = true;
    }

    if (!bloc_modifié) {
        return;
    }

    fonction_et_blocs.supprime_blocs_inatteignables(visiteuse);
}

static void supprime_valeurs_inutilisées(FonctionEtBlocs &fonction_et_blocs,
                                         TablesDesRelations &table_des_relations)
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

static void détecte_expressions_communes(FonctionEtBlocs &fonction_et_blocs,
                                         TablesDesRelations &table)
{
    kuri::tablet<PhiValeurIncrémentée, 6> phis_incréments;

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

                        phi->replaceBy(table, inc_existant.phi);
                    }
                }

                if (!remplacé) {
                    phis_incréments.ajoute({phi, op1, incrément});
                }
            }
        }
    }
}

static void supprime_code_inutile(FonctionEtBlocs &fonction_et_blocs)
{
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

    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        auto partition = kuri::partition_stable(bloc->valeurs, [](Valeur const *v) {
            return (v->drapeaux & DrapeauxValeur::PARTICIPE_AU_FLOT_DU_PROGRAMME) !=
                   DrapeauxValeur::ZÉRO;
        });

        bloc->valeurs.redimensionne(partition.vrai.taille());
    }
}

void convertis_ssa(EspaceDeTravail &espace,
                   AtomeFonction *fonction,
                   ConstructriceRI &constructrice)
{
    if (!fonction->decl || !fonction->decl->possède_drapeau(DrapeauxNoeudFonction::FSAU)) {
        return;
    }

    dbg() << imprime_fonction(fonction);

    FonctionEtBlocs fonction_et_blocs;
    if (!fonction_et_blocs.convertis_en_blocs(espace, fonction)) {
        return;
    }

    auto convertisseuse_ssa = ConvertisseuseSSA{constructrice};

    kuri::file<Bloc *> blocs;
    blocs.enfile(fonction_et_blocs.blocs[0]);

    while (!blocs.est_vide()) {
        auto bloc = blocs.defile();

        // dbg() << "dépile bloc " << bloc;

        if (bloc->tous_les_parents_furent_remplis()) {
            convertisseuse_ssa.sealBlock(bloc);
            if (bloc->fut_remplis) {
                continue;
            }
        }

        if (!bloc->fut_remplis) {
            POUR_NOMME (inst, bloc->instructions) {
                convertisseuse_ssa.crée_valeurs_depuis_instruction(bloc, inst);
            }
            bloc->fut_remplis = true;
        }

        if (!bloc->tous_les_parents_furent_remplis()) {
            blocs.enfile(bloc);
            // dbg() << "empile bloc " << bloc << " car tous les parents ne furent pas remplis";
            POUR (bloc->parents) {
                if (it->fut_remplis) {
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

    TablesDesRelations &table_des_relations = convertisseuse_ssa.donne_table_des_relations();

    imprime_blocs(fonction_et_blocs);

    auto visiteuse = VisiteuseBlocs(fonction_et_blocs);
    supprime_branches_inutiles(fonction_et_blocs, visiteuse);

    détecte_expressions_communes(fonction_et_blocs, table_des_relations);

    supprime_valeurs_inutilisées(fonction_et_blocs, table_des_relations);
    supprime_code_inutile(fonction_et_blocs);

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
            if (valeur->est_controle_de_flux()) {
                continue;
            }

            auto utilisateurs = table_des_relations.donne_utilisateurs(valeur);

            dbg() << "v" << valeur->numéro << ", utilisateurs : ";
            POUR (utilisateurs) {
                if (it->est_controle_de_flux()) {
                    dbg() << "-- " << it->genre;
                }
                else {
                    dbg() << "-- v" << it->numéro;
                }
            }
        }
    }
}

/* ------------------------------------------------------------------------- */
/** \name Implémentation de la table des relations.
 * \{ */

void TablesDesRelations::remplace_ou_ajoute_utilisateur(Valeur *utilisée,
                                                        Valeur *ancien,
                                                        Valeur *par)
{
    if (ancien) {
        supprime_utilisateur(ancien, par);
    }

    ajoute_utilisateur(utilisée, par);
}

void TablesDesRelations::ajoute_utilisateur(Valeur *utilisée, Valeur *par)
{
    auto index_données_utilisée = donne_index_pour_valeur(utilisée);

    auto &index = m_index[int32_t(index_données_utilisée)];

    auto info = UtilisateurValeur{par, index_utilisateur_invalide, index_utilisateur_invalide};

    if (index.premier_utilisateur == index_utilisateur_invalide) {
        /* Insère un nouvelle utilisateur. */
        index.premier_utilisateur = index_table_utilisateur(m_utilisateurs.taille());
    }
    else {
        /* Trouve le dernier utilisateur. */
        auto utilisateur = &m_utilisateurs[int32_t(index.premier_utilisateur)];
        info.précédent = index.premier_utilisateur;
        while (utilisateur->suivant != index_utilisateur_invalide) {
            info.précédent = utilisateur->suivant;
            utilisateur = &m_utilisateurs[int32_t(utilisateur->suivant)];
        }

        utilisateur->suivant = index_table_utilisateur(m_utilisateurs.taille());
    }

    // dbg() << __func__ << " : "
    //       << "précédent " << info.précédent << " suivant " << info.suivant;

    m_utilisateurs.ajoute(info);
}

bool TablesDesRelations::est_utilisée(const Valeur *valeur) const
{
    auto const index_données_utilisation = valeur->index_relations;
    if (index_données_utilisation == index_relation_invalide) {
        return false;
    }
    return m_index[int32_t(index_données_utilisation)].premier_utilisateur !=
           index_utilisateur_invalide;
}

void TablesDesRelations::supprime(const Valeur *valeur)
{
    /* Supprime la valeur de la liste des utilisateurs des valeurs qu'elle utilise. */
    for (int i = 0; i < m_index.taille(); i++) {
        if (m_index[i].premier_utilisateur == index_utilisateur_invalide) {
            continue;
        }

        supprime_utilisateur(index_table_relation(i), valeur);
    }
}

kuri::tablet<Valeur *, 6> TablesDesRelations::donne_utilisateurs(const Valeur *valeur) const
{
    kuri::tablet<Valeur *, 6> résultat;

    auto const index_données_utilisation = valeur->index_relations;
    // assert(index_données_utilisation != -1);
    if (index_données_utilisation == index_relation_invalide) {
        return résultat;
    }

    auto &index = m_index[int32_t(index_données_utilisation)];

    auto index_utilisateur = index.premier_utilisateur;
    while (index_utilisateur != index_utilisateur_invalide) {
        auto utilisateur = m_utilisateurs[int32_t(index_utilisateur)].utilisateur;
        résultat.ajoute(utilisateur);
        index_utilisateur = m_utilisateurs[int32_t(index_utilisateur)].suivant;
    }

    return résultat;
}

void TablesDesRelations::supprime_utilisateur(Valeur *utilisée, Valeur const *par)
{
    auto index_données_utilisée = utilisée->index_relations;
    assert(index_données_utilisée != index_relation_invalide);
    supprime_utilisateur(index_données_utilisée, par);
}

void TablesDesRelations::supprime_utilisateur(index_table_relation index_données_utilisée,
                                              Valeur const *par)
{
    auto &index = m_index[int32_t(index_données_utilisée)];

    kuri::tablet<index_table_utilisateur, 6> index_libres{};

    auto index_utilisateur = index.premier_utilisateur;
    assert(index_utilisateur != index_utilisateur_invalide);
    while (index_utilisateur != index_utilisateur_invalide) {
        auto utilisateur = &m_utilisateurs[int32_t(index_utilisateur)];
        auto sauvegarde = index_utilisateur;
        index_utilisateur = utilisateur->suivant;

        if (utilisateur->utilisateur == par) {
            déconnecte(utilisateur);

            if (sauvegarde == index.premier_utilisateur) {
                index.premier_utilisateur = index_utilisateur;
            }

            index_libres.ajoute(sauvegarde);
        }
    }

    // if (index.premier_utilisateur == -1) {
    //     dbg() << __func__ << " : n'est plus utilisée";
    // }
    // dbg() << __func__ << " : index libres " << index_libres.taille();
}

void TablesDesRelations::déconnecte(UtilisateurValeur *utilisateur)
{
    if (utilisateur->précédent != index_utilisateur_invalide) {
        auto précédent = &m_utilisateurs[int32_t(utilisateur->précédent)];
        précédent->suivant = utilisateur->suivant;
    }

    if (utilisateur->suivant != index_utilisateur_invalide) {
        auto suivant = &m_utilisateurs[int32_t(utilisateur->suivant)];
        suivant->précédent = utilisateur->précédent;
    }

    utilisateur->utilisateur = nullptr;
    utilisateur->précédent = index_utilisateur_invalide;
    utilisateur->suivant = index_utilisateur_invalide;
}

index_table_relation TablesDesRelations::donne_index_pour_valeur(Valeur *valeur)
{
    if (valeur->index_relations != index_relation_invalide) {
        return valeur->index_relations;
    }

    valeur->index_relations = index_table_relation(m_index.taille());
    m_index.ajoute({});
    return valeur->index_relations;
}

/** \} */

/*

  Pour la rétroconversion vers RI.

bloc: 0                 %0  label 0
  v1 = 0                %1    alloue z32 a
                        %2    stocke *z32 %1, z32 0
  br %1                 %3    branche %4

bloc: 1                 %4  label 1
  v2 = 15
  v3 = 1
  v4 = phi <v1, v6>     %5    charge z32 %1
  v5 = v4 >= v2         %6    supeg bool %5, 15
  si v5 %4 sinon %5     %7    si %6 alors %8 sinon %11

bloc: 4                 %8  label 4
  ret v4                %9    charge z32 %1
                        %10   retourne %9

bloc: 5                 %11 label 5
  v6 = v4 + v3          %12   charge z32 %1
                        %13   ajt z32 %12, 1
                        %14   stocke *z32 %1, z32 %13
  br %1                 %15   branche %4

*/
