/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

namespace kuri {
struct chaine;
struct chaine_statique;
struct chemin_systeme;
}  // namespace kuri

enum class ArchitectureCible : int;
struct OptionsDeCompilation;

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

/* Retourne le suffixe utilisé pour les chemins des bibliothèques des modules.
 *
 * La feuille du suffixe est de la forme "machine"-"système d'exploitation", où "machine" est soit
 * x86_64 ou i386, et "système d'exploitation" est l'identifiant du système, par exemple
 * "linux-gnu" pour GNU/Linux.
 *
 * Par exemple, pour un module "Module", le suffixe sur Linux 64-bit sera "lib/x86_64-linux-gnu",
 * pour générer le chemin "Module/lib/x86_64-linux-gnu". */
kuri::chemin_systeme suffixe_chemin_module_pour_bibliotheque(ArchitectureCible architecture_cible);

/* Retourne le chemin où la bibliothèque r16 sera installée pour l'architecture donnée. */
kuri::chemin_systeme chemin_de_base_pour_bibliothèque_r16(ArchitectureCible architecture_cible);

/* Retourne le chemin vers le fichier objet des tables r16 pour l'architecture donné. */
kuri::chemin_systeme chemin_fichier_objet_r16(ArchitectureCible architecture_cible);

/* Retourne une commande pour appeler le compilateur C afin de compiler un fichier objet depuis le
 * fichier donné en entrée.
 */
kuri::chaine commande_pour_fichier_objet(OptionsDeCompilation const &options,
                                         kuri::chaine_statique fichier_entrée,
                                         kuri::chaine_statique fichier_sortie);

bool precompile_objet_r16(kuri::chemin_systeme const &chemin_racine_kuri);

bool compile_objet_r16(kuri::chemin_systeme const &chemin_racine_kuri,
                       ArchitectureCible architecture_cible);
