/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "outils_independants_des_lexemes.hh"

#include <filesystem>
#include <fstream>

#include "structures/chaine.hh"
#include "structures/enchaineuse.hh"

static std::string vers_std_string(kuri::chaine_statique chn)
{
    return std::string(chn.pointeur(), static_cast<size_t>(chn.taille()));
}

bool remplace(std::string &std_string, std::string_view motif, std::string_view remplacement)
{
    bool remplacement_effectue = false;
    size_t index = 0;
    while (true) {
        /* Locate the substring to replace. */
        index = std_string.find(motif, index);

        if (index == std::string::npos) {
            break;
        }

        /* Make the replacement. */
        std_string.replace(index, motif.size(), remplacement);

        /* Advance index forward so the next iteration doesn't pick it up as well. */
        index += motif.size();
        remplacement_effectue = true;
    }

    return remplacement_effectue;
}

kuri::chaine supprime_accents(kuri::chaine_statique avec_accent)
{
    auto std_string = vers_std_string(avec_accent);

    remplace(std_string, "à", "a");
    remplace(std_string, "é", "e");
    remplace(std_string, "è", "e");
    remplace(std_string, "ê", "e");
    remplace(std_string, "û", "u");
    remplace(std_string, "ç", "c");
    remplace(std_string, "É", "E");
    remplace(std_string, "È", "E");
    remplace(std_string, "Ê", "E");

    return kuri::chaine(std_string.c_str(), static_cast<long>(std_string.size()));
}

void inclus_systeme(std::ostream &os, kuri::chaine_statique fichier)
{
    os << "#include <" << fichier << ">\n";
}

void inclus(std::ostream &os, kuri::chaine_statique fichier)
{
    os << "#include \"" << fichier << "\"\n";
}

void prodeclare_struct(std::ostream &os, kuri::chaine_statique nom)
{
    os << "struct " << nom << ";\n";
}

void prodeclare_struct_espace(std::ostream &os,
                              kuri::chaine_statique nom,
                              kuri::chaine_statique espace,
                              kuri::chaine_statique param_gabarit)
{
    os << "namespace " << espace << " {\n";
    if (param_gabarit != "") {
        os << "template <" << param_gabarit << ">\n";
    }
    os << "struct " << nom << ";\n";
    os << "}\n";
}

static bool fichier_sont_egaux(kuri::chaine_statique nom_source, kuri::chaine_statique nom_dest)
{
    std::ifstream f1(vers_std_string(nom_source));
    std::ifstream f2(vers_std_string(nom_dest));
    if (f1.fail() || f2.fail()) {
        return false;  // file problem
    }

    if (f1.tellg() != f2.tellg()) {
        return false;  // size mismatch
    }

    // seek back to beginning and use std::equal to compare contents
    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);
    return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
                      std::istreambuf_iterator<char>(),
                      std::istreambuf_iterator<char>(f2.rdbuf()));
}

void remplace_si_different(kuri::chaine_statique nom_source, kuri::chaine_statique nom_dest)
{
    if (nom_source == nom_dest) {
        return;
    }

    if (fichier_sont_egaux(nom_source, nom_dest)) {
        return;
    }

    std::filesystem::remove(vers_std_string(nom_dest));
    std::filesystem::copy(vers_std_string(nom_source), vers_std_string(nom_dest));
}
