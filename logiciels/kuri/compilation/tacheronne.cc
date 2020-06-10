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

#include "compilatrice.hh"
#include "lexeuse.hh"
#include "modules.hh"
#include "syntaxeuse.hh"
#include "validation_semantique.hh"

Tacheronne::Tacheronne(Compilatrice &comp)
	: compilatrice(comp)
{}

void Tacheronne::gere_tache()
{
	while (true) {
		if (compilatrice.file_compilation.est_vide()) {
			break;
		}

		//std::cerr << "-----------------------------------------\n";
		//std::cerr << "-- taille file compilation : " << compilatrice.file_compilation.taille() << '\n';
		auto unite = compilatrice.file_compilation.effronte();

		if (unite.cycle <= 10) {
			gere_unite(unite);
			continue;
		}

		while (true) {
			if (unite.etat == UniteCompilation::Etat::ATTEND_SUR_SYMBOLE) {
				erreur::lance_erreur("Trop de cycles : arrêt de la compilation sur un symbole inconnu", compilatrice, unite.lexeme_attendu);
			}

			if (compilatrice.file_compilation.est_vide()) {
				break;
			}

			unite = compilatrice.file_compilation.effronte();
		}

		static Lexeme lexeme = { "", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0 };
		erreur::lance_erreur("Trop de cycles : arrêt de la compilation", compilatrice, &lexeme);
		return;
	}
}

void Tacheronne::gere_unite(UniteCompilation unite)
{
	switch (unite.etat) {
		case UniteCompilation::Etat::PARSAGE_ATTENDU:
		{
			//std::cerr << "-- lèxe fichier : " << unite.fichier->chemin << '\n';
			auto debut_lexage = dls::chrono::compte_seconde();
			auto lexeuse = Lexeuse(compilatrice, unite.fichier);
			lexeuse.performe_lexage();
			temps_lexage += debut_lexage.temps();

			//std::cerr << "-- parse fichier : " << unite.fichier->chemin << '\n';
			auto debut_parsage = dls::chrono::compte_seconde();
			auto syntaxeuse = Syntaxeuse(compilatrice, unite.fichier, compilatrice.racine_kuri);
			syntaxeuse.lance_analyse();
			temps_parsage += debut_parsage.temps();
			break;
		}
		case UniteCompilation::Etat::TYPAGE_ENTETE_FONCTION_ATTENDU:
		{
			auto debut_validation = dls::chrono::compte_seconde();
			auto contexte = ContexteValidationCode(compilatrice);
			contexte.unite = &unite;
			auto decl = static_cast<NoeudDeclarationFonction *>(unite.noeud);

//			std::cerr << "-- valide entête fonction : " << decl->lexeme->chaine << '\n';
			if (contexte.valide_type_fonction(decl)) {
//				std::cerr << "-- une erreur est survenue\n";
//				std::cerr << "-- regère dans état : " << unite.etat << '\n';
//				std::cerr << "-- état original    : " << unite.etat_original << '\n';
				unite.etat_original = UniteCompilation::Etat::TYPAGE_ENTETE_FONCTION_ATTENDU;
				compilatrice.file_compilation.pousse(unite);
			}

			temps_validation += debut_validation.temps();
			break;
		}
		case UniteCompilation::Etat::TYPAGE_ATTENDU:
		{
			auto debut_validation = dls::chrono::compte_seconde();
			auto contexte = ContexteValidationCode(compilatrice);
			contexte.unite = &unite;

			switch (unite.noeud->genre) {
				case GenreNoeud::DECLARATION_COROUTINE:
				case GenreNoeud::DECLARATION_FONCTION:
				{
					auto decl = static_cast<NoeudDeclarationFonction *>(unite.noeud);

					//std::cerr << "-- valide fonction : " << decl->lexeme->chaine << '\n';
					if ((decl->drapeaux & DECLARATION_FUT_VALIDEE) == 0) {
						//std::cerr << "-- l'entête de la fonction ne fut pas encore validé\n";
						unite.etat = UniteCompilation::Etat::ATTEND_SUR_DECLARATION;
						unite.declaration_attendue = decl;
						compilatrice.file_compilation.pousse(unite);
						return;
					}

					if (contexte.valide_fonction(decl)) {
						//std::cerr << "-- Une erreur est survenue\n";
						compilatrice.file_compilation.pousse(unite);
						return;
					}

					if (!decl->est_gabarit || decl->est_instantiation_gabarit) {
						unite.etat = UniteCompilation::Etat::RI_ATTENDUE;
						compilatrice.file_compilation.pousse(unite);
					}

					break;
				}
				case GenreNoeud::DECLARATION_OPERATEUR:
				{
					auto decl = static_cast<NoeudDeclarationFonction *>(unite.noeud);
					//std::cerr << "-- valide opérateur : " << decl->lexeme->chaine << '\n';
					if (contexte.valide_operateur(decl)) {
						//std::cerr << "-- Une erreur est survenue\n";
						compilatrice.file_compilation.pousse(unite);
						return;
					}

					unite.etat = UniteCompilation::Etat::RI_ATTENDUE;
					compilatrice.file_compilation.pousse(unite);

					break;
				}
				case GenreNoeud::DECLARATION_ENUM:
				{
					auto decl = static_cast<NoeudEnum *>(unite.noeud);
					//std::cerr << "-- valide énum : " << decl->lexeme->chaine << '\n';
					if (contexte.valide_enum(decl)) {
						//std::cerr << "-- Une erreur est survenue\n";
						compilatrice.file_compilation.pousse(unite);
						return;
					}
					break;
				}
				case GenreNoeud::DECLARATION_STRUCTURE:
				{
					auto decl = static_cast<NoeudStruct *>(unite.noeud);
					//std::cerr << "-- valide structure : " << decl->lexeme->chaine << '\n';
					if (contexte.valide_structure(decl)) {
						//std::cerr << "-- Une erreur est survenue\n";
						compilatrice.file_compilation.pousse(unite);
						return;
					}

					unite.etat = UniteCompilation::Etat::RI_ATTENDUE;
					compilatrice.file_compilation.pousse(unite);
					break;
				}
				case GenreNoeud::DECLARATION_VARIABLE:
				{
					auto decl = static_cast<NoeudDeclarationVariable *>(unite.noeud);
					auto arbre_aplatis = kuri::tableau<NoeudExpression *>();
					aplatis_arbre(decl, arbre_aplatis, drapeaux_noeud::AUCUN);

					//std::cerr << "-- valide globale : " << decl->valeur->lexeme->chaine << '\n';
					for (auto &n : arbre_aplatis) {
						if (contexte.valide_semantique_noeud(n)) {
							//std::cerr << "-- Une erreur est survenue\n";
							compilatrice.file_compilation.pousse(unite);
							return;
						}
					}

					auto noeud_dependance = compilatrice.graphe_dependance.cree_noeud_globale(decl);
					compilatrice.graphe_dependance.ajoute_dependances(*noeud_dependance, contexte.donnees_dependance);

					unite.etat = UniteCompilation::Etat::RI_ATTENDUE;
					compilatrice.file_compilation.pousse(unite);

					break;
				}
				case GenreNoeud::DIRECTIVE_EXECUTION:
				{
					auto arbre_aplatis = kuri::tableau<NoeudExpression *>();
					aplatis_arbre(unite.noeud, arbre_aplatis, drapeaux_noeud::AUCUN);

					//std::cerr << "-- valide directive exécution\n";
					for (auto &n : arbre_aplatis) {
						if (contexte.valide_semantique_noeud(n)) {
							//std::cerr << "-- Une erreur est survenue\n";
							compilatrice.file_compilation.pousse(unite);
							return;
						}
					}

					unite.etat = UniteCompilation::Etat::RI_ATTENDUE;
					compilatrice.file_compilation.pousse(unite);

					break;
				}
				default:
				{
					assert(false);
					break;
				}
			}

			temps_validation += debut_validation.temps();
			break;
		}
		case UniteCompilation::Etat::RI_ATTENDUE:
		{
			auto noeud = unite.noeud;

			auto debut_generation = dls::chrono::compte_seconde();

			if (est_declaration(noeud->genre)) {
				compilatrice.constructrice_ri.genere_ri_pour_noeud(noeud);
			}
			else if (noeud->genre == GenreNoeud::DIRECTIVE_EXECUTION) {
				auto noeud_dir = static_cast<NoeudDirectiveExecution *>(noeud);
				compilatrice.constructrice_ri.genere_ri_pour_noeud(noeud_dir->fonction);
				// À FAIRE : il faut attendre sur les différents types pour construire le contexte
				//compilatrice.constructrice_ri.genere_ri_pour_fonction_metaprogramme(noeud_dir);
			}

			compilatrice.constructrice_ri.temps_generation += debut_generation.temps();

			break;
		}
		case UniteCompilation::Etat::CODE_MACHINE_ATTENDU:
		{
			break;
		}
		case UniteCompilation::Etat::ATTEND_SUR_TYPE:
		{
			unite.cycle += 1;

			//std::cerr << "-- regère unité attendant sur type : " << chaine_type(unite.type_attendu) << "\n";
			//std::cerr << "-- cycle : " << unite.cycle << '\n';

			auto type = unite.type_attendu;

			if ((type->drapeaux & TYPE_FUT_VALIDE) != 0) {
				unite.etat = unite.etat_original;
				unite.type_attendu = nullptr;
				gere_unite(unite);
			}
			else {
				//std::cerr << "-- le type ne fut pas validée\n";
				compilatrice.file_compilation.pousse(unite);
			}

			break;
		}
		case UniteCompilation::Etat::ATTEND_SUR_DECLARATION:
		{
			unite.cycle += 1;

//			std::cerr << "-- regère unité attendant sur déclaration : "
//					  << unite.declaration_attendue->lexeme->chaine
//					  << " (" << unite.declaration_attendue << ")\n";
//			std::cerr << "-- cycle : " << unite.cycle << '\n';
//			std::cerr << "-- état original : " << unite.etat_original << '\n';

			auto declaration = unite.declaration_attendue;

			if ((declaration->drapeaux & DECLARATION_FUT_VALIDEE) != 0 || unite.etat_original == UniteCompilation::Etat::TYPAGE_ENTETE_FONCTION_ATTENDU) {
				unite.etat = unite.etat_original;
				unite.declaration_attendue = nullptr;
				gere_unite(unite);
			}
			else {
				//std::cerr << "-- la déclaration ne fut pas validée\n";
				compilatrice.file_compilation.pousse(unite);
			}

			break;
		}
		case UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI:
		{
			unite.cycle += 1;
			unite.etat = unite.etat_original;
			//std::cerr << "-- regère unité attendant sur interface kuri\n";
			//std::cerr << "-- cycle : " << unite.cycle << '\n';
			gere_unite(unite);
			break;
		}
		case UniteCompilation::Etat::ATTEND_SUR_SYMBOLE:
		{
			unite.cycle += 1;
			unite.etat = unite.etat_original;
			//std::cerr << "-- regère unité attendant sur symbole\n";
			//std::cerr << "-- cycle : " << unite.cycle << '\n';
			gere_unite(unite);
			break;
		}
	}
}
