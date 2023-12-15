/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include <optional>

#include "structures/chaine.hh"

class Broyeuse;
struct Compilatrice;
struct CompilatriceRI;
struct EspaceDeTravail;
struct OptionsDeCompilation;
struct Programme;

/* Arguments pour la génération de code.
 *
 * compilatrice est requise pour :
 * - les chemins de compilations (racine_kuri)
 *
 * compilatrice_ri est requise pour :
 * - générer la fonction principale du programme
 */
struct ArgsGénérationCode {
    Compilatrice *compilatrice = nullptr;
    EspaceDeTravail *espace = nullptr;
    Programme const *programme = nullptr;
    CompilatriceRI *compilatrice_ri = nullptr;
    Broyeuse *broyeuse = nullptr;
};

ArgsGénérationCode crée_args_génération_code(Compilatrice &compilatrice,
                                             EspaceDeTravail &espace,
                                             Programme const *programme,
                                             CompilatriceRI &compilatrice_ri,
                                             Broyeuse &broyeuse);

/* Arguments pour la création des fichiers d'objets. */
struct ArgsCréationFichiersObjets {
    Compilatrice *compilatrice = nullptr;
    EspaceDeTravail *espace = nullptr;
    Programme const *programme = nullptr;
    CompilatriceRI *compilatrice_ri = nullptr;
};

ArgsCréationFichiersObjets crée_args_création_fichier_objet(Compilatrice &compilatrice,
                                                            EspaceDeTravail &espace,
                                                            Programme const *programme,
                                                            CompilatriceRI &compilatrice_ri);

/* Arguments pour la liaison de l'exécutable depuis le fichier objet.
 *
 * compilatrice est requise pour :
 * - les chemins de compilations (racine_kuri, bibliothèques, definitions, chemins)
 */
struct ArgsLiaisonObjets {
    Compilatrice *compilatrice = nullptr;
    EspaceDeTravail *espace = nullptr;
    Programme const *programme = nullptr;
};

ArgsLiaisonObjets crée_args_liaison_objets(Compilatrice &compilatrice,
                                           EspaceDeTravail &espace,
                                           Programme *programme);

struct ErreurCoulisse {
    kuri::chaine message;
};

struct Coulisse {
    double temps_génération_code = 0.0;
    double temps_fichier_objet = 0.0;
    double temps_exécutable = 0.0;

    virtual ~Coulisse();

    static Coulisse *crée_pour_options(OptionsDeCompilation options);

    static Coulisse *crée_pour_metaprogramme();

    static void détruit(Coulisse *coulisse);

    /* Génère le code machine. */
    bool crée_fichier_objet(ArgsGénérationCode const &args);

    /* Crée l'exécutable depuis le fichier objet. */
    bool crée_exécutable(ArgsLiaisonObjets const &args);

  protected:
    virtual std::optional<ErreurCoulisse> génère_code_impl(ArgsGénérationCode const &args) = 0;

    virtual std::optional<ErreurCoulisse> crée_fichier_objet_impl(
        ArgsCréationFichiersObjets const &args) = 0;

    virtual std::optional<ErreurCoulisse> crée_exécutable_impl(ArgsLiaisonObjets const &args) = 0;

  private:
    bool est_coulisse_métaprogramme() const;
};

/* Retourne le nom de sortie pour le fichier objet, soit le compilat intermédiaire, soit le final
 * dans le cas où la compilation résulte en un fichier objet. */
kuri::chaine nom_sortie_fichier_objet(OptionsDeCompilation const &ops);

/* Retourne le nom de sortie pour le fichier final (exécutable ou bibliothèque). */
kuri::chaine nom_sortie_résultat_final(OptionsDeCompilation const &ops);
