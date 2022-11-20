/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "espace_de_travail.hh"

#include <fstream>

#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"

#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/instructions.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "compilatrice.hh"
#include "coulisse.hh"
#include "programme.hh"
#include "statistiques/statistiques.hh"

/* ************************************************************************** */

EspaceDeTravail::EspaceDeTravail(Compilatrice &compilatrice,
                                 OptionsDeCompilation opts,
                                 kuri::chaine nom_)
    : nom(nom_), options(opts), m_compilatrice(compilatrice)
{
    programme = Programme::cree_pour_espace(this);
}

EspaceDeTravail::~EspaceDeTravail()
{
    memoire::deloge("Programme", programme);
}

long EspaceDeTravail::memoire_utilisee() const
{
    auto memoire = 0l;
    memoire += programme->memoire_utilisee();
    return memoire;
}

void EspaceDeTravail::rassemble_statistiques(Statistiques &stats) const
{
    programme->rassemble_statistiques(stats);
}

void EspaceDeTravail::tache_chargement_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
    change_de_phase(messagere, PhaseCompilation::PARSAGE_EN_COURS);
    nombre_taches_chargement += 1;
}

void EspaceDeTravail::tache_lexage_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
    change_de_phase(messagere, PhaseCompilation::PARSAGE_EN_COURS);
    nombre_taches_lexage += 1;
}

void EspaceDeTravail::tache_parsage_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
    change_de_phase(messagere, PhaseCompilation::PARSAGE_EN_COURS);
    nombre_taches_parsage += 1;
}

void EspaceDeTravail::tache_typage_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
    if (phase > PhaseCompilation::PARSAGE_TERMINE) {
        change_de_phase(messagere, PhaseCompilation::PARSAGE_TERMINE);
    }

    nombre_taches_typage += 1;
}

void EspaceDeTravail::tache_ri_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
    if (phase > PhaseCompilation::TYPAGE_TERMINE) {
        change_de_phase(messagere, PhaseCompilation::TYPAGE_TERMINE);
    }

    nombre_taches_ri += 1;
}

void EspaceDeTravail::tache_optimisation_ajoutee(dls::outils::Synchrone<Messagere> & /*messagere*/)
{
    nombre_taches_optimisation += 1;
}

void EspaceDeTravail::tache_execution_ajoutee(dls::outils::Synchrone<Messagere> & /*messagere*/)
{
    nombre_taches_execution += 1;
}

void EspaceDeTravail::tache_chargement_terminee(dls::outils::Synchrone<Messagere> &messagere,
                                                Fichier *fichier)
{
    messagere->ajoute_message_fichier_ferme(this, fichier->chemin());

    /* Une fois que nous avons fini de charger un fichier, il faut le lexer. */
    tache_lexage_ajoutee(messagere);

    nombre_taches_chargement -= 1;
    assert(nombre_taches_chargement >= 0);
}

void EspaceDeTravail::tache_lexage_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    /* Une fois que nous lexer quelque chose, il faut le parser. */
    tache_parsage_ajoutee(messagere);
    nombre_taches_lexage -= 1;
    assert(nombre_taches_lexage >= 0);
}

void EspaceDeTravail::tache_parsage_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    nombre_taches_parsage -= 1;
    assert(nombre_taches_parsage >= 0);

    if (parsage_termine()) {
        change_de_phase(messagere, PhaseCompilation::PARSAGE_TERMINE);
    }
}

void EspaceDeTravail::tache_typage_terminee(dls::outils::Synchrone<Messagere> &messagere,
                                            bool peut_envoyer_changement_de_phase)
{
    nombre_taches_typage -= 1;
    assert(nombre_taches_typage >= 0);

    if (nombre_taches_typage == 0 && phase == PhaseCompilation::PARSAGE_TERMINE &&
        peut_envoyer_changement_de_phase) {
        change_de_phase(messagere, PhaseCompilation::TYPAGE_TERMINE);

        /* Il est possible que les dernières tâches de typages soient pour des choses qui n'ont pas
         * de RI, donc avançons jusqu'à GENERATION_CODE_TERMINEE. */
        if (nombre_taches_ri == 0) {
            change_de_phase(messagere, PhaseCompilation::GENERATION_CODE_TERMINEE);
        }
    }
}

void EspaceDeTravail::tache_ri_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    nombre_taches_ri -= 1;
    assert(nombre_taches_ri >= 0);

    if (optimisations) {
        tache_optimisation_ajoutee(messagere);
    }

    if (nombre_taches_ri == 0 && nombre_taches_optimisation == 0 &&
        phase == PhaseCompilation::TYPAGE_TERMINE) {
        change_de_phase(messagere, PhaseCompilation::GENERATION_CODE_TERMINEE);
    }
}

void EspaceDeTravail::tache_optimisation_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    nombre_taches_optimisation -= 1;
    assert(nombre_taches_optimisation >= 0);

    if (nombre_taches_ri == 0 && nombre_taches_optimisation == 0 &&
        phase == PhaseCompilation::TYPAGE_TERMINE) {
        change_de_phase(messagere, PhaseCompilation::GENERATION_CODE_TERMINEE);
    }
}

void EspaceDeTravail::tache_execution_terminee(dls::outils::Synchrone<Messagere> & /*messagere*/)
{
    nombre_taches_execution -= 1;
    assert(nombre_taches_execution >= 0);
}

void EspaceDeTravail::tache_generation_objet_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    change_de_phase(messagere, PhaseCompilation::APRES_GENERATION_OBJET);
}

void EspaceDeTravail::tache_liaison_executable_terminee(
    dls::outils::Synchrone<Messagere> &messagere)
{
    change_de_phase(messagere, PhaseCompilation::APRES_LIAISON_EXECUTABLE);
}

bool EspaceDeTravail::peut_generer_code_final() const
{
    if (phase != PhaseCompilation::GENERATION_CODE_TERMINEE) {
        return false;
    }

    if (nombre_taches_execution == 0) {
        return true;
    }

    if (nombre_taches_execution == 1 && metaprogramme) {
        return true;
    }

    return false;
}

bool EspaceDeTravail::parsage_termine() const
{
    return nombre_taches_chargement == 0 && nombre_taches_lexage == 0 &&
           nombre_taches_parsage == 0;
}

void EspaceDeTravail::imprime_compte_taches(std::ostream &os) const
{
    os << "nombre_taches_chargement : " << nombre_taches_chargement << '\n';
    os << "nombre_taches_lexage : " << nombre_taches_lexage << '\n';
    os << "nombre_taches_parsage : " << nombre_taches_parsage << '\n';
    os << "nombre_taches_typage : " << nombre_taches_typage << '\n';
    os << "nombre_taches_ri : " << nombre_taches_ri << '\n';
    os << "nombre_taches_execution : " << nombre_taches_execution << '\n';
    os << "nombre_taches_optimisation : " << nombre_taches_optimisation << '\n';
}

Message *EspaceDeTravail::change_de_phase(dls::outils::Synchrone<Messagere> &messagere,
                                          PhaseCompilation nouvelle_phase)
{
#define IMPRIME_CHANGEMENT_DE_PHASE(nom_espace)                                                   \
    if (nom == nom_espace) {                                                                      \
        std::cerr << __func__ << " : " << nouvelle_phase << '\n';                                 \
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
    return messagere->ajoute_message_phase_compilation(this);

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
    std::cerr << genere_entete_erreur(
        this, site_source_pour(site), erreur::Genre::AVERTISSEMENT, message);
}

void EspaceDeTravail::rapporte_avertissement(kuri::chaine const &chemin_fichier,
                                             int ligne,
                                             kuri::chaine const &message) const
{
    const Fichier *f = m_compilatrice.fichier(chemin_fichier);
    std::cerr << genere_entete_erreur(
        this, SiteSource(f, ligne - 1), erreur::Genre::AVERTISSEMENT, message);
}

Erreur EspaceDeTravail::rapporte_erreur(NoeudExpression const *site,
                                        kuri::chaine_statique message,
                                        erreur::Genre genre) const
{
    possede_erreur = true;

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
    possede_erreur = true;
    auto fichier = compilatrice().fichier(chemin_fichier);
    return ::rapporte_erreur(this, SiteSource(fichier, ligne), message, genre);
}

Erreur EspaceDeTravail::rapporte_erreur(SiteSource site,
                                        kuri::chaine const &message,
                                        erreur::Genre genre) const
{
    possede_erreur = true;
    return ::rapporte_erreur(this, site, message, genre);
}

Erreur EspaceDeTravail::rapporte_erreur_sans_site(const kuri::chaine &message,
                                                  erreur::Genre genre) const
{
    possede_erreur = true;
    return ::rapporte_erreur(this, {}, message, genre);
}
