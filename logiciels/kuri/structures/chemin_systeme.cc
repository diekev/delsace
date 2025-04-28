/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "chemin_systeme.hh"

#ifdef _MSC_VER
#    include <windows.h>
#endif

#include "utilitaires/log.hh"

namespace kuri {

#ifdef _MSC_VER
/* https://stackoverflow.com/questions/6693010/how-do-i-use-multibytetowidechar */
std::string vers_utf8(const std::wstring &wstr)
{
    auto const count = WideCharToMultiByte(
        CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
    std::string str(count, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, nullptr, nullptr);
    return str;
}

static std::wstring vers_utf16(const std::string &str)
{
    auto const count = MultiByteToWideChar(
        CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), &wstr[0], count);
    return wstr;
}
#endif

std::filesystem::path vers_std_path(chaine_statique chn)
{
    std::string std_string(chn.pointeur(), size_t(chn.taille()));
#ifdef _MSC_VER
    /* Convertis vers UTF-16. */
    auto std_wstring = vers_utf16(std_string);
    auto std_path = std::filesystem::path(std_wstring);
#else
    auto std_path = std::filesystem::path(std_string);
#endif
    return std_path.make_preferred();
}

static chaine chaine_depuis_std_path(std::filesystem::path const &std_path)
{
#ifdef _MSC_VER
    /* Convertis vers UTF-8. */
    auto string = vers_utf8(std_path.wstring());
#else
    auto string = std_path.string();
#endif
    return {string.c_str(), int64_t(string.size())};
}

static chemin_systeme vers_chemin_systeme(std::filesystem::path const &chemin)
{
    return chemin_systeme(chaine_depuis_std_path(chemin));
}

/* Retourne le caractère utilisé par préférence pour le système. */
static char séparateur_préféré()
{
#ifdef _MSC_VER
    return '\\';
#else
    return '/';
#endif
}

static const char *trouve_depuis_la_fin(const char *debut, const char *fin, char motif)
{
    while (fin != debut) {
        --fin;

        if (*fin == motif) {
            break;
        }
    }

    return fin;
}

chemin_systeme::chemin_systeme(const char *str)
{
    /* Garantie que nous avons les sépérateurs préférés du système. */
    auto std_path = vers_std_path(str);
    donnees = chaine_depuis_std_path(std_path);
}

chemin_systeme::chemin_systeme(chaine_statique chemin)
{
    /* Garantie que nous avons les sépérateurs préférés du système. */
    auto std_path = vers_std_path(chemin);
    donnees = chaine_depuis_std_path(std_path);
}

chemin_systeme::chemin_systeme(chaine chemin)
{
    /* Garantie que nous avons les sépérateurs préférés du système. */
    auto std_path = vers_std_path(chemin);
    donnees = chaine_depuis_std_path(std_path);
}

chaine_statique chemin_systeme::nom_fichier() const
{
    auto debut = donnees.begin();
    auto fin = donnees.end();
    auto pos = trouve_depuis_la_fin(debut, fin, séparateur_préféré());
    auto distance = std::distance(debut, pos);
    auto taille = std::distance(pos, fin);
    return {donnees.pointeur() + distance + 1, taille - 1};
}

chaine_statique chemin_systeme::nom_fichier_sans_extension() const
{
    auto nom = nom_fichier();
    for (auto i = nom.taille() - 1; i >= 0; i--) {
        if (nom.pointeur()[i] == '.') {
            return {nom.pointeur(), i};
        }
    }

    return "";
}

chaine_statique chemin_systeme::extension() const
{
    auto debut = donnees.begin();
    auto fin = donnees.end();
    auto pos = trouve_depuis_la_fin(debut, fin, '.');

    if (pos == fin) {
        return "";
    }

    if (pos == debut) {
        if (*debut != '.') {
            return "";
        }
    }

    auto distance = std::distance(debut, pos);
    auto taille = std::distance(pos, donnees.end());
    return {donnees.pointeur() + distance, taille};
}

chaine_statique chemin_systeme::chemin_parent() const
{
    auto debut = donnees.begin();
    auto fin = donnees.end();
    auto pos = trouve_depuis_la_fin(debut, fin, séparateur_préféré());
    auto distance = std::distance(debut, pos);
    return {donnees.pointeur(), distance};
}

chemin_systeme chemin_systeme::remplace_extension(chaine_statique extension) const
{
    auto std_path = vers_std_path(donnees);
    auto const std_path_extension = vers_std_path(extension);
    auto result = std_path.replace_extension(std_path_extension);
    return vers_chemin_systeme(result);
}

void chemin_systeme::remplace_nom_fichier(chaine_statique nouveau_nom)
{
    auto std_path = vers_std_path(donnees);
    auto const std_path_nouveau_nom = vers_std_path(nouveau_nom);
    auto result = std_path.replace_filename(std_path_nouveau_nom);
    this->donnees = vers_chemin_systeme(result).donnees;
}

/* Fonctions statiques.  */

chemin_systeme chemin_systeme::chemin_courant()
{
    return vers_chemin_systeme(std::filesystem::current_path());
}

void chemin_systeme::change_chemin_courant(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);
    std::filesystem::current_path(std_path);
}

bool chemin_systeme::existe(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);
    return std::filesystem::exists(std_path);
}

bool chemin_systeme::est_dossier(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);
    return std::filesystem::is_directory(std_path);
}

bool chemin_systeme::est_fichier_regulier(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);
    return std::filesystem::is_regular_file(std_path);
}

chemin_systeme chemin_systeme::absolu(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);
    auto chemin_absolu = std::filesystem::absolute(std_path);
    return vers_chemin_systeme(chemin_absolu);
}

chemin_systeme chemin_systeme::canonique_absolu(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);
    auto chemin_absolu = std::filesystem::canonical(std::filesystem::absolute(std_path));
    return vers_chemin_systeme(chemin_absolu);
}

chemin_systeme chemin_systeme::chemin_temporaire(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);
    return vers_chemin_systeme(std::filesystem::temp_directory_path() / std_path);
}

void chemin_systeme::crée_dossiers(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);
    std::filesystem::create_directories(std_path);
}

static bool est_fichier_kuri_impl(const std::filesystem::path &chemin)
{
    if (!std::filesystem::is_regular_file(chemin)) {
        return false;
    }

    if (chemin.extension() != ".kuri") {
        return false;
    }

    return true;
}

bool chemin_systeme::est_fichier_kuri(chaine_statique chemin)
{
    return est_fichier_kuri_impl(vers_std_path(chemin));
}

tablet<chemin_systeme, 16> chemin_systeme::fichiers_du_dossier(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);

    tablet<chemin_systeme, 16> résultat;

    for (auto const &entree : std::filesystem::directory_iterator(std_path)) {
        auto chemin_entree = entree.path();
        if (!est_fichier_kuri_impl(chemin_entree)) {
            continue;
        }

        résultat.ajoute(vers_chemin_systeme(chemin_entree));
    }

    return résultat;
}

tablet<chemin_systeme, 16> chemin_systeme::fichiers_du_dossier_recursif(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);

    tablet<chemin_systeme, 16> résultat;

    for (auto const &entree : std::filesystem::recursive_directory_iterator(std_path)) {
        auto chemin_entree = entree.path();
        if (!est_fichier_kuri_impl(chemin_entree)) {
            continue;
        }

        résultat.ajoute(vers_chemin_systeme(chemin_entree));
    }

    return résultat;
}

bool chemin_systeme::supprime(chaine_statique chemin)
{
    auto const std_path = vers_std_path(chemin);
    if (!std::filesystem::exists(std_path)) {
        /* Pas d'erreur si le fichier n'existe pas. */
        return true;
    }

    if (!std::filesystem::is_regular_file(std_path)) {
        return false;
    }

    auto ec = std::error_code();
    if (!std::filesystem::remove(std_path, ec)) {
        dbg() << ec.message();
        return false;
    }

    return true;
}

static chemin_systeme concatene_chemins(chaine_statique base, chaine_statique feuille)
{
    auto const std_path_base = vers_std_path(base);
    auto const std_path_feuille = vers_std_path(feuille);
    return vers_chemin_systeme(std_path_base / std_path_feuille);
}

chemin_systeme operator/(chemin_systeme const &chemin, chaine_statique chn)
{
    return concatene_chemins(chemin, chn);
}

bool operator==(chemin_systeme const &chemin_a, chemin_systeme const &chemin_b)
{
    return kuri::chaine_statique(chemin_a.pointeur(), chemin_a.taille()) ==
           kuri::chaine_statique(chemin_b.pointeur(), chemin_b.taille());
}

bool operator!=(chemin_systeme const &chemin_a, chemin_systeme const &chemin_b)
{
    return !(chemin_a == chemin_b);
}

bool operator<(chemin_systeme const &chemin_a, chemin_systeme const &chemin_b)
{
    return kuri::chaine_statique(chemin_a.pointeur(), chemin_a.taille()) <
           kuri::chaine_statique(chemin_b.pointeur(), chemin_b.taille());
}

std::ostream &operator<<(std::ostream &os, chemin_systeme const &chemin)
{
    os << kuri::chaine_statique(chemin.pointeur(), chemin.taille());
    return os;
}

}  // namespace kuri
