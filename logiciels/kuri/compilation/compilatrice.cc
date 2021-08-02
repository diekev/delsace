/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "compilatrice.hh"

#include <stdarg.h>

#include "biblinternes/flux/outils.h"

#include "statistiques/statistiques.hh"

#include "parsage/lexeuse.hh"

#include "erreur.h"
#include "espace_de_travail.hh"
#include "ipa.hh"

/* ************************************************************************** */

Compilatrice::Compilatrice() : ordonnanceuse(this)
{
    this->definitions->ajoute("_REENTRANT");

    initialise_identifiants_ipa(*table_identifiants.verrou_ecriture());
}

Compilatrice::~Compilatrice()
{
    POUR ((*espaces_de_travail.verrou_ecriture())) {
        memoire::deloge("EspaceDeTravail", it);
    }
}

Module *Compilatrice::importe_module(EspaceDeTravail *espace,
                                     const kuri::chaine &nom,
                                     NoeudExpression const *site)
{
    auto chemin = dls::chaine(nom.pointeur(), nom.taille());

    if (!std::filesystem::exists(chemin.c_str())) {
        /* essaie dans la racine kuri */
        chemin = dls::chaine(racine_kuri.pointeur(), racine_kuri.taille()) + "/modules/" + chemin;

        if (!std::filesystem::exists(chemin.c_str())) {
            erreur::lance_erreur("Impossible de trouver le dossier correspondant au module",
                                 *espace,
                                 site,
                                 erreur::Genre::MODULE_INCONNU);

            return nullptr;
        }
    }

    if (!std::filesystem::is_directory(chemin.c_str())) {
        erreur::lance_erreur("Le nom du module ne pointe pas vers un dossier",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);

        return nullptr;
    }

    /* trouve le chemin absolu du module (cannonique pour supprimer les "../../" */
    auto chemin_absolu = std::filesystem::canonical(std::filesystem::absolute(chemin.c_str()));
    auto nom_dossier = chemin_absolu.filename();

    // @concurrence critique
    auto module = espace->trouve_ou_cree_module(
        sys_module,
        table_identifiants->identifiant_pour_nouvelle_chaine(nom_dossier.c_str()),
        chemin_absolu.c_str());

    if (module->importe) {
        return module;
    }

    module->importe = true;

    messagere->ajoute_message_module_ouvert(espace, module);

    for (auto const &entree : std::filesystem::directory_iterator(chemin_absolu)) {
        auto chemin_entree = entree.path();

        if (!std::filesystem::is_regular_file(chemin_entree)) {
            continue;
        }

        if (chemin_entree.extension() != ".kuri") {
            continue;
        }

        auto resultat = espace->trouve_ou_cree_fichier(
            sys_module, module, chemin_entree.stem().c_str(), chemin_entree.c_str(), importe_kuri);

        if (resultat.est<FichierNeuf>()) {
            ordonnanceuse->cree_tache_pour_chargement(espace,
                                                      resultat.resultat<FichierNeuf>().fichier);
        }
    }

    if (module->nom() == ID::Kuri) {
        auto resultat = espace->trouve_ou_cree_fichier(
            sys_module, module, "constantes", "constantes.kuri", false);

        if (resultat.est<FichierNeuf>()) {
            auto donnees_fichier = resultat.resultat<FichierNeuf>().fichier->donnees_constantes;
            if (!donnees_fichier->fut_charge) {
                const char *source = "SYS_EXP :: SystèmeExploitation.LINUX\n";
                donnees_fichier->charge_tampon(lng::tampon_source(source));
            }

            ordonnanceuse->cree_tache_pour_lexage(espace,
                                                  resultat.resultat<FichierNeuf>().fichier);
        }
    }

    messagere->ajoute_message_module_ferme(espace, module);

    return module;
}

/* ************************************************************************** */

void Compilatrice::ajoute_fichier_a_la_compilation(EspaceDeTravail *espace,
                                                   const kuri::chaine &nom,
                                                   Module *module,
                                                   NoeudExpression const *site)
{
    auto chemin = dls::chaine(module->chemin()) + dls::chaine(nom) + ".kuri";

    if (!std::filesystem::exists(chemin.c_str())) {
        erreur::lance_erreur("Impossible de trouver le fichier correspondant au module",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);
    }

    if (!std::filesystem::is_regular_file(chemin.c_str())) {
        erreur::lance_erreur("Le nom du fichier ne pointe pas vers un fichier régulier",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);
    }

    /* trouve le chemin absolu du fichier */
    auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

    auto resultat = espace->trouve_ou_cree_fichier(
        sys_module, module, nom, chemin_absolu.c_str(), importe_kuri);

    if (resultat.est<FichierNeuf>()) {
        ordonnanceuse->cree_tache_pour_chargement(espace,
                                                  resultat.resultat<FichierNeuf>().fichier);
    }
}

/* ************************************************************************** */

long Compilatrice::memoire_utilisee() const
{
    auto memoire = taille_de(Compilatrice);

    memoire += definitions->taille() * taille_de(dls::vue_chaine_compacte);

    memoire += ordonnanceuse->memoire_utilisee();
    memoire += table_identifiants->memoire_utilisee();

    memoire += gerante_chaine->memoire_utilisee();

    POUR ((*espaces_de_travail.verrou_lecture())) {
        memoire += it->memoire_utilisee();
    }

    memoire += messagere->memoire_utilisee();

    memoire += sys_module->memoire_utilisee();

    return memoire;
}

void Compilatrice::rassemble_statistiques(Statistiques &stats) const
{
    stats.memoire_compilatrice = memoire_utilisee();

    POUR ((*espaces_de_travail.verrou_lecture())) {
        it->rassemble_statistiques(stats);
    }

    stats.nombre_identifiants = table_identifiants->taille();

    sys_module->rassemble_stats(stats);
}

void Compilatrice::rapporte_erreur(EspaceDeTravail const *espace,
                                   kuri::chaine_statique message,
                                   erreur::Genre genre)
{
    if (espace) {
        // Toutes les erreurs ne transitent pas forcément par EspaceDeTravail
        // (comme les erreurs de syntaxage ou de lexage).
        espace->possede_erreur = true;
    }

    if (espace && espace->options.continue_si_erreur) {
        ordonnanceuse->supprime_toutes_les_taches_pour_espace(espace);
    }
    else {
        ordonnanceuse->supprime_toutes_les_taches();
        m_possede_erreur = true;
        m_code_erreur = genre;
    }

    std::cerr << message << '\n';
}

bool Compilatrice::possede_erreur(const EspaceDeTravail *espace) const
{
    return espace->possede_erreur;
}

/* ************************************************************************** */

EspaceDeTravail *Compilatrice::demarre_un_espace_de_travail(OptionsDeCompilation const &options,
                                                            const kuri::chaine &nom)
{
    auto espace = memoire::loge<EspaceDeTravail>("EspaceDeTravail", *this, options, nom);
    espace->module_kuri = importe_module(espace, "Kuri", {});
    espaces_de_travail->ajoute(espace);
    return espace;
}

ContexteLexage Compilatrice::contexte_lexage()
{
    auto rappel_erreur = [this](kuri::chaine message) {
        this->rapporte_erreur(nullptr, message, erreur::Genre::LEXAGE);
    };

    return {gerante_chaine, table_identifiants, rappel_erreur};
}

// -----------------------------------------------------------------------------
// Implémentation des fonctions d'interface afin d'éviter les erreurs, toutes les
// fonctions ne sont pas implémentées dans la Compilatrice, d'autres appelent
// directement les fonctions se trouvant sur EspaceDeTravail, ou enlignent la
// logique dans la MachineVirtuelle.

OptionsDeCompilation *Compilatrice::options_compilation()
{
    return &espace_de_travail_defaut->options;
}

void Compilatrice::ajourne_options_compilation(OptionsDeCompilation *options)
{
    espace_de_travail_defaut->options = *options;
}

void Compilatrice::ajoute_chaine_compilation(EspaceDeTravail *espace, kuri::chaine_statique c)
{
    auto module = espace->trouve_ou_cree_module(sys_module, ID::chaine_vide, "");
    ajoute_chaine_au_module(espace, module, c);
}

void Compilatrice::ajoute_chaine_au_module(EspaceDeTravail *espace,
                                           Module *module,
                                           kuri::chaine_statique c)
{
    auto chaine = dls::chaine(c.pointeur(), c.taille());

    chaines_ajoutees_a_la_compilation->ajoute(kuri::chaine(c.pointeur(), c.taille()));

    auto resultat = espace->trouve_ou_cree_fichier(
        sys_module, module, "métaprogramme", "", importe_kuri);

    if (resultat.est<FichierNeuf>()) {
        auto donnees_fichier = resultat.resultat<FichierNeuf>().fichier->donnees_constantes;
        if (!donnees_fichier->fut_charge) {
            donnees_fichier->charge_tampon(lng::tampon_source(std::move(chaine)));
        }
        ordonnanceuse->cree_tache_pour_lexage(espace, resultat.resultat<FichierNeuf>().fichier);
    }
}

void Compilatrice::ajoute_fichier_compilation(EspaceDeTravail *espace, kuri::chaine_statique c)
{
    auto vue = dls::chaine(c.pointeur(), c.taille());
    auto chemin = std::filesystem::current_path() / vue.c_str();

    if (!std::filesystem::exists(chemin)) {
        espace->rapporte_erreur_sans_site(enchaine("Le fichier ", chemin, " n'existe pas !"));
        return;
    }

    auto module = espace->trouve_ou_cree_module(sys_module, ID::chaine_vide, "");
    ajoute_fichier_a_la_compilation(espace, chemin.stem().c_str(), module, {});
}

Message const *Compilatrice::attend_message()
{
    if (!messagere->possede_message()) {
        return nullptr;
    }

    return messagere->defile();
}

EspaceDeTravail *Compilatrice::espace_defaut_compilation()
{
    return espace_de_travail_defaut;
}

static kuri::tableau<kuri::Lexeme> converti_tableau_lexemes(
    kuri::tableau<Lexeme, int> const &lexemes)
{
    auto resultat = kuri::tableau<kuri::Lexeme>();
    resultat.reserve(lexemes.taille());

    POUR (lexemes) {
        resultat.ajoute({static_cast<int>(it.genre), it.chaine});
    }

    return resultat;
}

kuri::tableau<kuri::Lexeme> Compilatrice::lexe_fichier(kuri::chaine_statique chemin_donne,
                                                       NoeudExpression const *site)
{
    auto espace = espace_de_travail_defaut;
    auto chemin = dls::chaine(chemin_donne.pointeur(), chemin_donne.taille());

    if (!std::filesystem::exists(chemin.c_str())) {
        erreur::lance_erreur("Impossible de trouver le fichier correspondant au chemin",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);
    }

    if (!std::filesystem::is_regular_file(chemin.c_str())) {
        erreur::lance_erreur("Le nom du fichier ne pointe pas vers un fichier régulier",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);
    }

    auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

    auto module = espace->module(ID::chaine_vide);

    auto resultat = espace->trouve_ou_cree_fichier(
        sys_module, module, chemin_absolu.stem().c_str(), chemin_absolu.c_str(), importe_kuri);

    if (resultat.est<FichierExistant>()) {
        auto donnees_fichier = resultat.resultat<FichierExistant>().fichier->donnees_constantes;
        return converti_tableau_lexemes(donnees_fichier->lexemes);
    }

    auto donnees_fichier = resultat.resultat<FichierNeuf>().fichier->donnees_constantes;
    auto tampon = charge_contenu_fichier(chemin);
    donnees_fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

    auto lexeuse = Lexeuse(
        contexte_lexage(), donnees_fichier, INCLUS_COMMENTAIRES | INCLUS_CARACTERES_BLANC);
    lexeuse.performe_lexage();

    return converti_tableau_lexemes(donnees_fichier->lexemes);
}

/* ************************************************************************** */

// fonction pour tester les appels de fonctions variadiques externe dans la machine virtuelle
int fonction_test_variadique_externe(int sentinel, ...)
{
    va_list ap;
    va_start(ap, sentinel);

    int i = 0;
    for (;; ++i) {
        int t = va_arg(ap, int);

        if (t == sentinel) {
            break;
        }
    }

    va_end(ap);

    return i;
}
