/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "metaprogramme.hh"

#include <fstream>
#include <iostream>

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "statistiques/statistiques.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "programme.hh"
#include "typage.hh"
#include "unite_compilation.hh"
#include "utilitaires/log.hh"

int DonnéesConstantesExécutions::ajoute_globale(Type const *type,
                                                IdentifiantCode *ident,
                                                const void *adresse_pour_exécution)
{
    auto globale = Globale{};
    globale.type = type;
    globale.ident = ident;
    globale.adresse = données_globales.taille();
    globale.adresse_pour_exécution = const_cast<void *>(adresse_pour_exécution);

    données_globales.redimensionne(données_globales.taille() +
                                   static_cast<int>(type->taille_octet));

    auto ptr = globales.taille();

    globales.ajoute(globale);

    return ptr;
}

void DonnéesConstantesExécutions::rassemble_statistiques(Statistiques &stats) const
{
    auto memoire_mv = 0l;
    memoire_mv += globales.taille_memoire();
    memoire_mv += données_constantes.taille_memoire();
    memoire_mv += données_globales.taille_memoire();
    memoire_mv += patchs_données_constantes.taille_memoire();

    stats.ajoute_mémoire_utilisée("Machine Virtuelle", memoire_mv);
}

MetaProgramme::~MetaProgramme()
{
    POUR (logueuses) {
        if (it) {
            memoire::deloge("Enchaineuse", it);
        }
    }
    memoire::deloge("Programme", programme);
}

Enchaineuse &MetaProgramme::donne_logueuse(TypeLogMétaprogramme type_log)
{
    assert(type_log != TypeLogMétaprogramme::NOMBRE_DE_LOGS);
    auto index_logueuse = static_cast<int>(type_log);
    if (logueuses[index_logueuse] == nullptr) {
        logueuses[index_logueuse] = memoire::loge<Enchaineuse>("Enchaineuse");
    }

    return *logueuses[index_logueuse];
}

static void imprime_date_format_iso(Date date, Enchaineuse &os)
{
#define IMPRIME_AVEC_ZERO(x)                                                                      \
    if ((x) < 10) {                                                                               \
        os << '0';                                                                                \
    }                                                                                             \
    os << x

    os << date.annee;
    IMPRIME_AVEC_ZERO(date.mois);
    IMPRIME_AVEC_ZERO(date.jour);
    IMPRIME_AVEC_ZERO(date.heure);
    IMPRIME_AVEC_ZERO(date.minute);
    IMPRIME_AVEC_ZERO(date.seconde);

#undef IMPRIME_AVEC_ZERO
}

kuri::chaine_statique MetaProgramme::donne_nom_pour_fichier_log()
{
    if (m_nom_pour_fichier_log) {
        return m_nom_pour_fichier_log;
    }

    Enchaineuse enchaineuse;

    auto const espace = unite->espace;
    auto const fichier_directive = espace->compilatrice().fichier(directive->lexeme->fichier);
    auto const hiérarchie = donne_les_noms_de_la_hiérarchie(directive->bloc_parent);
    auto const date = espace->compilatrice().donne_date_début_compilation();

    imprime_date_format_iso(date, enchaineuse);
    enchaineuse << '_';

    enchaineuse << directive->ident->nom;

    for (auto i = hiérarchie.taille() - 1; i >= 0; i--) {
        auto nom = hiérarchie[i];
        if (nom->nom.taille() != 0) {
            enchaineuse << '_' << nom->nom;
        }
    }

    enchaineuse << '_' << fichier_directive->nom() << '_' << directive->lexeme->ligne;

    auto nom = enchaineuse.chaine_statique();
    auto nombre_occurences = espace->compilatrice().donne_nombre_occurences_chaine(nom);
    if (nombre_occurences != 0) {
        enchaineuse << '_' << nombre_occurences;
    }

    m_nom_pour_fichier_log = enchaineuse.chaine();
    return m_nom_pour_fichier_log;
}

void MetaProgramme::vidange_logs_sur_disque()
{
    if (!m_le_log_d_empilage_doit_être_préservé) {
        memoire::deloge("Enchaineuse", logueuses[int(TypeLogMétaprogramme::PILE_DE_VALEURS)]);
    }

    for (int i = 0; i < int(TypeLogMétaprogramme::NOMBRE_DE_LOGS); i++) {
        vidange_log_sur_disque(TypeLogMétaprogramme(i));
    }
}

static kuri::chaine_statique donne_suffixe_pour_type_log(TypeLogMétaprogramme type_log)
{
    switch (type_log) {
        case TypeLogMétaprogramme::APPEL:
        {
            return "appel";
        }
        case TypeLogMétaprogramme::INSTRUCTION:
        {
            return "flux_instructions";
        }
        case TypeLogMétaprogramme::STAT_INSTRUCTION:
        {
            return "stats_instructions";
        }
        case TypeLogMétaprogramme::PROFILAGE:
        {
            return "profilage";
        }
        case TypeLogMétaprogramme::FUITES_DE_MÉMOIRE:
        {
            return "fuites_de_mémoire";
        }
        case TypeLogMétaprogramme::PILE_DE_VALEURS:
        {
            return "pile_de_valeurs";
        }
        case TypeLogMétaprogramme::NOMBRE_DE_LOGS:
        {
            break;
        }
    }
    return "inconnu";
}

void MetaProgramme::vidange_log_sur_disque(TypeLogMétaprogramme type_log)
{
    assert(type_log != TypeLogMétaprogramme::NOMBRE_DE_LOGS);
    auto index_logueuse = static_cast<int>(type_log);
    if (!logueuses[index_logueuse]) {
        return;
    }
    auto &logueuse = logueuses[index_logueuse];

    auto nom_fichier = enchaine(
        donne_nom_pour_fichier_log(), "_", donne_suffixe_pour_type_log(type_log), ".txt");

    auto chemin_fichier_entete = kuri::chemin_systeme::chemin_temporaire(nom_fichier);

    info() << "Écriture du log de métaprogramme " << chemin_fichier_entete << "...";

    std::ofstream of(vers_std_path(chemin_fichier_entete));
    logueuse->imprime_dans_flux(of);
    of.close();
}
