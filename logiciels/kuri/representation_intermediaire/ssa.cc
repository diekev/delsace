/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "ssa.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "utilitaires/algorithmes.hh"
#include "utilitaires/log.hh"

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

// https://pp.ipd.kit.edu/uploads/publikationen/braun13cc.pdf

namespace SSA {

#define ENUMERE_GENRE_VALEUR_SSA(O)                                                               \
    O(INDÉFINIE, ValeurIndéfinie, indéfinie)                                                      \
    O(CONSTANTE, ValeurConstante, constante)                                                      \
    O(FONCTION, ValeurFonction, fonction)                                                         \
    O(GLOBALE, ValeurGlobale, globale)                                                            \
    O(APPEL, ValeurAppel, appel)                                                                  \
    O(ACCÈS_MEMBRE, ValeurAccèdeMembre, accès_membre)                                             \
    O(ACCÈS_INDEX, ValeurAccèdeIndex, accès_index)                                                \
    O(OPÉRATEUR_BINAIRE, ValeurOpérateurBinaire, opérateur_binaire)                               \
    O(OPÉRATEUR_UNAIRE, ValeurOpérateurUnaire, opérateur_unaire)                                  \
    O(BRANCHE, ValeurBranche, branche)                                                            \
    O(BRANCHE_COND, ValeurBrancheCond, branche_cond)                                              \
    O(RETOUR, ValeurRetour, retour)                                                               \
    O(PHI, NoeudPhi, phi)

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
    void définis_##nom(Valeur *v)                                                                 \
    {                                                                                             \
        nom = v;                                                                                  \
        ajoute_utilisateur_si_phi(v, this);                                                       \
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
};
DEFINIS_OPERATEURS_DRAPEAU(DrapeauxValeur)

struct Valeur {
    GenreValeur genre{};
    DrapeauxValeur drapeaux = DrapeauxValeur::ZÉRO;
    uint32_t numéro = 0;

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
    kuri::tableau<Valeur *> utilisateurs{};

    CONSTRUCTEUR_VALEUR(NoeudPhi, PHI);

    void ajoute_opérande(Valeur *valeur);

    void définis_opérande(int index, Valeur *v);

    [[nodiscard]] kuri::tableau<Valeur *> supprime_utilisateur(Valeur *utilisateur);

    void replaceBy(Valeur *valeur);

    void ajoute_utilisateur(Valeur *utilisateur)
    {
        utilisateurs.ajoute(utilisateur);
    }

  private:
    void remplace_dans_utisateur(Valeur *utilisateur, Valeur *par);
};

static void ajoute_utilisateur_si_phi(Valeur *phi_potentiel, Valeur *utilisateur)
{
    if (!phi_potentiel->est_phi()) {
        return;
    }
    phi_potentiel->comme_phi()->utilisateurs.ajoute(utilisateur);
}

struct ValeurIndéfinie : public Valeur {
    CONSTRUCTEUR_VALEUR(ValeurIndéfinie, INDÉFINIE);
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
    void ajoute_argument(Valeur *v)
    {
        arguments.ajoute(v);
        ajoute_utilisateur_si_phi(v, this);
    }

    void définis_argument(int index, Valeur *v)
    {
        arguments[index] = v;
        ajoute_utilisateur_si_phi(v, this);
    }

    kuri::tableau_statique<Valeur *> donne_arguments() const
    {
        return arguments;
    }
};

void NoeudPhi::ajoute_opérande(Valeur *valeur)
{
    opérandes.ajoute(valeur);
    ajoute_utilisateur_si_phi(valeur, this);
}

void NoeudPhi::définis_opérande(int index, Valeur *v)
{
    opérandes[index] = v;
    ajoute_utilisateur_si_phi(v, this);
}

kuri::tableau<Valeur *> NoeudPhi::supprime_utilisateur(Valeur *utilisateur)
{
    dbg() << "[" << __func__ << "] : utilisateurs " << utilisateurs.taille();
    kuri::tableau<Valeur *> résultat;
    POUR (utilisateurs) {
        if (it == utilisateur) {
            continue;
        }

        résultat.ajoute(it);
    }

    return résultat;
}

void NoeudPhi::replaceBy(Valeur *valeur)
{
    dbg() << "[" << __func__ << "] : utilisateurs " << utilisateurs.taille();
    POUR (utilisateurs) {
        if (it == this) {
            continue;
        }

        remplace_dans_utisateur(it, valeur);
    }
}

void NoeudPhi::remplace_dans_utisateur(Valeur *utilisateur, Valeur *par)
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
            branche->définis_condition(par);
            break;
        }
        case GenreValeur::RETOUR:
        {
            auto retour = utilisateur->comme_retour();
            assert(retour->donne_valeur() == this);
            retour->définis_valeur(par);
            break;
        }
        case GenreValeur::ACCÈS_MEMBRE:
        {
            auto accès_membre = utilisateur->comme_accès_membre();
            assert(accès_membre->donne_accédée() == this);
            accès_membre->définis_accédée(par);
            break;
        }
        case GenreValeur::ACCÈS_INDEX:
        {
            auto accès_index = utilisateur->comme_accès_index();
            if (accès_index->donne_accédée() == this) {
                accès_index->définis_accédée(par);
            }
            if (accès_index->donne_index() == this) {
                accès_index->définis_index(par);
            }
            break;
        }
        case GenreValeur::APPEL:
        {
            auto appel = utilisateur->comme_appel();
            if (appel->donne_valeur_appelée() == this) {
                appel->définis_valeur_appelée(par);
            }
            POUR_INDEX (appel->donne_arguments()) {
                if (it == this) {
                    appel->définis_argument(index_it, par);
                }
            }
            break;
        }
        case GenreValeur::OPÉRATEUR_BINAIRE:
        {
            auto op_binaire = utilisateur->comme_opérateur_binaire();
            if (op_binaire->donne_gauche() == this) {
                op_binaire->définis_gauche(par);
            }
            if (op_binaire->donne_droite() == this) {
                op_binaire->définis_droite(par);
            }
            break;
        }
        case GenreValeur::OPÉRATEUR_UNAIRE:
        {
            auto op_unaire = utilisateur->comme_opérateur_unaire();
            if (op_unaire->donne_droite() == this) {
                op_unaire->définis_droite(par);
            }
            break;
        }
        case GenreValeur::PHI:
        {
            auto phi = utilisateur->comme_phi();
            POUR_INDEX (phi->opérandes) {
                if (it == this) {
                    phi->définis_opérande(index_it, it);
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
            imprime_tableau(phi->utilisateurs, " (", ")", sortie);
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

    ConstructriceRI &m_constructrice;

  public:
    ConvertisseuseSSA(ConstructriceRI &constructrice_ri) : m_constructrice(constructrice_ri)
    {
    }

    void crée_valeurs_depuis_instruction(Bloc *bloc, Instruction const *inst);

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
            dbg() << "[" << __func__ << "] : !m_sealed_blocks.possède(bloc)";
            /* Graphe de controle incomplet. */
            auto phi = crée_noeud_phi(bloc);
            ajoute_phi_incomplet(bloc, variable, phi);
            résultat = phi;
        }
        else if (bloc->parents.taille() == 1) {
            dbg() << "[" << __func__ << "] : bloc->parents.taille() == 1";
            /* Optimisation du cas commun d'un ancêtre : aucun Phi n'est nécessaire. */
            résultat = readVariable(variable, bloc->parents[0]);
        }
        else {
            dbg() << "[" << __func__ << "] : else";
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
        dbg() << __func__;
        POUR (phi->bloc->parents) {
            phi->ajoute_opérande(readVariable(variable, it));
        }

        return tryRemoveTrivialPhi(phi);
    }

    // algorithm 3 : détection et suppression récursive des phi triviaux

    Valeur *tryRemoveTrivialPhi(NoeudPhi *phi)
    {
        Valeur *same = nullptr;

        POUR_NOMME (op, phi->opérandes) {
            if (op == same || op == phi) {
                dbg() << "[" << __func__ << "] : op == same || op == phi";
                /* Valeur unique ou auto-référence. */
                continue;
            }

            if (same != nullptr) {
                dbg() << "[" << __func__ << "] : same != nullptr";
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
        auto users = phi->supprime_utilisateur(phi);

        /* Dévie toutes les utilisations de phi vers same et supprime phi. */
        phi->replaceBy(same);

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

        dbg() << __func__ << " : phis_incomplets " << phis_incomplets.taille();
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
            branche->définis_condition(readVariable(inst_branche->condition, bloc));
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
            résultat->définis_gauche(valeur_gauche);
            résultat->définis_droite(valeur_droite);
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
            auto valeur = readVariable(charge->chargee, bloc);
            writeVariable(charge, bloc, valeur);
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto stocke = inst->comme_stocke_mem();
            auto valeur_stockée = donne_valeur_pour_atome(bloc, stocke->valeur);
            writeVariable(stocke->ou, bloc, valeur_stockée);
            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto retour = inst->comme_retour();
            auto valeur = m_retours.ajoute_element();
            if (retour->valeur) {
                valeur->définis_valeur(readVariable(retour->valeur, bloc));
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

static void supprime_valeurs_inutilisées(FonctionEtBlocs &fonction_et_blocs)
{
    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        POUR_NOMME (valeur, bloc->valeurs) {
            if (valeur->est_controle_de_flux()) {
                valeur->drapeaux |= DrapeauxValeur::EST_UTILISÉE;
            }

            kuri::ensemble<Valeur *> visitées;
            visite_valeur(valeur, visitées, [&](Valeur *opérande) {
                if (valeur == opérande) {
                    return;
                }

                opérande->drapeaux |= DrapeauxValeur::EST_UTILISÉE;
            });
        }
    }

    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        auto partition = kuri::partition_stable(bloc->valeurs, [](Valeur const *v) {
            return (v->drapeaux & DrapeauxValeur::EST_UTILISÉE) != DrapeauxValeur::ZÉRO;
        });

        bloc->valeurs.redimensionne(partition.vrai.taille());
    }

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

void convertis_ssa(EspaceDeTravail &espace,
                   AtomeFonction *fonction,
                   ConstructriceRI &constructrice)
{
    if (!fonction->decl || fonction->decl->ident != ID::principale) {
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

    imprime_blocs(fonction_et_blocs);

    auto visiteuse = VisiteuseBlocs(fonction_et_blocs);
    supprime_branches_inutiles(fonction_et_blocs, visiteuse);

    supprime_valeurs_inutilisées(fonction_et_blocs);

    imprime_blocs(fonction_et_blocs);
}
