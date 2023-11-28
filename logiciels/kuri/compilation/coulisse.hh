/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "structures/chaine.hh"

class Broyeuse;
struct Compilatrice;
struct CompilatriceRI;
struct EspaceDeTravail;
struct OptionsDeCompilation;
struct Programme;

struct Coulisse {
    double temps_generation_code = 0.0;
    double temps_fichier_objet = 0.0;
    double temps_executable = 0.0;

    virtual ~Coulisse();

    static Coulisse *crée_pour_options(OptionsDeCompilation options);

    static Coulisse *crée_pour_metaprogramme();

    static void detruit(Coulisse *coulisse);

    /* Crée un fichier objet.
     *
     * compilatrice est requise pour :
     * - les chemins de compilations (racine_kuri)
     *
     * compilatrice_ri est requise pour :
     * - générer la fonction principale du programme
     */
    bool crée_fichier_objet(Compilatrice &compilatrice,
                            EspaceDeTravail &espace,
                            Programme const *programme,
                            CompilatriceRI &compilatrice_ri,
                            Broyeuse &);

    /* Crée l'exécutable depuis le fichier objet.
     *
     * compilatrice est requise pour :
     * - les chemins de compilations (racine_kuri, bibliothèques, definitions, chemins)
     */
    bool crée_exécutable(Compilatrice &compilatrice,
                         EspaceDeTravail &espace,
                         Programme *programme);

  protected:
    virtual bool génère_code_impl(Compilatrice &compilatrice,
                                  EspaceDeTravail &espace,
                                  Programme const *programme,
                                  CompilatriceRI &compilatrice_ri,
                                  Broyeuse &) = 0;

    virtual bool crée_fichier_objet_impl(Compilatrice &compilatrice,
                                         EspaceDeTravail &espace,
                                         Programme const *programme,
                                         CompilatriceRI &compilatrice_ri) = 0;

    virtual bool crée_exécutable_impl(Compilatrice &compilatrice,
                                      EspaceDeTravail &espace,
                                      Programme const *programme) = 0;

  private:
    bool est_coulisse_métaprogramme() const;
};

/* Retourne le nom de sortie pour le fichier objet, soit le compilat intermédiaire, soit le final
 * dans le cas où la compilation résulte en un fichier objet. */
kuri::chaine nom_sortie_fichier_objet(OptionsDeCompilation const &ops);

/* Retourne le nom de sortie pour le fichier final (exécutable ou bibliothèque). */
kuri::chaine nom_sortie_resultat_final(OptionsDeCompilation const &ops);
