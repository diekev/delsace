/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

namespace kuri {
struct chaine;
struct chaine_statique;
struct chemin_systeme;
}  // namespace kuri

enum class ArchitectureCible : int;

/* Retourne le nom suffixé de l'extension native pour un fichier objet. */
kuri::chaine nom_fichier_objet_pour(kuri::chaine_statique nom_base);

/* Retourne un chemin dans le dossier temporaire du système pour un fichier objet du nom de base
 * donné. */
kuri::chemin_systeme chemin_fichier_objet_temporaire_pour(kuri::chaine_statique nom_base);

/* Retourne le nom suffixé de l'extension native pour une bibliothèque dynamique. */
kuri::chaine nom_bibliothèque_dynamique_pour(kuri::chaine_statique nom_base);

/* Retourne un chemin dans le dossier temporaire du système pour une bibliothèque dynamique du nom
 * de base donné. */
kuri::chemin_systeme chemin_bibliothèque_dynamique_temporaire_pour(kuri::chaine_statique nom_base);

/* Retourne le nom suffixé de l'extension native pour une bibliothèque statique. */
kuri::chaine nom_bibliothèque_statique_pour(kuri::chaine_statique nom_base);

/* Retourne un chemin dans le dossier temporaire du système pour une bibliothèque statique du nom
 * de base donné. */
kuri::chemin_systeme chemin_bibliothèque_statique_temporaire_pour(kuri::chaine_statique nom_base);

/* Retourne le nom suffixé de l'extension native pour fichier exécutable. */
kuri::chaine nom_executable_pour(kuri::chaine_statique nom_base);

/* Retourne un chemin dans le dossier temporaire du système pour un exécutable du nom de base
 * donné. */
kuri::chemin_systeme chemin_executable_temporaire_pour(kuri::chaine_statique nom_base);

/* Retourne le chemin vers le fichier objet des tables r16 pour l'architecture donné. */
kuri::chemin_systeme chemin_fichier_objet_r16(ArchitectureCible architecture_cible);

bool precompile_objet_r16(kuri::chemin_systeme const &chemin_racine_kuri);

bool compile_objet_r16(kuri::chemin_systeme const &chemin_racine_kuri,
                       ArchitectureCible architecture_cible);
