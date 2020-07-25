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

#include <unistd.h>

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/structures/file.hh"

#include "compilatrice.hh"
#include "lexeuse.hh"
#include "modules.hh"
#include "syntaxeuse.hh"
#include "validation_semantique.hh"

static int id_tacheronne = 0;

static auto dors_millisecondes(int millisecondes)
{
	assert(millisecondes >= 0);
	usleep(static_cast<unsigned>(millisecondes * 1000));
}

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

Tache Tache::dors()
{
	Tache t;
	t.genre = GenreTache::DORS;
	return t;
}

Tache Tache::compilation_terminee()
{
	Tache t;
	t.genre = GenreTache::COMPILATION_TERMINEE;
	return t;
}

OrdonnanceuseTache::OrdonnanceuseTache(Compilatrice *compilatrice)
	: m_compilatrice(compilatrice)
{}

void OrdonnanceuseTache::cree_tache_pour_lexage(EspaceDeTravail *espace, Fichier *fichier)
{
	auto unite = unites.ajoute_element(espace);
	unite->fichier = fichier;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = GenreTache::LEXE;

	taches_lexage.enfile(tache);
}

void OrdonnanceuseTache::cree_tache_pour_parsage(EspaceDeTravail *espace, Fichier *fichier)
{
	assert(fichier->fut_lexe);
	espace->nombre_taches_parsage += 1;

	auto unite = unites.ajoute_element(espace);
	unite->fichier = fichier;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = GenreTache::PARSE;

	taches_parsage.enfile(tache);
}

void OrdonnanceuseTache::cree_tache_pour_typage(EspaceDeTravail *espace, NoeudExpression *noeud)
{
	cree_tache_pour_typage(espace, noeud, GenreTache::TYPAGE);
}

void OrdonnanceuseTache::cree_tache_pour_typage_fonction(EspaceDeTravail *espace, NoeudDeclarationFonction *noeud)
{
	cree_tache_pour_typage(espace, noeud, GenreTache::TYPAGE_ENTETE_FONCTION);
}

void OrdonnanceuseTache::cree_tache_pour_typage(EspaceDeTravail *espace, NoeudExpression *noeud, GenreTache genre_tache)
{
	espace->nombre_taches_typage += 1;

	auto unite = unites.ajoute_element(espace);
	unite->noeud = noeud;

	noeud->unite = unite;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = genre_tache;

	taches_typage.enfile(tache);
}

void OrdonnanceuseTache::renseigne_etat_tacheronne(int id, GenreTache genre_tache)
{
	if (id >= etats_tacheronnes.taille()) {
		etats_tacheronnes.redimensionne(id + 1);
	}

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

void OrdonnanceuseTache::cree_tache_pour_generation_ri(EspaceDeTravail *espace, NoeudExpression *noeud)
{
	espace->nombre_taches_ri += 1;

	auto unite = unites.ajoute_element(espace);
	unite->noeud = noeud;

	noeud->unite = unite;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = GenreTache::GENERE_RI;

	taches_generation_ri.enfile(tache);
}

void OrdonnanceuseTache::cree_tache_pour_execution(EspaceDeTravail *espace, NoeudDirectiveExecution *noeud)
{
	espace->nombre_taches_execution += 1;

	auto unite = unites.ajoute_element(espace);
	unite->noeud = noeud;

	noeud->unite = unite;

	auto tache = Tache();
	tache.unite = unite;
	tache.genre = GenreTache::EXECUTE;

	taches_execution.enfile(tache);
}

Tache OrdonnanceuseTache::tache_suivante(const Tache &tache_terminee, bool tache_completee, int id, bool premiere)
{
	auto nouvelle_tache = tache_terminee;

	auto unite = nouvelle_tache.unite;
	auto espace = static_cast<EspaceDeTravail *>(nullptr);

	// unité peut-être nulle pour les tâches DORS du début de la compilation
	if (unite) {
		espace = unite->espace;
	}

	switch (tache_terminee.genre) {
		case GenreTache::DORS:
		case GenreTache::COMPILATION_TERMINEE:
		case GenreTache::EXECUTE:
		{
			// rien à faire, ces tâches-là sont considérées comme à la fin de leurs cycles
			break;
		}
		case GenreTache::LEXE:
		{
			espace->nombre_taches_parsage += 1;
			nouvelle_tache.genre = GenreTache::PARSE;
			taches_parsage.enfile(nouvelle_tache);
			break;
		}
		case GenreTache::PARSE:
		{
			espace->nombre_taches_parsage -= 1;
			if (espace->nombre_taches_parsage == 0) {
				m_compilatrice->messagere->ajoute_message_phase_compilation(espace, PhaseCompilation::PARSAGE_TERMINE);
			}

			break;
		}
		case GenreTache::TYPAGE_ENTETE_FONCTION:
		{
			if (!tache_completee) {
				nouvelle_tache.unite->cycle += 1;
				taches_typage.enfile(nouvelle_tache);
				break;
			}

			espace->nombre_taches_typage -= 1;
			if (espace->nombre_taches_typage == 0) {
				m_compilatrice->messagere->ajoute_message_phase_compilation(espace, PhaseCompilation::TYPAGE_TERMINE);
			}

			auto decl = static_cast<NoeudDeclarationFonction *>(unite->noeud);

			if (decl->est_externe) {
				espace->nombre_taches_ri += 1;
				nouvelle_tache.genre = GenreTache::GENERE_RI;
				nouvelle_tache.unite->cycle = 0;
				taches_generation_ri.enfile(nouvelle_tache);
			}

			break;
		}
		case GenreTache::TYPAGE:
		{
			// La tâche ne pût être complétée (une définition est attendue, etc.), remets-là dans la file en attendant.
			if (!tache_completee) {
				nouvelle_tache.unite->cycle += 1;
				taches_typage.enfile(nouvelle_tache);
				break;
			}

			espace->nombre_taches_typage -= 1;
			if (espace->nombre_taches_typage == 0) {
				m_compilatrice->messagere->ajoute_message_phase_compilation(espace, PhaseCompilation::TYPAGE_TERMINE);
			}

			auto generation_ri_requise = true;
			auto noeud = unite->noeud;

			if (dls::outils::est_element(noeud->genre, GenreNoeud::DECLARATION_FONCTION, GenreNoeud::DECLARATION_COROUTINE, GenreNoeud::DECLARATION_OPERATEUR)) {
				auto decl_fonc = static_cast<NoeudDeclarationFonction *>(noeud);

				if (decl_fonc->est_gabarit && !decl_fonc->est_instantiation_gabarit) {
					generation_ri_requise = false;
				}
			}

			if (generation_ri_requise) {
				espace->nombre_taches_ri += 1;
				nouvelle_tache.genre = GenreTache::GENERE_RI;
				nouvelle_tache.unite->cycle = 0;
				taches_generation_ri.enfile(nouvelle_tache);
			}

			if (noeud->genre != GenreNoeud::DIRECTIVE_EXECUTION) {
				m_compilatrice->messagere->ajoute_message_typage_code(unite->espace, static_cast<NoeudDeclaration *>(noeud));
			}

			break;
		}
		case GenreTache::GENERE_RI:
		{
			// La tâche ne pût être complétée (une définition est attendue, etc.), remets-là dans la file en attendant.
			// Ici, seules les métaprogrammes peuvent nous revenir.
			if (!tache_completee) {
				nouvelle_tache.unite->cycle += 1;
				taches_generation_ri.enfile(nouvelle_tache);
				break;
			}

			espace->nombre_taches_ri -= 1;
			if (espace->nombre_taches_ri == 0) {
				m_compilatrice->messagere->ajoute_message_phase_compilation(espace, PhaseCompilation::GENRERATION_CODE_TERMINEE);
			}
			break;
		}
	}

	// au début de la compilation les tâcheronnes nous donne une Tache::DORS qu'elles ont achevées (ceci pour avoir une IPA simple)
	// or, ces tâches n'ont pas été données par la plannifieuse et le nombre_de_taches_en_proces est alors de 0
	// pour éviter les nombres négatif vérifie que nous ne sommes pas au début de la compilation
	if (!premiere) {
		nombre_de_taches_en_proces -= 1;
	}

	if (!taches_lexage.est_vide()) {
		nombre_de_taches_en_proces += 1;
		renseigne_etat_tacheronne(id, GenreTache::LEXE);
		return taches_lexage.defile();
	}

	if (!taches_parsage.est_vide()) {
		nombre_de_taches_en_proces += 1;
		renseigne_etat_tacheronne(id, GenreTache::LEXE);
		return taches_parsage.defile();
	}

	if (!taches_typage.est_vide()) {
		nombre_de_taches_en_proces += 1;
		renseigne_etat_tacheronne(id, GenreTache::TYPAGE);
		return taches_typage.defile();
	}

	if (!taches_generation_ri.est_vide()) {
		nombre_de_taches_en_proces += 1;
		renseigne_etat_tacheronne(id, GenreTache::GENERE_RI);
		return taches_generation_ri.defile();
	}

	// À FAIRE : plusieurs tacheronnes pour exécuter le code
	if (!taches_execution.est_vide()) {
		nombre_de_taches_en_proces += 1;
		renseigne_etat_tacheronne(id, GenreTache::DORS);
		return Tache::dors();
	}

	if (nombre_de_taches_en_proces != 0 && !toutes_les_tacheronnes_dorment()) {
		nombre_de_taches_en_proces += 1;
		renseigne_etat_tacheronne(id, GenreTache::DORS);
		return Tache::dors();
	}

	// À FAIRE(ordonnanceuse) : notifie proprement les métaprogrammes
	compilation_terminee = true;

	return Tache::compilation_terminee();
}

Tache OrdonnanceuseTache::tache_metaprogramme_suivante(const Tache &/*tache_terminee*/, int id, bool premiere)
{
	if (!premiere) {
		nombre_de_taches_en_proces -= 1;
	}

	if (!taches_execution.est_vide()) {
		nombre_de_taches_en_proces += 1;
		renseigne_etat_tacheronne(id, GenreTache::EXECUTE);
		return taches_execution.defile();
	}

	if (nombre_de_taches_en_proces != 0 || !compilation_terminee) {
		nombre_de_taches_en_proces += 1;
		renseigne_etat_tacheronne(id, GenreTache::DORS);
		return Tache::dors();
	}

	return Tache::compilation_terminee();
}

long OrdonnanceuseTache::memoire_utilisee() const
{
	auto memoire = 0l;
	memoire += static_cast<long>(unites.memoire_utilisee());
	return memoire;
}

Tacheronne::Tacheronne(Compilatrice &comp)
	: compilatrice(comp)
	, id(id_tacheronne++)
{}

void Tacheronne::gere_tache()
{
	auto tache = Tache::dors();
	auto premiere = true;
	auto tache_fut_completee = true;
	auto &ordonnanceuse = compilatrice.ordonnanceuse;

	while (!compilatrice.possede_erreur) {
		tache = ordonnanceuse->tache_suivante(tache, tache_fut_completee, id, premiere);
		premiere = false;

		switch (tache.genre) {
			case GenreTache::COMPILATION_TERMINEE:
			{
				compilatrice.temps_lexage = temps_lexage;
				compilatrice.temps_validation = temps_validation;
				return;
			}
			case GenreTache::DORS:
			{
				dors_millisecondes(1);
				tache_fut_completee = true;
				break;
			}
			case GenreTache::LEXE:
			{
				auto unite = tache.unite;
				auto debut_lexage = dls::chrono::compte_seconde();
				auto lexeuse = Lexeuse(compilatrice, unite->fichier);
				lexeuse.performe_lexage();
				temps_lexage += debut_lexage.temps();
				tache_fut_completee = true;
				break;
			}
			case GenreTache::PARSE:
			{
				auto unite = tache.unite;
				auto debut_parsage = dls::chrono::compte_seconde();
				auto syntaxeuse = Syntaxeuse(compilatrice, unite->fichier, unite, compilatrice.racine_kuri);
				syntaxeuse.lance_analyse();
				temps_parsage += debut_parsage.temps();
				tache_fut_completee = true;
				break;
			}
			case GenreTache::TYPAGE_ENTETE_FONCTION:
			{
				auto unite = tache.unite;
				auto debut_validation = dls::chrono::compte_seconde();
				auto contexte = ContexteValidationCode(compilatrice, *unite);
				auto decl = static_cast<NoeudDeclarationFonction *>(unite->noeud);

				tache_fut_completee = !contexte.valide_type_fonction(decl);

				temps_validation += debut_validation.temps();
				break;
			}
			case GenreTache::TYPAGE:
			{
				auto unite = tache.unite;

				if (unite->cycle > 10) {
					if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_SYMBOLE) {
						erreur::lance_erreur("Trop de cycles : arrêt de la compilation sur un symbole inconnu", *unite->espace, unite->lexeme_attendu);
					}

					if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_DECLARATION) {
						erreur::lance_erreur("Trop de cycles : arrêt de la compilation sur un déclaration non validée", *unite->espace, unite->declaration_attendue->lexeme);
					}

					if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_TYPE) {
						erreur::lance_erreur("Trop de cycles : arrêt de la compilation sur un type non validée", *unite->espace, unite->noeud->lexeme);
					}

					if (unite->etat() == UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI) {
						erreur::lance_erreur("Trop de cycles : arrêt de la compilation car une déclaration attend sur une interface de Kuri", *unite->espace, unite->noeud->lexeme);
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
				auto debut_generation = dls::chrono::compte_seconde();
				tache_fut_completee = gere_unite_pour_ri(tache.unite);
				constructrice_ri.temps_generation += debut_generation.temps();
				break;
			}
			case GenreTache::EXECUTE:
			{
				break;
			}
		}
	}

	compilatrice.temps_lexage = temps_lexage;
	compilatrice.temps_validation = temps_validation;
}

void Tacheronne::gere_tache_metaprogramme()
{
	auto tache = Tache::dors();
	auto premiere = true;
	auto &ordonnanceuse = compilatrice.ordonnanceuse;

	while (!compilatrice.possede_erreur) {
		tache = ordonnanceuse->tache_metaprogramme_suivante(tache, id, premiere);
		premiere = false;

		if (tache.genre == GenreTache::DORS) {
			dors_millisecondes(1);
			continue;
		}

		if (tache.genre == GenreTache::COMPILATION_TERMINEE) {
			break;
		}

		gere_unite_pour_execution(tache.unite);
	}
}

static bool dependances_eurent_ri_generees(NoeudDependance *noeud)
{
	auto visite = dls::ensemblon<NoeudDependance *, 32>();
	auto a_visiter = dls::file<NoeudDependance *>();

	a_visiter.enfile(noeud);

	while (!a_visiter.est_vide()) {
		auto n = a_visiter.defile();

		visite.insere(n);

		POUR (n->relations) {
			auto noeud_fin = it.noeud_fin;

			if (noeud_fin->type == TypeNoeudDependance::TYPE) {
				if ((noeud_fin->type_->drapeaux & RI_TYPE_FUT_GENEREE) == 0) {
					return false;
				}
			}
			else {
				auto noeud_syntaxique = noeud_fin->noeud_syntaxique;

				if ((noeud_syntaxique->drapeaux & RI_FUT_GENEREE) == 0) {
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
			auto contexte = ContexteValidationCode(compilatrice, *unite);

			switch (unite->noeud->genre) {
				case GenreNoeud::DECLARATION_COROUTINE:
				case GenreNoeud::DECLARATION_FONCTION:
				{
					auto decl = static_cast<NoeudDeclarationFonction *>(unite->noeud);

					if ((decl->drapeaux & DECLARATION_FUT_VALIDEE) == 0) {
						unite->attend_sur_declaration(decl);
						return false;
					}

					return !contexte.valide_fonction(decl);
				}
				case GenreNoeud::DECLARATION_OPERATEUR:
				{
					auto decl = static_cast<NoeudDeclarationFonction *>(unite->noeud);
					return !contexte.valide_operateur(decl);
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

					POUR (decl->arbre_aplatis) {
						if (contexte.valide_semantique_noeud(it)) {
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

					POUR (dir->arbre_aplatis) {
						if (contexte.valide_semantique_noeud(it)) {
							return false;
						}
					}

					return true;
				}
				default:
				{
					assert(false);
					break;
				}
			}

			return true;
		}
		case UniteCompilation::Etat::ATTEND_SUR_TYPE:
		{
			auto type = unite->type_attendu;

			if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
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
		case UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI:
		case UniteCompilation::Etat::ATTEND_SUR_SYMBOLE:
		{
			unite->restaure_etat_original();
			return gere_unite_pour_typage(unite);
		}
	}

	return false;
}

bool Tacheronne::gere_unite_pour_ri(UniteCompilation *unite)
{
	auto noeud = unite->noeud;

	if (est_declaration(noeud->genre)) {
		constructrice_ri.genere_ri_pour_noeud(unite->espace, noeud);
		noeud->drapeaux |= RI_FUT_GENEREE;
		noeud->type->drapeaux |= RI_TYPE_FUT_GENEREE;
	}
	else if (noeud->genre == GenreNoeud::DIRECTIVE_EXECUTION) {
		auto noeud_dir = static_cast<NoeudDirectiveExecution *>(noeud);

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
			unite->attend_sur_interface_kuri();
			return false;
		}

		if ((unite->espace->interface_kuri->decl_creation_contexte->drapeaux & RI_FUT_GENEREE) == 0) {
			unite->attend_sur_declaration(unite->espace->interface_kuri->decl_creation_contexte);
			return false;
		}

		if (!dependances_eurent_ri_generees(noeud_dir->fonction->noeud_dependance)) {
			return false;
		}

		constructrice_ri.genere_ri_pour_fonction_metaprogramme(unite->espace, noeud_dir);
		compilatrice.ordonnanceuse->cree_tache_pour_execution(unite->espace, noeud_dir);
	}

	return true;
}

void Tacheronne::gere_unite_pour_execution(UniteCompilation *unite)
{
	auto noeud = static_cast<NoeudDirectiveExecution *>(unite->noeud);
	auto espace = unite->espace;

	auto index_dans_table_type = 1u;

	dls::tableau<AtomeGlobale *> globales;
	dls::tableau<AtomeFonction *> fonctions;

	globales.efface();
	fonctions.efface();

	traverse_graphe(noeud->fonction->noeud_dependance, [&](NoeudDependance *noeud_dep)
	{
		if (noeud_dep->type == TypeNoeudDependance::FONCTION) {
			auto decl_noeud = static_cast<NoeudDeclarationFonction *>(noeud_dep->noeud_syntaxique);

			if ((decl_noeud->drapeaux & CODE_BINAIRE_FUT_GENERE) != 0) {
				return;
			}

			auto atome_fonction = espace->trouve_fonction(decl_noeud->nom_broye);
			fonctions.pousse(atome_fonction);
			decl_noeud->drapeaux |= CODE_BINAIRE_FUT_GENERE;
		}
		else if (noeud_dep->type == TypeNoeudDependance::TYPE) {
			auto type = noeud_dep->type_;

			if ((type->drapeaux & CODE_BINAIRE_TYPE_FUT_GENERE) != 0) {
				return;
			}

			type->index_dans_table_types = index_dans_table_type++;

			if (type->genre == GenreType::STRUCTURE || type->genre == GenreType::UNION) {
				auto nom_fonction = "initialise_" + dls::vers_chaine(type);
				auto atome_fonction = espace->trouve_fonction(nom_fonction);
				fonctions.pousse(atome_fonction);
				type->drapeaux |= CODE_BINAIRE_TYPE_FUT_GENERE;
			}
		}
		else if (noeud_dep->type == TypeNoeudDependance::GLOBALE) {
			auto decl_noeud = static_cast<NoeudDeclaration *>(noeud_dep->noeud_syntaxique);

			if ((decl_noeud->drapeaux & EST_CONSTANTE) != 0) {
				return;
			}

			if ((decl_noeud->drapeaux & CODE_BINAIRE_FUT_GENERE) != 0) {
				return;
			}

			auto atome_globale = espace->trouve_globale(decl_noeud);

			if (atome_globale->index == -1) {
				atome_globale->index = compilatrice.mv.ajoute_globale(decl_noeud->type, decl_noeud->ident);
			}

			globales.pousse(atome_globale);

			decl_noeud->drapeaux |= CODE_BINAIRE_FUT_GENERE;
		}
	});

	POUR_TABLEAU_PAGE (espace->graphe_dependance->noeuds) {
		it.fut_visite = false;
	}

	auto fonction = espace->trouve_fonction(noeud->fonction->nom_broye);

	if (!fonction) {
		std::cerr << "Impossible de trouver la fonction métaprogramme pour " << noeud->fonction->nom_broye << '\n';
		imprime_fichier_ligne(*espace, *noeud->lexeme);
	}

	//desassemble(fonction->chunk, noeud->fonction->nom_broye.c_str(), std::cout);

	if (globales.taille() != 0) {
		auto fonc_init = constructrice_ri.genere_fonction_init_globales_et_appel(espace, globales, fonction);
		fonctions.pousse(fonc_init);
	}

	POUR (fonctions) {
		genere_code_binaire_pour_fonction(it, &compilatrice.mv);
	}

	auto res = compilatrice.mv.interprete(fonction);

	if (res == MachineVirtuelle::ResultatInterpretation::ERREUR) {
		// À FAIRE : erreur de compilation si une erreur d'exécution
		std::cerr << "Erreur lors de l'exécution du métaprogramme !\n-- ";
		imprime_fichier_ligne(*espace, *noeud->lexeme);
	}
	else {
		if (noeud->ident == ID::assert_) {
			auto resultat = *reinterpret_cast<bool *>(compilatrice.mv.pointeur_pile);

			if (!resultat) {
				// À FAIRE : erreur de compilation si une assertion échoue
				std::cerr << "Échec de l'assertion !\n-- ";
				imprime_fichier_ligne(*espace, *noeud->lexeme);
			}
		}
	}
}
