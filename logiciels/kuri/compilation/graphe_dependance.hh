/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "biblinternes/outils/badge.hh"

#include "arbre_syntaxique/prodeclaration.hh"

#include "structures/ensemblon.hh"
#include "structures/tableau.hh"
#include "structures/tableau_compresse.hh"
#include "structures/tableau_page.hh"

struct AtomeFonction;
struct EspaceDeTravail;
struct GrapheDépendance;
struct NoeudDépendance;
struct Statistiques;
using Type = NoeudDéclarationType;

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
    ENUMERE_TYPE_RELATION_EX(UTILISE_INIT_TYPE)                                                   \
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
    NoeudDépendance *noeud_début = nullptr;
    NoeudDépendance *noeud_fin = nullptr;
};

inline bool operator==(Relation const &r1, Relation const &r2)
{
    return r1.type == r2.type && r1.noeud_début == r2.noeud_début && r1.noeud_fin == r2.noeud_fin;
}

struct NoeudDépendance {
  private:
    kuri::tableau_compresse<Relation> m_relations{};

    union {
        Type *m_type;
        NoeudDéclarationEntêteFonction *m_noeud_fonction;
        NoeudDéclarationVariable *m_noeud_globale;
    };

    TypeNoeudDependance m_type_noeud = TypeNoeudDependance::INVALIDE;

  public:
    int index_visite = 0;

    /* pour certains algorithmes de travail sur le graphe */
    char drapeaux = 0;

    explicit NoeudDépendance(NoeudDéclarationVariable *globale);
    explicit NoeudDépendance(NoeudDéclarationEntêteFonction *fonction);
    explicit NoeudDépendance(Type *t);

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

    inline NoeudDéclarationEntêteFonction *fonction() const
    {
        assert(est_fonction());
        return m_noeud_fonction;
    }

    inline NoeudDéclarationVariable *globale() const
    {
        assert(est_globale());
        return m_noeud_globale;
    }

    void ajoute_relation(Badge<GrapheDépendance>, const Relation &relation);

    kuri::tableau_compresse<Relation> const &relations() const;

    void relations(Badge<GrapheDépendance>, kuri::tableau_compresse<Relation> &&relations);
};

struct DonnéesDépendance {
    kuri::ensemblon<NoeudDéclarationEntêteFonction *, 16> fonctions_utilisées{};
    kuri::ensemblon<NoeudDéclarationVariable *, 16> globales_utilisées{};
    kuri::ensemblon<Type *, 16> types_utilisés{};
    kuri::ensemblon<Type *, 16> init_types_utilisés{};

    void fusionne(DonnéesDépendance const &autre);

    void efface()
    {
        fonctions_utilisées.efface();
        globales_utilisées.efface();
        types_utilisés.efface();
        init_types_utilisés.efface();
    }

    int64_t mémoire_utilisée() const
    {
        auto résultat = int64_t(0);
        résultat += fonctions_utilisées.mémoire_utilisée();
        résultat += types_utilisés.mémoire_utilisée();
        résultat += globales_utilisées.mémoire_utilisée();
        résultat += init_types_utilisés.mémoire_utilisée();
        return résultat;
    }
};

void imprime_dépendances(const DonnéesDépendance &dépendances,
                         EspaceDeTravail *espace,
                         const char *message,
                         std::ostream &flux);

struct GrapheDépendance {
  private:
    /* Index de la visite. Chaque traversée du graphe peut avoir un index de visite différent
     * (#prepare_visite() doit être utilisé pour indiquer une nouvelle visite). Les noeuds sont
     * considérés comme visités si leur index de visite est celui de celle-ci. */
    int index_visite = 0;

  public:
    kuri::tableau_page<NoeudDépendance> noeuds{};

    // CRÉE (:FONCTION { nom = $nom })
    NoeudDépendance *crée_noeud_fonction(NoeudDéclarationEntêteFonction *noeud_syntaxique);

    // CRÉE (:GLOBALE { nom = $nom })
    NoeudDépendance *crée_noeud_globale(NoeudDéclarationVariable *noeud_syntaxique);

    // FUSIONNE (:TYPE { index = $index })
    NoeudDépendance *crée_noeud_type(Type *type);

    // CHERCHE (type1 :TYPE { index = $index1 })
    // CHERCHE (type2 :TYPE { index = $index1 })
    // FUSIONNE (type1)-[:UTILISE_TYPE]->(type2)
    void connecte_type_type(NoeudDépendance &type1,
                            NoeudDépendance &type2,
                            TypeRelation type_rel = TypeRelation::UTILISE_TYPE);

    void connecte_type_type(Type *type1,
                            Type *type2,
                            TypeRelation type_rel = TypeRelation::UTILISE_TYPE);

    void ajoute_dépendances(NoeudDépendance &noeud, DonnéesDépendance &donnees);

    void connecte_noeuds(NoeudDépendance &noeud1,
                         NoeudDépendance &noeud2,
                         TypeRelation type_relation);

    void rassemble_statistiques(Statistiques &stats) const;

    void réduction_transitive();

    void prépare_visite();

    void rassemble_fonctions_utilisées(NoeudDépendance *racine,
                                       kuri::tableau<AtomeFonction *> &fonctions,
                                       kuri::ensemble<AtomeFonction *> &utilises);

    /* Crée un noeud de dépendance pour le noeud spécifié en paramètre, et retourne un pointeur
     * vers celui-ci. Retourne nul si le noeud n'est pas supposé avoir un noeud de dépendance. */
    NoeudDépendance *garantie_noeud_dépendance(EspaceDeTravail *espace, NoeudExpression *noeud);

    template <typename Rappel>
    void traverse(NoeudDépendance *racine, Rappel rappel)
    {
        racine->index_visite = index_visite;

        for (auto const &relation : racine->relations().plage()) {
            auto accepte = relation.type == TypeRelation::UTILISE_TYPE;
            accepte |= relation.type == TypeRelation::UTILISE_FONCTION;
            accepte |= relation.type == TypeRelation::UTILISE_GLOBALE;
            accepte |= relation.type == TypeRelation::UTILISE_INIT_TYPE;

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

void imprime_fonctions_inutilisées(GrapheDépendance &graphe_dépendance);

/* Impression des dépendances directes. */
[[nodiscard]] kuri::chaine imprime_dépendances(NoeudDéclarationSymbole const *symbole);
[[nodiscard]] kuri::chaine imprime_dépendances(Type const *type);
