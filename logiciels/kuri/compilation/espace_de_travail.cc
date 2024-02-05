/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "espace_de_travail.hh"

#include <fstream>
#include <iostream>

#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"

#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/instructions.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "compilatrice.hh"
#include "coulisse.hh"
#include "programme.hh"
#include "statistiques/statistiques.hh"
#include "utilitaires/log.hh"

#define NOMBRE_DE_TACHES(x) nombre_de_taches[size_t(GenreTâche::x)]

/* ************************************************************************** */

EspaceDeTravail::EspaceDeTravail(Compilatrice &compilatrice,
                                 OptionsDeCompilation opts,
                                 kuri::chaine nom_)
    : nom(nom_), options(opts), m_compilatrice(compilatrice)
{
    programme = Programme::crée_pour_espace(this);

    POUR (nombre_de_taches) {
        it = 0;
    }
}

EspaceDeTravail::~EspaceDeTravail()
{
    memoire::deloge("Programme", programme);
}

int64_t EspaceDeTravail::memoire_utilisee() const
{
    auto memoire = int64_t(0);
    memoire += programme->memoire_utilisee();
    return memoire;
}

void EspaceDeTravail::rassemble_statistiques(Statistiques &stats) const
{
    programme->rassemble_statistiques(stats);
}

void EspaceDeTravail::tache_ajoutee(GenreTâche genre_tache,
                                    dls::outils::Synchrone<Messagère> &messagère)
{
    nombre_de_taches[size_t(genre_tache)] += 1;
    regresse_phase_pour_tache_ajoutee(genre_tache, messagère);
}

void EspaceDeTravail::tache_terminee(GenreTâche genre_tache,
                                     dls::outils::Synchrone<Messagère> &messagère,
                                     bool peut_envoyer_changement_de_phase)
{
    nombre_de_taches[size_t(genre_tache)] -= 1;
    assert(nombre_de_taches[size_t(genre_tache)] >= 0);
    progresse_phase_pour_tache_terminee(genre_tache, messagère, peut_envoyer_changement_de_phase);
}

void EspaceDeTravail::progresse_phase_pour_tache_terminee(
    GenreTâche genre_tache,
    dls::outils::Synchrone<Messagère> &messagère,
    bool peut_envoyer_changement_de_phase)
{
    PhaseCompilation nouvelle_phase = phase;
    switch (genre_tache) {
        case GenreTâche::CHARGEMENT:
        case GenreTâche::LEXAGE:
        {
            nouvelle_phase = PhaseCompilation::PARSAGE_EN_COURS;
            break;
        }
        case GenreTâche::PARSAGE:
        {
            if (parsage_termine()) {
                nouvelle_phase = PhaseCompilation::PARSAGE_TERMINE;
            }
            break;
        }
        case GenreTâche::TYPAGE:
        {
            if (nombre_de_taches[size_t(genre_tache)] == 0 &&
                phase == PhaseCompilation::PARSAGE_TERMINE && peut_envoyer_changement_de_phase) {
                nouvelle_phase = PhaseCompilation::TYPAGE_TERMINE;

                /* Il est possible que les dernières tâches de typages soient pour des choses qui
                 * n'ont pas de RI, donc avançons jusqu'à GENERATION_CODE_TERMINEE. */
                if (nombre_de_taches[size_t(GenreTâche::GENERATION_RI)] == 0) {
                    /* Notifie pour le changement de phase précédent. */
                    change_de_phase(messagère, nouvelle_phase);
                    nouvelle_phase = PhaseCompilation::GENERATION_CODE_TERMINEE;
                }
            }
            break;
        }
        case GenreTâche::GENERATION_RI:
        case GenreTâche::OPTIMISATION:
        {
            if (nombre_de_taches[size_t(GenreTâche::GENERATION_RI)] == 0 &&
                nombre_de_taches[size_t(GenreTâche::OPTIMISATION)] == 0 &&
                phase == PhaseCompilation::TYPAGE_TERMINE) {
                nouvelle_phase = PhaseCompilation::GENERATION_CODE_TERMINEE;
            }
            break;
        }
        case GenreTâche::DORS:
        case GenreTâche::COMPILATION_TERMINEE:
        case GenreTâche::CREATION_FONCTION_INIT_TYPE:
        case GenreTâche::CONVERSION_NOEUD_CODE:
        case GenreTâche::ENVOIE_MESSAGE:
        case GenreTâche::GENERATION_CODE_MACHINE:
        case GenreTâche::LIAISON_PROGRAMME:
        case GenreTâche::EXECUTION:
        case GenreTâche::NOMBRE_ELEMENTS:
        {
            break;
        }
    }

    if (nouvelle_phase != phase) {
        change_de_phase(messagère, nouvelle_phase);
    }
}

void EspaceDeTravail::regresse_phase_pour_tache_ajoutee(
    GenreTâche genre_tache, dls::outils::Synchrone<Messagère> &messagère)
{
    PhaseCompilation nouvelle_phase = phase;
    switch (genre_tache) {
        case GenreTâche::CHARGEMENT:
        case GenreTâche::LEXAGE:
        case GenreTâche::PARSAGE:
        {
            nouvelle_phase = PhaseCompilation::PARSAGE_EN_COURS;
            break;
        }
        case GenreTâche::TYPAGE:
        {
            if (phase > PhaseCompilation::PARSAGE_TERMINE) {
                nouvelle_phase = PhaseCompilation::PARSAGE_TERMINE;
            }
            break;
        }
        case GenreTâche::GENERATION_RI:
        case GenreTâche::OPTIMISATION:
        {
            if (phase > PhaseCompilation::TYPAGE_TERMINE) {
                nouvelle_phase = PhaseCompilation::TYPAGE_TERMINE;
            }
            break;
        }
        case GenreTâche::GENERATION_CODE_MACHINE:
        {
            if (phase > PhaseCompilation::APRES_GENERATION_OBJET) {
                nouvelle_phase = PhaseCompilation::APRES_GENERATION_OBJET;
            }
            break;
        }
        case GenreTâche::LIAISON_PROGRAMME:
        {
            if (phase > PhaseCompilation::APRES_LIAISON_EXECUTABLE) {
                nouvelle_phase = PhaseCompilation::APRES_LIAISON_EXECUTABLE;
            }
            break;
        }
        case GenreTâche::DORS:
        case GenreTâche::COMPILATION_TERMINEE:
        case GenreTâche::CREATION_FONCTION_INIT_TYPE:
        case GenreTâche::CONVERSION_NOEUD_CODE:
        case GenreTâche::ENVOIE_MESSAGE:
        case GenreTâche::EXECUTION:
        case GenreTâche::NOMBRE_ELEMENTS:
        {
            break;
        }
    }

    if (nouvelle_phase != phase) {
        id_phase += 1;
        change_de_phase(messagère, nouvelle_phase);
    }
}

bool EspaceDeTravail::peut_generer_code_final() const
{
    if (phase != PhaseCompilation::GENERATION_CODE_TERMINEE) {
        return false;
    }

    if (NOMBRE_DE_TACHES(EXECUTION) == 0) {
        return true;
    }

    if (NOMBRE_DE_TACHES(EXECUTION) == 1 && metaprogramme) {
        return true;
    }

    return false;
}

bool EspaceDeTravail::parsage_termine() const
{
    return NOMBRE_DE_TACHES(CHARGEMENT) == 0 && NOMBRE_DE_TACHES(LEXAGE) == 0 &&
           NOMBRE_DE_TACHES(PARSAGE) == 0;
}

void EspaceDeTravail::imprime_compte_taches(std::ostream &os) const
{
    os << "nombre_taches_chargement : " << NOMBRE_DE_TACHES(CHARGEMENT) << '\n';
    os << "nombre_taches_lexage : " << NOMBRE_DE_TACHES(LEXAGE) << '\n';
    os << "nombre_taches_parsage : " << NOMBRE_DE_TACHES(PARSAGE) << '\n';
    os << "nombre_taches_typage : " << NOMBRE_DE_TACHES(TYPAGE) << '\n';
    os << "nombre_taches_ri : " << NOMBRE_DE_TACHES(GENERATION_RI) << '\n';
    os << "nombre_taches_execution : " << NOMBRE_DE_TACHES(EXECUTION) << '\n';
    os << "nombre_taches_optimisation : " << NOMBRE_DE_TACHES(OPTIMISATION) << '\n';
}

Message *EspaceDeTravail::change_de_phase(dls::outils::Synchrone<Messagère> &messagère,
                                          PhaseCompilation nouvelle_phase)
{
#define IMPRIME_CHANGEMENT_DE_PHASE(nom_espace)                                                   \
    if (nom == nom_espace) {                                                                      \
        dbg() << __func__ << " : " << nouvelle_phase << ", id " << id_phase;                      \
    }

    if (phase == PhaseCompilation::COMPILATION_TERMINEE) {
        /* Il est possible qu'un espace ajoute des choses à compiler mais que celui-ci n'utilise
         * pas le code. Or, les tâches de typage subséquentes feront regresser sa phase de
         * compilation si la compilation est terminée pour celui-ci. Si tel est le cas, la
         * compilation sera infinie. Empêchons donc de modifier la phase de compilation de l'espace
         * si sa compilation fut déjà terminée. */
        return nullptr;
    }

    phase = nouvelle_phase;
    return messagère->ajoute_message_phase_compilation(this);

#undef IMPRIME_CHANGEMENT_DE_PHASE
}

SiteSource EspaceDeTravail::site_source_pour(const NoeudExpression *noeud) const
{
    if (!noeud) {
        return {};
    }

    auto const lexeme = noeud->lexeme;
    auto const fichier = compilatrice().fichier(lexeme->fichier);
    return SiteSource::cree(fichier, lexeme);
}

void EspaceDeTravail::rapporte_avertissement(const NoeudExpression *site,
                                             kuri::chaine_statique message) const
{
    dbg() << genere_entete_erreur(
        this, site_source_pour(site), erreur::Genre::AVERTISSEMENT, message);
}

void EspaceDeTravail::rapporte_avertissement(kuri::chaine const &chemin_fichier,
                                             int ligne,
                                             kuri::chaine const &message) const
{
    const Fichier *f = m_compilatrice.fichier(chemin_fichier);
    dbg() << genere_entete_erreur(
        this, SiteSource(f, ligne - 1), erreur::Genre::AVERTISSEMENT, message);
}

Erreur EspaceDeTravail::rapporte_erreur(NoeudExpression const *site,
                                        kuri::chaine_statique message,
                                        erreur::Genre genre) const
{
    possède_erreur = true;

    if (!site) {
        return rapporte_erreur_sans_site(message, genre);
    }

    return ::rapporte_erreur(this, site_source_pour(site), message, genre);
}

Erreur EspaceDeTravail::rapporte_erreur(kuri::chaine const &chemin_fichier,
                                        int ligne,
                                        kuri::chaine const &message,
                                        erreur::Genre genre) const
{
    possède_erreur = true;
    auto fichier = compilatrice().fichier(chemin_fichier);
    return ::rapporte_erreur(this, SiteSource(fichier, ligne), message, genre);
}

Erreur EspaceDeTravail::rapporte_erreur(SiteSource site,
                                        kuri::chaine const &message,
                                        erreur::Genre genre) const
{
    possède_erreur = true;
    return ::rapporte_erreur(this, site, message, genre);
}

Erreur EspaceDeTravail::rapporte_erreur_sans_site(const kuri::chaine &message,
                                                  erreur::Genre genre) const
{
    possède_erreur = true;
    return ::rapporte_erreur(this, {}, message, genre);
}
