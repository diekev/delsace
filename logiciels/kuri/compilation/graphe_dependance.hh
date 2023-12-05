/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "biblinternes/outils/badge.hh"
#include "biblinternes/structures/tableau_page.hh"

#include "structures/ensemblon.hh"
#include "structures/tableau.hh"
#include "structures/tableau_compresse.hh"

struct AtomeFonction;
struct EspaceDeTravail;
struct GrapheDependance;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationSymbole;
struct NoeudDeclarationVariable;
struct NoeudDependance;
struct NoeudExpression;
struct Statistiques;
struct Type;

namespace kuri {
struct chaine;
}

/**
 * GrapheDependance de dépendance entre les fonctions et les types.
 */

enum class TypeNoeudDependance {
    INVALIDE,
    FONCTION,
    TYPE,
    GLOBALE,
};

#define ENUMERE_TYPES_RELATION                                                                    \
    ENUMERE_TYPE_RELATION_EX(INVALIDE)                                                            \
    ENUMERE_TYPE_RELATION_EX(UTILISE_TYPE)                                                        \
    ENUMERE_TYPE_RELATION_EX(UTILISE_FONCTION)                                                    \
    ENUMERE_TYPE_RELATION_EX(UTILISE_GLOBALE)

enum class TypeRelation : int {
#define ENUMERE_TYPE_RELATION_EX(type) type,
    ENUMERE_TYPES_RELATION
#undef ENUMERE_TYPE_RELATION_EX
};

const char *chaine_type_relation(TypeRelation type);
std::ostream &operator<<(std::ostream &os, TypeRelation type);

struct Relation {
    TypeRelation type = TypeRelation::INVALIDE;
    NoeudDependance *noeud_debut = nullptr;
    NoeudDependance *noeud_fin = nullptr;
};

inline bool operator==(Relation const &r1, Relation const &r2)
{
    return r1.type == r2.type && r1.noeud_debut == r2.noeud_debut && r1.noeud_fin == r2.noeud_fin;
}

struct NoeudDependance {
  private:
    kuri::tableau_compresse<Relation> m_relations{};

    union {
        Type *m_type;
        NoeudDeclarationEnteteFonction *m_noeud_fonction;
        NoeudDeclarationVariable *m_noeud_globale;
    };

    TypeNoeudDependance m_type_noeud = TypeNoeudDependance::INVALIDE;

  public:
    int index_visite = 0;

    /* pour certains algorithmes de travail sur le graphe */
    char drapeaux = 0;

    explicit NoeudDependance(NoeudDeclarationVariable *globale);
    explicit NoeudDependance(NoeudDeclarationEnteteFonction *fonction);
    explicit NoeudDependance(Type *t);

    inline bool est_type() const
    {
        return m_type_noeud == TypeNoeudDependance::TYPE;
    }

    inline bool est_globale() const
    {
        return m_type_noeud == TypeNoeudDependance::GLOBALE;
    }

    inline bool est_fonction() const
    {
        return m_type_noeud == TypeNoeudDependance::FONCTION;
    }

    inline Type *type() const
    {
        assert(est_type());
        return m_type;
    }

    inline NoeudDeclarationEnteteFonction *fonction() const
    {
        assert(est_fonction());
        return m_noeud_fonction;
    }

    inline NoeudDeclarationVariable *globale() const
    {
        assert(est_globale());
        return m_noeud_globale;
    }

    void ajoute_relation(Badge<GrapheDependance>, const Relation &relation);

    kuri::tableau_compresse<Relation> const &relations() const;

    void relations(Badge<GrapheDependance>, kuri::tableau_compresse<Relation> &&relations);
};

struct DonneesDependance {
    kuri::ensemblon<NoeudDeclarationEnteteFonction *, 16> fonctions_utilisees{};
    kuri::ensemblon<NoeudDeclarationVariable *, 16> globales_utilisees{};
    kuri::ensemblon<Type *, 16> types_utilises{};

    void fusionne(DonneesDependance const &autre);

    void efface()
    {
        fonctions_utilisees.efface();
        globales_utilisees.efface();
        types_utilises.efface();
    }
};

void imprime_dependances(const DonneesDependance &dependances,
                         EspaceDeTravail *espace,
                         const char *message,
                         std::ostream &flux);

struct GrapheDependance {
  private:
    /* Index de la visite. Chaque traversée du graphe peut avoir un index de visite différent
     * (#prepare_visite() doit être utilisé pour indiquer une nouvelle visite). Les noeuds sont
     * considérés comme visités si leur index de visite est celui de celle-ci. */
    int index_visite = 0;

  public:
    tableau_page<NoeudDependance> noeuds{};

    // CRÉE (:FONCTION { nom = $nom })
    NoeudDependance *crée_noeud_fonction(NoeudDeclarationEnteteFonction *noeud_syntaxique);

    // CRÉE (:GLOBALE { nom = $nom })
    NoeudDependance *crée_noeud_globale(NoeudDeclarationVariable *noeud_syntaxique);

    // FUSIONNE (:TYPE { index = $index })
    NoeudDependance *crée_noeud_type(Type *type);

    // CHERCHE (type1 :TYPE { index = $index1 })
    // CHERCHE (type2 :TYPE { index = $index1 })
    // FUSIONNE (type1)-[:UTILISE_TYPE]->(type2)
    void connecte_type_type(NoeudDependance &type1,
                            NoeudDependance &type2,
                            TypeRelation type_rel = TypeRelation::UTILISE_TYPE);

    void connecte_type_type(Type *type1,
                            Type *type2,
                            TypeRelation type_rel = TypeRelation::UTILISE_TYPE);

    void ajoute_dependances(NoeudDependance &noeud, DonneesDependance &donnees);

    void connecte_noeuds(NoeudDependance &noeud1,
                         NoeudDependance &noeud2,
                         TypeRelation type_relation);

    void rassemble_statistiques(Statistiques &stats) const;

    void reduction_transitive();

    void prepare_visite();

    void rassemble_fonctions_utilisees(NoeudDependance *racine,
                                       kuri::tableau<AtomeFonction *> &fonctions,
                                       kuri::ensemble<AtomeFonction *> &utilises);

    /* Crée un noeud de dépendance pour le noeud spécifié en paramètre, et retourne un pointeur
     * vers celui-ci. Retourne nul si le noeud n'est pas supposé avoir un noeud de dépendance. */
    NoeudDependance *garantie_noeud_dépendance(EspaceDeTravail *espace, NoeudExpression *noeud);

    template <typename Rappel>
    void traverse(NoeudDependance *racine, Rappel rappel)
    {
        racine->index_visite = index_visite;

        for (auto const &relation : racine->relations().plage()) {
            auto accepte = relation.type == TypeRelation::UTILISE_TYPE;
            accepte |= relation.type == TypeRelation::UTILISE_FONCTION;
            accepte |= relation.type == TypeRelation::UTILISE_GLOBALE;

            if (!accepte) {
                continue;
            }

            if (relation.noeud_fin->index_visite == index_visite) {
                continue;
            }

            traverse(relation.noeud_fin, rappel);
        }

        rappel(racine);
    }
};

void imprime_fonctions_inutilisees(GrapheDependance &graphe_dependance);

/* Impression des dépendances directes. */
[[nodiscard]] kuri::chaine imprime_dépendances(NoeudDeclarationSymbole const *symbole);
[[nodiscard]] kuri::chaine imprime_dépendances(Type const *type);
