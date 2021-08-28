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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/ensemble.hh"

#include "structures/tableau.hh"

#include "message.hh"  // pour PhaseCompilation

struct AtomeGlobale;
struct AtomeFonction;
struct Coulisse;
struct EspaceDeTravail;
struct Fichier;
struct MetaProgramme;
struct NoeudDeclaration;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationVariable;
struct Type;

/* Machine à état pour la PhaseCompilation. */
class EtatCompilation {
    PhaseCompilation m_phase_courante{};

  public:
    void avance_etat()
    {
        if (m_phase_courante == PhaseCompilation::COMPILATION_TERMINEE) {
            return;
        }

        const auto index_phase_courante = static_cast<int>(m_phase_courante);
        const auto index_phase_suivante = index_phase_courante + 1;
        m_phase_courante = static_cast<PhaseCompilation>(index_phase_suivante);
    }

    void essaie_d_aller_a(PhaseCompilation cible)
    {
        if (static_cast<int>(cible) != static_cast<int>(m_phase_courante) + 1) {
            return;
        }

        m_phase_courante = cible;
    }

    void recule_vers(PhaseCompilation cible)
    {
        m_phase_courante = cible;
    }

    PhaseCompilation phase_courante() const
    {
        return m_phase_courante;
    }
};

struct DiagnostiqueEtatCompilation {
    bool tous_les_fichiers_sont_charges = false;
    bool tous_les_fichiers_sont_lexes = false;
    bool tous_les_fichiers_sont_parses = false;
    bool toutes_les_declarations_a_typer_le_sont = false;
    bool toutes_les_ri_sont_generees = false;

    Type *type_a_valider = nullptr;
    NoeudDeclaration *declaration_a_valider = nullptr;

    Type *ri_type_a_generer = nullptr;
    Type *fonction_initialisation_type_a_creer = nullptr;
    NoeudDeclaration *ri_declaration_a_generer = nullptr;
};

void imprime_diagnostique(DiagnostiqueEtatCompilation const &diagnositic);

/* Représentation d'un programme. Ceci peut être le programme final tel que généré par la
 * compilation ou bien un métaprogramme. Il contient toutes les globales et tous les types utilisés
 * par les fonctions qui le composent. */
struct Programme {
  protected:
    kuri::tableau<NoeudDeclarationEnteteFonction *> m_fonctions{};
    dls::ensemble<NoeudDeclarationEnteteFonction *> m_fonctions_utilisees{};

    kuri::tableau<NoeudDeclarationVariable *> m_globales{};
    dls::ensemble<NoeudDeclarationVariable *> m_globales_utilisees{};

    kuri::tableau<Type *> m_types{};
    dls::ensemble<Type *> m_types_utilises{};

    /* Tous les fichiers utilisés dans le programme. */
    kuri::tableau<Fichier *> m_fichiers{};
    dls::ensemble<Fichier *> m_fichiers_utilises{};

    EspaceDeTravail *m_espace = nullptr;

    /* Non-nul si le programme est celui d'un métaprogramme. */
    MetaProgramme *m_pour_metaprogramme = nullptr;

    // la coulisse à utiliser pour générer le code du programme
    Coulisse *m_coulisse = nullptr;

    EtatCompilation m_etat_compilation{};

  public:
    /* Création. */

    static Programme *cree(EspaceDeTravail *espace);

    static Programme *cree_pour_espace(EspaceDeTravail *espace);

    static Programme *cree_pour_metaprogramme(EspaceDeTravail *espace,
                                              MetaProgramme *metaprogramme);

    /* Modifications. */

    void ajoute_fonction(NoeudDeclarationEnteteFonction *fonction);

    void ajoute_globale(NoeudDeclarationVariable *globale);

    void ajoute_type(Type *type);

    bool possede(NoeudDeclarationEnteteFonction *fonction) const
    {
        return m_fonctions_utilisees.possede(fonction);
    }

    bool possede(NoeudDeclarationVariable *globale) const
    {
        return m_globales_utilisees.possede(globale);
    }

    bool possede(Type *type) const
    {
        return m_types_utilises.possede(type);
    }

    kuri::tableau<NoeudDeclarationEnteteFonction *> const &fonctions() const
    {
        return m_fonctions;
    }

    kuri::tableau<NoeudDeclarationVariable *> const &globales() const
    {
        return m_globales;
    }

    kuri::tableau<Type *> const &types() const
    {
        return m_types;
    }

    /* Retourne vrai si toutes les fonctions, toutes les globales, et tous les types utilisés par
     * le programme ont eu leurs types validés. */
    bool typages_termines(DiagnostiqueEtatCompilation &diagnositique) const;

    /* Retourne vrai si toutes les fonctions, toutes les globales, et tous les types utilisés par
     * le programme ont eu leurs RI générées. */
    bool ri_generees(DiagnostiqueEtatCompilation &diagnositique) const;

    bool ri_generees() const;

    MetaProgramme *pour_metaprogramme() const
    {
        return m_pour_metaprogramme;
    }

    void ajoute_racine(NoeudDeclarationEnteteFonction *racine);

    Coulisse *coulisse() const
    {
        return m_coulisse;
    }

    EspaceDeTravail *espace() const
    {
        return m_espace;
    }

    DiagnostiqueEtatCompilation diagnositique_compilation() const;

    EtatCompilation ajourne_etat_compilation();

    void change_de_phase(PhaseCompilation phase);

  private:
    void verifie_etat_compilation_fichier(DiagnostiqueEtatCompilation &diagnostique) const;

    void ajoute_fichier(Fichier *fichier);
};

enum {
    IMPRIME_TOUT = 0,
    IMPRIME_TYPES = 1,
    IMPRIME_FONCTIONS = 2,
    IMPRIME_GLOBALES = 4,
};

void imprime_contenu_programme(Programme const &programme, unsigned int quoi, std::ostream &os);

/* La représentation intermédiaire des fonctions et globles contenues dans un Programme, ainsi que
 * tous les types utilisées. */
struct ProgrammeRepreInter {
    kuri::tableau<AtomeGlobale *> globales{};
    kuri::tableau<AtomeFonction *> fonctions{};
    kuri::tableau<Type *> types{};

    void ajoute_fonction(AtomeFonction *fonction);
};

void imprime_contenu_programme(const ProgrammeRepreInter &programme,
                               unsigned int quoi,
                               std::ostream &os);

ProgrammeRepreInter representation_intermediaire_programme(Programme const &programme);
