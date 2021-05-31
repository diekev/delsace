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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "tacheronne.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/chrono/outils.hh"
#include "biblinternes/outils/assert.hh"
#include "biblinternes/structures/file.hh"

#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"

#include "arbre_syntaxique/assembleuse.hh"
#include "compilatrice.hh"
#include "coulisse.hh"
#include "espace_de_travail.hh"
#include "syntaxeuse.hh"
#include "validation_semantique.hh"

#include "representation_intermediaire/optimisations.hh"

const char *chaine_genre_tache(GenreTache genre)
{
#define ENUMERE_GENRE_TACHE_EX(genre)                                                             \
    case GenreTache::genre:                                                                       \
        return #genre;
    switch (genre) {
        ENUMERE_GENRES_TACHE
    }
#undef ENUMERE_GENRE_TACHE_EX

    return "erreur";
}

std::ostream &operator<<(std::ostream &os, GenreTache genre)
{
    os << chaine_genre_tache(genre);
    return os;
}

Tache Tache::dors(EspaceDeTravail *espace_)
{
    Tache t;
    t.genre = GenreTache::DORS;
    t.espace = espace_;
    return t;
}

Tache Tache::compilation_terminee()
{
    Tache t;
    t.genre = GenreTache::COMPILATION_TERMINEE;
    return t;
}

Tache Tache::genere_fichier_objet(EspaceDeTravail *espace_)
{
    Tache t;
    t.genre = GenreTache::GENERE_FICHIER_OBJET;
    t.espace = espace_;
    return t;
}

Tache Tache::liaison_objet(EspaceDeTravail *espace_)
{
    Tache t;
    t.genre = GenreTache::LIAISON_EXECUTABLE;
    t.espace = espace_;
    return t;
}

Tache Tache::attend_message(UniteCompilation *unite_)
{
    Tache t;
    t.genre = GenreTache::ENVOIE_MESSAGE;
    t.unite = unite_;
    unite_->message_recu = false;
    return t;
}

#undef STATS_PIQUE_TAILLE

#ifdef STATS_PIQUE_TAILLE
struct PiqueTailleFile {
    long taches_chargement = 0;
    long taches_execution = 0;
    long taches_generation_ri = 0;
    long taches_lexage = 0;
    long taches_message = 0;
    long taches_optimisation = 0;
    long taches_parsage = 0;
    long taches_typage = 0;

    ~PiqueTailleFile()
    {
        std::cerr << "Pique taille files :\n";
        std::cerr << "-- taches_chargement    " << taches_chargement << '\n';
        std::cerr << "-- taches_execution     " << taches_execution << '\n';
        std::cerr << "-- taches_generation_ri " << taches_generation_ri << '\n';
        std::cerr << "-- taches_lexage        " << taches_lexage << '\n';
        std::cerr << "-- taches_message       " << taches_message << '\n';
        std::cerr << "-- taches_optimisation  " << taches_optimisation << '\n';
        std::cerr << "-- taches_parsage       " << taches_parsage << '\n';
        std::cerr << "-- taches_typage        " << taches_typage << '\n';
    }
};

static PiqueTailleFile pique_taille;
#endif

#if 0
void OrdonnanceuseTache::enfile(Tache tache, int index_file)
{
	taches[index_file].enfile(tache);
}
#endif

OrdonnanceuseTache::OrdonnanceuseTache(Compilatrice *compilatrice) : m_compilatrice(compilatrice)
{
}

void OrdonnanceuseTache::cree_tache_pour_chargement(EspaceDeTravail *espace, Fichier *fichier)
{
    espace->tache_chargement_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->fichier = fichier;

    auto tache = Tache();
    tache.unite = unite;
    tache.genre = GenreTache::CHARGE_FICHIER;

    taches_chargement.enfile(tache);
#ifdef STATS_PIQUE_TAILLE
    pique_taille.taches_chargement = std::max(pique_taille.taches_chargement,
                                              taches_chargement.taille());
#endif
}

void OrdonnanceuseTache::cree_tache_pour_lexage(EspaceDeTravail *espace, Fichier *fichier)
{
    assert(fichier->donnees_constantes->fut_charge);
    espace->tache_lexage_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->fichier = fichier;

    auto tache = Tache();
    tache.unite = unite;
    tache.genre = GenreTache::LEXE;

    taches_lexage.enfile(tache);
#ifdef STATS_PIQUE_TAILLE
    pique_taille.taches_lexage = std::max(pique_taille.taches_lexage, taches_lexage.taille());
#endif
}

void OrdonnanceuseTache::cree_tache_pour_parsage(EspaceDeTravail *espace, Fichier *fichier)
{
    assert(fichier->donnees_constantes->fut_lexe);
    espace->tache_parsage_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->fichier = fichier;

    auto tache = Tache();
    tache.unite = unite;
    tache.genre = GenreTache::PARSE;

    taches_parsage.enfile(tache);
#ifdef STATS_PIQUE_TAILLE
    pique_taille.taches_parsage = std::max(pique_taille.taches_parsage, taches_parsage.taille());
#endif
}

void OrdonnanceuseTache::cree_tache_pour_typage(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    espace->tache_typage_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->noeud = noeud;

    noeud->unite = unite;

    auto tache = Tache();
    tache.unite = unite;
    tache.genre = GenreTache::TYPAGE;

    taches_typage.enfile(tache);
#ifdef STATS_PIQUE_TAILLE
    pique_taille.taches_typage = std::max(pique_taille.taches_typage, taches_typage.taille());
#endif
}

void OrdonnanceuseTache::renseigne_etat_tacheronne(int id, GenreTache genre_tache)
{
    etats_tacheronnes[id] = genre_tache;
}

bool OrdonnanceuseTache::toutes_les_tacheronnes_dorment() const
{
    POUR (etats_tacheronnes) {
        if (it != GenreTache::DORS) {
            return false;
        }
    }

    return true;
}

bool OrdonnanceuseTache::autre_tacheronne_dans_etat(int id, GenreTache genre_tache)
{
    for (auto i = 0; i < etats_tacheronnes.taille(); ++i) {
        if (i == id) {
            continue;
        }

        if (etats_tacheronnes[i] == genre_tache) {
            return true;
        }
    }

    return false;
}

long OrdonnanceuseTache::nombre_de_taches_en_attente() const
{
    return taches_chargement.taille() + taches_lexage.taille() + taches_parsage.taille() +
           taches_typage.taille() + taches_generation_ri.taille() + taches_optimisation.taille() +
           taches_execution.taille() + taches_message.taille();
}

void OrdonnanceuseTache::cree_tache_pour_generation_ri(EspaceDeTravail *espace,
                                                       NoeudExpression *noeud)
{
    espace->tache_ri_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->noeud = noeud;

    noeud->unite = unite;

    auto tache = Tache();
    tache.unite = unite;
    tache.genre = GenreTache::GENERE_RI;

    taches_generation_ri.enfile(tache);
#ifdef STATS_PIQUE_TAILLE
    pique_taille.taches_generation_ri = std::max(pique_taille.taches_generation_ri,
                                                 taches_generation_ri.taille());
#endif
}

void OrdonnanceuseTache::cree_tache_pour_execution(EspaceDeTravail *espace,
                                                   MetaProgramme *metaprogramme)
{
    espace->tache_execution_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->metaprogramme = metaprogramme;

    metaprogramme->unite = unite;

    auto tache = Tache();
    tache.unite = unite;
    tache.genre = GenreTache::EXECUTE;

    taches_execution.enfile(tache);
#ifdef STATS_PIQUE_TAILLE
    pique_taille.taches_execution = std::max(pique_taille.taches_execution,
                                             taches_execution.taille());
#endif
}

Tache OrdonnanceuseTache::tache_suivante(Tache &tache_terminee,
                                         bool tache_completee,
                                         int id,
                                         DrapeauxTacheronne drapeaux,
                                         bool mv_en_execution)
{
    auto unite = tache_terminee.unite;
    auto espace = EspaceDeTravail::nul();

    // unité peut-être nulle pour les tâches DORS du début de la compilation
    if (unite) {
        espace = unite->espace;
    }
    else {
        espace = tache_terminee.espace;
    }

    /* Assigne une nouvelle tâche avant de traiter la dernière, afin d'éviter les
     * problèmes de cycles, par exemple quand une tâche de typage est la seule dans
     * la liste et que les métaprogrammes n'ont pas encore générés le symbole à définir. */
    auto nouvelle_tache = tache_suivante(espace, id, drapeaux);

    if (espace->possede_erreur) {
        return nouvelle_tache;
    }

    switch (tache_terminee.genre) {
        case GenreTache::DORS:
        case GenreTache::COMPILATION_TERMINEE:
        {
            // rien à faire, ces tâches-là sont considérées comme à la fin de leurs cycles
            break;
        }
        case GenreTache::EXECUTE:
        {
            if (!tache_completee) {
                taches_execution.enfile(tache_terminee);
            }

            break;
        }
        case GenreTache::ENVOIE_MESSAGE:
        {
            if (!tache_completee) {
                taches_message.enfile(tache_terminee);
            }

            break;
        }
        case GenreTache::CHARGE_FICHIER:
        {
            if (!tache_completee) {
                taches_chargement.enfile(tache_terminee);
                break;
            }

            espace->tache_chargement_terminee(m_compilatrice->messagere, unite->fichier);
            tache_terminee.genre = GenreTache::LEXE;
            taches_lexage.enfile(tache_terminee);
            break;
        }
        case GenreTache::LEXE:
        {
            if (!tache_completee) {
                taches_lexage.enfile(tache_terminee);
                break;
            }

            espace->tache_lexage_terminee(m_compilatrice->messagere);
            tache_terminee.genre = GenreTache::PARSE;
            taches_parsage.enfile(tache_terminee);
            break;
        }
        case GenreTache::PARSE:
        {
            espace->tache_parsage_terminee(m_compilatrice->messagere);
            break;
        }
        case GenreTache::TYPAGE:
        {
            // La tâche ne pût être complétée (une définition est attendue, etc.), remets-là dans
            // la file en attendant.
            if (!tache_completee) {
                if (unite->etat() != UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME &&
                    espace->parsage_termine() &&
                    (unite->index_courant == unite->index_precedent)) {
                    unite->cycle += 1;
                }

                if (unite->index_courant > unite->index_precedent) {
                    unite->cycle = 0;
                }

                unite->index_precedent = unite->index_courant;

                taches_typage.enfile(tache_terminee);
                break;
            }

            auto generation_ri_requise = true;
            auto noeud = unite->noeud;

            if (noeud->est_entete_fonction()) {
                auto entete = noeud->comme_entete_fonction();
                generation_ri_requise = entete->est_externe;
            }
            else if (noeud->est_corps_fonction()) {
                auto entete = noeud->comme_corps_fonction()->entete;
                generation_ri_requise = (!entete->est_polymorphe || entete->est_monomorphisation);
            }
            else if (noeud->est_structure()) {
                auto structure = noeud->comme_structure();
                generation_ri_requise = !structure->est_polymorphe;
            }

            if (generation_ri_requise) {
                espace->tache_ri_ajoutee(m_compilatrice->messagere);
                tache_terminee.genre = GenreTache::GENERE_RI;
                tache_terminee.unite->cycle = 0;
                taches_generation_ri.enfile(tache_terminee);
            }

            if (noeud->genre != GenreNoeud::DIRECTIVE_EXECUTE) {
                auto message_enfile = m_compilatrice->messagere->ajoute_message_typage_code(
                    unite->espace, static_cast<NoeudDeclaration *>(noeud), unite);

                if (message_enfile) {
                    taches_message.enfile(Tache::attend_message(unite));
                }
            }

            /* Décrémente ceci après avoir ajouté le message de typage de code
             * pour éviter de prévenir trop tôt un métaprogramme. */
            espace->tache_typage_terminee(m_compilatrice->messagere);

            break;
        }
        case GenreTache::GENERE_RI:
        {
            // La tâche ne pût être complétée (une définition est attendue, etc.), remets-là dans
            // la file en attendant. Ici, seules les métaprogrammes peuvent nous revenir.
            if (!tache_completee) {
                tache_terminee.unite->cycle += 1;
                taches_generation_ri.enfile(tache_terminee);
                break;
            }

            espace->tache_ri_terminee(m_compilatrice->messagere);

            if (espace->optimisations) {
                tache_terminee.genre = GenreTache::OPTIMISATION;
                taches_optimisation.enfile(tache_terminee);
            }

            break;
        }
        case GenreTache::OPTIMISATION:
        {
            if (!tache_completee) {
                tache_terminee.unite->cycle += 1;
                taches_optimisation.enfile(tache_terminee);
                break;
            }

            espace->tache_optimisation_terminee(m_compilatrice->messagere);
            break;
        }
        case GenreTache::GENERE_FICHIER_OBJET:
        {
            espace->tache_generation_objet_terminee(m_compilatrice->messagere);

            if (espace->options.resultat == ResultatCompilation::EXECUTABLE) {
                espace->change_de_phase(m_compilatrice->messagere,
                                        PhaseCompilation::AVANT_LIAISON_EXECUTABLE);
                renseigne_etat_tacheronne(id, GenreTache::LIAISON_EXECUTABLE);
                return Tache::liaison_objet(espace);
            }
            else {
                espace->change_de_phase(m_compilatrice->messagere,
                                        PhaseCompilation::COMPILATION_TERMINEE);
            }

            break;
        }
        case GenreTache::LIAISON_EXECUTABLE:
        {
            espace->tache_liaison_executable_terminee(m_compilatrice->messagere);
            espace->change_de_phase(m_compilatrice->messagere,
                                    PhaseCompilation::COMPILATION_TERMINEE);
            break;
        }
    }

    /* indique que la tâcheronne est en exécution si elle a toujours des métaprogrammes à exécuter,
     * ceci car les métaprogrammes n'ont pas forcément finis leurs exécutions quand la tâcheronne
     * nous rend la tâche */
    if (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_EXECUTER) &&
        mv_en_execution && nouvelle_tache.genre == GenreTache::DORS) {
        renseigne_etat_tacheronne(id, GenreTache::EXECUTE);
    }
    else {
        renseigne_etat_tacheronne(id, nouvelle_tache.genre);
    }

    return nouvelle_tache;
}

Tache OrdonnanceuseTache::tache_suivante(EspaceDeTravail *espace,
                                         int id,
                                         DrapeauxTacheronne drapeaux)
{
#if 0
	using dls::outils::possede_drapeau;

	for (int i = 0; i < NOMBRE_FILES; ++i) {
		if (!possede_drapeau(drapeaux, static_cast<DrapeauxTacheronne>(1 << i))) {
			continue;
		}

		if (!taches[i].est_vide()) {
			return taches[i].defile();
		}
	}
#endif

    /* toute tâcheronne pouvant lexer peut charger */
    if (!taches_chargement.est_vide() &&
        (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_LEXER))) {
        return taches_chargement.defile();
    }

    if (!taches_lexage.est_vide() &&
        (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_LEXER))) {
        return taches_lexage.defile();
    }

    if (!taches_parsage.est_vide() &&
        (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_PARSER))) {
        return taches_parsage.defile();
    }

    // gère les tâches de messages avant les tâches de typage pour éviter les
    // problèmes de symbole non définis si un métaprogramme n'a pas encore généré
    // ce symbole
    // ce n'est pas la solution finale pour un tel problème, mais c'est un début
    // il nous faudra sans doute un système pour définir qu'un métaprogramme définira
    // tel ou tel symbole, et trouver de meilleures heuristiques pour arrêter la
    // compilation en cas d'indéfinition de symbole
    if (!taches_message.est_vide() &&
        (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_ENVOYER_MESSAGE))) {
        return taches_message.defile();
    }

    /* Nous pouvons avoir des dépendances entre le typage et la génération de RI
     * quand nous exécutons des métaprogrammes #corps_texte : on ne peut typer une
     * fonction restante tant que le #corps_texte ne fut pas généré, mais nous ne
     * pouvons générer les #corps_texte tant que toutes les dépendances n'ont pas
     * eu leurs RI générées. Or, à cause de l'ordre dans lequel nous donnons les
     * tâches, le typage se fait avant la génération de RI. Donc afin d'éviter
     * de rester bloqué dans le typage, prenons une tâche de typage et une de RI
     * et donnons celle de RI si le typage attend sur un métaprogramme.
     *
     * Il nous faudra une meilleure manière de gérer ce cas.
     */
    auto tache_typage = static_cast<Tache *>(nullptr);
    auto tache_ri = static_cast<Tache *>(nullptr);

    if (!taches_typage.est_vide() &&
        (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_TYPER) &&
         !autre_tacheronne_dans_etat(id, GenreTache::TYPAGE))) {
        tache_typage = &taches_typage.front();
    }

    if (!taches_generation_ri.est_vide() &&
        (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_RI) &&
         !autre_tacheronne_dans_etat(id, GenreTache::GENERE_RI))) {
        tache_ri = &taches_generation_ri.front();
    }

    if (tache_typage && !tache_ri) {
        return taches_typage.defile();
    }

    if (tache_ri && !tache_typage) {
        return taches_generation_ri.defile();
    }

    if (tache_ri && tache_typage) {
        if (tache_typage->unite->etat() == UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME &&
            taches_generation_ri.taille() > taches_typage.taille()) {
            return taches_generation_ri.defile();
        }

        return taches_typage.defile();
    }

    if (!taches_optimisation.est_vide() &&
        dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_OPTIMISER)) {
        return taches_optimisation.defile();
    }

    if (!espace->possede_erreur && espace->peut_generer_code_final()) {
        if (espace->options.resultat == ResultatCompilation::RIEN) {
            espace->change_de_phase(m_compilatrice->messagere,
                                    PhaseCompilation::COMPILATION_TERMINEE);
        }
        else if (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_CODE)) {
            espace->change_de_phase(m_compilatrice->messagere,
                                    PhaseCompilation::AVANT_GENERATION_OBJET);
            return Tache::genere_fichier_objet(espace);
        }
    }

    if (!taches_execution.est_vide() &&
        (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_EXECUTER))) {
        return taches_execution.defile();
    }

    if (!compilation_terminee) {
        if (espace->possede_erreur) {
            espace = m_compilatrice->espace_defaut_compilation();
        }

        if (!toutes_les_tacheronnes_dorment()) {
            return Tache::dors(espace);
        }

        // La Tâcheronne n'a pas pu recevoir une tâche lui étant spécifique.
        if (nombre_de_taches_en_attente() != 0) {
            return Tache::dors(espace);
        }
    }

    compilation_terminee = true;

    return Tache::compilation_terminee();
}

long OrdonnanceuseTache::memoire_utilisee() const
{
    auto memoire = 0l;
    memoire += unites.memoire_utilisee();
    return memoire;
}

int OrdonnanceuseTache::enregistre_tacheronne(Badge<Tacheronne> /*badge*/)
{
    etats_tacheronnes.ajoute(GenreTache::DORS);
    return nombre_de_tacheronnes++;
}

void OrdonnanceuseTache::purge_messages()
{
    while (!taches_message.est_vide()) {
        taches_message.defile();
    }
}

void OrdonnanceuseTache::supprime_toutes_les_taches()
{
    taches_chargement.efface();
    taches_lexage.efface();
    taches_parsage.efface();
    taches_typage.efface();
    taches_generation_ri.efface();
    taches_execution.efface();
    taches_message.efface();
    taches_optimisation.efface();

    for (int i = 0; i < nombre_de_tacheronnes; ++i) {
        taches_chargement.enfile(Tache::compilation_terminee());
        taches_lexage.enfile(Tache::compilation_terminee());
        taches_parsage.enfile(Tache::compilation_terminee());
        taches_typage.enfile(Tache::compilation_terminee());
        taches_generation_ri.enfile(Tache::compilation_terminee());
        taches_execution.enfile(Tache::compilation_terminee());
        taches_message.enfile(Tache::compilation_terminee());
        taches_optimisation.enfile(Tache::compilation_terminee());
    }
}

void OrdonnanceuseTache::supprime_toutes_les_taches_pour_espace(const EspaceDeTravail *espace)
{
    auto predicat = [&](Tache const &tache) { return tache.espace == espace; };

    taches_chargement.efface_si(predicat);
    taches_lexage.efface_si(predicat);
    taches_parsage.efface_si(predicat);
    taches_typage.efface_si(predicat);
    taches_generation_ri.efface_si(predicat);
    taches_execution.efface_si(predicat);
    taches_message.efface_si(predicat);
    taches_optimisation.efface_si(predicat);
}

Tacheronne::Tacheronne(Compilatrice &comp)
    : compilatrice(comp),
      assembleuse(memoire::loge<AssembleuseArbre>("AssembleuseArbre", this->allocatrice_noeud)),
      id(compilatrice.ordonnanceuse->enregistre_tacheronne({}))
{
}

Tacheronne::~Tacheronne()
{
    memoire::deloge("AssembleuseArbre", assembleuse);
}

void Tacheronne::gere_tache()
{
    auto temps_debut = dls::chrono::compte_seconde();
    auto tache = Tache::dors(compilatrice.espace_de_travail_defaut);
    auto tache_fut_completee = true;
    auto &ordonnanceuse = compilatrice.ordonnanceuse;

    while (true) {
        tache = ordonnanceuse->tache_suivante(
            tache, tache_fut_completee, id, drapeaux, !mv.terminee());

        if (tache.genre != GenreTache::DORS) {
            nombre_dodos = 0;
        }

        switch (tache.genre) {
            case GenreTache::COMPILATION_TERMINEE:
            {
                temps_scene = temps_debut.temps() - temps_executable - temps_fichier_objet;
                return;
            }
            case GenreTache::ENVOIE_MESSAGE:
            {
                assert(dls::outils::possede_drapeau(drapeaux,
                                                    DrapeauxTacheronne::PEUT_ENVOYER_MESSAGE));
                tache_fut_completee = tache.unite->message_recu;
                break;
            }
            case GenreTache::DORS:
            {
                /* les tâches d'exécutions sont marquées comme terminées dès que leurs
                 * métaprogrammes sont ajoutés à la machine virtuelle, il se peut qu'il en reste à
                 * exécuter alors qu'il n'y a plus de tâches à exécuter */
                if (!mv.terminee()) {
                    nombre_dodos = 0;
                    execute_metaprogrammes();
                }
                else {
                    nombre_dodos += 1;
                    dls::chrono::dors_microsecondes(100 * nombre_dodos);
                    tache_fut_completee = true;
                    temps_passe_a_dormir += 0.1 * nombre_dodos;
                }

                break;
            }
            case GenreTache::CHARGE_FICHIER:
            {
                auto fichier = tache.unite->fichier;
                auto donnees = fichier->donnees_constantes;

                if (!donnees->fut_charge) {
                    donnees->mutex.lock();

                    if (!donnees->fut_charge) {
                        auto debut_chargement = dls::chrono::compte_seconde();
                        auto texte = charge_contenu_fichier(dls::chaine(donnees->chemin));
                        temps_chargement += debut_chargement.temps();

                        auto debut_tampon = dls::chrono::compte_seconde();
                        donnees->charge_tampon(lng::tampon_source(std::move(texte)));
                        temps_tampons += debut_tampon.temps();
                    }

                    donnees->mutex.unlock();
                }

                tache_fut_completee = donnees->fut_charge;
                break;
            }
            case GenreTache::LEXE:
            {
                assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_LEXER));
                auto unite = tache.unite;
                auto fichier = unite->fichier;
                auto donnees = fichier->donnees_constantes;

                if (!donnees->fut_lexe) {
                    donnees->mutex.lock();

                    if (!donnees->en_lexage) {
                        donnees->en_lexage = true;
                        auto debut_lexage = dls::chrono::compte_seconde();
                        auto lexeuse = Lexeuse(compilatrice.contexte_lexage(), donnees);
                        lexeuse.performe_lexage();
                        temps_lexage += debut_lexage.temps();
                        donnees->en_lexage = false;
                    }

                    donnees->mutex.unlock();
                }

                tache_fut_completee = donnees->fut_lexe;
                break;
            }
            case GenreTache::PARSE:
            {
                assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_PARSER));
                auto unite = tache.unite;
                auto debut_parsage = dls::chrono::compte_seconde();
                auto syntaxeuse = Syntaxeuse(*this, unite);
                syntaxeuse.analyse();
                temps_parsage += debut_parsage.temps();
                tache_fut_completee = true;
                break;
            }
            case GenreTache::TYPAGE:
            {
                assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_TYPER));
                auto unite = tache.unite;

                if (unite->est_bloquee()) {
                    mv.stop = true;

                    if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_SYMBOLE) {
                        erreur::lance_erreur(
                            "Trop de cycles : arrêt de la compilation sur un symbole inconnu",
                            *unite->espace,
                            unite->symbole_attendu);
                    }
                    else if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_DECLARATION) {
                        auto decl = unite->declaration_attendue;
                        auto unite_decl = decl->unite;
                        auto erreur = rapporte_erreur(unite->espace,
                                                      unite->declaration_attendue,
                                                      "Je ne peux pas continuer la compilation "
                                                      "car une déclaration ne peut être typée.");

                        // À FAIRE : ne devrait pas arriver
                        if (unite_decl) {
                            erreur
                                .ajoute_message(
                                    "Note : l'unité de compilation est dans cette état :\n")
                                .ajoute_message(chaine_attentes_recursives(unite))
                                .ajoute_message("\n");
                        }
                    }
                    else if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_TYPE) {
                        auto site = unite->noeud;

                        if (site->est_corps_fonction()) {
                            auto corps = site->comme_corps_fonction();
                            site = corps->arbre_aplatis[unite->index_courant];
                        }

                        rapporte_erreur(unite->espace,
                                        site,
                                        "Je ne peux pas continuer la compilation car je n'arrive "
                                        "pas à déterminer un type pour l'expression",
                                        erreur::Genre::TYPE_INCONNU)
                            .ajoute_message("Note : le type attendu est ")
                            .ajoute_message(chaine_type(unite->type_attendu))
                            .ajoute_message("\n")
                            .ajoute_message(
                                "Note : l'unité de compilation est dans cette état :\n")
                            .ajoute_message(chaine_attentes_recursives(unite))
                            .ajoute_message("\n");
                    }
                    else if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI) {
                        erreur::lance_erreur("Trop de cycles : arrêt de la compilation car une "
                                             "déclaration attend sur une interface de Kuri",
                                             *unite->espace,
                                             unite->noeud);
                    }
                    else if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_OPERATEUR) {
                        if (unite->operateur_attendu->genre == GenreNoeud::OPERATEUR_BINAIRE) {
                            auto expression_operation = static_cast<NoeudExpressionBinaire *>(
                                unite->operateur_attendu);
                            auto type1 = expression_operation->operande_gauche->type;
                            auto type2 = expression_operation->operande_droite->type;
                            rapporte_erreur(unite->espace,
                                            unite->operateur_attendu,
                                            "Je ne peux pas continuer la compilation car je "
                                            "n'arrive pas à déterminer quel opérateur appeler.",
                                            erreur::Genre::TYPE_INCONNU)
                                .ajoute_message("Le type à gauche de l'opérateur est ")
                                .ajoute_message(chaine_type(type1))
                                .ajoute_message("\nLe type à droite de l'opérateur est ")
                                .ajoute_message(chaine_type(type2))
                                .ajoute_message(
                                    "\n\nMais aucun opérateur ne correspond à ces types-là.\n\n")
                                .ajoute_conseil(
                                    "Si vous voulez performer une opération sur des types "
                                    "non-communs, vous pouvez définir vos propres opérateurs avec "
                                    "la syntaxe suivante :\n\nopérateur op :: fonc (a: type1, b: "
                                    "type2) -> type_retour\n{\n\t...\n}\n");
                        }
                        else {
                            auto expression_operation = static_cast<NoeudExpressionUnaire *>(
                                unite->operateur_attendu);
                            auto type = expression_operation->operande->type;
                            rapporte_erreur(unite->espace,
                                            unite->operateur_attendu,
                                            "Je ne peux pas continuer la compilation car je "
                                            "n'arrive pas à déterminer quel opérateur appeler.",
                                            erreur::Genre::TYPE_INCONNU)
                                .ajoute_message("\nLe type à droite de l'opérateur est ")
                                .ajoute_message(chaine_type(type))
                                .ajoute_message(
                                    "\n\nMais aucun opérateur ne correspond à ces types-là.\n\n")
                                .ajoute_conseil(
                                    "Si vous voulez performer une opération sur des types "
                                    "non-communs, vous pouvez définir vos propres opérateurs avec "
                                    "la syntaxe suivante :\n\nopérateur op :: fonc (a: type) -> "
                                    "type_retour\n{\n\t...\n}\n");
                        }
                    }
                    else {
                        rapporte_erreur(unite->espace,
                                        unite->noeud,
                                        "Je ne peux pas continuer la compilation car une unité est "
                                        "bloqué dans un cycle")
                            .ajoute_message("\nNote : l'unité est dans l'état : ")
                            .ajoute_message(chaine_attentes_recursives(unite))
                            .ajoute_message("\n");
                    }

                    break;
                }

                auto debut_validation = dls::chrono::compte_seconde();
                tache_fut_completee = gere_unite_pour_typage(unite);
                temps_validation += debut_validation.temps();
                break;
            }
            case GenreTache::GENERE_RI:
            {
                assert(
                    dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_RI));
                auto debut_generation = dls::chrono::compte_seconde();
                tache_fut_completee = gere_unite_pour_ri(tache.unite);
                constructrice_ri.temps_generation += debut_generation.temps();
                break;
            }
            case GenreTache::OPTIMISATION:
            {
                assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_OPTIMISER));
                auto debut_generation = dls::chrono::compte_seconde();
                tache_fut_completee = gere_unite_pour_optimisation(tache.unite);
                temps_optimisation += debut_generation.temps();
                break;
            }
            case GenreTache::EXECUTE:
            {
                assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_EXECUTER));
                tache_fut_completee = gere_unite_pour_execution(tache.unite);
                break;
            }
            case GenreTache::GENERE_FICHIER_OBJET:
            {
                assert(
                    dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_CODE));
                tache.espace->coulisse->cree_fichier_objet(
                    compilatrice, *tache.espace, constructrice_ri);
                temps_generation_code += tache.espace->coulisse->temps_generation_code;
                temps_fichier_objet += tache.espace->coulisse->temps_fichier_objet;
                tache_fut_completee = true;
                break;
            }
            case GenreTache::LIAISON_EXECUTABLE:
            {
                assert(
                    dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_CODE));
                tache.espace->coulisse->cree_executable(compilatrice, *tache.espace);
                temps_executable += tache.espace->coulisse->temps_executable;
                tache_fut_completee = true;
                break;
            }
        }
    }

    temps_scene = temps_debut.temps() - temps_executable - temps_fichier_objet;
}

static bool dependances_eurent_ri_generees(NoeudDependance *noeud)
{
    auto visite = dls::ensemblon<NoeudDependance *, 32>();
    auto a_visiter = dls::file<NoeudDependance *>();

    a_visiter.enfile(noeud);

    while (!a_visiter.est_vide()) {
        auto n = a_visiter.defile();

        visite.insere(n);

        POUR (n->relations().plage()) {
            auto noeud_fin = it.noeud_fin;

            if (noeud_fin->est_type()) {
                if ((noeud_fin->type()->drapeaux & RI_TYPE_FUT_GENEREE) == 0) {
                    return false;
                }
            }
            else if (noeud_fin->est_fonction()) {
                auto noeud_syntaxique = noeud_fin->fonction();

                if (!noeud_syntaxique->possede_drapeau(RI_FUT_GENEREE)) {
                    return false;
                }
            }
            else if (noeud_fin->est_globale()) {
                auto noeud_syntaxique = noeud_fin->globale();

                if (!noeud_syntaxique->possede_drapeau(RI_FUT_GENEREE)) {
                    return false;
                }
            }

            if (visite.possede(noeud_fin)) {
                continue;
            }

            a_visiter.enfile(noeud_fin);
        }
    }

    return true;
}

bool Tacheronne::gere_unite_pour_typage(UniteCompilation *unite)
{
    switch (unite->etat()) {
        case UniteCompilation::Etat::PRETE:
        {
            auto contexte = ContexteValidationCode(compilatrice, *this, *unite);

            switch (unite->noeud->genre) {
                case GenreNoeud::DECLARATION_ENTETE_FONCTION:
                {
                    auto decl = unite->noeud->comme_entete_fonction();
                    return contexte.valide_type_fonction(decl) == ResultatValidation::OK;
                }
                case GenreNoeud::DECLARATION_CORPS_FONCTION:
                {
                    auto decl = static_cast<NoeudDeclarationCorpsFonction *>(unite->noeud);

                    if (!decl->entete->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                        unite->attend_sur_declaration(decl->entete);
                        return false;
                    }

                    if (decl->entete->est_operateur) {
                        return contexte.valide_operateur(decl) == ResultatValidation::OK;
                    }

                    return contexte.valide_fonction(decl) == ResultatValidation::OK;
                }
                case GenreNoeud::DECLARATION_ENUM:
                {
                    auto decl = static_cast<NoeudEnum *>(unite->noeud);
                    return contexte.valide_enum(decl) == ResultatValidation::OK;
                }
                case GenreNoeud::DECLARATION_STRUCTURE:
                {
                    auto decl = static_cast<NoeudStruct *>(unite->noeud);
                    return contexte.valide_structure(decl) == ResultatValidation::OK;
                }
                case GenreNoeud::DECLARATION_VARIABLE:
                {
                    auto decl = static_cast<NoeudDeclarationVariable *>(unite->noeud);
                    aplatis_arbre(decl);

                    for (; unite->index_courant < decl->arbre_aplatis.taille();
                         ++unite->index_courant) {
                        if (contexte.valide_semantique_noeud(
                                decl->arbre_aplatis[unite->index_courant]) ==
                            ResultatValidation::Erreur) {
                            auto graphe = unite->espace->graphe_dependance.verrou_ecriture();
                            auto noeud_dependance = graphe->cree_noeud_globale(decl);
                            graphe->ajoute_dependances(*noeud_dependance,
                                                       contexte.donnees_dependance);
                            return false;
                        }
                    }

                    auto graphe = unite->espace->graphe_dependance.verrou_ecriture();
                    auto noeud_dependance = graphe->cree_noeud_globale(decl);
                    graphe->ajoute_dependances(*noeud_dependance, contexte.donnees_dependance);

                    return true;
                }
                case GenreNoeud::DIRECTIVE_EXECUTE:
                {
                    auto dir = static_cast<NoeudDirectiveExecute *>(unite->noeud);
                    aplatis_arbre(dir);

                    // À FAIRE : ne peut pas préserver les dépendances si nous échouons avant la
                    // fin
                    for (unite->index_courant = 0;
                         unite->index_courant < dir->arbre_aplatis.taille();
                         ++unite->index_courant) {
                        if (contexte.valide_semantique_noeud(
                                dir->arbre_aplatis[unite->index_courant]) ==
                            ResultatValidation::Erreur) {
                            return false;
                        }
                    }

                    return true;
                }
                case GenreNoeud::INSTRUCTION_IMPORTE:
                case GenreNoeud::INSTRUCTION_CHARGE:
                {
                    if (contexte.valide_semantique_noeud(unite->noeud) ==
                        ResultatValidation::Erreur) {
                        return false;
                    }

                    temps_validation -= contexte.temps_chargement;
                    return true;
                }
                default:
                {
                    assert_rappel(false, [&]() {
                        std::cerr << "Genre de noeud inattendu dans la tâche de typage : "
                                  << unite->noeud->genre << '\n';
                    });
                    break;
                }
            }

            return true;
        }
        case UniteCompilation::Etat::ATTEND_SUR_TYPE:
        {
            auto type = unite->type_attendu;

            if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {

                if (type->est_structure()) {
                    auto decl_struct = type->comme_structure()->decl;

                    if (decl_struct->unite->etat() ==
                        UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME) {
                        unite->cycle -= 1;
                    }
                }

                return false;
            }

            unite->restaure_etat_original();
            unite->type_attendu = nullptr;
            return gere_unite_pour_typage(unite);
        }
        case UniteCompilation::Etat::ATTEND_SUR_DECLARATION:
        {
            unite->restaure_etat_original();
            unite->declaration_attendue = nullptr;
            return gere_unite_pour_typage(unite);
        }
        case UniteCompilation::Etat::ATTEND_SUR_OPERATEUR:
        {
            unite->restaure_etat_original();
            unite->operateur_attendu = nullptr;
            return gere_unite_pour_typage(unite);
        }
        case UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI:
        case UniteCompilation::Etat::ATTEND_SUR_SYMBOLE:
        {
            unite->restaure_etat_original();
            return gere_unite_pour_typage(unite);
        }
        case UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME:
        {
            auto metaprogramme = unite->metaprogramme_attendu;

            if (!metaprogramme->fut_execute) {
                return false;
            }

            // À FAIRE: nous avons un problème de concurrence critique apparement vis-à-vis de
            // l'exécution des métaprogrammes et du typage de code, je ne sais pas encore la cause
            // du problème, mais pour le moment, afin de pouvoir développer sereinement dans le
            // langage, j'ajoute cette ligne pour ralentir le thread en attendant que la
            // MachineVirtuelle finisse son travail (!?)
            std::cerr << "Ralentis la compilation....\n";

            unite->restaure_etat_original();
            return gere_unite_pour_typage(unite);
        }
    }

    return false;
}

bool Tacheronne::gere_unite_pour_ri(UniteCompilation *unite)
{
    auto noeud = unite->noeud;
    auto pour_metaprogramme = false;

    if (noeud->est_corps_fonction()) {
        auto corps = noeud->comme_corps_fonction();
        pour_metaprogramme = corps->entete->est_metaprogramme;
    }

    if (noeud->est_declaration() && !pour_metaprogramme) {
        constructrice_ri.genere_ri_pour_noeud(unite->espace, noeud);
        noeud->drapeaux |= RI_FUT_GENEREE;

        if (noeud->type == nullptr) {
            unite->espace->rapporte_erreur(
                noeud, "Erreur interne: type nul sur une déclaration après la génération de RI");
            return true;
        }

        noeud->type->drapeaux |= RI_TYPE_FUT_GENEREE;
    }
    else if (pour_metaprogramme) {
        auto corps = noeud->comme_corps_fonction();

#define ATTEND_SUR_TYPE_SI_NECESSAIRE(type)                                                       \
    if (type == nullptr) {                                                                        \
        return false;                                                                             \
    }                                                                                             \
    if ((type->drapeaux & RI_TYPE_FUT_GENEREE) == 0) {                                            \
        unite->attend_sur_type(type);                                                             \
        return false;                                                                             \
    }

        ATTEND_SUR_TYPE_SI_NECESSAIRE(unite->espace->typeuse.type_contexte);
        ATTEND_SUR_TYPE_SI_NECESSAIRE(unite->espace->typeuse.type_base_allocatrice);
        ATTEND_SUR_TYPE_SI_NECESSAIRE(unite->espace->typeuse.type_stockage_temporaire);
        ATTEND_SUR_TYPE_SI_NECESSAIRE(unite->espace->typeuse.type_trace_appel);
        ATTEND_SUR_TYPE_SI_NECESSAIRE(unite->espace->typeuse.type_info_appel_trace_appel);
        ATTEND_SUR_TYPE_SI_NECESSAIRE(unite->espace->typeuse.type_info_fonction_trace_appel);

        if (unite->espace->interface_kuri->decl_creation_contexte == nullptr) {
            unite->attend_sur_interface_kuri("creation_contexte");
            return false;
        }

        auto decl_creation_contexte = unite->espace->interface_kuri->decl_creation_contexte;
        if (decl_creation_contexte->corps->unite == nullptr) {
            compilatrice.ordonnanceuse->cree_tache_pour_typage(unite->espace,
                                                               decl_creation_contexte->corps);
        }

        if (!decl_creation_contexte->possede_drapeau(RI_FUT_GENEREE)) {
            unite->attend_sur_declaration(unite->espace->interface_kuri->decl_creation_contexte);
            return false;
        }

        if (!dependances_eurent_ri_generees(corps->entete->noeud_dependance)) {
            return false;
        }

        constructrice_ri.genere_ri_pour_fonction_metaprogramme(unite->espace, corps->entete);
    }

    return true;
}

bool Tacheronne::gere_unite_pour_optimisation(UniteCompilation *unite)
{
    auto noeud = unite->noeud;

    if (noeud->est_entete_fonction()) {
        auto entete = noeud->comme_entete_fonction();

        if (entete->est_externe) {
            return true;
        }

        /* n'optimise pas cette fonction car le manque de retour fait supprimer tout le code */
        if (entete == unite->espace->interface_kuri->decl_creation_contexte) {
            return true;
        }

        if (!dependances_eurent_ri_generees(entete->noeud_dependance)) {
            return false;
        }

        optimise_code(constructrice_ri, static_cast<AtomeFonction *>(entete->atome));
    }
    else if (noeud->est_corps_fonction()) {
        auto corps = noeud->comme_corps_fonction();
        auto entete = corps->entete;

        if (entete->est_externe) {
            return true;
        }

        /* n'optimise pas cette fonction car le manque de retour fait supprimer tout le code */
        if (entete == unite->espace->interface_kuri->decl_creation_contexte) {
            return true;
        }

        if (!dependances_eurent_ri_generees(entete->noeud_dependance)) {
            return false;
        }

        optimise_code(constructrice_ri, static_cast<AtomeFonction *>(entete->atome));
    }
    else if (noeud->est_structure()) {
        // À FAIRE(optimisations) : fonctions d'initialisation des types
    }

    return true;
}

static void rassemble_globales_et_fonctions(EspaceDeTravail *espace,
                                            MachineVirtuelle &mv,
                                            NoeudDependance *racine,
                                            kuri::tableau<AtomeGlobale *> &globales,
                                            kuri::tableau<AtomeFonction *> &fonctions)
{
    auto graphe = espace->graphe_dependance.verrou_ecriture();

    graphe->traverse(racine, [&](NoeudDependance *noeud_dep) {
        if (noeud_dep->est_fonction()) {
            auto decl_noeud = noeud_dep->fonction();

            if (decl_noeud->possede_drapeau(CODE_BINAIRE_FUT_GENERE)) {
                return;
            }

            auto atome_fonction = static_cast<AtomeFonction *>(decl_noeud->atome);
            fonctions.ajoute(atome_fonction);
            decl_noeud->drapeaux |= CODE_BINAIRE_FUT_GENERE;
        }
        else if (noeud_dep->est_type()) {
            auto type = noeud_dep->type();

            if ((type->drapeaux & CODE_BINAIRE_TYPE_FUT_GENERE) != 0) {
                return;
            }

            if (type->genre == GenreType::STRUCTURE || type->genre == GenreType::UNION) {
                auto atome_fonction = type->fonction_init;
                assert(atome_fonction);
                fonctions.ajoute(atome_fonction);
                type->drapeaux |= CODE_BINAIRE_TYPE_FUT_GENERE;
            }
        }
        else if (noeud_dep->est_globale()) {
            auto decl_noeud = noeud_dep->globale();

            if (decl_noeud->possede_drapeau(EST_CONSTANTE)) {
                return;
            }

            if (decl_noeud->possede_drapeau(CODE_BINAIRE_FUT_GENERE)) {
                return;
            }

            auto atome_globale = espace->trouve_globale(decl_noeud);

            if (atome_globale->index == -1) {
                atome_globale->index = mv.ajoute_globale(decl_noeud->type, decl_noeud->ident);
            }

            globales.ajoute(atome_globale);

            decl_noeud->drapeaux |= CODE_BINAIRE_FUT_GENERE;
        }
    });

    POUR_TABLEAU_PAGE (graphe->noeuds) {
        it.fut_visite = false;
    }
}

bool Tacheronne::gere_unite_pour_execution(UniteCompilation *unite)
{
    auto metaprogramme = unite->metaprogramme;
    auto espace = unite->espace;

    auto peut_executer = (metaprogramme->fonction->drapeaux & RI_FUT_GENEREE) != 0;

    if (peut_executer) {
        kuri::tableau<AtomeGlobale *> globales;
        kuri::tableau<AtomeFonction *> fonctions;
        rassemble_globales_et_fonctions(
            espace, mv, metaprogramme->fonction->noeud_dependance, globales, fonctions);

        auto fonction = static_cast<AtomeFonction *>(metaprogramme->fonction->atome);

        if (!fonction) {
            espace->rapporte_erreur(metaprogramme->fonction,
                                    "Impossible de trouver la fonction pour le métaprogramme");
        }

        if (globales.taille() != 0) {
            auto fonc_init = constructrice_ri.genere_fonction_init_globales_et_appel(
                espace, globales, fonction);
            fonctions.ajoute(fonc_init);
        }

        POUR (fonctions) {
            genere_code_binaire_pour_fonction(it, &mv);
        }

        // desassemble(fonction->chunk, metaprogramme->fonction->nom_broye(unite->espace).c_str(),
        // std::cerr);

        metaprogramme->donnees_execution = mv.loge_donnees_execution();
        mv.ajoute_metaprogramme(metaprogramme);
    }

    execute_metaprogrammes();
    return peut_executer;
}

void Tacheronne::execute_metaprogrammes()
{
    mv.execute_metaprogrammes_courants();

    POUR (mv.metaprogrammes_termines()) {
        auto espace = it->unite->espace;

        // À FAIRE : précision des messages d'erreurs
        if (it->resultat == MetaProgramme::ResultatExecution::ERREUR) {
            espace->rapporte_erreur(it->directive, "Erreur lors de l'exécution du métaprogramme");
        }
        else {
            if (it->directive && it->directive->ident == ID::assert_) {
                auto resultat = *reinterpret_cast<bool *>(it->donnees_execution->pointeur_pile);

                if (!resultat) {
                    espace->rapporte_erreur(it->directive, "Échec de l'assertion");
                }
            }
            else if (it->directive && it->directive->ident == ID::execute) {
                auto type = it->directive->type;
                auto pointeur = it->donnees_execution->pointeur_pile;

                // Les directives pour des expressions dans des fonctions n'ont pas d'unités
                if (!it->directive->unite) {
                    it->directive->substitution = noeud_syntaxique_depuis_resultat(
                        espace, it->directive, it->directive->lexeme, type, pointeur);
                }
            }
            else if (it->corps_texte) {
                auto resultat = *reinterpret_cast<kuri::chaine_statique *>(
                    it->donnees_execution->pointeur_pile);

                if (resultat.taille() == 0) {
                    espace->rapporte_erreur(it->corps_texte,
                                            "Le corps-texte a retourné une chaine vide");
                }

                auto fichier_racine = espace->fichier(it->corps_texte->lexeme->fichier);
                auto module = fichier_racine->module;

                auto tampon = dls::chaine(resultat.pointeur(), resultat.taille());

                if (*tampon.fin() != '\n') {
                    tampon.ajoute('\n');
                }

                auto nom_fichier = enchaine(it);
                auto resultat_fichier = espace->trouve_ou_cree_fichier(
                    compilatrice.sys_module, module, nom_fichier, nom_fichier, false);

                if (resultat_fichier.est<FichierNeuf>()) {
                    auto fichier = resultat_fichier.resultat<FichierNeuf>().fichier;
                    auto donnees_fichier = fichier->donnees_constantes;

                    fichier->module = module;
                    fichier->metaprogramme_corps_texte = it;

                    donnees_fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

                    compilatrice.chaines_ajoutees_a_la_compilation->ajoute(resultat);
                    compilatrice.ordonnanceuse->cree_tache_pour_lexage(espace, fichier);
                }
            }
        }

        /* attend d'avoir générer le résultat des opérations avant de marquer comme exécuté */
        it->fut_execute = true;

        mv.deloge_donnees_execution(it->donnees_execution);

        espace->tache_execution_terminee(compilatrice.messagere);
    }
}

NoeudExpression *Tacheronne::noeud_syntaxique_depuis_resultat(EspaceDeTravail *espace,
                                                              NoeudDirectiveExecute *directive,
                                                              Lexeme const *lexeme,
                                                              Type *type,
                                                              octet_t *pointeur)
{
    switch (type->genre) {
        case GenreType::EINI:
        case GenreType::POINTEUR:
        case GenreType::POLYMORPHIQUE:
        case GenreType::REFERENCE:
        case GenreType::VARIADIQUE:
        {
            espace
                ->rapporte_erreur(directive,
                                  "Type non pris en charge pour le résutat des exécutions !")
                .ajoute_message("Le type est : ", chaine_type(type), "\n");
            break;
        }
        case GenreType::RIEN:
        {
            break;
        }
        case GenreType::TUPLE:
        {
            // pour les tuples de retours, nous les convertissons en expression-virgule
            auto tuple = type->comme_tuple();
            auto virgule = assembleuse->cree_virgule(lexeme);

            POUR (tuple->membres) {
                auto pointeur_membre = pointeur + it.decalage;
                virgule->expressions.ajoute(noeud_syntaxique_depuis_resultat(
                    espace, directive, lexeme, it.type, pointeur_membre));
            }

            return virgule;
        }
        case GenreType::OCTET:
        case GenreType::ENTIER_CONSTANT:
        case GenreType::ENTIER_RELATIF:
        {
            unsigned long valeur = 0;

            if (type->taille_octet == 1) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<char *>(pointeur));
            }
            else if (type->taille_octet == 2) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<short *>(pointeur));
            }
            else if (type->taille_octet == 4 || type->taille_octet == 0) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<int *>(pointeur));
            }
            else if (type->taille_octet == 8) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<long *>(pointeur));
            }

            return assembleuse->cree_litterale_entier(lexeme, type, valeur);
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        case GenreType::ENTIER_NATUREL:
        {
            unsigned long valeur = 0;
            if (type->taille_octet == 1) {
                valeur = static_cast<unsigned long>(*pointeur);
            }
            else if (type->taille_octet == 2) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<unsigned short *>(pointeur));
            }
            else if (type->taille_octet == 4) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<unsigned int *>(pointeur));
            }
            else if (type->taille_octet == 8) {
                valeur = *reinterpret_cast<unsigned long *>(pointeur);
            }

            return assembleuse->cree_litterale_entier(lexeme, type, valeur);
        }
        case GenreType::BOOL:
        {
            auto valeur = *reinterpret_cast<bool *>(pointeur);
            auto noeud_syntaxique = assembleuse->cree_litterale_bool(lexeme);
            noeud_syntaxique->valeur = valeur;
            noeud_syntaxique->type = type;
            return noeud_syntaxique;
        }
        case GenreType::REEL:
        {
            double valeur = 0.0;

            // À FAIRE(r16)
            if (type->taille_octet == 4) {
                valeur = static_cast<double>(*reinterpret_cast<float *>(pointeur));
            }
            else if (type->taille_octet == 8) {
                valeur = *reinterpret_cast<double *>(pointeur);
            }

            return assembleuse->cree_litterale_reel(lexeme, type, valeur);
        }
        case GenreType::STRUCTURE:
        {
            auto type_structure = type->comme_structure();

            auto construction_structure = assembleuse->cree_construction_structure(lexeme,
                                                                                   type_structure);

            POUR (type_structure->membres) {
                if (it.drapeaux & TypeCompose::Membre::EST_CONSTANT) {
                    continue;
                }

                auto pointeur_membre = pointeur + it.decalage;
                auto noeud_membre = noeud_syntaxique_depuis_resultat(
                    espace, directive, lexeme, it.type, pointeur_membre);
                construction_structure->parametres_resolus.ajoute(noeud_membre);
            }

            return construction_structure;
        }
        case GenreType::UNION:
        {
            auto type_union = type->comme_union();
            auto construction_union = assembleuse->cree_construction_structure(lexeme, type_union);

            if (type_union->est_nonsure) {
                auto expr = noeud_syntaxique_depuis_resultat(
                    espace, directive, lexeme, type_union->type_le_plus_grand, pointeur);
                construction_union->parametres_resolus.ajoute(expr);
            }
            else {
                auto pointeur_donnees = pointeur;
                auto pointeur_discriminant = *reinterpret_cast<int *>(pointeur +
                                                                      type_union->decalage_index);
                auto index_membre = pointeur_discriminant - 1;

                auto type_donnees = type_union->membres[index_membre].type;

                for (auto i = 0; i < index_membre; ++i) {
                    construction_union->parametres_resolus.ajoute(nullptr);
                }

                auto expr = noeud_syntaxique_depuis_resultat(
                    espace, directive, lexeme, type_donnees, pointeur_donnees);
                construction_union->parametres_resolus.ajoute(expr);
            }

            return construction_union;
        }
        case GenreType::CHAINE:
        {
            auto valeur_pointeur = pointeur;
            auto valeur_chaine = *reinterpret_cast<long *>(pointeur + 8);

            kuri::chaine_statique chaine = {*reinterpret_cast<char **>(valeur_pointeur),
                                            valeur_chaine};

            auto lit_chaine = assembleuse->cree_litterale_chaine(lexeme);
            lit_chaine->valeur = compilatrice.gerante_chaine->ajoute_chaine(chaine);
            lit_chaine->type = type;
            return lit_chaine;
        }
        case GenreType::TYPE_DE_DONNEES:
        {
            auto type_de_donnees = *reinterpret_cast<Type **>(pointeur);
            type_de_donnees = espace->typeuse.type_type_de_donnees(type_de_donnees);
            return assembleuse->cree_reference_type(lexeme, type_de_donnees);
        }
        case GenreType::FONCTION:
        {
            auto fonction = *reinterpret_cast<AtomeFonction **>(pointeur);

            if (!fonction->decl) {
                espace->rapporte_erreur(directive,
                                        "La fonction retournée n'a pas de déclaration !\n");
            }

            return assembleuse->cree_reference_declaration(
                lexeme, const_cast<NoeudDeclarationEnteteFonction *>(fonction->decl));
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_opaque();
            auto expr = noeud_syntaxique_depuis_resultat(
                espace, directive, lexeme, type_opaque->type_opacifie, pointeur);

            /* comme dans la simplification de l'arbre, ceci doit être un transtypage vers le type
             * opaque */
            auto comme = assembleuse->cree_comme(lexeme);
            comme->type = type_opaque;
            comme->expression = expr;
            comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_opaque};
            comme->drapeaux |= TRANSTYPAGE_IMPLICITE;
            return comme;
        }
        case GenreType::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_tableau_fixe();

            auto virgule = assembleuse->cree_virgule(lexeme);
            virgule->expressions.reserve(type_tableau->taille);

            for (auto i = 0; i < type_tableau->taille; ++i) {
                auto pointeur_valeur = pointeur + type_tableau->type_pointe->taille_octet *
                                                      static_cast<unsigned>(i);
                auto expr = noeud_syntaxique_depuis_resultat(
                    espace, directive, lexeme, type_tableau->type_pointe, pointeur_valeur);
                virgule->expressions.ajoute(expr);
            }

            auto construction = assembleuse->cree_construction_tableau(lexeme);
            construction->type = type_tableau;
            construction->expression = virgule;
            return construction;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        {
            auto type_tableau = type->comme_tableau_dynamique();

            auto pointeur_donnees = *reinterpret_cast<octet_t **>(pointeur);
            auto taille_donnees = *reinterpret_cast<long *>(pointeur + 8);

            if (taille_donnees == 0) {
                espace->rapporte_erreur(directive, "Retour d'un tableau dynamique de taille 0 !");
            }

            /* crée un tableau fixe */
            auto type_tableau_fixe = espace->typeuse.type_tableau_fixe(
                type_tableau->type_pointe, static_cast<int>(taille_donnees));
            auto construction = noeud_syntaxique_depuis_resultat(
                espace, directive, lexeme, type_tableau_fixe, pointeur_donnees);

            /* convertis vers un tableau dynamique */
            auto comme = assembleuse->cree_comme(lexeme);
            comme->type = type_tableau;
            comme->expression = construction;
            comme->transformation = {TypeTransformation::CONVERTI_TABLEAU, type_tableau};
            comme->drapeaux |= TRANSTYPAGE_IMPLICITE;

            return comme;
        }
    }

    return nullptr;
}
