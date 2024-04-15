/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include <filesystem>

#include "chaine.hh"
#include "chaine_statique.hh"
#include "tablet.hh"

namespace kuri {

/**
 * Représente un chemin vers quelque chose sur le système.
 */
struct chemin_systeme {
  private:
    chaine donnees{};

  public:
    chemin_systeme() = default;

    chemin_systeme(const char *str);
    chemin_systeme(chaine_statique chemin);
    chemin_systeme(chaine chemin);

    chaine_statique donne_chaine() const
    {
        return donnees;
    }

    /**
     * Retourne le nom de fichier à la fin du chemin.
     */
    chaine_statique nom_fichier() const;

    /**
     * Retourne le nom de fichier sans son extension à la fin du chemin.
     */
    chaine_statique nom_fichier_sans_extension() const;

    /**
     * Retourne l'extension du nom de fichier à la fin du chemin.
     */
    chaine_statique extension() const;

    /**
     * Retourne le chemin parent du chemin.
     */
    chaine_statique chemin_parent() const;

    /**
     * Retourne un nouveau chemin correspondant à celui-ci mais dont l'extension à été remplacée
     * par celle donnée.
     */
    chemin_systeme remplace_extension(chaine_statique extension) const;

    /**
     * Remplace le nom du fichier à la fin du chemin par celui donné;
     */
    void remplace_nom_fichier(chaine_statique nouveau_nom);

    const char *pointeur() const
    {
        return donnees.pointeur();
    }

    int64_t taille() const
    {
        return donnees.taille();
    }

    bool est_vide() const
    {
        return taille() == 0;
    }

    operator chaine_statique() const
    {
        return donnees;
    }

    operator bool() const
    {
        return !est_vide();
    }

    /* Fonctions statiques. */

    /**
     * Retourne le répertoire courant du processus.
     */
    static chemin_systeme chemin_courant();

    /**
     * Change le répertoire courant du processus par celui donné.
     */
    static void change_chemin_courant(chaine_statique chemin);

    /**
     * Retourne vrai si le chemin existe.
     */
    static bool existe(chaine_statique chemin);

    /**
     * Retourne vrai si le chemin pointe vers un dossier.
     */
    static bool est_dossier(chaine_statique chemin);

    /**
     * Retourne vrai si le chemin pointe vers un fichier régulier.
     */
    static bool est_fichier_regulier(chaine_statique chemin);

    /**
     * Retourne le chemin_systeme absolu pour le chemin donné.
     */
    static chemin_systeme absolu(chaine_statique chemin);

    /**
     * Retourne le chemin_systeme canonique et absolu pour le chemin donné.
     */
    static chemin_systeme canonique_absolu(chaine_statique chemin);

    /**
     * Retourne un chemin dans le dossier temporaire du système pour le nom de fichier donné.
     */
    static chemin_systeme chemin_temporaire(chaine_statique chemin);

    /**
     * Crée tous les dossiers du chemin spécifié.
     */
    static void crée_dossiers(chaine_statique chemin);

    /**
     * Retourne vrai si le chemin spécifié est un fichier kuri.
     */
    static bool est_fichier_kuri(chaine_statique chemin);

    /**
     * Retourne un tableau contenant les chemins des fichiers .kuri du dossier pointé par chemin.
     */
    static tablet<chemin_systeme, 16> fichiers_du_dossier(chaine_statique chemin);

    /**
     * Retourne un tableau contenant les chemins des fichiers .kuri du dossier et des sous-dossiers
     * récursivement pointé par chemin.
     */
    static tablet<chemin_systeme, 16> fichiers_du_dossier_recursif(chaine_statique chemin);

    /**
     * Supprime le fichier au chemin spécifié. Retourne faux si le fichier ne fut supprimé, ou si
     * le chemin n'est pas l'adresse d'un fichier régulier.
     */
    [[nodiscard]] static bool supprime(chaine_statique chemin);
};

chemin_systeme operator/(chemin_systeme const &chemin, chaine_statique chn);

std::filesystem::path vers_std_path(chaine_statique chn);

bool operator<(chemin_systeme const &chemin_a, chemin_systeme const &chemin_b);

std::ostream &operator<<(std::ostream &os, chemin_systeme const &chemin);

}  // namespace kuri
