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

#include "assembleuse_arbre.h"
#include "compilatrice.hh"
#include "generation_code_c.hh"
#include "generation_code_llvm.hh"
#include "lexeuse.hh"
#include "modules.hh"
#include "syntaxeuse.hh"
#include "validation_semantique.hh"

const char *chaine_genre_tache(GenreTache genre)
{
#define ENUMERE_GENRE_TACHE_EX(genre) \
	case GenreTache::genre: return #genre;
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

OrdonnanceuseTache::OrdonnanceuseTache(Compilatrice *compilatrice)
	: m_compilatrice(compilatrice)
{}

void OrdonnanceuseTache::cree_tache_pour_chargement(EspaceDeTravail *espace, Fichier *fichier)
{
	espace->tache_chargement_ajoutee();

	auto unite = unites.ajoute_element(espace);
	unite->fichier = fichier;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = GenreTache::CHARGE_FICHIER;

	taches_chargement.enfile(tache);
}

void OrdonnanceuseTache::cree_tache_pour_lexage(EspaceDeTravail *espace, Fichier *fichier)
{
	assert(fichier->donnees_constantes->fut_charge);
	espace->tache_lexage_ajoutee();

	auto unite = unites.ajoute_element(espace);
	unite->fichier = fichier;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = GenreTache::LEXE;

	taches_lexage.enfile(tache);
}

void OrdonnanceuseTache::cree_tache_pour_parsage(EspaceDeTravail *espace, Fichier *fichier)
{
	assert(fichier->donnees_constantes->fut_lexe);
	espace->tache_parsage_ajoutee();

	auto unite = unites.ajoute_element(espace);
	unite->fichier = fichier;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = GenreTache::PARSE;

	taches_parsage.enfile(tache);
}

void OrdonnanceuseTache::cree_tache_pour_typage(EspaceDeTravail *espace, NoeudExpression *noeud)
{
	espace->tache_typage_ajoutee();

	auto unite = unites.ajoute_element(espace);
	unite->noeud = noeud;

	noeud->unite = unite;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = GenreTache::TYPAGE;

	taches_typage.enfile(tache);
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

long OrdonnanceuseTache::nombre_de_taches_en_attente() const
{
	return taches_chargement.taille()
			+ taches_lexage.taille()
			+ taches_parsage.taille()
			+ taches_typage.taille()
			+ taches_generation_ri.taille()
			+ taches_execution.taille()
			+ taches_message.taille();
}

void OrdonnanceuseTache::cree_tache_pour_generation_ri(EspaceDeTravail *espace, NoeudExpression *noeud)
{
	espace->tache_ri_ajoutee();

	auto unite = unites.ajoute_element(espace);
	unite->noeud = noeud;

	noeud->unite = unite;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = GenreTache::GENERE_RI;

	taches_generation_ri.enfile(tache);
}

void OrdonnanceuseTache::cree_tache_pour_execution(EspaceDeTravail *espace, MetaProgramme *metaprogramme)
{
	espace->tache_execution_ajoutee();

	auto unite = unites.ajoute_element(espace);
	unite->metaprogramme = metaprogramme;

	metaprogramme->unite = unite;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = GenreTache::EXECUTE;

	taches_execution.enfile(tache);
}

Tache OrdonnanceuseTache::tache_suivante(Tache &tache_terminee, bool tache_completee, int id, DrapeauxTacheronne drapeaux)
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
	auto nouvelle_tache = tache_suivante(espace, drapeaux);

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

			espace->tache_chargement_terminee(&(*m_compilatrice->messagere.verrou_ecriture()), unite->fichier);
			espace->tache_lexage_ajoutee();
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

			espace->tache_lexage_terminee(&(*m_compilatrice->messagere.verrou_ecriture()));
			espace->tache_parsage_ajoutee();
			tache_terminee.genre = GenreTache::PARSE;
			taches_parsage.enfile(tache_terminee);
			break;
		}
		case GenreTache::PARSE:
		{
			espace->tache_parsage_terminee(&(*m_compilatrice->messagere.verrou_ecriture()));
			break;
		}
		case GenreTache::TYPAGE:
		{
			// La tâche ne pût être complétée (une définition est attendue, etc.), remets-là dans la file en attendant.
			if (!tache_completee) {
				if (unite->etat() != UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME && espace->parsage_termine() && (unite->index_courant == unite->index_precedent)) {
					unite->cycle += 1;
				}

				if (unite->index_courant > unite->index_precedent) {
					unite->cycle = 0;
				}

				unite->index_precedent = unite->index_courant;

				taches_typage.enfile(tache_terminee);
				break;
			}

			espace->tache_typage_terminee(&(*m_compilatrice->messagere.verrou_ecriture()));

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
				espace->tache_ri_ajoutee();
				tache_terminee.genre = GenreTache::GENERE_RI;
				tache_terminee.unite->cycle = 0;
				taches_generation_ri.enfile(tache_terminee);
			}

			if (noeud->genre != GenreNoeud::DIRECTIVE_EXECUTION) {
				auto message_enfile = m_compilatrice->messagere->ajoute_message_typage_code(unite->espace, static_cast<NoeudDeclaration *>(noeud), unite);

				if (message_enfile) {
					taches_message.enfile(Tache::attend_message(unite));
				}
			}

			break;
		}
		case GenreTache::GENERE_RI:
		{
			// La tâche ne pût être complétée (une définition est attendue, etc.), remets-là dans la file en attendant.
			// Ici, seules les métaprogrammes peuvent nous revenir.
			if (!tache_completee) {
				tache_terminee.unite->cycle += 1;
				taches_generation_ri.enfile(tache_terminee);
				break;
			}

			espace->tache_ri_terminee(&(*m_compilatrice->messagere.verrou_ecriture()));
			break;
		}
		case GenreTache::GENERE_FICHIER_OBJET:
		{
			espace->tache_generation_objet_terminee(&(*m_compilatrice->messagere.verrou_ecriture()));

			if (espace->options.objet_genere == ObjetGenere::Executable) {
				espace->phase = PhaseCompilation::AVANT_LIAISON_EXECUTABLE;
				m_compilatrice->messagere->ajoute_message_phase_compilation(espace, PhaseCompilation::AVANT_LIAISON_EXECUTABLE);
				renseigne_etat_tacheronne(id, GenreTache::LIAISON_EXECUTABLE);
				return Tache::liaison_objet(espace);
			}
			else {
				m_compilatrice->messagere->ajoute_message_phase_compilation(espace, PhaseCompilation::COMPILATION_TERMINEE);
			}

			break;
		}
		case GenreTache::LIAISON_EXECUTABLE:
		{
			espace->tache_liaison_executable_terminee(&(*m_compilatrice->messagere.verrou_ecriture()));
			m_compilatrice->messagere->ajoute_message_phase_compilation(espace, PhaseCompilation::COMPILATION_TERMINEE);
			break;
		}
	}

	renseigne_etat_tacheronne(id, nouvelle_tache.genre);

	return nouvelle_tache;
}

Tache OrdonnanceuseTache::tache_suivante(EspaceDeTravail *espace, DrapeauxTacheronne drapeaux)
{
	/* toute tâcheronne pouvant lexer peut charger */
	if (!taches_chargement.est_vide() && (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_LEXER))) {
		return taches_chargement.defile();
	}

	if (!taches_lexage.est_vide() && (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_LEXER))) {
		return taches_lexage.defile();
	}

	if (!taches_parsage.est_vide() && (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_PARSER))) {
		return taches_parsage.defile();
	}

	// gère les tâches de messages avant les tâches de typage pour éviter les
	// problèmes de symbole non définis si un métaprogramme n'a pas encore généré
	// ce symbole
	// ce n'est pas la solution finale pour un tel problème, mais c'est un début
	// il nous faudra sans doute un système pour définir qu'un métaprogramme définira
	// tel ou tel symbole, et trouver de meilleures heuristiques pour arrêter la
	// compilation en cas d'indéfinition de symbole
	if (!taches_message.est_vide() && (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_ENVOYER_MESSAGE))) {
		return taches_message.defile();
	}

	if (!taches_typage.est_vide() && (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_TYPER))) {
		return taches_typage.defile();
	}

	if (!taches_generation_ri.est_vide() && (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_RI))) {
		return taches_generation_ri.defile();
	}

	if (espace->peut_generer_code_final()) {
		if (espace->options.objet_genere == ObjetGenere::Rien) {
			m_compilatrice->messagere->ajoute_message_phase_compilation(espace, PhaseCompilation::COMPILATION_TERMINEE);
		}
		else if (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_CODE)) {
			m_compilatrice->messagere->ajoute_message_phase_compilation(espace, PhaseCompilation::AVANT_GENERATION_OBJET);
			return Tache::genere_fichier_objet(espace);
		}
	}

	if (!taches_execution.est_vide() && (dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_EXECUTER))) {
		return taches_execution.defile();
	}

	if (!compilation_terminee) {
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
	etats_tacheronnes.pousse(GenreTache::DORS);
	return nombre_de_tacheronnes++;
}

void OrdonnanceuseTache::purge_messages()
{
	while (!taches_message.est_vide()) {
		taches_message.defile();
	}
}

Tacheronne::Tacheronne(Compilatrice &comp)
	: compilatrice(comp)
	, assembleuse(memoire::loge<AssembleuseArbre>("AssembleuseArbre", this->allocatrice_noeud))
	, id(compilatrice.ordonnanceuse->enregistre_tacheronne({}))
{}

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

	while (!compilatrice.possede_erreur) {
		tache = ordonnanceuse->tache_suivante(tache, tache_fut_completee, id, drapeaux);

		switch (tache.genre) {
			case GenreTache::COMPILATION_TERMINEE:
			{
				temps_scene = temps_debut.temps() - temps_executable - temps_fichier_objet;
				return;
			}
			case GenreTache::ENVOIE_MESSAGE:
			{
				assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_ENVOYER_MESSAGE));
				tache_fut_completee = tache.unite->message_recu;
				break;
			}
			case GenreTache::DORS:
			{
				dls::chrono::dors_microsecondes(100);
				tache_fut_completee = true;
				temps_passe_a_dormir += 0.1;
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
						auto texte = charge_fichier(donnees->chemin, *tache.unite->espace, {});
						temps_chargement += debut_chargement.temps();

						auto debut_tampon = dls::chrono::compte_seconde();
						donnees->charge_tampon(lng::tampon_source(texte));
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
						auto lexeuse = Lexeuse(compilatrice, donnees);
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
				syntaxeuse.lance_analyse();
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
					compilatrice.possede_erreur = true;

					if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_SYMBOLE) {
						erreur::lance_erreur("Trop de cycles : arrêt de la compilation sur un symbole inconnu", *unite->espace, unite->lexeme_attendu);
					}

					if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_DECLARATION) {
						auto decl = unite->declaration_attendue;
						auto unite_decl = decl->unite;
						auto erreur = rapporte_erreur(unite->espace, unite->declaration_attendue, "Je ne peux pas continuer la compilation car une déclaration ne peut être typée.");

						// À FAIRE : ne devrait pas arriver
						if (unite_decl) {
							erreur.ajoute_message("Note : l'unité de compilation est dans cette état :\n")
									.ajoute_message(chaine_attentes_recursives(unite))
									.ajoute_message("\n");
						}
					}

					if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_TYPE) {
						auto site = unite->noeud;

						if (site->est_corps_fonction()) {
							auto corps = site->comme_corps_fonction();
							site = corps->arbre_aplatis[unite->index_courant];
						}

						rapporte_erreur(unite->espace, site, "Je ne peux pas continuer la compilation car je n'arrive pas à déterminer un type pour l'expression", erreur::Genre::TYPE_INCONNU)
								.ajoute_message("Note : le type attendu est ")
								.ajoute_message(chaine_type(unite->type_attendu))
								.ajoute_message("\n");
					}

					if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI) {
						erreur::lance_erreur("Trop de cycles : arrêt de la compilation car une déclaration attend sur une interface de Kuri", *unite->espace, unite->noeud->lexeme);
					}

					if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_OPERATEUR) {
						if (unite->operateur_attendu->genre == GenreNoeud::OPERATEUR_BINAIRE) {
							auto expression_operation = static_cast<NoeudExpressionBinaire *>(unite->operateur_attendu);
							auto type1 = expression_operation->expr1->type;
							auto type2 = expression_operation->expr2->type;
							rapporte_erreur(unite->espace, unite->operateur_attendu, "Je ne peux pas continuer la compilation car je n'arrive pas à déterminer quel opérateur appelé.", erreur::Genre::TYPE_INCONNU)
									.ajoute_message("Le type à gauche de l'opérateur est ")
									.ajoute_message(chaine_type(type1))
									.ajoute_message("\nLe type à droite de l'opérateur est ")
									.ajoute_message(chaine_type(type2))
									.ajoute_message("\n\nMais aucun opérateur ne correspond à ces types-là.\n\n")
									.ajoute_conseil("Si vous voulez performer une opération sur des types non-communs, vous pouvez définir vos propres opérateurs avec la syntaxe suivante :\n\nopérateur op :: fonc (a: type1, b: type2) -> type_retour\n{\n\t...\n}\n");
						}
						else {
							auto expression_operation = static_cast<NoeudExpressionUnaire *>(unite->operateur_attendu);
							auto type = expression_operation->expr->type;
							rapporte_erreur(unite->espace, unite->operateur_attendu, "Je ne peux pas continuer la compilation car je n'arrive pas à déterminer quel opérateur appelé.", erreur::Genre::TYPE_INCONNU)
									.ajoute_message("\nLe type à droite de l'opérateur est ")
									.ajoute_message(chaine_type(type))
									.ajoute_message("\n\nMais aucun opérateur ne correspond à ces types-là.\n\n")
									.ajoute_conseil("Si vous voulez performer une opération sur des types non-communs, vous pouvez définir vos propres opérateurs avec la syntaxe suivante :\n\nopérateur op :: fonc (a: type) -> type_retour\n{\n\t...\n}\n");
						}
					}

					rapporte_erreur(unite->espace, unite->noeud, "Je ne peux pas continuer la compilation car une unité est bloqué dans un cycle")
							.ajoute_message("\nNote : l'unité est dans l'état : ")
							.ajoute_message(chaine_etat_unite(unite->etat()))
							.ajoute_message("\n");

					break;
				}

				auto debut_validation = dls::chrono::compte_seconde();
				tache_fut_completee = gere_unite_pour_typage(unite);
				temps_validation += debut_validation.temps();
				break;
			}
			case GenreTache::GENERE_RI:
			{
				assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_RI));
				auto debut_generation = dls::chrono::compte_seconde();
				tache_fut_completee = gere_unite_pour_ri(tache.unite);
				constructrice_ri.temps_generation += debut_generation.temps();
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
				assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_CODE));
				coulisse_C_cree_fichier_objet(compilatrice, constructrice_ri, *tache.espace, temps_generation_code, temps_fichier_objet);
				tache_fut_completee = true;
				break;
			}
			case GenreTache::LIAISON_EXECUTABLE:
			{
				assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_CODE));
				coulisse_C_cree_executable(compilatrice, *tache.espace, temps_executable);
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

		POUR (n->relations()) {
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
					return !contexte.valide_type_fonction(decl);
				}
				case GenreNoeud::DECLARATION_CORPS_FONCTION:
				{
					auto decl = static_cast<NoeudDeclarationCorpsFonction *>(unite->noeud);

					if (!decl->entete->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
						unite->attend_sur_declaration(decl->entete);
						return false;
					}

					if (decl->entete->est_operateur) {
						return !contexte.valide_operateur(decl);
					}

					return !contexte.valide_fonction(decl);
				}
				case GenreNoeud::DECLARATION_ENUM:
				{
					auto decl = static_cast<NoeudEnum *>(unite->noeud);
					return !contexte.valide_enum(decl);
				}
				case GenreNoeud::DECLARATION_STRUCTURE:
				{
					auto decl = static_cast<NoeudStruct *>(unite->noeud);
					return !contexte.valide_structure(decl);
				}
				case GenreNoeud::DECLARATION_VARIABLE:
				{
					auto decl = static_cast<NoeudDeclarationVariable *>(unite->noeud);

					for (; unite->index_courant < decl->arbre_aplatis.taille; ++unite->index_courant) {
						if (contexte.valide_semantique_noeud(decl->arbre_aplatis[unite->index_courant])) {
							auto graphe = unite->espace->graphe_dependance.verrou_ecriture();
							auto noeud_dependance = graphe->cree_noeud_globale(decl);
							graphe->ajoute_dependances(*noeud_dependance, contexte.donnees_dependance);
							return false;
						}
					}

					auto graphe = unite->espace->graphe_dependance.verrou_ecriture();
					auto noeud_dependance = graphe->cree_noeud_globale(decl);
					graphe->ajoute_dependances(*noeud_dependance, contexte.donnees_dependance);

					return true;
				}
				case GenreNoeud::DIRECTIVE_EXECUTION:
				{
					auto dir = static_cast<NoeudDirectiveExecution *>(unite->noeud);

					// À FAIRE : ne peut pas préserver les dépendances si nous échouons avant la fin
					for (unite->index_courant = 0; unite->index_courant < dir->arbre_aplatis.taille; ++unite->index_courant) {
						if (contexte.valide_semantique_noeud(dir->arbre_aplatis[unite->index_courant])) {
							return false;
						}
					}

					return true;
				}
				case GenreNoeud::INSTRUCTION_IMPORTE:
				case GenreNoeud::INSTRUCTION_CHARGE:
				{
					if (contexte.valide_semantique_noeud(unite->noeud)) {
						return false;
					}

					temps_validation -= contexte.temps_chargement;
					return true;
				}
				default:
				{
					assert_rappel(false, [&]() { std::cerr << "Genre de noeud inattendu dans la tâche de typage : " << unite->noeud->genre << '\n'; });
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

					if (decl_struct->unite->etat() == UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME) {
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
		noeud->type->drapeaux |= RI_TYPE_FUT_GENEREE;
	}
	else if (pour_metaprogramme) {
		auto corps = noeud->comme_corps_fonction();

#define ATTEND_SUR_TYPE_SI_NECESSAIRE(type) \
	if (type == nullptr) { \
		return false; \
	} \
	if ((type->drapeaux & RI_TYPE_FUT_GENEREE) == 0) { \
		unite->attend_sur_type(type); \
		return false; \
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
			compilatrice.ordonnanceuse->cree_tache_pour_typage(unite->espace, decl_creation_contexte->corps);
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

static void rassemble_globales_et_fonctions(
		EspaceDeTravail *espace,
		MachineVirtuelle &mv,
		NoeudDependance *racine,
		dls::tableau<AtomeGlobale *> &globales,
		dls::tableau<AtomeFonction *> &fonctions)
{
	auto graphe = espace->graphe_dependance.verrou_ecriture();
	auto index_dans_table_type = 1u;

	graphe->traverse(racine, [&](NoeudDependance *noeud_dep)
	{
		if (noeud_dep->est_fonction()) {
			auto decl_noeud = noeud_dep->fonction();

			if (decl_noeud->possede_drapeau(CODE_BINAIRE_FUT_GENERE)) {
				return;
			}

			auto atome_fonction = decl_noeud->atome_fonction;
			fonctions.pousse(atome_fonction);
			decl_noeud->drapeaux |= CODE_BINAIRE_FUT_GENERE;
		}
		else if (noeud_dep->est_type()) {
			auto type = noeud_dep->type();

			if ((type->drapeaux & CODE_BINAIRE_TYPE_FUT_GENERE) != 0) {
				return;
			}

			type->index_dans_table_types = index_dans_table_type++;

			if (type->genre == GenreType::STRUCTURE || type->genre == GenreType::UNION) {
				auto atome_fonction = type->fonction_init;
				assert(atome_fonction);
				fonctions.pousse(atome_fonction);
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

			globales.pousse(atome_globale);

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

	if ((metaprogramme->fonction->drapeaux & RI_FUT_GENEREE) == 0) {
		return false;
	}

	auto espace = unite->espace;

	dls::tableau<AtomeGlobale *> globales;
	dls::tableau<AtomeFonction *> fonctions;
	rassemble_globales_et_fonctions(espace, mv, metaprogramme->fonction->noeud_dependance, globales, fonctions);

	auto fonction = metaprogramme->fonction->atome_fonction;

	if (!fonction) {
		rapporte_erreur(espace, metaprogramme->fonction, "Impossible de trouver la fonction pour le métaprogramme");
	}

	if (globales.taille() != 0) {
		auto fonc_init = constructrice_ri.genere_fonction_init_globales_et_appel(espace, globales, fonction);
		fonctions.pousse(fonc_init);
	}

	POUR (fonctions) {
		genere_code_binaire_pour_fonction(it, &mv);
	}

	//desassemble(fonction->chunk, metaprogramme->fonction->nom_broye(unite->espace).c_str(), std::cerr);

	auto res = mv.interprete(metaprogramme);

	// À FAIRE : précision des messages d'erreurs
	if (res == MachineVirtuelle::ResultatInterpretation::ERREUR) {
		rapporte_erreur(espace, metaprogramme->directive, "Erreur lors de l'exécution du métaprogramme");
	}
	else {
		if (metaprogramme->directive && metaprogramme->directive->ident == ID::assert_) {
			auto resultat = *reinterpret_cast<bool *>(mv.pointeur_pile);

			if (!resultat) {
				rapporte_erreur(espace, metaprogramme->directive, "Échec de l'assertion");
			}

			metaprogramme->fut_execute = true;
		}
		else if (metaprogramme->corps_texte) {
			auto resultat = *reinterpret_cast<kuri::chaine *>(mv.pointeur_pile);

			if (resultat.taille == 0) {
				rapporte_erreur(espace, metaprogramme->corps_texte, "Le corps-texte a retourné une chaine vide");
			}

			auto fichier_racine = espace->fichier(metaprogramme->corps_texte->lexeme->fichier);
			auto module = fichier_racine->module;

			auto tampon = dls::chaine(resultat.pointeur, resultat.taille);

			if (*tampon.fin() != ' ') {
				tampon.pousse('\n');
			}

			auto nom_fichier = dls::vers_chaine(metaprogramme);
			auto resultat_fichier = espace->trouve_ou_cree_fichier(compilatrice.sys_module, module, nom_fichier, nom_fichier, false);

			if (resultat_fichier.tag_type() == FichierNeuf::tag) {
				auto fichier = resultat_fichier.t2().fichier;
				auto donnees_fichier = fichier->donnees_constantes;

				fichier->module = module;
				fichier->metaprogramme_corps_texte = metaprogramme;

				donnees_fichier->charge_tampon(lng::tampon_source(tampon));

				// À FAIRE : ajoute source aux chaines ajoutées
				compilatrice.ordonnanceuse->cree_tache_pour_lexage(espace, fichier);
			}
		}
	}

	unite->espace->tache_execution_terminee(&(*compilatrice.messagere.verrou_ecriture()));

	return true;
}
