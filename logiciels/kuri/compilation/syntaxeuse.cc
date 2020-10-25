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

#include "syntaxeuse.hh"

#include <filesystem>

#include "biblinternes/outils/assert.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "arbre_syntaxique.hh"
#include "assembleuse_arbre.h"
#include "compilatrice.hh"
#include "modules.hh"
#include "outils_lexemes.hh"
#include "profilage.hh"
#include "typage.hh"

// Pour les bibliothèques externes ou les inclusions, détermine le chemin absolu selon le fichier courant, au cas où la bibliothèque serait dans le même dossier que le fichier
static auto trouve_chemin_si_dans_dossier(Module *module, dls::chaine const &chaine)
{
	/* vérifie si le chemin est relatif ou absolu */
	auto chemin = std::filesystem::path(chaine.c_str());

	if (!std::filesystem::exists(chemin)) {
		/* le chemin n'est pas absolu, détermine s'il est dans le même dossier */
		auto chemin_abs = module->chemin + chaine;

		chemin = std::filesystem::path(chemin_abs.c_str());

		if (std::filesystem::exists(chemin)) {
			return chemin_abs;
		}
	}

	return chaine;
}

static auto renseigne_fonction_interface(dls::outils::Synchrone<InterfaceKuri> &interface, NoeudDeclarationEnteteFonction *noeud)
{
#define INIT_MEMBRE(membre, nom) \
	if (noeud->ident == nom) { \
		interface->membre = noeud; \
		return; \
	}

	INIT_MEMBRE(decl_panique, ID::panique)
	INIT_MEMBRE(decl_panique_memoire, ID::panique_hors_memoire)
	INIT_MEMBRE(decl_panique_tableau, ID::panique_depassement_limites_tableau)
	INIT_MEMBRE(decl_panique_chaine, ID::panique_depassement_limites_chaine)
	INIT_MEMBRE(decl_panique_membre_union, ID::panique_membre_union)
	INIT_MEMBRE(decl_panique_erreur, ID::panique_erreur_non_geree)
	INIT_MEMBRE(decl_rappel_panique_defaut, ID::__rappel_panique_defaut)
	INIT_MEMBRE(decl_dls_vers_r32, ID::DLS_vers_r32)
	INIT_MEMBRE(decl_dls_vers_r64, ID::DLS_vers_r64)
	INIT_MEMBRE(decl_dls_depuis_r32, ID::DLS_depuis_r32)
	INIT_MEMBRE(decl_dls_depuis_r64, ID::DLS_depuis_r64)

#undef INIT_MEMBRE
}

static auto renseigne_type_interface(Typeuse &typeuse, IdentifiantCode *ident, Type *type)
{
#define INIT_TYPE(membre, nom) \
	if (ident == nom) { \
		typeuse.membre = type; \
		return; \
	}

	INIT_TYPE(type_info_type_, ID::InfoType)
	INIT_TYPE(type_info_type_enum, ID::InfoTypeEnum)
	INIT_TYPE(type_info_type_structure, ID::InfoTypeStructure)
	INIT_TYPE(type_info_type_union, ID::InfoTypeUnion)
	INIT_TYPE(type_info_type_membre_structure, ID::InfoTypeMembreStructure)
	INIT_TYPE(type_info_type_entier, ID::InfoTypeEntier)
	INIT_TYPE(type_info_type_tableau, ID::InfoTypeTableau)
	INIT_TYPE(type_info_type_pointeur, ID::InfoTypePointeur)
	INIT_TYPE(type_info_type_fonction, ID::InfoTypeFonction)
	INIT_TYPE(type_position_code_source, ID::PositionCodeSource)
	INIT_TYPE(type_info_fonction_trace_appel, ID::InfoFonctionTraceAppel)
	INIT_TYPE(type_trace_appel, ID::TraceAppel)
	INIT_TYPE(type_base_allocatrice, ID::BaseAllocatrice)
	INIT_TYPE(type_info_appel_trace_appel, ID::InfoAppelTraceAppel)
	INIT_TYPE(type_stockage_temporaire, ID::StockageTemporaire)

#undef INIT_TYPE
}

template <typename T, unsigned long N>
static auto copie_tablet_tableau(dls::tablet<T, N> const &src, kuri::tableau<T> &dst)
{
	dst.reserve(src.taille());

	POUR (src) {
		dst.pousse(it);
	}
}

static bool est_operateur_surchargeable(GenreLexeme genre)
{
	switch (genre) {
		default:
		{
			return false;
		}
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::EGALITE:
		case GenreLexeme::PLUS:
		case GenreLexeme::MOINS:
		case GenreLexeme::FOIS:
		case GenreLexeme::DIVISE:
		case GenreLexeme::DECALAGE_DROITE:
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::POURCENT:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::BARRE:
		case GenreLexeme::TILDE:
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::CHAPEAU:
		{
			return true;
		}
	}
}

static constexpr int PRECEDENCE_VIRGULE = 3;
static constexpr int PRECEDENCE_TYPE    = 4;

static int precedence_pour_operateur(GenreLexeme genre_operateur)
{
	switch (genre_operateur) {
		case GenreLexeme::TROIS_POINTS:
		{
			return 1;
		}
		case GenreLexeme::EGAL:
		case GenreLexeme::DECLARATION_VARIABLE:
		case GenreLexeme::DECLARATION_CONSTANTE:
		case GenreLexeme::PLUS_EGAL:
		case GenreLexeme::MOINS_EGAL:
		case GenreLexeme::DIVISE_EGAL:
		case GenreLexeme::MULTIPLIE_EGAL:
		case GenreLexeme::MODULO_EGAL:
		case GenreLexeme::ET_EGAL:
		case GenreLexeme::OU_EGAL:
		case GenreLexeme::OUX_EGAL:
		case GenreLexeme::DEC_DROITE_EGAL:
		case GenreLexeme::DEC_GAUCHE_EGAL:
		{
			return 2;
		}
		case GenreLexeme::VIRGULE:
		{
			return PRECEDENCE_VIRGULE;
		}
		case GenreLexeme::DOUBLE_POINTS:
		{
			return PRECEDENCE_TYPE;
		}
		case GenreLexeme::BARRE_BARRE:
		{
			return 5;
		}
		case GenreLexeme::ESP_ESP:
		{
			return 6;
		}
		case GenreLexeme::BARRE:
		{
			return 7;
		}
		case GenreLexeme::CHAPEAU:
		{
			return 8;
		}
		case GenreLexeme::ESPERLUETTE:
		{
			return 9;
		}
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::EGALITE:
		{
			return 10;
		}
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		{
			return 11;
		}
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DECALAGE_DROITE:
		{
			return 12;
		}
		case GenreLexeme::PLUS:
		case GenreLexeme::MOINS:
		{
			return 13;
		}
		case GenreLexeme::FOIS:
		case GenreLexeme::DIVISE:
		case GenreLexeme::POURCENT:
		{
			return 14;
		}
		case GenreLexeme::COMME:
		{
			return 15;
		}
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::TILDE:
		case GenreLexeme::PLUS_UNAIRE:
		case GenreLexeme::MOINS_UNAIRE:
		case GenreLexeme::FOIS_UNAIRE:
		case GenreLexeme::ESP_UNAIRE:
		case GenreLexeme::EXPANSION_VARIADIQUE:
		{
			return 16;
		}
		case GenreLexeme::PARENTHESE_OUVRANTE:
		case GenreLexeme::POINT:
		case GenreLexeme::CROCHET_OUVRANT:
		{
			return 17;
		}
		default:
		{
			assert_rappel(false, [&]() { std::cerr << "Aucune précédence pour l'opérateur : " << chaine_du_lexeme(genre_operateur) << '\n'; });
			return -1;
		}
	}
}

static Associativite associativite_pour_operateur(GenreLexeme genre_operateur)
{
	switch (genre_operateur) {
		case GenreLexeme::VIRGULE:
		case GenreLexeme::TROIS_POINTS:
		case GenreLexeme::EGAL:
		case GenreLexeme::DECLARATION_VARIABLE:
		case GenreLexeme::DECLARATION_CONSTANTE:
		case GenreLexeme::PLUS_EGAL:
		case GenreLexeme::MOINS_EGAL:
		case GenreLexeme::DIVISE_EGAL:
		case GenreLexeme::MULTIPLIE_EGAL:
		case GenreLexeme::MODULO_EGAL:
		case GenreLexeme::ET_EGAL:
		case GenreLexeme::OU_EGAL:
		case GenreLexeme::OUX_EGAL:
		case GenreLexeme::DEC_DROITE_EGAL:
		case GenreLexeme::DEC_GAUCHE_EGAL:
		case GenreLexeme::BARRE_BARRE:
		case GenreLexeme::ESP_ESP:
		case GenreLexeme::BARRE:
		case GenreLexeme::CHAPEAU:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::EGALITE:
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DECALAGE_DROITE:
		case GenreLexeme::PLUS:
		case GenreLexeme::MOINS:
		case GenreLexeme::FOIS:
		case GenreLexeme::DIVISE:
		case GenreLexeme::POURCENT:
		case GenreLexeme::POINT:
		case GenreLexeme::CROCHET_OUVRANT:
		case GenreLexeme::PARENTHESE_OUVRANTE:
		case GenreLexeme::COMME:
		case GenreLexeme::DOUBLE_POINTS:
		{
			return Associativite::GAUCHE;
		}
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::TILDE:
		case GenreLexeme::PLUS_UNAIRE:
		case GenreLexeme::FOIS_UNAIRE:
		case GenreLexeme::ESP_UNAIRE:
		case GenreLexeme::MOINS_UNAIRE:
		case GenreLexeme::EXPANSION_VARIADIQUE:
		{
			return Associativite::DROITE;
		}
		default:
		{
			assert_rappel(false, [&]() { std::cerr << "Aucune associativité pour l'opérateur : " << chaine_du_lexeme(genre_operateur) << '\n'; });
			return static_cast<Associativite>(-1);
		}
	}
}

#define CREE_NOEUD(Type, Genre, Lexeme) static_cast<Type *>(m_tacheronne.assembleuse->cree_noeud(Genre, Lexeme))

// sans transtypage pour éviter les erreurs de compilations avec drapeaux stricts
#define CREE_NOEUD_EXPRESSION(Genre, Lexeme) m_tacheronne.assembleuse->cree_noeud(Genre, Lexeme)

Syntaxeuse::Syntaxeuse(Tacheronne &tacheronne, UniteCompilation *unite)
	: m_compilatrice(tacheronne.compilatrice)
	, m_tacheronne(tacheronne)
	, m_fichier(unite->fichier)
	, m_unite(unite)
	, m_lexemes(m_fichier->lexemes)
{
	if (m_lexemes.taille() > 0) {
		m_lexeme_courant = &m_lexemes[m_position];
	}

	auto module = m_fichier->module;

	m_tacheronne.assembleuse->depile_tout();

	module->mutex.lock();
	{
		if (module->bloc == nullptr) {
			module->bloc = m_tacheronne.assembleuse->empile_bloc();
		}
		else {
			m_tacheronne.assembleuse->bloc_courant(module->bloc);
		}
	}
	module->mutex.unlock();
}

void Syntaxeuse::lance_analyse()
{
	Prof(Syntaxeuse_lance_analyse);

	m_position = 0;
	m_fichier->temps_analyse = 0.0;

	if (m_lexemes.taille() == 0) {
		return;
	}

	m_chrono_analyse.commence();

	if (m_fichier->metaprogramme_corps_texte) {
		auto recipiente = m_fichier->metaprogramme_corps_texte->recipiente_corps_texte;
		m_tacheronne.assembleuse->bloc_courant(recipiente->bloc_parametres);

		recipiente->corps->bloc = analyse_bloc(false);
		aplatis_arbre(recipiente->corps->bloc, recipiente->corps->arbre_aplatis, {});
		recipiente->corps->est_corps_texte = false;
		recipiente->est_metaprogramme = false;

		m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, recipiente->corps);

		m_fichier->temps_analyse += m_chrono_analyse.arrete();

		return;
	}

	while (!fini()) {
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			continue;
		}

		auto lexeme = lexeme_courant();
		auto genre_lexeme = lexeme->genre;

		if (genre_lexeme == GenreLexeme::IMPORTE) {
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::INSTRUCTION_IMPORTE, lexeme);
			noeud->bloc_parent->expressions->pousse(noeud);

			if (!apparie(GenreLexeme::CHAINE_LITTERALE) && !apparie(GenreLexeme::CHAINE_CARACTERE)) {
				lance_erreur("Attendu une chaine littérale après 'importe'");
			}

			noeud->expr = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme_courant());

			m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud);

			consomme();
		}
		else if (genre_lexeme == GenreLexeme::CHARGE) {
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::INSTRUCTION_CHARGE, lexeme);
			noeud->bloc_parent->expressions->pousse(noeud);

			if (!apparie(GenreLexeme::CHAINE_LITTERALE) && !apparie(GenreLexeme::CHAINE_CARACTERE)) {
				lance_erreur("Attendu une chaine littérale après 'charge'");
			}

			noeud->expr = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme_courant());

			m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud);

			consomme();
		}
		else if (apparie_expression()) {
			auto nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds();
			auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);
			nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds() - nombre_noeuds_alloues;

			// noeud peut-être nul si nous avons une directive
			if (noeud != nullptr) {
				if (noeud->est_declaration()) {
					noeud->drapeaux |= EST_GLOBALE;

					if (noeud->est_decl_var()) {
						auto decl_var = noeud->comme_decl_var();
						decl_var->arbre_aplatis.reserve(nombre_noeuds_alloues);
						aplatis_arbre(decl_var, decl_var->arbre_aplatis, DrapeauxNoeud::AUCUN);
						noeud->bloc_parent->membres->pousse(decl_var);
						m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud);
					}
				}

				noeud->bloc_parent->expressions->pousse(noeud);
			}
		}
		else {
			lance_erreur("attendu une expression ou une instruction");
		}
	}

	m_fichier->temps_analyse += m_chrono_analyse.arrete();

	m_tacheronne.assembleuse->depile_tout();
}

Lexeme Syntaxeuse::consomme()
{
	auto vieux_lexeme = m_lexemes[m_position];
	m_position += 1;

	if (!fini()) {
		m_lexeme_courant = &m_lexemes[m_position];
	}

	return vieux_lexeme;
}

Lexeme Syntaxeuse::consomme(GenreLexeme genre_lexeme, const char *message)
{
	if (m_lexemes[m_position].genre != genre_lexeme) {
		lance_erreur(message);
	}

	return consomme();
}

Lexeme *Syntaxeuse::lexeme_courant()
{
	return m_lexeme_courant;
}

const Lexeme *Syntaxeuse::lexeme_courant() const
{
	return m_lexeme_courant;
}

bool Syntaxeuse::fini() const
{
	return m_position >= m_lexemes.taille();
}

bool Syntaxeuse::apparie(GenreLexeme genre_lexeme) const
{
	return m_lexeme_courant->genre == genre_lexeme;
}

bool Syntaxeuse::apparie_expression() const
{
	auto genre = lexeme_courant()->genre;

	switch (genre) {
		case GenreLexeme::CARACTERE:
		case GenreLexeme::CHAINE_CARACTERE:
		case GenreLexeme::CHAINE_LITTERALE:
		case GenreLexeme::COROUT:
		case GenreLexeme::CROCHET_OUVRANT: // construit tableau
		case GenreLexeme::DIRECTIVE:
		case GenreLexeme::DELOGE:
		case GenreLexeme::DOLLAR:
		case GenreLexeme::EMPL:
		case GenreLexeme::ENUM:
		case GenreLexeme::ENUM_DRAPEAU:
		case GenreLexeme::ERREUR:
		case GenreLexeme::EXTERNE:
		case GenreLexeme::FAUX:
		case GenreLexeme::FONC:
		case GenreLexeme::INFO_DE:
		case GenreLexeme::INIT_DE:
		case GenreLexeme::LOGE:
		case GenreLexeme::MEMOIRE:
		case GenreLexeme::NOMBRE_ENTIER:
		case GenreLexeme::NOMBRE_REEL:
		case GenreLexeme::NON_INITIALISATION:
		case GenreLexeme::NUL:
		case GenreLexeme::OPERATEUR:
		case GenreLexeme::PARENTHESE_OUVRANTE: // expression entre parenthèse
		case GenreLexeme::RELOGE:
		case GenreLexeme::STRUCT:
		case GenreLexeme::TAILLE_DE:
		case GenreLexeme::TYPE_DE:
		case GenreLexeme::TENTE:
		case GenreLexeme::UNION:
		case GenreLexeme::VRAI:
		{
			return true;
		}
		default:
		{
			return apparie_expression_unaire() || est_identifiant_type(genre);
		}
	}
}

bool Syntaxeuse::apparie_expression_unaire() const
{
	auto genre = lexeme_courant()->genre;

	switch (genre) {
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::MOINS:
		case GenreLexeme::PLUS:
		case GenreLexeme::TILDE:
		case GenreLexeme::FOIS:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::TROIS_POINTS:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool Syntaxeuse::apparie_expression_secondaire() const
{
	auto genre = lexeme_courant()->genre;

	switch (genre) {
		case GenreLexeme::BARRE:
		case GenreLexeme::BARRE_BARRE:
		case GenreLexeme::CHAPEAU:
		case GenreLexeme::CROCHET_OUVRANT:
		case GenreLexeme::DECALAGE_DROITE:
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DECLARATION_CONSTANTE:
		case GenreLexeme::DECLARATION_VARIABLE:
		case GenreLexeme::DEC_DROITE_EGAL:
		case GenreLexeme::DEC_GAUCHE_EGAL:
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::DIVISE:
		case GenreLexeme::DIVISE_EGAL:
		case GenreLexeme::EGAL:
		case GenreLexeme::EGALITE:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::ESP_ESP:
		case GenreLexeme::ET_EGAL:
		case GenreLexeme::FOIS:
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::MODULO_EGAL:
		case GenreLexeme::MOINS:
		case GenreLexeme::MOINS_EGAL:
		case GenreLexeme::MULTIPLIE_EGAL:
		case GenreLexeme::OUX_EGAL:
		case GenreLexeme::OU_EGAL:
		case GenreLexeme::PARENTHESE_OUVRANTE:
		case GenreLexeme::PLUS:
		case GenreLexeme::PLUS_EGAL:
		case GenreLexeme::POINT:
		case GenreLexeme::POURCENT:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		case GenreLexeme::TROIS_POINTS:
		case GenreLexeme::VIRGULE:
		case GenreLexeme::COMME:
		case GenreLexeme::DOUBLE_POINTS:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool Syntaxeuse::apparie_instruction() const
{
	auto genre = lexeme_courant()->genre;

	switch (genre) {
		case GenreLexeme::ACCOLADE_OUVRANTE:
		case GenreLexeme::ARRETE:
		case GenreLexeme::BOUCLE:
		case GenreLexeme::CONTINUE:
		case GenreLexeme::DIFFERE:
		case GenreLexeme::DISCR:
		case GenreLexeme::NONSUR:
		case GenreLexeme::POUR:
		case GenreLexeme::POUSSE_CONTEXTE:
		case GenreLexeme::REPETE:
		case GenreLexeme::RETIENS:
		case GenreLexeme::RETOURNE:
		case GenreLexeme::SAUFSI:
		case GenreLexeme::SI:
		case GenreLexeme::TANTQUE:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

NoeudExpression *Syntaxeuse::analyse_expression(DonneesPrecedence const &donnees_precedence, GenreLexeme racine_expression, GenreLexeme lexeme_final)
{
	Prof(Syntaxeuse_analyse_expression);

	auto expression = analyse_expression_primaire(racine_expression, lexeme_final);

	while (apparie_expression_secondaire() && lexeme_courant()->genre != lexeme_final) {
		auto nouvelle_precedence = precedence_pour_operateur(lexeme_courant()->genre);

		if (nouvelle_precedence < donnees_precedence.precedence) {
			break;
		}

		if (nouvelle_precedence == donnees_precedence.precedence && donnees_precedence.associativite == Associativite::GAUCHE) {
			break;
		}

		auto nouvelle_associativite = associativite_pour_operateur(lexeme_courant()->genre);
		expression = analyse_expression_secondaire(expression, { nouvelle_precedence, nouvelle_associativite }, racine_expression, lexeme_final);
	}

	return expression;
}

NoeudExpression *Syntaxeuse::analyse_expression_unaire(GenreLexeme lexeme_final)
{
	Prof(Syntaxeuse_analyse_expression_unaire);

	auto lexeme = lexeme_courant();
	auto genre_noeud = GenreNoeud::OPERATEUR_UNAIRE;

	switch (lexeme->genre) {
		case GenreLexeme::MOINS:
		{
			lexeme->genre = GenreLexeme::MOINS_UNAIRE;
			break;
		}
		case GenreLexeme::PLUS:
		{
			lexeme->genre = GenreLexeme::PLUS_UNAIRE;
			break;
		}
		case GenreLexeme::TROIS_POINTS:
		{
			lexeme->genre = GenreLexeme::EXPANSION_VARIADIQUE;
			genre_noeud = GenreNoeud::EXPANSION_VARIADIQUE;
			break;
		}
		case GenreLexeme::FOIS:
		{
			lexeme->genre = GenreLexeme::FOIS_UNAIRE;
			break;
		}
		case GenreLexeme::ESPERLUETTE:
		{
			lexeme->genre = GenreLexeme::ESP_UNAIRE;
			break;
		}
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::TILDE:
		{
			break;
		}
		default:
		{
			assert_rappel(false, [&]() { std::cerr << "Lexème inattendu comme opérateur unaire : " << chaine_du_lexeme(lexeme->genre) << '\n'; });
			return nullptr;
		}
	}

	consomme();

	auto precedence = precedence_pour_operateur(lexeme->genre);
	auto associativite = associativite_pour_operateur(lexeme->genre);

	auto noeud = CREE_NOEUD(NoeudExpressionUnaire, genre_noeud, lexeme);

	// cette vérification n'est utile que pour les arguments variadiques sans type
	if (apparie_expression()) {
		noeud->expr = analyse_expression({ precedence, associativite }, GenreLexeme::INCONNU, lexeme_final);
	}

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_expression_primaire(GenreLexeme racine_expression, GenreLexeme lexeme_final)
{
	Prof(Syntaxeuse_analyse_expression_primaire);

	if (apparie_expression_unaire()) {
		return analyse_expression_unaire(lexeme_final);
	}

	auto lexeme = lexeme_courant();

	switch (lexeme->genre) {
		case GenreLexeme::CARACTERE:
		{
			consomme();
			return CREE_NOEUD_EXPRESSION(GenreNoeud::EXPRESSION_LITTERALE_CARACTERE, lexeme);
		}
		case GenreLexeme::CHAINE_CARACTERE:
		{
			consomme();
			return CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme);
		}
		case GenreLexeme::CHAINE_LITTERALE:
		{
			consomme();
			return CREE_NOEUD_EXPRESSION(GenreNoeud::EXPRESSION_LITTERALE_CHAINE, lexeme);
		}
		case GenreLexeme::CROCHET_OUVRANT:
		{
			consomme();
			lexeme->genre = GenreLexeme::TABLEAU;

			auto expression_entre_crochets = NoeudExpression::nul();
			if (apparie_expression()) {
				auto ancien_noeud_virgule = m_noeud_expression_virgule;
				m_noeud_expression_virgule = nullptr;
				expression_entre_crochets = analyse_expression({}, GenreLexeme::CROCHET_OUVRANT, GenreLexeme::INCONNU);
				m_noeud_expression_virgule = ancien_noeud_virgule;
			}

			// point-virgule implicite dans l'expression
			if (apparie(GenreLexeme::POINT_VIRGULE)) {
				consomme();
			}

			consomme(GenreLexeme::CROCHET_FERMANT, "Attendu un crochet fermant");

			if (apparie_expression()) {
				// nous avons l'expression d'un type
				auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::OPERATEUR_BINAIRE, lexeme);

				if (m_noeud_logement) {
					m_noeud_logement->expr_taille = expression_entre_crochets;
					// seule la première expression doit être considérer pour l'expression de la taille
					m_noeud_logement = nullptr;
				}
				else {
					noeud->expr1 = expression_entre_crochets;
				}

				noeud->expr2 = analyse_expression_primaire(GenreLexeme::INCONNU, GenreLexeme::INCONNU);

				return noeud;
			}

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU, lexeme);
			noeud->expr = expression_entre_crochets;

			return noeud;
		}
		case GenreLexeme::EMPL:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::INSTRUCTION_EMPL, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::EMPL, GenreLexeme::INCONNU);
			return noeud;
		}
		case GenreLexeme::EXTERNE:
		{
			consomme();

			auto expr = analyse_expression({}, GenreLexeme::EXTERNE, GenreLexeme::INCONNU);
			expr->drapeaux |= EST_EXTERNE;

			consomme();

			return expr;
		}
		case GenreLexeme::FAUX:
		case GenreLexeme::VRAI:
		{
			lexeme->genre = GenreLexeme::BOOL;
			consomme();
			return CREE_NOEUD_EXPRESSION(GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN, lexeme);
		}
		case GenreLexeme::INFO_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'info_de'");

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_INFO_DE, lexeme);
			noeud->expr = analyse_expression_primaire(GenreLexeme::INFO_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression de 'taille_de'");

			return noeud;
		}
		case GenreLexeme::INIT_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'init_de'");

			auto noeud = CREE_NOEUD_EXPRESSION(GenreNoeud::EXPRESSION_INIT_DE, lexeme);
			noeud->expression_type = analyse_expression_primaire(GenreLexeme::INIT_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression de 'init_de'");

			return noeud;
		}
		case GenreLexeme::MEMOIRE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'mémoire'");

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_MEMOIRE, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::MEMOIRE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression");

			return noeud;
		}
		case GenreLexeme::NOMBRE_ENTIER:
		{
			consomme();
			return CREE_NOEUD_EXPRESSION(GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER, lexeme);
		}
		case GenreLexeme::NOMBRE_REEL:
		{
			consomme();
			return CREE_NOEUD_EXPRESSION(GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL, lexeme);
		}
		case GenreLexeme::NON_INITIALISATION:
		{
			consomme();
			return CREE_NOEUD_EXPRESSION(GenreNoeud::INSTRUCTION_NON_INITIALISATION, lexeme);
		}
		case GenreLexeme::NUL:
		{
			consomme();
			return CREE_NOEUD_EXPRESSION(GenreNoeud::EXPRESSION_LITTERALE_NUL, lexeme);
		}
		case GenreLexeme::OPERATEUR:
		{
			return analyse_declaration_operateur();
		}
		case GenreLexeme::PARENTHESE_OUVRANTE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_PARENTHESE, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::PARENTHESE_OUVRANTE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "attenud une parenthèse fermante");

			return noeud;
		}
		case GenreLexeme::TAILLE_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'taille_de'");

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_TAILLE_DE, lexeme);
			noeud->expr = analyse_expression_primaire(GenreLexeme::TAILLE_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'taille_de'");

			return noeud;
		}
		case GenreLexeme::TYPE_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'type_de'");

			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::EXPRESSION_TYPE_DE, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::TYPE_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'type_de'");

			return noeud;
		}
		case GenreLexeme::LOGE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionLogement, GenreNoeud::EXPRESSION_LOGE, lexeme);
			m_noeud_logement = noeud;
			noeud->expression_type = analyse_expression_primaire(GenreLexeme::LOGE, GenreLexeme::INCONNU);
			m_noeud_logement = nullptr;

			if (noeud->expression_type->lexeme->genre == GenreLexeme::CHAINE) {
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une paranthèse ouvrante '(");
				noeud->expr_taille = analyse_expression({}, GenreLexeme::LOGE, GenreLexeme::INCONNU);
				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu une paranthèse fermante ')");
			}

			if (apparie(GenreLexeme::SINON)) {
				consomme();
				noeud->bloc = analyse_bloc();
			}

			return noeud;
		}
		case GenreLexeme::DELOGE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionLogement, GenreNoeud::EXPRESSION_DELOGE, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::DELOGE, GenreLexeme::INCONNU);
			return noeud;
		}
		case GenreLexeme::RELOGE:
		{
			consomme();

			/* reloge nom : type; */
			auto noeud = CREE_NOEUD(NoeudExpressionLogement, GenreNoeud::EXPRESSION_RELOGE, lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::RELOGE, GenreLexeme::DOUBLE_POINTS);

			consomme(GenreLexeme::DOUBLE_POINTS, "Attendu un double-points ':' après l'expression à reloger");

			m_noeud_logement = noeud;
			noeud->expression_type = analyse_expression_primaire(GenreLexeme::RELOGE, GenreLexeme::INCONNU);
			m_noeud_logement = nullptr;

			if (noeud->expression_type->lexeme->genre == GenreLexeme::CHAINE) {
				consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une paranthèse ouvrante '(");
				noeud->expr_taille = analyse_expression({}, GenreLexeme::RELOGE, GenreLexeme::INCONNU);
				consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu une paranthèse fermante ')");
			}

			if (apparie(GenreLexeme::SINON)) {
				consomme();
				noeud->bloc = analyse_bloc();
			}

			return noeud;
		}
		case GenreLexeme::TENTE:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudTente, GenreNoeud::INSTRUCTION_TENTE, lexeme);
			noeud->expr_appel = analyse_expression({}, GenreLexeme::TENTE, GenreLexeme::INCONNU);

			if (apparie(GenreLexeme::PIEGE)) {
				consomme();

				if (apparie(GenreLexeme::NONATTEIGNABLE)) {
					consomme();
				}
				else {
					noeud->expr_piege = analyse_expression({}, GenreLexeme::PIEGE, GenreLexeme::INCONNU);
					noeud->bloc = analyse_bloc();
				}
			}

			return noeud;
		}
		case GenreLexeme::DIRECTIVE:
		{
			consomme();

			lexeme = lexeme_courant();
			auto directive = lexeme->ident;

			consomme();

			if (directive == ID::bibliotheque_dynamique) {
				auto chaine_bib = lexeme_courant()->chaine;
				consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

				auto chaine = trouve_chemin_si_dans_dossier(m_fichier->module, chaine_bib);
				m_compilatrice.bibliotheques_dynamiques->pousse(chaine);
			}
			else if (directive == ID::bibliotheque_statique) {
				auto chaine_bib = lexeme_courant()->chaine;
				consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

				auto chaine = trouve_chemin_si_dans_dossier(m_fichier->module, chaine_bib);
				m_compilatrice.bibliotheques_statiques->pousse(chaine);
			}
			else if (directive == ID::def) {
				auto chaine = lexeme_courant()->chaine;
				consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

				m_compilatrice.definitions->pousse(chaine);
			}
			else if (directive == ID::execute || directive == ID::assert_ || directive == ID::test) {
				auto noeud = CREE_NOEUD(NoeudDirectiveExecution, GenreNoeud::DIRECTIVE_EXECUTION, lexeme);
				noeud->ident = directive;

				if (directive == ID::test) {
					noeud->expr = analyse_bloc();
				}
				else {
					noeud->expr = analyse_expression({}, GenreLexeme::DIRECTIVE, GenreLexeme::INCONNU);
				}

				aplatis_arbre(noeud, noeud->arbre_aplatis, DrapeauxNoeud::AUCUN);

				if (!est_dans_fonction && (directive != ID::test || m_compilatrice.active_tests)) {
					m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud);
				}

				return noeud;
			}
			else if (directive == ID::chemin) {
				auto chaine = lexeme_courant()->chaine;
				consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

				m_compilatrice.chemins->pousse(chaine);
			}
			else if (directive == ID::nulctx) {
				lexeme = lexeme_courant();
				auto noeud_fonc = analyse_declaration_fonction(lexeme);
				noeud_fonc->drapeaux |= FORCE_NULCTX;
				return noeud_fonc;
			}
			else if (directive == ID::si) {
				return analyse_instruction_si_statique(lexeme);
			}
			else {
				lance_erreur("Directive inconnue");
			}

			return nullptr;
		}
		case GenreLexeme::DOLLAR:
		{
			consomme();
			lexeme = lexeme_courant();

			consomme(GenreLexeme::CHAINE_CARACTERE, "attendu une chaine de caractère après '$'");

			auto noeud = CREE_NOEUD_EXPRESSION(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme);
			noeud->drapeaux |= DECLARATION_TYPE_POLYMORPHIQUE;
			return noeud;
		}
		case GenreLexeme::FONC:
		{
			return analyse_declaration_fonction(lexeme);
		}
		/* Ceux-ci doivent déjà avoir été gérés. */
		case GenreLexeme::COROUT:
		case GenreLexeme::ENUM:
		case GenreLexeme::ENUM_DRAPEAU:
		case GenreLexeme::ERREUR:
		case GenreLexeme::STRUCT:
		case GenreLexeme::UNION:
		{
			assert_rappel(false, [&]() { std::cerr << "Lexème inattendu, supposément déjà géré, comme expression primaire : " << chaine_du_lexeme(lexeme->genre) << '\n'; });
			return nullptr;
		}
		default:
		{
			if (est_identifiant_type(lexeme->genre)) {
				consomme();
				return CREE_NOEUD_EXPRESSION(GenreNoeud::EXPRESSION_REFERENCE_TYPE, lexeme);
			}

			lance_erreur("attendu une expression primaire");
		}
	}
}

NoeudExpression *Syntaxeuse::analyse_expression_secondaire(NoeudExpression *gauche, const DonneesPrecedence &donnees_precedence, GenreLexeme racine_expression, GenreLexeme lexeme_final)
{
	Prof(Syntaxeuse_analyse_expression_secondaire);

	auto lexeme = lexeme_courant();

	switch (lexeme->genre) {
		case GenreLexeme::BARRE:
		case GenreLexeme::BARRE_BARRE:
		case GenreLexeme::CHAPEAU:
		case GenreLexeme::DECALAGE_DROITE:
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DEC_DROITE_EGAL:
		case GenreLexeme::DEC_GAUCHE_EGAL:
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::DIVISE:
		case GenreLexeme::DIVISE_EGAL:
		case GenreLexeme::EGALITE:
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::ESP_ESP:
		case GenreLexeme::ET_EGAL:
		case GenreLexeme::FOIS:
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::MODULO_EGAL:
		case GenreLexeme::MOINS:
		case GenreLexeme::MOINS_EGAL:
		case GenreLexeme::MULTIPLIE_EGAL:
		case GenreLexeme::OUX_EGAL:
		case GenreLexeme::OU_EGAL:
		case GenreLexeme::PLUS:
		case GenreLexeme::PLUS_EGAL:
		case GenreLexeme::POURCENT:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::OPERATEUR_BINAIRE, lexeme);
			noeud->expr1 = gauche;
			noeud->expr2 = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			return noeud;
		}
		case GenreLexeme::VIRGULE:
		{
			consomme();

			auto noeud_expression_virgule = m_noeud_expression_virgule;

			if (!m_noeud_expression_virgule) {
				m_noeud_expression_virgule = CREE_NOEUD(NoeudExpressionVirgule, GenreNoeud::EXPRESSION_VIRGULE, lexeme);
				m_noeud_expression_virgule->expressions.pousse(gauche);
				noeud_expression_virgule = m_noeud_expression_virgule;
			}

			auto droite = analyse_expression(donnees_precedence, racine_expression, lexeme_final);

			m_noeud_expression_virgule = noeud_expression_virgule;
			m_noeud_expression_virgule->expressions.pousse(droite);

			return m_noeud_expression_virgule;
		}
		case GenreLexeme::CROCHET_OUVRANT:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::EXPRESSION_INDEXAGE, lexeme);
			noeud->expr1 = gauche;
			noeud->expr2 = analyse_expression({}, GenreLexeme::CROCHET_OUVRANT, GenreLexeme::INCONNU);

			consomme(GenreLexeme::CROCHET_FERMANT, "attendu un crochet fermant");

			return noeud;
		}
		case GenreLexeme::DECLARATION_CONSTANTE:
		{
			consomme();

			switch (lexeme_courant()->genre) {
				default:
				{
					break;
				}
				case GenreLexeme::COROUT:
				case GenreLexeme::FONC:
				{
					auto noeud_fonction = analyse_declaration_fonction(gauche->lexeme);

					if (noeud_fonction->est_declaration_type) {
						auto noeud = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, lexeme);
						noeud->ident = gauche->ident;
						noeud->valeur = gauche;
						noeud->expression = noeud_fonction;
						noeud->drapeaux |= EST_CONSTANTE;
						gauche->drapeaux |= EST_CONSTANTE;

						return noeud;
					}

					return noeud_fonction;
				}
				case GenreLexeme::STRUCT:
				case GenreLexeme::UNION:
				{
					return analyse_declaration_structure(gauche);
				}
				case GenreLexeme::ENUM:
				case GenreLexeme::ENUM_DRAPEAU:
				case GenreLexeme::ERREUR:
				{
					return analyse_declaration_enum(gauche);
				}
			}

			auto noeud = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, lexeme);
			noeud->ident = gauche->ident;
			noeud->valeur = gauche;
			noeud->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			noeud->drapeaux |= EST_CONSTANTE;
			gauche->drapeaux |= EST_CONSTANTE;

			return noeud;
		}
		case GenreLexeme::DOUBLE_POINTS:
		{
			consomme();

			if (gauche->est_ref_decl()) {
				// nous avons la déclaration d'un type (a: z32)
				auto decl = cree_declaration_pour_ref(gauche->comme_ref_decl());
				decl->expression_type = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
				return decl;
			}

			if (gauche->est_decl_var()) {
				// nous avons la déclaration d'une constante (a: z32 : 12)
				auto decl = gauche->comme_decl_var();
				decl->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
				decl->drapeaux |= EST_CONSTANTE;
				return decl;
			}

			rapporte_erreur(m_unite->espace, gauche, "Expression inattendu à gauche du double-point");
			return nullptr;
		}
		case GenreLexeme::DECLARATION_VARIABLE:
		{
			if (gauche->est_decl_var()) {
				lance_erreur("Utilisation de « := » alors qu'un type fut déclaré avec « : »");
			}

			consomme();

			m_noeud_expression_virgule = nullptr;

			auto noeud = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, lexeme);
			noeud->ident = gauche->ident;
			noeud->valeur = gauche;
			noeud->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);

			m_noeud_expression_virgule = nullptr;

			return noeud;
		}
		case GenreLexeme::EGAL:
		{
			consomme();

			m_noeud_expression_virgule = nullptr;

			if (gauche->est_decl_var()) {
				auto decl = gauche->comme_decl_var();
				decl->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);

				m_noeud_expression_virgule = nullptr;

				return decl;
			}

			auto noeud = CREE_NOEUD(NoeudAssignation, GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE, lexeme);
			noeud->variable = gauche;
			noeud->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);

			m_noeud_expression_virgule = nullptr;

			return noeud;
		}
		case GenreLexeme::POINT:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionMembre, GenreNoeud::EXPRESSION_REFERENCE_MEMBRE, lexeme);
			noeud->accede = gauche;
			noeud->membre = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			return noeud;
		}
		case GenreLexeme::TROIS_POINTS:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudExpressionBinaire, GenreNoeud::EXPRESSION_PLAGE, lexeme);
			noeud->expr1 = gauche;
			noeud->expr2 = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			return noeud;
		}
		case GenreLexeme::PARENTHESE_OUVRANTE:
		{
			return analyse_appel_fonction(gauche);
		}
		case GenreLexeme::COMME:
		{
			consomme();

			auto noeud = CREE_NOEUD(NoeudComme, GenreNoeud::EXPRESSION_COMME, lexeme);
			noeud->expression = gauche;
			noeud->expression_type = analyse_expression_primaire(GenreLexeme::COMME, GenreLexeme::INCONNU);
			return noeud;
		}
		default:
		{
			assert_rappel(false, [&]() { std::cerr << "Lexème inattendu comme expression secondaire : " << chaine_du_lexeme(lexeme->genre) << '\n'; });
			return nullptr;
		}
	}
}

NoeudExpression *Syntaxeuse::analyse_instruction()
{
	Prof(Syntaxeuse_analyse_instruction);

	auto lexeme = lexeme_courant();

	switch (lexeme->genre) {
		case GenreLexeme::ACCOLADE_OUVRANTE:
		{
			return analyse_bloc();
		}
		case GenreLexeme::DIFFERE:
		{
			consomme();
			auto bloc = analyse_bloc();
			bloc->est_differe = true;
			return bloc;
		}
		case GenreLexeme::NONSUR:
		{
			consomme();
			auto bloc = analyse_bloc();
			bloc->est_nonsur = true;
			return bloc;
		}
		case GenreLexeme::ARRETE:
		case GenreLexeme::CONTINUE:
		{
			auto noeud = CREE_NOEUD(NoeudExpressionUnaire, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE, lexeme);
			consomme();

			if (apparie(GenreLexeme::CHAINE_CARACTERE)) {
				noeud->expr = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme_courant());
				consomme();
			}

			return noeud;
		}
		case GenreLexeme::RETIENS:
		{
			auto noeud = CREE_NOEUD(NoeudRetour, GenreNoeud::INSTRUCTION_RETIENS, lexeme);
			consomme();

			if (apparie_expression()) {
				dls::tablet<NoeudExpression *, 6> expressions;
				Lexeme *lexeme_virgule = nullptr;

				while (true) {
					auto expr = analyse_expression({}, GenreLexeme::RETIENS, GenreLexeme::VIRGULE);
					expressions.pousse(expr);

					if (!apparie(GenreLexeme::VIRGULE)) {
						break;
					}

					if (lexeme_virgule == nullptr) {
						lexeme_virgule = lexeme_courant();
					}

					consomme();
				}

				if (expressions.taille() == 1) {
					noeud->expr = expressions[0];
				}
				else {
					auto virgule = CREE_NOEUD(NoeudExpressionVirgule, GenreNoeud::EXPRESSION_VIRGULE, lexeme_virgule);
					copie_tablet_tableau(expressions, virgule->expressions);
					noeud->expr = virgule;
				}
			}

			m_noeud_expression_virgule = nullptr;

			return noeud;
		}
		case GenreLexeme::RETOURNE:
		{
			auto noeud = CREE_NOEUD(NoeudRetour, GenreNoeud::INSTRUCTION_RETOUR, lexeme);
			consomme();

			if (apparie_expression()) {
				dls::tablet<NoeudExpression *, 6> expressions;
				Lexeme *lexeme_virgule = nullptr;

				while (true) {
					auto expr = analyse_expression({}, GenreLexeme::RETOURNE, GenreLexeme::VIRGULE);
					expressions.pousse(expr);

					if (!apparie(GenreLexeme::VIRGULE)) {
						break;
					}

					if (lexeme_virgule == nullptr) {
						lexeme_virgule = lexeme_courant();
					}

					consomme();
				}

				if (expressions.taille() == 1) {
					noeud->expr = expressions[0];
				}
				else {
					auto virgule = CREE_NOEUD(NoeudExpressionVirgule, GenreNoeud::EXPRESSION_VIRGULE, lexeme_virgule);
					copie_tablet_tableau(expressions, virgule->expressions);
					noeud->expr = virgule;
				}
			}

			m_noeud_expression_virgule = nullptr;

			return noeud;
		}
		case GenreLexeme::BOUCLE:
		{
			return analyse_instruction_boucle();
		}
		case GenreLexeme::DISCR:
		{
			return analyse_instruction_discr();
		}
		case GenreLexeme::POUR:
		{
			return analyse_instruction_pour();
		}
		case GenreLexeme::POUSSE_CONTEXTE:
		{
			return analyse_instruction_pousse_contexte();
		}
		case GenreLexeme::REPETE:
		{
			return analyse_instruction_repete();
		}
		case GenreLexeme::SAUFSI:
		{
			return analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
		}
		case GenreLexeme::SI:
		{
			return analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
		}
		case GenreLexeme::TANTQUE:
		{
			return analyse_instruction_tantque();
		}
		default:
		{
			assert_rappel(false, [&]() { std::cerr << "Lexème inattendu comme instruction : " << chaine_du_lexeme(lexeme->genre) << '\n'; });
			return nullptr;
		}
	}
}

NoeudBloc *Syntaxeuse::analyse_bloc(bool accolade_requise)
{
	Prof(Syntaxeuse_analyse_bloc);

	empile_etat("dans l'analyse du bloc", lexeme_courant());

	if (accolade_requise) {
		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{'");
	}

	auto bloc = m_tacheronne.assembleuse->empile_bloc();
	auto expressions = dls::tablet<NoeudExpression *, 32>();

	while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			continue;
		}

		if (apparie_instruction()) {
			auto noeud = analyse_instruction();
			expressions.pousse(noeud);
		}
		else if (apparie_expression()) {
			auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);
			expressions.pousse(noeud);
		}
		else {
			lance_erreur("attendu une expression ou une instruction");
		}
	}

	copie_tablet_tableau(expressions, *bloc->expressions.verrou_ecriture());
	m_tacheronne.assembleuse->depile_bloc();

	if (accolade_requise) {
		consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}'");
	}

	depile_etat();

	return bloc;
}

NoeudExpression *Syntaxeuse::analyse_appel_fonction(NoeudExpression *gauche)
{
	Prof(Syntaxeuse_analyse_appel_fonction);

	auto noeud = CREE_NOEUD(NoeudExpressionAppel, GenreNoeud::EXPRESSION_APPEL_FONCTION, lexeme_courant());
	noeud->appelee = gauche;

	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "attendu une parenthèse ouvrante");

	auto params = dls::tablet<NoeudExpression *, 16>();

	while (apparie_expression()) {
		auto expr = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);
		params.pousse(expr);

		/* point-virgule implicite */
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			continue;
		}

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	copie_tablet_tableau(params, noeud->params);

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "attenu ')' à la fin des argument de l'appel");

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_boucle()
{
	Prof(Syntaxeuse_analyse_instruction_boucle);

	auto noeud = CREE_NOEUD(NoeudBoucle, GenreNoeud::INSTRUCTION_BOUCLE, lexeme_courant());
	consomme();
	noeud->bloc = analyse_bloc();
	noeud->bloc->appartiens_a_boucle = noeud;
	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_discr()
{
	Prof(Syntaxeuse_analyse_instruction_discr);

	auto noeud_discr = CREE_NOEUD(NoeudDiscr, GenreNoeud::INSTRUCTION_DISCR, lexeme_courant());
	consomme();

	noeud_discr->expr = analyse_expression({}, GenreLexeme::DISCR, GenreLexeme::INCONNU);

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{' après l'expression de « discr »");

	auto sinon_rencontre = false;

	auto paires_discr = dls::tablet<std::pair<NoeudExpression *, NoeudBloc *>, 32>();

	while (!apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
		if (apparie(GenreLexeme::SINON)) {
			consomme();

			if (sinon_rencontre) {
				lance_erreur("Redéfinition d'un bloc sinon");
			}

			sinon_rencontre = true;

			noeud_discr->bloc_sinon = analyse_bloc();
		}
		else {
			auto expr = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);
			auto bloc = analyse_bloc();

			if (!expr->est_virgule()) {
				auto noeud_virgule = CREE_NOEUD(NoeudExpressionVirgule, GenreNoeud::EXPRESSION_VIRGULE, expr->lexeme);
				noeud_virgule->expressions.pousse(expr);

				expr = noeud_virgule;
			}
			else {
				m_noeud_expression_virgule = nullptr;
			}

			paires_discr.pousse({ expr, bloc });
		}
	}

	copie_tablet_tableau(paires_discr, noeud_discr->paires_discr);

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}' à la fin du bloc de « discr »");

	return noeud_discr;
}

NoeudDeclarationVariable *Syntaxeuse::cree_declaration_pour_ref(NoeudExpressionReference *ref)
{
	auto decl = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, ref->lexeme);
	decl->valeur = ref;
	decl->ident = ref->ident;
	return decl;
}

NoeudDeclarationVariable *Syntaxeuse::cree_declaration(Lexeme *lexeme)
{
	auto ref  = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme);
	return cree_declaration_pour_ref(ref);
}

NoeudExpression *Syntaxeuse::analyse_instruction_pour()
{
	Prof(Syntaxeuse_analyse_instruction_pour);

	auto noeud = CREE_NOEUD(NoeudPour, GenreNoeud::INSTRUCTION_POUR, lexeme_courant());
	consomme();

	auto expression = analyse_expression({}, GenreLexeme::POUR, GenreLexeme::INCONNU);

	if (apparie(GenreLexeme::DANS)) {
		consomme();

		if (!expression->est_virgule()) {
			static Lexeme lexeme_virgule = { ",", {}, GenreLexeme::VIRGULE, 0, 0, 0 };
			auto noeud_virgule = CREE_NOEUD(NoeudExpressionVirgule, GenreNoeud::EXPRESSION_VIRGULE, &lexeme_virgule);
			noeud_virgule->expressions.pousse(expression);

			expression = noeud_virgule;
		}
		else {
			m_noeud_expression_virgule = nullptr;
		}

		auto expression_virgule = expression->comme_virgule();

		for (auto &expr : expression_virgule->expressions) {
			expr = cree_declaration_pour_ref(expr->comme_ref_decl());
		}

		noeud->variable = expression;
		noeud->expression = analyse_expression({}, GenreLexeme::DANS, GenreLexeme::INCONNU);
	}
	else {
		static Lexeme lexeme_it = { "it", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0 };
		lexeme_it.ident = ID::it;
		auto noeud_it = cree_declaration(&lexeme_it);

		static Lexeme lexeme_index = { "index_it", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0 };
		lexeme_index.ident = ID::index_it;
		auto noeud_index = cree_declaration(&lexeme_index);

		static Lexeme lexeme_virgule = { ",", {}, GenreLexeme::VIRGULE, 0, 0, 0 };
		auto noeud_virgule = CREE_NOEUD(NoeudExpressionVirgule, GenreNoeud::EXPRESSION_VIRGULE, &lexeme_virgule);
		noeud_virgule->expressions.pousse(noeud_it);
		noeud_virgule->expressions.pousse(noeud_index);

		noeud->variable = noeud_virgule;
		noeud->expression = expression;
	}

	noeud->bloc = analyse_bloc();	
	noeud->bloc->appartiens_a_boucle = noeud;

	if (apparie(GenreLexeme::SANSARRET)) {
		consomme();
		noeud->bloc_sansarret = analyse_bloc();
	}

	if (apparie(GenreLexeme::SINON)) {
		consomme();
		noeud->bloc_sinon = analyse_bloc();
	}

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_pousse_contexte()
{
	Prof(Syntaxeuse_analyse_instruction_pousse_contexte);

	auto noeud = CREE_NOEUD(NoeudPousseContexte, GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE, lexeme_courant());
	consomme();

	noeud->expr = analyse_expression({}, GenreLexeme::POUSSE_CONTEXTE, GenreLexeme::INCONNU);
	noeud->bloc = analyse_bloc();

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_repete()
{
	Prof(Syntaxeuse_analyse_instruction_repete);

	auto noeud = CREE_NOEUD(NoeudBoucle, GenreNoeud::INSTRUCTION_REPETE, lexeme_courant());
	consomme();

	noeud->bloc = analyse_bloc();
	noeud->bloc->appartiens_a_boucle = noeud;

	consomme(GenreLexeme::TANTQUE, "Attendu une 'tantque' après le bloc de 'répète'");

	noeud->condition = analyse_expression({}, GenreLexeme::REPETE, GenreLexeme::INCONNU);

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_si(GenreNoeud genre_noeud)
{
	Prof(Syntaxeuse_analyse_instruction_si);

	empile_etat("dans l'analyse de l'instruction si", lexeme_courant());

	auto noeud = CREE_NOEUD(NoeudSi, genre_noeud, lexeme_courant());
	consomme();

	noeud->condition = analyse_expression({}, GenreLexeme::SI, GenreLexeme::INCONNU);

	noeud->bloc_si_vrai = analyse_bloc();

	if (apparie(GenreLexeme::SINON)) {
		consomme();

		/* Si le 'sinon' contient un « si » ou un « saufsi », nous ajoutons un
		 * bloc pour créer un niveau d'indirection. Car dans le cas où nous
		 * avons un contrôle du type si/sinon si dans une boucle, la génération
		 * de blocs LLVM dans l'arbre syntaxic devient plus compliquée sans
		 * cette indirection : certaines instructions de branchage ne sont pas
		 * ajoutées alors qu'elles devraient l'être et la logique pour
		 * correctement traiter ce cas sans l'indirection semble être complexe.
		 * LLVM devrait pouvoir effacer cette indirection en enlevant les
		 * branchements redondants.
		 */

		if (apparie(GenreLexeme::SI)) {
			noeud->bloc_si_faux = m_tacheronne.assembleuse->empile_bloc();

			auto noeud_si = analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
			noeud->bloc_si_faux->expressions->pousse(noeud_si);

			m_tacheronne.assembleuse->depile_bloc();
		}
		else if (apparie(GenreLexeme::SAUFSI)) {
			noeud->bloc_si_faux = m_tacheronne.assembleuse->empile_bloc();

			auto noeud_saufsi = analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
			noeud->bloc_si_faux->expressions->pousse(noeud_saufsi);

			m_tacheronne.assembleuse->depile_bloc();
		}
		else {
			noeud->bloc_si_faux = analyse_bloc();
		}
	}

	depile_etat();

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_si_statique(Lexeme *lexeme)
{
	Prof(Syntaxeuse_analyse_instruction_si_statique);

	empile_etat("dans l'analyse de l'instruction #si", lexeme);

	auto noeud = CREE_NOEUD(NoeudSiStatique, GenreNoeud::INSTRUCTION_SI_STATIQUE, lexeme);

	noeud->condition = analyse_expression({}, GenreLexeme::SI, GenreLexeme::INCONNU);

	noeud->bloc_si_vrai = analyse_bloc();

	if (apparie(GenreLexeme::SINON)) {
		consomme();
		noeud->bloc_si_faux = analyse_bloc();
	}

	depile_etat();

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_tantque()
{
	Prof(Syntaxeuse_analyse_instruction_tantque);

	auto noeud = CREE_NOEUD(NoeudBoucle, GenreNoeud::INSTRUCTION_TANTQUE, lexeme_courant());
	consomme();

	noeud->condition = analyse_expression({}, GenreLexeme::TANTQUE, GenreLexeme::INCONNU);
	noeud->bloc = analyse_bloc();
	noeud->bloc->appartiens_a_boucle = noeud;

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_declaration_enum(NoeudExpression *gauche)
{
	Prof(Syntaxeuse_analyse_declaration_enum);

	auto lexeme = lexeme_courant();
	empile_etat("dans le syntaxage de l'énumération", lexeme);
	consomme();

	auto noeud_decl = CREE_NOEUD(NoeudEnum, GenreNoeud::DECLARATION_ENUM, gauche->lexeme);
	noeud_decl->bloc_parent->membres->pousse(noeud_decl);

	if (lexeme->genre != GenreLexeme::ERREUR) {
		if (!apparie(GenreLexeme::ACCOLADE_OUVRANTE)) {
			noeud_decl->expression_type = analyse_expression_primaire(GenreLexeme::ENUM, GenreLexeme::INCONNU);
		}

		auto type = m_unite->espace->typeuse.reserve_type_enum(noeud_decl);
		type->est_drapeau = lexeme->genre == GenreLexeme::ENUM_DRAPEAU;
		noeud_decl->type = type;
	}
	else {
		auto type = m_unite->espace->typeuse.reserve_type_erreur(noeud_decl);
		type->est_erreur = true;
		noeud_decl->type = type;
	}

	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après 'énum'");

	auto bloc = m_tacheronne.assembleuse->empile_bloc();

	auto expressions = dls::tablet<NoeudExpression *, 16>();

	while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			continue;
		}

		if (apparie_expression()) {
			auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

			if (noeud->est_ref_decl()) {
				expressions.pousse(cree_declaration_pour_ref(noeud->comme_ref_decl()));
			}
			else {
				expressions.pousse(noeud);
			}
		}
	}

	copie_tablet_tableau(expressions, *bloc->expressions.verrou_ecriture());

	m_tacheronne.assembleuse->depile_bloc();
	noeud_decl->bloc = bloc;

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de l'énum");

	depile_etat();

	return noeud_decl;
}

NoeudDeclarationEnteteFonction *Syntaxeuse::analyse_declaration_fonction(Lexeme const *lexeme)
{
	Prof(Syntaxeuse_analyse_declaration_fonction);

	auto lexeme_mot_cle = lexeme_courant();

	empile_etat("dans le syntaxage de la fonction", lexeme_mot_cle);
	consomme();

	auto noeud = CREE_NOEUD(NoeudDeclarationEnteteFonction, GenreNoeud::DECLARATION_ENTETE_FONCTION, lexeme);
	noeud->est_coroutine = lexeme_mot_cle->genre == GenreLexeme::COROUT;

	// @concurrence critique, si nous avons plusieurs définitions
	if (noeud->ident == ID::principale) {
		m_unite->espace->fonction_principale = noeud;
	}

	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après le nom de la fonction");

	noeud->bloc_constantes = m_tacheronne.assembleuse->empile_bloc();
	noeud->bloc_parametres = m_tacheronne.assembleuse->empile_bloc();

	/* analyse les paramètres de la fonction */
	auto params = dls::tablet<NoeudDeclaration *, 16>();
	auto nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds();

	auto eu_declarations = false;

	while (!apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
		auto param = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::VIRGULE);

		if (param->est_decl_var()) {
			auto decl_var = static_cast<NoeudDeclaration *>(param);
			decl_var->drapeaux |= EST_PARAMETRE;
			params.pousse(decl_var);
			eu_declarations = true;
		}
		else {
			// XXX - hack
			params.pousse(static_cast<NoeudDeclaration *>(param));
		}

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds() - nombre_noeuds_alloues;
	noeud->arbre_aplatis.reserve(nombre_noeuds_alloues);

	copie_tablet_tableau(params, noeud->params);

	POUR (noeud->params) {
		aplatis_arbre(it, noeud->arbre_aplatis, DrapeauxNoeud::AUCUN);
	}

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' à la fin des paramètres de la fonction");

	/* analyse les types de retour de la fonction */
	if (apparie(GenreLexeme::PARENTHESE_OUVRANTE)) {
		// nous avons la déclaration d'un type
		noeud->est_declaration_type = true;
		consomme();

		if (eu_declarations) {
			POUR (noeud->params) {
				if (it->est_decl_var()) {
					rapporte_erreur(m_unite->espace, it, "Obtenu la déclaration d'une variable dans la déclartion d'un type de fonction");
				}
			}
		}

		while (true) {
			nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds();
			auto type_declare = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);
			nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds() - nombre_noeuds_alloues;
			noeud->arbre_aplatis.reserve_delta(nombre_noeuds_alloues);
			noeud->params_sorties.pousse(static_cast<NoeudDeclaration *>(type_declare));
			aplatis_arbre(type_declare, noeud->arbre_aplatis, DrapeauxNoeud::AUCUN);

			if (!apparie(GenreLexeme::VIRGULE)) {
				break;
			}

			consomme();
		}

		consomme(GenreLexeme::PARENTHESE_FERMANTE, "attendu une parenthèse fermante");
	}
	else {
		noeud->bloc_parent->membres->pousse(noeud);

		// nous avons la déclaration d'une fonction
		if (apparie(GenreLexeme::RETOUR_TYPE)) {
			consomme();

			auto eu_parenthese = false;
			if (apparie(GenreLexeme::PARENTHESE_OUVRANTE)) {
				consomme();
				eu_parenthese = true;
			}

			while (true) {
				nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds();
				auto decl_sortie = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);

				if (!decl_sortie->est_decl_var()) {
					auto ident = m_compilatrice.table_identifiants->identifiant_pour_nouvelle_chaine("__ret" + dls::vers_chaine(noeud->params_sorties.taille));

					auto ref = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, decl_sortie->lexeme);
					ref->ident = ident;
					ref->expression_type = decl_sortie;

					auto decl = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, decl_sortie->lexeme);
					decl->valeur = ref;
					decl->ident = ident;
					decl->expression_type = decl_sortie;
					decl->bloc_parent = decl_sortie->bloc_parent;

					decl_sortie = decl;
				}

				decl_sortie->drapeaux |= EST_PARAMETRE;

				nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds() - nombre_noeuds_alloues;
				noeud->arbre_aplatis.reserve_delta(nombre_noeuds_alloues);
				noeud->params_sorties.pousse(decl_sortie->comme_decl_var());
				aplatis_arbre(decl_sortie, noeud->arbre_aplatis, DrapeauxNoeud::AUCUN);

				if (!apparie(GenreLexeme::VIRGULE)) {
					break;
				}

				consomme();
			}

			if (eu_parenthese) {
				consomme(GenreLexeme::PARENTHESE_FERMANTE, "attendu une parenthèse fermante après la liste des retours de la fonction");
			}
		}
		else {
			static const Lexeme lexeme_rien = { "rien", {}, GenreLexeme::RIEN, 0, 0, 0 };
			auto type_declare = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_TYPE, &lexeme_rien);

			auto ident = m_compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");

			auto ref = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, &lexeme_rien);
			ref->ident = ident;
			ref->expression_type = type_declare;

			auto decl = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, &lexeme_rien);
			decl->valeur = ref;
			decl->ident = ident;
			decl->expression_type = type_declare;

			noeud->params_sorties.pousse(decl);
			aplatis_arbre(type_declare, noeud->arbre_aplatis, DrapeauxNoeud::AUCUN);
		}

		/* ignore les points-virgules implicites */
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
		}

		while (apparie(GenreLexeme::DIRECTIVE)) {
			consomme();

			auto ident_directive = lexeme_courant()->ident;

			if (ident_directive == ID::enligne) {
				noeud->drapeaux |= FORCE_ENLIGNE;
			}
			else if (ident_directive == ID::horsligne) {
				noeud->drapeaux |= FORCE_HORSLIGNE;
			}
			else if (ident_directive == ID::nulctx) {
				noeud->drapeaux |= FORCE_NULCTX;
				noeud->drapeaux |= FORCE_SANSTRACE;
			}
			else if (ident_directive == ID::externe) {
				noeud->drapeaux |= EST_EXTERNE;
				noeud->est_externe = true;

				if (lexeme_mot_cle->genre == GenreLexeme::COROUT) {
					lance_erreur("Une coroutine ne peut pas être externe");
				}
			}
			else if (ident_directive == ID::sanstrace) {
				noeud->drapeaux |= FORCE_SANSTRACE;
			}
			else if (ident_directive == ID::interface) {
				renseigne_fonction_interface(m_unite->espace->interface_kuri, noeud);
			}
			else if (ident_directive == ID::creation_contexte) {
				noeud->drapeaux |= FORCE_NULCTX;
				noeud->drapeaux |= FORCE_SANSTRACE;
				m_unite->espace->interface_kuri->decl_creation_contexte = noeud;
			}
			else if (ident_directive == ID::compilatrice) {
				noeud->drapeaux |= (FORCE_SANSTRACE | FORCE_NULCTX | COMPILATRICE);
				noeud->est_externe = true;
			}
			else if (ident_directive == ID::sansbroyage) {
				noeud->drapeaux |= (FORCE_SANSBROYAGE);
			}
			else if (ident_directive == ID::racine) {
				noeud->drapeaux |= (EST_RACINE);
			}
			else if (ident_directive == ID::corps_texte) {
				noeud->corps->est_corps_texte = true;
			}
			else {
				lance_erreur("Directive inconnue");
			}

			consomme();
		}

		m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud);

		if (noeud->est_externe) {
			consomme(GenreLexeme::POINT_VIRGULE, "Attendu un point-virgule ';' après la déclaration de la fonction externe");

			if (noeud->params_sorties.taille > 1) {
				lance_erreur("Ne peut avoir plusieurs valeur de retour pour une fonction externe");
			}

			/* ajoute un bloc même pour les fonctions externes, afin de stocker les paramètres */
			noeud->corps->bloc = m_tacheronne.assembleuse->empile_bloc();
			m_tacheronne.assembleuse->depile_bloc();
		}
		else {
			/* ignore les points-virgules implicites */
			if (apparie(GenreLexeme::POINT_VIRGULE)) {
				consomme();
			}

			auto noeud_corps = noeud->corps;

			nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds();
			auto ancien_est_dans_fonction = est_dans_fonction;
			est_dans_fonction = true;
			noeud_corps->bloc = analyse_bloc();
			est_dans_fonction = ancien_est_dans_fonction;
			nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds() - nombre_noeuds_alloues;

			/* À FAIRE : quand nous aurons des fonctions dans des fonctions, il
			 * faudra soustraire le nombre de noeuds des fonctions enfants. Il
			 * faudra également faire attention au moultfilage futur.
			 */
			noeud_corps->arbre_aplatis.reserve(nombre_noeuds_alloues);
			aplatis_arbre(noeud_corps->bloc, noeud_corps->arbre_aplatis, DrapeauxNoeud::AUCUN);

			while (apparie(GenreLexeme::AROBASE)) {
				consomme();

				if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
					lance_erreur("Attendu une chaine de caractère après '@'");
				}

				auto lexeme_annotation = lexeme_courant();
				noeud->annotations.pousse(lexeme_annotation->chaine);

				consomme();
			}

			auto const doit_etre_type = noeud->ident == ID::principale || noeud->possede_drapeau(EST_RACINE);
			if (doit_etre_type) {
				m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud_corps);
			}
		}
	}

	/* dépile le bloc des paramètres */
	m_tacheronne.assembleuse->depile_bloc();

	/* dépile le bloc des constantes */
	m_tacheronne.assembleuse->depile_bloc();

	depile_etat();

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_declaration_operateur()
{
	Prof(Syntaxeuse_analyse_declaration_operateur);

	empile_etat("dans le syntaxage de l'opérateur", lexeme_courant());
	consomme();

	auto lexeme = lexeme_courant();
	auto genre_operateur = lexeme->genre;

	if (!est_operateur_surchargeable(genre_operateur)) {
		lance_erreur("L'opérateur n'est pas surchargeable");
	}

	consomme();

	// :: fonc
	consomme(GenreLexeme::DECLARATION_CONSTANTE, "Attendu :: après la déclaration de l'opérateur");
	consomme(GenreLexeme::FONC, "Attendu fonc après ::");

	auto noeud = CREE_NOEUD(NoeudDeclarationEnteteFonction, GenreNoeud::DECLARATION_ENTETE_FONCTION, lexeme);
	noeud->est_operateur = true;

	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après le nom de la fonction");

	noeud->bloc_constantes = m_tacheronne.assembleuse->empile_bloc();
	noeud->bloc_parametres = m_tacheronne.assembleuse->empile_bloc();

	/* analyse les paramètres de la fonction */
	auto params = dls::tablet<NoeudDeclaration *, 16>();
	auto nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds();

	while (!apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
		auto param = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::VIRGULE);

		if (!param->est_decl_var()) {
			rapporte_erreur(m_unite->espace, param, "Expression inattendue dans la déclaration des paramètres de l'opérateur");
		}

		auto decl_var = param->comme_decl_var();
		decl_var->drapeaux |= EST_PARAMETRE;
		params.pousse(decl_var);

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	copie_tablet_tableau(params, noeud->params);

	if (noeud->params.taille > 2) {
		erreur::lance_erreur(
					"La surcharge d'opérateur ne peut prendre au plus 2 paramètres",
					*m_unite->espace,
					lexeme);
	}
	else if (noeud->params.taille == 1) {
		if (genre_operateur == GenreLexeme::PLUS) {
			lexeme->genre = GenreLexeme::PLUS_UNAIRE;
		}
		else if (genre_operateur == GenreLexeme::MOINS) {
			lexeme->genre = GenreLexeme::MOINS_UNAIRE;
		}
		else if (genre_operateur != GenreLexeme::TILDE && genre_operateur != GenreLexeme::EXCLAMATION) {
			erreur::lance_erreur(
						"La surcharge d'opérateur unaire n'est possible que pour '+', '-', '~', ou '!'",
						*m_unite->espace,
						lexeme);
		}
	}

	nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds() - nombre_noeuds_alloues;
	noeud->arbre_aplatis.reserve(nombre_noeuds_alloues);

	POUR (noeud->params) {
		aplatis_arbre(it, noeud->arbre_aplatis, DrapeauxNoeud::AUCUN);
	}

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' à la fin des paramètres de la fonction");

	/* analyse les types de retour de la fonction */
	consomme(GenreLexeme::RETOUR_TYPE, "Attendu un retour de type");

	while (true) {
		nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds();
		auto decl_sortie = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);

		if (!decl_sortie->est_decl_var()) {
			auto ident = m_compilatrice.table_identifiants->identifiant_pour_nouvelle_chaine("__ret" + dls::vers_chaine(noeud->params_sorties.taille));

			auto ref = CREE_NOEUD(NoeudExpressionReference, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, decl_sortie->lexeme);
			ref->ident = ident;
			ref->expression_type = decl_sortie;

			auto decl = CREE_NOEUD(NoeudDeclarationVariable, GenreNoeud::DECLARATION_VARIABLE, decl_sortie->lexeme);
			decl->valeur = ref;
			decl->ident = ident;
			decl->expression_type = decl_sortie;

			decl_sortie = decl;
		}

		decl_sortie->drapeaux |= EST_PARAMETRE;

		nombre_noeuds_alloues = m_tacheronne.allocatrice_noeud.nombre_noeuds() - nombre_noeuds_alloues;
		noeud->arbre_aplatis.reserve_delta(nombre_noeuds_alloues);
		noeud->params_sorties.pousse(decl_sortie->comme_decl_var());
		aplatis_arbre(decl_sortie, noeud->arbre_aplatis, DrapeauxNoeud::AUCUN);

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	if (noeud->params_sorties.taille > 1) {
		lance_erreur("Il est impossible d'avoir plusieurs de sortie pour un opérateur");
	}

	while (apparie(GenreLexeme::DIRECTIVE)) {
		consomme();
		consomme();

		auto directive = lexeme_courant()->ident;

		if (directive == ID::enligne) {
			noeud->drapeaux |= FORCE_ENLIGNE;
		}
		else if (directive == ID::horsligne) {
			noeud->drapeaux |= FORCE_HORSLIGNE;
		}
		else if (directive == ID::nulctx) {
			noeud->drapeaux |= FORCE_NULCTX;
		}
		else {
			lance_erreur("Directive inconnue");
		}
	}

	/* ignore les points-virgules implicites */
	if (apparie(GenreLexeme::POINT_VIRGULE)) {
		consomme();
	}

	m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud);

	auto noeud_corps = noeud->corps;
	auto ancien_est_dans_fonction = est_dans_fonction;
	est_dans_fonction = true;
	noeud_corps->bloc = analyse_bloc();
	est_dans_fonction = ancien_est_dans_fonction;
	aplatis_arbre(noeud_corps->bloc, noeud_corps->arbre_aplatis, DrapeauxNoeud::AUCUN);

	while (apparie(GenreLexeme::AROBASE)) {
		consomme();

		if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
			lance_erreur("Attendu une chaine de caractère après '@'");
		}

		auto lexeme_annotation = lexeme_courant();
		noeud->annotations.pousse(lexeme_annotation->chaine);

		consomme();
	}

	/* dépile le bloc des paramètres */
	m_tacheronne.assembleuse->depile_bloc();

	/* dépile le bloc des constantes */
	m_tacheronne.assembleuse->depile_bloc();

	depile_etat();

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_declaration_structure(NoeudExpression *gauche)
{
	Prof(Syntaxeuse_analyse_declaration_structure);

	auto lexeme_mot_cle = lexeme_courant();
	empile_etat("dans le syntaxage de la structure", lexeme_mot_cle);
	consomme();

	auto noeud_decl = CREE_NOEUD(NoeudStruct, GenreNoeud::DECLARATION_STRUCTURE, gauche->lexeme);
	noeud_decl->est_union = (lexeme_mot_cle->genre == GenreLexeme::UNION);
	noeud_decl->bloc_parent->membres->pousse(noeud_decl);

	auto cree_tache = false;

	if (gauche->ident == ID::InfoType) {
		noeud_decl->type = m_unite->espace->typeuse.type_info_type_;
		auto type_info_type = m_unite->espace->typeuse.type_info_type_->comme_structure();
		type_info_type->decl = noeud_decl;
		type_info_type->nom = noeud_decl->ident->nom;
	}
	else if (gauche->ident == ID::ContexteProgramme) {
		auto type_contexte = m_unite->espace->typeuse.type_contexte->comme_structure();
		noeud_decl->type = type_contexte;
		type_contexte->decl = noeud_decl;
		type_contexte->nom = noeud_decl->ident->nom;
		cree_tache = true;
	}
	else {
		if (noeud_decl->est_union) {
			noeud_decl->type = m_unite->espace->typeuse.reserve_type_union(noeud_decl);
		}
		else {
			noeud_decl->type = m_unite->espace->typeuse.reserve_type_structure(noeud_decl);
		}
	}

	if (apparie(GenreLexeme::NONSUR)) {
		noeud_decl->est_nonsure = true;
		consomme();
	}

	/* paramètres polymorphiques */
	if (apparie(GenreLexeme::PARENTHESE_OUVRANTE)) {
		consomme();

		noeud_decl->bloc_constantes = m_tacheronne.assembleuse->empile_bloc();

		while (!fini() && !apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
			auto drapeaux = DrapeauxNoeud::AUCUN;

			if (apparie(GenreLexeme::DOLLAR)) {
				consomme();

				drapeaux |= DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE;
			}

			auto expression = analyse_expression({}, GenreLexeme::PARENTHESE_OUVRANTE, GenreLexeme::VIRGULE);

			if (!expression->est_decl_var()) {
				rapporte_erreur(m_unite->espace, expression, "Attendu une déclaration de variable dans les paramètres polymorphiques de la structure");
			}

			auto decl_var = expression->comme_decl_var();
			decl_var->drapeaux |= drapeaux;

			noeud_decl->params_polymorphiques.pousse(decl_var);
			aplatis_arbre(decl_var, noeud_decl->arbre_aplatis_params, {});

			if (!apparie(GenreLexeme::VIRGULE)) {
				break;
			}

			consomme();
		}

		/* permet la déclaration de structures sans paramètres, pourtant ayant des parenthèse */
		if (noeud_decl->params_polymorphiques.taille != 0) {
			noeud_decl->est_polymorphe = true;
			cree_tache = true;
		}

		consomme();
	}

	if (apparie(GenreLexeme::DIRECTIVE)) {
		consomme();

		if (lexeme_courant()->ident == ID::interface) {
			renseigne_type_interface(m_unite->espace->typeuse, noeud_decl->ident, noeud_decl->type);
			cree_tache = true;
		}
		else if (lexeme_courant()->ident == ID::externe) {
			noeud_decl->est_externe = true;
		}
		else {
			lance_erreur("Directive inconnue");
		}

		consomme();
	}

	auto analyse_membres = true;

	if (noeud_decl->est_externe) {
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			analyse_membres = false;
		}
	}

	if (analyse_membres) {
		auto bloc = m_tacheronne.assembleuse->empile_bloc();
		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après le nom de la structure");

		auto expressions = dls::tablet<NoeudExpression *, 16>();

		while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
			if (apparie(GenreLexeme::POINT_VIRGULE)) {
				consomme();
				continue;
			}

			if (apparie_expression()) {
				auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

				if (!noeud->est_declaration() && !noeud->est_assignation() && !noeud->est_empl()) {
					rapporte_erreur(m_unite->espace, noeud, "Attendu une déclaration ou une assignation dans le bloc de la structure");
				}

				expressions.pousse(noeud);
			}
			else {
				lance_erreur("attendu une expression ou une instruction");
			}
		}

		copie_tablet_tableau(expressions, *bloc->expressions.verrou_ecriture());

		POUR (*bloc->expressions.verrou_lecture()) {
			aplatis_arbre(it, noeud_decl->arbre_aplatis, DrapeauxNoeud::AUCUN);
		}

		consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de la structure");

		m_tacheronne.assembleuse->depile_bloc();
		noeud_decl->bloc = bloc;
	}

	if (cree_tache) {
		m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud_decl);
	}

	if (noeud_decl->bloc_constantes) {
		m_tacheronne.assembleuse->depile_bloc();
	}

	depile_etat();

	return noeud_decl;
}

void Syntaxeuse::lance_erreur(const dls::chaine &quoi, erreur::Genre genre)
{
	auto lexeme = lexeme_courant();
	auto fichier = m_unite->espace->fichier(lexeme->fichier);

	auto flux = dls::flux_chaine();

	flux << "\n";
	flux << fichier->chemin << ':' << lexeme->ligne + 1 << " : erreur de syntaxage :\n";

	POUR (m_donnees_etat_syntaxage) {
		erreur::imprime_ligne_avec_message(flux, fichier, it.lexeme, it.message);
	}

	erreur::imprime_ligne_avec_message(flux, fichier, lexeme, quoi.c_str());

	throw erreur::frappe(flux.chn().c_str(), genre);
}

void Syntaxeuse::empile_etat(const char *message, Lexeme *lexeme)
{
	m_donnees_etat_syntaxage.pousse({ lexeme, message });
}

void Syntaxeuse::depile_etat()
{
	m_donnees_etat_syntaxage.pop_back();
}
