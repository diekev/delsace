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

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/structures/dico.hh"

#include "structures/table_hachage.hh"

#include "representation_intermediaire/instructions.hh"

#include "parsage/modules.hh"

#include "erreur.h"
#include "messagere.hh"
#include "options.hh"

struct Coulisse;
struct ConstructriceRI;
struct Programme;

/* IPA :
 * - crée_un_espace_de_travail
 * - espace_de_travail_défaut
 *
 * Problèmes :
 * - les modules ne sont ouvert qu'une seule fois
 * - il faudra stocker les modules pour chaque espace de travail, et partager les données
 * constantes des fichiers
 * - séparer les données constantes des données dynamiques
 * -- données constantes : tampon du fichier, lexèmes
 * -- données dynamiques : arbres syntaxiques, types, noeuds dépendances
 */
struct EspaceDeTravail {
  private:
    std::atomic<int> nombre_taches_chargement = 0;
    std::atomic<int> nombre_taches_lexage = 0;
    std::atomic<int> nombre_taches_parsage = 0;
    std::atomic<int> nombre_taches_typage = 0;
    std::atomic<int> nombre_taches_ri = 0;
    std::atomic<int> nombre_taches_execution = 0;
    std::atomic<int> nombre_taches_optimisation = 0;

    PhaseCompilation phase = PhaseCompilation::PARSAGE_EN_COURS;

  public:
    kuri::chaine nom{};
    OptionsDeCompilation options{};

    Programme *programme = nullptr;

    /* mise en cache de la fonction principale, si vue dans la Syntaxeuse */
    NoeudDeclarationEnteteFonction *fonction_principale = nullptr;

    NoeudDeclarationEnteteFonction *fonction_point_d_entree = nullptr;

    /* Le métaprogramme controlant la compilation dans cette espace. */
    MetaProgramme *metaprogramme = nullptr;

    /* pour activer ou désactiver les optimisations */
    bool optimisations = false;
    mutable std::atomic<bool> possede_erreur{false};

    Compilatrice &m_compilatrice;

    EspaceDeTravail(Compilatrice &compilatrice, OptionsDeCompilation opts, kuri::chaine nom_);

    COPIE_CONSTRUCT(EspaceDeTravail);

    ~EspaceDeTravail();

    POINTEUR_NUL(EspaceDeTravail)

    long memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats) const;

    void tache_chargement_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_lexage_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_parsage_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_typage_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_ri_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_optimisation_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_execution_ajoutee(dls::outils::Synchrone<Messagere> &messagere);

    void tache_chargement_terminee(dls::outils::Synchrone<Messagere> &messagere, Fichier *fichier);
    void tache_lexage_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_parsage_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_typage_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_ri_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_optimisation_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_execution_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_generation_objet_terminee(dls::outils::Synchrone<Messagere> &messagere);
    void tache_liaison_executable_terminee(dls::outils::Synchrone<Messagere> &messagere);

    bool peut_generer_code_final() const;
    bool parsage_termine() const;

    Message *change_de_phase(dls::outils::Synchrone<Messagere> &messagere,
                             PhaseCompilation nouvelle_phase);
    PhaseCompilation phase_courante() const;

    void rapporte_avertissement(NoeudExpression *site, kuri::chaine_statique message) const;
    void rapporte_avertissement(kuri::chaine const &fichier,
                                int ligne,
                                kuri::chaine const &message) const;

    Erreur rapporte_erreur(NoeudExpression const *site,
                           kuri::chaine_statique message,
                           erreur::Genre genre = erreur::Genre::NORMAL) const;
    Erreur rapporte_erreur(kuri::chaine const &fichier,
                           int ligne,
                           kuri::chaine const &message) const;
    Erreur rapporte_erreur_sans_site(const kuri::chaine &message,
                                     erreur::Genre genre = erreur::Genre::NORMAL) const;

    Compilatrice &compilatrice()
    {
        return m_compilatrice;
    }

    Compilatrice &compilatrice() const
    {
        return m_compilatrice;
    }

    void imprime_compte_taches(std::ostream &os) const;
};
