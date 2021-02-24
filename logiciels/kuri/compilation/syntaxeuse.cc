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
#include "typage.hh"

// Pour les bibliothèques externes ou les inclusions, détermine le chemin absolu selon le fichier courant, au cas où la bibliothèque serait dans le même dossier que le fichier
static auto trouve_chemin_si_dans_dossier(Module *module, kuri::chaine const &chaine)
{
	auto chaine_ = dls::chaine(chaine);
	/* vérifie si le chemin est relatif ou absolu */
	auto chemin = std::filesystem::path(chaine_.c_str());

	if (!std::filesystem::exists(chemin)) {
		/* le chemin n'est pas absolu, détermine s'il est dans le même dossier */
		auto chemin_abs = dls::chaine(module->chemin()) + chaine_;

		chemin = std::filesystem::path(chemin_abs.c_str());

		if (std::filesystem::exists(chemin)) {
			return kuri::chaine(chemin_abs.c_str(), chemin_abs.taille());
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
	INIT_TYPE(type_info_type_opaque, ID::InfoTypeOpaque)

#undef INIT_TYPE
}

template <typename T, unsigned long N, typename TypeIndex>
static auto copie_tablet_tableau(dls::tablet<T, N> const &src, kuri::tableau<T, TypeIndex> &dst)
{
	dst.reserve(static_cast<TypeIndex>(src.taille()));

	POUR (src) {
		dst.ajoute(it);
	}
}

enum {
	OPERATEUR_EST_SURCHARGEABLE   = (1 << 0),
	EST_EXPRESSION = (1 << 1),
	EST_EXPRESSION_UNAIRE = (1 << 2),
	EST_EXPRESSION_SECONDAIRE = (1 << 3),
	EST_LEXEME_TYPE = (1 << 4),
	EST_INSTRUCTION = (1 << 5),
};

static constexpr auto table_drapeaux_lexemes = [] {
	std::array<u_char, 256> t{};

	for (auto i = 0u; i < 256; ++i) {
		t[i] = 0;

		switch (static_cast<GenreLexeme>(i)) {
			default:
			{
				break;
			}
			case GenreLexeme::INFERIEUR:
			case GenreLexeme::INFERIEUR_EGAL:
			case GenreLexeme::SUPERIEUR:
			case GenreLexeme::SUPERIEUR_EGAL:
			case GenreLexeme::DIFFERENCE:
			case GenreLexeme::EGALITE:
			case GenreLexeme::PLUS:
			case GenreLexeme::PLUS_UNAIRE:
			case GenreLexeme::MOINS:
			case GenreLexeme::MOINS_UNAIRE:
			case GenreLexeme::FOIS:
			case GenreLexeme::FOIS_UNAIRE:
			case GenreLexeme::DIVISE:
			case GenreLexeme::DECALAGE_DROITE:
			case GenreLexeme::DECALAGE_GAUCHE:
			case GenreLexeme::POURCENT:
			case GenreLexeme::ESPERLUETTE:
			case GenreLexeme::BARRE:
			case GenreLexeme::TILDE:
			case GenreLexeme::EXCLAMATION:
			case GenreLexeme::CHAPEAU:
			case GenreLexeme::CROCHET_OUVRANT:
			{
				t[i] |= OPERATEUR_EST_SURCHARGEABLE;
				break;
			}
		}

		// expression unaire
		switch (static_cast<GenreLexeme>(i)) {
			case GenreLexeme::EXCLAMATION:
			case GenreLexeme::MOINS:
			case GenreLexeme::MOINS_UNAIRE:
			case GenreLexeme::PLUS:
			case GenreLexeme::PLUS_UNAIRE:
			case GenreLexeme::TILDE:
			case GenreLexeme::FOIS:
			case GenreLexeme::FOIS_UNAIRE:
			case GenreLexeme::ESP_UNAIRE:
			case GenreLexeme::ESPERLUETTE:
			case GenreLexeme::TROIS_POINTS:
			case GenreLexeme::EXPANSION_VARIADIQUE:
			{
				t[i] |= EST_EXPRESSION_UNAIRE;
				t[i] |= EST_EXPRESSION;
				break;
			}
			default:
			{
				break;
			}
		}

		// expresssion
		switch (static_cast<GenreLexeme>(i)) {
			case GenreLexeme::CARACTERE:
			case GenreLexeme::CHAINE_CARACTERE:
			case GenreLexeme::CHAINE_LITTERALE:
			case GenreLexeme::COROUT:
			case GenreLexeme::CROCHET_OUVRANT: // construit tableau
			case GenreLexeme::DIRECTIVE:
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
			case GenreLexeme::MEMOIRE:
			case GenreLexeme::NOMBRE_ENTIER:
			case GenreLexeme::NOMBRE_REEL:
			case GenreLexeme::NON_INITIALISATION:
			case GenreLexeme::NUL:
			case GenreLexeme::OPERATEUR:
			case GenreLexeme::PARENTHESE_OUVRANTE: // expression entre parenthèse
			case GenreLexeme::STRUCT:
			case GenreLexeme::TABLEAU:
			case GenreLexeme::TAILLE_DE:
			case GenreLexeme::TYPE_DE:
			case GenreLexeme::TENTE:
			case GenreLexeme::UNION:
			case GenreLexeme::VRAI:
			{
				t[i] |= EST_EXPRESSION;
				break;
			}
			default:
			{
				break;
			}
		}

		// expresssion type
		switch (static_cast<GenreLexeme>(i)) {
			case GenreLexeme::N8:
			case GenreLexeme::N16:
			case GenreLexeme::N32:
			case GenreLexeme::N64:
			case GenreLexeme::R16:
			case GenreLexeme::R32:
			case GenreLexeme::R64:
			case GenreLexeme::Z8:
			case GenreLexeme::Z16:
			case GenreLexeme::Z32:
			case GenreLexeme::Z64:
			case GenreLexeme::BOOL:
			case GenreLexeme::RIEN:
			case GenreLexeme::EINI:
			case GenreLexeme::CHAINE:
			case GenreLexeme::CHAINE_CARACTERE:
			case GenreLexeme::OCTET:
			case GenreLexeme::TYPE_DE_DONNEES:
			{
				t[i] |= EST_LEXEME_TYPE;
				t[i] |= EST_EXPRESSION;
				break;
			}
			default:
				break;
		}

		// expresssion secondaire
		switch (static_cast<GenreLexeme>(i)) {
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
				t[i] |= EST_EXPRESSION_SECONDAIRE;
				break;
			}
			default:
			{
				break;
			}
		}

		switch (static_cast<GenreLexeme>(i)) {
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
			case GenreLexeme::REPRENDS:
			case GenreLexeme::RETIENS:
			case GenreLexeme::RETOURNE:
			case GenreLexeme::SAUFSI:
			case GenreLexeme::SI:
			case GenreLexeme::TANTQUE:
			{
				t[i] |= EST_INSTRUCTION;
				break;
			}
			default:
			{
				break;
			}
		}
	}

	return t;
}();

static constexpr auto table_associativite_lexemes = [] {
	std::array<Associativite, 256> t{};

	for (auto i = 0u; i < 256; ++i) {
		t[i] = static_cast<Associativite>(-1);

		switch (static_cast<GenreLexeme>(i)) {
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
				t[i] = Associativite::GAUCHE;
				break;
			}
			case GenreLexeme::EXCLAMATION:
			case GenreLexeme::TILDE:
			case GenreLexeme::PLUS_UNAIRE:
			case GenreLexeme::FOIS_UNAIRE:
			case GenreLexeme::ESP_UNAIRE:
			case GenreLexeme::MOINS_UNAIRE:
			case GenreLexeme::EXPANSION_VARIADIQUE:
			{
				t[i] = Associativite::DROITE;
				break;
			}
			default:
			{
				break;
			}
		}
	}

	return t;
}();

static constexpr int PRECEDENCE_VIRGULE = 3;
static constexpr int PRECEDENCE_TYPE    = 4;

static constexpr auto table_precedence_lexemes = [] {
	std::array<char, 256> t{};

	for (auto i = 0u; i < 256; ++i) {
		t[i] = -1;

		switch (static_cast<GenreLexeme>(i)) {
			case GenreLexeme::TROIS_POINTS:
			{
				t[i] = 1;
				break;
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
				t[i] = 2;
				break;
			}
			case GenreLexeme::VIRGULE:
			{
				t[i] = PRECEDENCE_VIRGULE;
				break;
			}
			case GenreLexeme::DOUBLE_POINTS:
			{
				t[i] = PRECEDENCE_TYPE;
				break;
			}
			case GenreLexeme::BARRE_BARRE:
			{
				t[i] = 5;
				break;
			}
			case GenreLexeme::ESP_ESP:
			{
				t[i] = 6;
				break;
			}
			case GenreLexeme::BARRE:
			{
				t[i] = 7;
				break;
			}
			case GenreLexeme::CHAPEAU:
			{
				t[i] = 8;
				break;
			}
			case GenreLexeme::ESPERLUETTE:
			{
				t[i] = 9;
				break;
			}
			case GenreLexeme::DIFFERENCE:
			case GenreLexeme::EGALITE:
			{
				t[i] = 10;
				break;
			}
			case GenreLexeme::INFERIEUR:
			case GenreLexeme::INFERIEUR_EGAL:
			case GenreLexeme::SUPERIEUR:
			case GenreLexeme::SUPERIEUR_EGAL:
			{
				t[i] = 11;
				break;
			}
			case GenreLexeme::DECALAGE_GAUCHE:
			case GenreLexeme::DECALAGE_DROITE:
			{
				t[i] = 12;
				break;
			}
			case GenreLexeme::PLUS:
			case GenreLexeme::MOINS:
			{
				t[i] = 13;
				break;
			}
			case GenreLexeme::FOIS:
			case GenreLexeme::DIVISE:
			case GenreLexeme::POURCENT:
			{
				t[i] = 14;
				break;
			}
			case GenreLexeme::COMME:
			{
				t[i] = 15;
				break;
			}
			case GenreLexeme::EXCLAMATION:
			case GenreLexeme::TILDE:
			case GenreLexeme::PLUS_UNAIRE:
			case GenreLexeme::MOINS_UNAIRE:
			case GenreLexeme::FOIS_UNAIRE:
			case GenreLexeme::ESP_UNAIRE:
			case GenreLexeme::EXPANSION_VARIADIQUE:
			{
				t[i] = 16;
				break;
			}
			case GenreLexeme::PARENTHESE_OUVRANTE:
			case GenreLexeme::POINT:
			case GenreLexeme::CROCHET_OUVRANT:
			{
				t[i] = 17;
				break;
			}
			default:
			{

				break;
			}
		}
	}

	return t;
}();

static inline bool est_operateur_surchargeable(GenreLexeme genre)
{
	return (table_drapeaux_lexemes[static_cast<size_t>(genre)] & OPERATEUR_EST_SURCHARGEABLE) != 0;
}

static inline int precedence_pour_operateur(GenreLexeme genre_operateur)
{
	int precedence = table_precedence_lexemes[static_cast<size_t>(genre_operateur)];
	assert_rappel(precedence != -1, [&]() { std::cerr << "Aucune précédence pour l'opérateur : " << chaine_du_lexeme(genre_operateur) << '\n'; });
	return precedence;
}

static inline Associativite associativite_pour_operateur(GenreLexeme genre_operateur)
{
	auto associativite = table_associativite_lexemes[static_cast<size_t>(genre_operateur)];
	assert_rappel(associativite != static_cast<Associativite>(-1), [&]() { std::cerr << "Aucune précédence pour l'opérateur : " << chaine_du_lexeme(genre_operateur) << '\n'; });
	return associativite;
}

Syntaxeuse::Syntaxeuse(Tacheronne &tacheronne, UniteCompilation *unite)
	: m_compilatrice(tacheronne.compilatrice)
	, m_tacheronne(tacheronne)
	, m_fichier(unite->fichier)
	, m_unite(unite)
	, m_lexemes(m_fichier->donnees_constantes->lexemes)
{
	if (m_lexemes.taille() > 0) {
		m_lexeme_courant = &m_lexemes[m_position];
	}

	auto module = m_fichier->module;

	m_tacheronne.assembleuse->depile_tout();

	module->mutex.lock();
	{
		if (module->bloc == nullptr) {
			module->bloc = m_tacheronne.assembleuse->empile_bloc(lexeme_courant());
			module->bloc->ident = module->nom();
		}
		else {
			m_tacheronne.assembleuse->bloc_courant(module->bloc);
		}
	}
	module->mutex.unlock();
}

void Syntaxeuse::lance_analyse()
{
	m_position = 0;
	m_fichier->temps_analyse = 0.0;

	if (m_lexemes.taille() == 0) {
		return;
	}

	m_chrono_analyse.commence();

	if (m_fichier->metaprogramme_corps_texte) {
		auto metaprogramme = m_fichier->metaprogramme_corps_texte;

		if (metaprogramme->corps_texte_pour_fonction) {
			auto recipiente = metaprogramme->corps_texte_pour_fonction;
			m_tacheronne.assembleuse->bloc_courant(recipiente->bloc_parametres);

			recipiente->corps->bloc = analyse_bloc(false);
			recipiente->corps->est_corps_texte = false;
			recipiente->est_metaprogramme = false;
			m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, recipiente->corps);
		}
		else if (metaprogramme->corps_texte_pour_structure) {
			auto recipiente = metaprogramme->corps_texte_pour_structure;
			recipiente->arbre_aplatis.efface();

			m_tacheronne.assembleuse->bloc_courant(recipiente->bloc_constantes);

			recipiente->bloc = analyse_bloc(false);

			POUR ((*recipiente->bloc_constantes->membres.verrou_lecture())) {
				recipiente->bloc->membres->ajoute(it);
			}

			recipiente->est_corps_texte = false;
		}

		metaprogramme->fut_execute = true;

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

			auto noeud = m_tacheronne.assembleuse->cree_importe(lexeme);
			noeud->bloc_parent->expressions->ajoute(noeud);

			if (!apparie(GenreLexeme::CHAINE_LITTERALE) && !apparie(GenreLexeme::CHAINE_CARACTERE)) {
				lance_erreur("Attendu une chaine littérale après 'importe'");
			}

			noeud->expr = m_tacheronne.assembleuse->cree_ref_decl(lexeme_courant());

			m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud);

			consomme();
		}
		else if (genre_lexeme == GenreLexeme::CHARGE) {
			consomme();

			auto noeud = m_tacheronne.assembleuse->cree_charge(lexeme);
			noeud->bloc_parent->expressions->ajoute(noeud);

			if (!apparie(GenreLexeme::CHAINE_LITTERALE) && !apparie(GenreLexeme::CHAINE_CARACTERE)) {
				lance_erreur("Attendu une chaine littérale après 'charge'");
			}

			noeud->expr = m_tacheronne.assembleuse->cree_ref_decl(lexeme_courant());

			m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud);

			consomme();
		}
		else if (apparie_expression()) {
			auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

			// noeud peut-être nul si nous avons une directive
			if (noeud != nullptr) {
				if (noeud->est_declaration()) {
					noeud->drapeaux |= EST_GLOBALE;

					if (noeud->est_decl_var()) {
						auto decl_var = noeud->comme_decl_var();
						noeud->bloc_parent->membres->ajoute(decl_var);
						m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud);
					}
				}

				noeud->bloc_parent->expressions->ajoute(noeud);
			}
		}
		else {
			lance_erreur("attendu une expression ou une instruction");
		}
	}

	m_fichier->temps_analyse += m_chrono_analyse.arrete();

	m_tacheronne.assembleuse->depile_tout();
}

bool Syntaxeuse::apparie_expression() const
{
	auto genre = lexeme_courant()->genre;
	return (table_drapeaux_lexemes[static_cast<size_t>(genre)] & EST_EXPRESSION) != 0;
}

bool Syntaxeuse::apparie_expression_unaire() const
{
	auto genre = lexeme_courant()->genre;
	return (table_drapeaux_lexemes[static_cast<size_t>(genre)] & EST_EXPRESSION_UNAIRE) != 0;
}

bool Syntaxeuse::apparie_expression_secondaire() const
{
	auto genre = lexeme_courant()->genre;
	return (table_drapeaux_lexemes[static_cast<size_t>(genre)] & EST_EXPRESSION_SECONDAIRE) != 0;
}

bool Syntaxeuse::apparie_instruction() const
{
	auto genre = lexeme_courant()->genre;
	return (table_drapeaux_lexemes[static_cast<size_t>(genre)] & EST_INSTRUCTION) != 0;
}

NoeudExpression *Syntaxeuse::analyse_expression(DonneesPrecedence const &donnees_precedence, GenreLexeme racine_expression, GenreLexeme lexeme_final)
{
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
		case GenreLexeme::EXPANSION_VARIADIQUE:
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
		case GenreLexeme::ESP_UNAIRE:
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::TILDE:
		case GenreLexeme::PLUS_UNAIRE:
		case GenreLexeme::MOINS_UNAIRE:
		case GenreLexeme::FOIS_UNAIRE:
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

	auto noeud = static_cast<NoeudExpressionUnaire *>(m_tacheronne.assembleuse->cree_noeud(genre_noeud, lexeme));

	// cette vérification n'est utile que pour les arguments variadiques sans type
	if (apparie_expression()) {
		noeud->expr = analyse_expression({ precedence, associativite }, GenreLexeme::INCONNU, lexeme_final);
	}

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_expression_primaire(GenreLexeme racine_expression, GenreLexeme lexeme_final)
{
	if (apparie_expression_unaire()) {
		return analyse_expression_unaire(lexeme_final);
	}

	auto lexeme = lexeme_courant();

	switch (lexeme->genre) {
		case GenreLexeme::CARACTERE:
		{
			consomme();
			return m_tacheronne.assembleuse->cree_lit_caractere(lexeme);
		}
		case GenreLexeme::CHAINE_CARACTERE:
		{
			consomme();
			return m_tacheronne.assembleuse->cree_ref_decl(lexeme);
		}
		case GenreLexeme::CHAINE_LITTERALE:
		{
			consomme();
			return m_tacheronne.assembleuse->cree_lit_chaine(lexeme);
		}
		case GenreLexeme::TABLEAU:
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
				auto noeud = m_tacheronne.assembleuse->cree_op_binaire(lexeme);
				noeud->expr1 = expression_entre_crochets;
				noeud->expr2 = analyse_expression({ PRECEDENCE_TYPE, Associativite::GAUCHE }, racine_expression, lexeme_final);

				return noeud;
			}

			auto noeud = m_tacheronne.assembleuse->cree_construction_tableau(lexeme);

			/* Le reste de la pipeline suppose que l'expression est une virgule,
			 * donc créons une telle expression au cas où nous n'avons qu'un seul
			 * élément dans le tableau. */
			if (!expression_entre_crochets->est_virgule()) {
				auto virgule = m_tacheronne.assembleuse->cree_virgule(lexeme);
				virgule->expressions.ajoute(expression_entre_crochets);
				expression_entre_crochets = virgule;
			}

			noeud->expr = expression_entre_crochets;

			return noeud;
		}
		case GenreLexeme::EMPL:
		{
			consomme();

			auto noeud = m_tacheronne.assembleuse->cree_empl(lexeme);
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
			consomme();
			return m_tacheronne.assembleuse->cree_lit_bool(lexeme);
		}
		case GenreLexeme::INFO_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'info_de'");

			auto noeud = m_tacheronne.assembleuse->cree_info_de(lexeme);
			noeud->expr = analyse_expression_primaire(GenreLexeme::INFO_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression de 'taille_de'");

			return noeud;
		}
		case GenreLexeme::INIT_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'init_de'");

			auto noeud = m_tacheronne.assembleuse->cree_init_de(lexeme);
			noeud->expr = analyse_expression_primaire(GenreLexeme::INIT_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression de 'init_de'");

			return noeud;
		}
		case GenreLexeme::MEMOIRE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'mémoire'");

			auto noeud = m_tacheronne.assembleuse->cree_memoire(lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::MEMOIRE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression");

			return noeud;
		}
		case GenreLexeme::NOMBRE_ENTIER:
		{
			consomme();
			return m_tacheronne.assembleuse->cree_lit_entier(lexeme);
		}
		case GenreLexeme::NOMBRE_REEL:
		{
			consomme();
			return m_tacheronne.assembleuse->cree_lit_reel(lexeme);
		}
		case GenreLexeme::NON_INITIALISATION:
		{
			consomme();
			return m_tacheronne.assembleuse->cree_non_initialisation(lexeme);
		}
		case GenreLexeme::NUL:
		{
			consomme();
			return m_tacheronne.assembleuse->cree_lit_nul(lexeme);
		}
		case GenreLexeme::OPERATEUR:
		{
			return analyse_declaration_operateur();
		}
		case GenreLexeme::PARENTHESE_OUVRANTE:
		{
			consomme();

			auto noeud = m_tacheronne.assembleuse->cree_parenthese(lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::PARENTHESE_OUVRANTE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "attenud une parenthèse fermante");

			return noeud;
		}
		case GenreLexeme::TAILLE_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'taille_de'");

			auto noeud = m_tacheronne.assembleuse->cree_taille_de(lexeme);
			noeud->expr = analyse_expression_primaire(GenreLexeme::TAILLE_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'taille_de'");

			return noeud;
		}
		case GenreLexeme::TYPE_DE:
		{
			consomme();
			consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'type_de'");

			auto noeud = m_tacheronne.assembleuse->cree_type_de(lexeme);
			noeud->expr = analyse_expression({}, GenreLexeme::TYPE_DE, GenreLexeme::INCONNU);

			consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'type_de'");

			return noeud;
		}
		case GenreLexeme::TENTE:
		{
			consomme();

			auto noeud = m_tacheronne.assembleuse->cree_tente(lexeme);
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
				m_compilatrice.bibliotheques_dynamiques->ajoute(chaine);
			}
			else if (directive == ID::bibliotheque_statique) {
				auto chaine_bib = lexeme_courant()->chaine;
				consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

				auto chaine = trouve_chemin_si_dans_dossier(m_fichier->module, chaine_bib);
				m_compilatrice.bibliotheques_statiques->ajoute(chaine);
			}
			else if (directive == ID::def) {
				auto chaine = lexeme_courant()->chaine;
				consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

				m_compilatrice.definitions->ajoute(chaine);
			}
			else if (directive == ID::execute || directive == ID::assert_ || directive == ID::test) {
				auto noeud = m_tacheronne.assembleuse->cree_execution(lexeme);
				noeud->ident = directive;

				if (directive == ID::test) {
					noeud->expr = analyse_bloc();
				}
				else {
					noeud->expr = analyse_expression({}, GenreLexeme::DIRECTIVE, GenreLexeme::INCONNU);
				}

				if (!est_dans_fonction && (directive != ID::test || m_compilatrice.active_tests)) {
					m_compilatrice.ordonnanceuse->cree_tache_pour_typage(m_unite->espace, noeud);
				}

				return noeud;
			}
			else if (directive == ID::chemin) {
				auto chaine = lexeme_courant()->chaine;
				consomme(GenreLexeme::CHAINE_LITTERALE, "Attendu une chaine littérale après la directive");

				m_compilatrice.chemins->ajoute(chaine);
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
			else if (directive == ID::cuisine) {
				auto noeud = m_tacheronne.assembleuse->cree_cuisine(lexeme);
				noeud->ident = directive;
				noeud->expr = analyse_expression({}, GenreLexeme::DIRECTIVE, GenreLexeme::INCONNU);
				return noeud;
			}
			else {
				/* repositionne le lexème courant afin que les messages d'erreurs pointent au bon endroit */
				recule();
				lance_erreur("Directive inconnue");
			}

			return nullptr;
		}
		case GenreLexeme::DOLLAR:
		{
			consomme();
			lexeme = lexeme_courant();

			consomme(GenreLexeme::CHAINE_CARACTERE, "attendu une chaine de caractère après '$'");

			auto noeud = m_tacheronne.assembleuse->cree_ref_decl(lexeme);
			noeud->drapeaux |= DECLARATION_TYPE_POLYMORPHIQUE;

			auto noeud_decl_param = m_tacheronne.assembleuse->cree_declaration(lexeme);
			noeud_decl_param->drapeaux |= (DECLARATION_TYPE_POLYMORPHIQUE | EST_CONSTANTE);

			if (fonction_courante) {
				fonction_courante->bloc_constantes->membres->ajoute(noeud_decl_param);
				fonction_courante->est_polymorphe = true;
			}
			else if (structure_courante) {
				structure_courante->bloc_constantes->membres->ajoute(noeud_decl_param);
				structure_courante->est_polymorphe = true;
			}
			else if (!m_est_declaration_type_opaque) {
				lance_erreur("déclaration d'un type polymorphique hors d'une fonction, d'une structure, ou de la déclaration d'un type opaque");
			}

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
		case GenreLexeme::SI:
		{
			return analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
		}
		case GenreLexeme::SAUFSI:
		{
			return analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
		}
		default:
		{
			if (est_identifiant_type(lexeme->genre)) {
				consomme();
				return m_tacheronne.assembleuse->cree_ref_type(lexeme);
			}

			lance_erreur("attendu une expression primaire");
		}
	}
}

NoeudExpression *Syntaxeuse::analyse_expression_secondaire(NoeudExpression *gauche, const DonneesPrecedence &donnees_precedence, GenreLexeme racine_expression, GenreLexeme lexeme_final)
{
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

			auto noeud = m_tacheronne.assembleuse->cree_op_binaire(lexeme);
			noeud->expr1 = gauche;
			noeud->expr2 = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			return noeud;
		}
		case GenreLexeme::VIRGULE:
		{
			consomme();

			auto noeud_expression_virgule = m_noeud_expression_virgule;

			if (!m_noeud_expression_virgule) {
				m_noeud_expression_virgule = m_tacheronne.assembleuse->cree_virgule(lexeme);
				m_noeud_expression_virgule->expressions.ajoute(gauche);
				noeud_expression_virgule = m_noeud_expression_virgule;
			}

			auto droite = analyse_expression(donnees_precedence, racine_expression, lexeme_final);

			m_noeud_expression_virgule = noeud_expression_virgule;
			m_noeud_expression_virgule->expressions.ajoute(droite);

			return m_noeud_expression_virgule;
		}
		case GenreLexeme::CROCHET_OUVRANT:
		{
			consomme();

			auto noeud = m_tacheronne.assembleuse->cree_indexage(lexeme);
			noeud->expr1 = gauche;
			noeud->expr2 = analyse_expression({}, GenreLexeme::CROCHET_OUVRANT, GenreLexeme::INCONNU);

			consomme(GenreLexeme::CROCHET_FERMANT, "attendu un crochet fermant");

			return noeud;
		}
		case GenreLexeme::DECLARATION_CONSTANTE:
		{
			consomme();

			m_est_declaration_type_opaque = false;

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
						auto noeud = m_tacheronne.assembleuse->cree_declaration(lexeme);
						noeud->ident = gauche->ident;
						noeud->valeur = gauche;
						noeud->expression = noeud_fonction;
						noeud->drapeaux |= EST_CONSTANTE;
						gauche->drapeaux |= EST_CONSTANTE;
						gauche->comme_ref_decl()->decl = noeud;

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
				case GenreLexeme::DIRECTIVE:
				{
					auto position = m_position;

					consomme();

					auto directive = lexeme_courant()->ident;

					if (directive == ID::opaque) {
						m_est_declaration_type_opaque = true;
						consomme();
					}
					else {
						m_position = position - 1;
						consomme();
					}
				}
			}

			auto noeud = m_tacheronne.assembleuse->cree_declaration(lexeme);
			noeud->ident = gauche->ident;
			noeud->valeur = gauche;
			noeud->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			noeud->drapeaux |= EST_CONSTANTE;
			gauche->drapeaux |= EST_CONSTANTE;

			if (m_est_declaration_type_opaque) {
				noeud->drapeaux |= EST_DECLARATION_TYPE_OPAQUE;
				gauche->drapeaux |= EST_DECLARATION_TYPE_OPAQUE;
				m_est_declaration_type_opaque = false;
			}

			if (gauche->est_ref_decl()) {
				gauche->comme_ref_decl()->decl = noeud;
			}

			return noeud;
		}
		case GenreLexeme::DOUBLE_POINTS:
		{
			consomme();

			/* Deux cas :
			 * a, b : z32 = ...
			 * a, b : z32 : ...
			 * Dans les deux cas, nous vérifions que nous n'avons que des références séparées par des virgules.
			 */
			if (m_noeud_expression_virgule) {
				POUR (m_noeud_expression_virgule->expressions) {
					if (it->est_decl_var()) {
						rapporte_erreur(m_unite->espace, it, "Obtenu une déclaration de variable au sein d'une expression-virgule.");
					}

					if (!it->est_ref_decl()) {
						rapporte_erreur(m_unite->espace, it, "Expression inattendue dans l'expression virgule.");
					}
				}

				auto decl = m_tacheronne.assembleuse->cree_declaration(lexeme);
				decl->valeur = m_noeud_expression_virgule;
				decl->expression_type = analyse_expression(donnees_precedence, racine_expression, lexeme_final);

				m_noeud_expression_virgule = nullptr;
				return decl;
			}

			if (gauche->est_ref_decl()) {
				// nous avons la déclaration d'un type (a: z32)
				auto decl = m_tacheronne.assembleuse->cree_declaration(gauche->comme_ref_decl());
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
				rapporte_erreur(m_unite->espace, gauche, "Utilisation de « := » alors qu'un type fut déclaré avec « : »");
			}

			if (gauche->est_virgule()) {
				auto noeud_virgule = gauche->comme_virgule();

				// détecte les expressions du style : a : z32, b := ... , a[0] := ..., etc.
				POUR (noeud_virgule->expressions) {
					if (it->est_decl_var()) {
						rapporte_erreur(m_unite->espace, it, "Utilisation de « := » alors qu'un type fut déclaré avec « : ».");
					}

					if (!it->est_ref_decl()) {
						rapporte_erreur(m_unite->espace, it, "Expression innatendu à gauche de « := »");
					}

					m_tacheronne.assembleuse->cree_declaration(it->comme_ref_decl());
				}
			}
			else if (!gauche->est_ref_decl()) {
				rapporte_erreur(m_unite->espace, gauche, "Expression innatendu à gauche de « := »");
			}

			consomme();

			m_noeud_expression_virgule = nullptr;

			auto noeud = m_tacheronne.assembleuse->cree_declaration(lexeme);
			noeud->ident = gauche->ident;
			noeud->valeur = gauche;
			noeud->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);

			if (gauche->est_ref_decl()) {
				gauche->comme_ref_decl()->decl = noeud;
			}

			m_noeud_expression_virgule = nullptr;

			return noeud;
		}
		case GenreLexeme::EGAL:
		{
			consomme();

			m_noeud_expression_virgule = nullptr;

			if (gauche->est_decl_var()) {
				if (gauche->lexeme->genre == GenreLexeme::DECLARATION_VARIABLE) {
					/* repositionne le lexème courant afin que les messages d'erreurs pointent au bon endroit */
					recule();
					lance_erreur("utilisation de '=' alors que nous somme à droite de ':='");
				}

				auto decl = gauche->comme_decl_var();
				decl->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);

				m_noeud_expression_virgule = nullptr;

				return decl;
			}

			if (gauche->est_virgule()) {
				auto noeud_virgule = gauche->comme_virgule();

				// détecte les expressions du style : a : z32, b = ...
				POUR (noeud_virgule->expressions) {
					if (it->est_decl_var()) {
						rapporte_erreur(m_unite->espace, it, "Obtenu une déclaration de variable dans l'expression séparée par virgule à gauche d'une assignation. Les variables doivent être déclarées avant leurs assignations.");
					}
				}
			}

			auto noeud = m_tacheronne.assembleuse->cree_assignation(lexeme);
			noeud->variable = gauche;
			noeud->expression = analyse_expression(donnees_precedence, racine_expression, lexeme_final);

			m_noeud_expression_virgule = nullptr;

			return noeud;
		}
		case GenreLexeme::POINT:
		{
			consomme();

			auto noeud = m_tacheronne.assembleuse->cree_acces_membre(lexeme);
			noeud->accede = gauche;
			noeud->membre = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
			return noeud;
		}
		case GenreLexeme::TROIS_POINTS:
		{
			consomme();

			auto noeud = m_tacheronne.assembleuse->cree_plage(lexeme);
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

			auto noeud = m_tacheronne.assembleuse->cree_comme(lexeme);
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
		case GenreLexeme::REPRENDS:
		{
			auto noeud = m_tacheronne.assembleuse->cree_controle_boucle(lexeme);
			consomme();

			if (apparie(GenreLexeme::CHAINE_CARACTERE)) {
				noeud->expr = m_tacheronne.assembleuse->cree_ref_decl(lexeme_courant());
				consomme();
			}

			return noeud;
		}
		case GenreLexeme::RETIENS:
		{
			auto noeud = m_tacheronne.assembleuse->cree_retiens(lexeme);
			consomme();

			if (apparie_expression()) {
				dls::tablet<NoeudExpression *, 6> expressions;
				Lexeme *lexeme_virgule = nullptr;

				while (true) {
					auto expr = analyse_expression({}, GenreLexeme::RETIENS, GenreLexeme::VIRGULE);
					expressions.ajoute(expr);

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
					auto virgule = m_tacheronne.assembleuse->cree_virgule(lexeme_virgule);
					copie_tablet_tableau(expressions, virgule->expressions);
					noeud->expr = virgule;
				}
			}

			m_noeud_expression_virgule = nullptr;

			return noeud;
		}
		case GenreLexeme::RETOURNE:
		{
			auto noeud = m_tacheronne.assembleuse->cree_retour(lexeme);
			consomme();

			if (apparie_expression()) {
				dls::tablet<NoeudExpression *, 6> expressions;
				Lexeme *lexeme_virgule = nullptr;

				while (true) {
					auto expr = analyse_expression({}, GenreLexeme::RETOURNE, GenreLexeme::VIRGULE);
					expressions.ajoute(expr);

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
					auto virgule = m_tacheronne.assembleuse->cree_virgule(lexeme_virgule);
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

NoeudBloc *Syntaxeuse::analyse_bloc(bool accolade_requise, bool pour_pousse_contexte)
{
	auto lexeme = lexeme_courant();
	empile_etat("dans l'analyse du bloc", lexeme);

	if (accolade_requise) {
		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{'");
	}

	auto bloc = m_tacheronne.assembleuse->empile_bloc(lexeme);

	if (pour_pousse_contexte) {
		bloc->possede_contexte = true;
	}

	auto expressions = dls::tablet<NoeudExpression *, 32>();

	while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			continue;
		}

		if (apparie_instruction()) {
			auto noeud = analyse_instruction();
			expressions.ajoute(noeud);
		}
		else if (apparie_expression()) {
			auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);
			expressions.ajoute(noeud);
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
	auto noeud = m_tacheronne.assembleuse->cree_appel(lexeme_courant());
	noeud->appelee = gauche;

	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "attendu une parenthèse ouvrante");

	auto params = dls::tablet<NoeudExpression *, 16>();

	while (apparie_expression()) {
		auto expr = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);
		params.ajoute(expr);

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
	auto noeud = m_tacheronne.assembleuse->cree_boucle(lexeme_courant());
	consomme();
	noeud->bloc = analyse_bloc();
	noeud->bloc->appartiens_a_boucle = noeud;
	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_discr()
{
	auto noeud_discr = m_tacheronne.assembleuse->cree_discr(lexeme_courant());
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
				auto noeud_virgule = m_tacheronne.assembleuse->cree_virgule(expr->lexeme);
				noeud_virgule->expressions.ajoute(expr);

				expr = noeud_virgule;
			}
			else {
				m_noeud_expression_virgule = nullptr;
			}

			paires_discr.ajoute({ expr, bloc });
		}
	}

	copie_tablet_tableau(paires_discr, noeud_discr->paires_discr);

	consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}' à la fin du bloc de « discr »");

	return noeud_discr;
}

void Syntaxeuse::analyse_specifiants_instruction_pour(NoeudPour *noeud)
{
	bool eu_direction = false;

	while (true) {
		switch (lexeme_courant()->genre) {
			default:
			{
				return;
			}
			case GenreLexeme::ESPERLUETTE:
			case GenreLexeme::ESP_UNAIRE:
			{
				if (noeud->prend_reference) {
					lance_erreur("redéfinition d'une prise de référence");
				}

				if (noeud->prend_pointeur) {
					lance_erreur("définition d'une prise de référence alors qu'une prise de pointeur fut spécifiée");
				}

				noeud->prend_reference = true;
				consomme();
				break;
			}
			case GenreLexeme::FOIS:
			case GenreLexeme::FOIS_UNAIRE:
			{
				if (noeud->prend_pointeur) {
					lance_erreur("redéfinition d'une prise de pointeur");
				}

				if (noeud->prend_reference) {
					lance_erreur("définition d'une prise de pointeur alors qu'une prise de référence fut spécifiée");
				}

				noeud->prend_pointeur = true;
				consomme();
				break;
			}
			case GenreLexeme::INFERIEUR:
			case GenreLexeme::SUPERIEUR:
			{
				if (eu_direction) {
					lance_erreur("redéfinition d'une direction alors qu'une autre a déjà été spécifiée");
				}

				noeud->lexeme_op = lexeme_courant()->genre;
				eu_direction = true;
				consomme();
				break;
			}
		}
	}
}

NoeudExpression *Syntaxeuse::analyse_instruction_pour()
{
	auto noeud = m_tacheronne.assembleuse->cree_pour(lexeme_courant());
	consomme();

	analyse_specifiants_instruction_pour(noeud);

	auto expression = analyse_expression({}, GenreLexeme::POUR, GenreLexeme::INCONNU);

	if (apparie(GenreLexeme::DANS)) {
		consomme();

		if (!expression->est_virgule()) {
			static Lexeme lexeme_virgule = { ",", {}, GenreLexeme::VIRGULE, 0, 0, 0 };
			auto noeud_virgule = m_tacheronne.assembleuse->cree_virgule(&lexeme_virgule);
			noeud_virgule->expressions.ajoute(expression);

			expression = noeud_virgule;
		}
		else {
			m_noeud_expression_virgule = nullptr;
		}

		noeud->variable = expression;
		noeud->expression = analyse_expression({}, GenreLexeme::DANS, GenreLexeme::INCONNU);
	}
	else {
		auto noeud_it = m_tacheronne.assembleuse->cree_ref_decl(noeud->lexeme);
		noeud_it->ident = ID::it;

		auto noeud_index = m_tacheronne.assembleuse->cree_ref_decl(noeud->lexeme);
		noeud_index->ident = ID::index_it;

		static Lexeme lexeme_virgule = { ",", {}, GenreLexeme::VIRGULE, 0, 0, 0 };
		auto noeud_virgule = m_tacheronne.assembleuse->cree_virgule(&lexeme_virgule);
		noeud_virgule->expressions.ajoute(noeud_it);
		noeud_virgule->expressions.ajoute(noeud_index);

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
	auto noeud = m_tacheronne.assembleuse->cree_pousse_contexte(lexeme_courant());
	consomme();

	noeud->expr = analyse_expression({}, GenreLexeme::POUSSE_CONTEXTE, GenreLexeme::INCONNU);
	noeud->bloc = analyse_bloc(true, true);

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_repete()
{
	auto noeud = m_tacheronne.assembleuse->cree_repete(lexeme_courant());
	consomme();

	noeud->bloc = analyse_bloc();
	noeud->bloc->appartiens_a_boucle = noeud;

	consomme(GenreLexeme::TANTQUE, "Attendu une 'tantque' après le bloc de 'répète'");

	noeud->condition = analyse_expression({}, GenreLexeme::REPETE, GenreLexeme::INCONNU);

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_si(GenreNoeud genre_noeud)
{
	empile_etat("dans l'analyse de l'instruction si", lexeme_courant());

	auto noeud = m_tacheronne.assembleuse->cree_si(lexeme_courant(), genre_noeud);
	consomme();

	noeud->condition = analyse_expression({}, GenreLexeme::SI, GenreLexeme::INCONNU);

	noeud->bloc_si_vrai = analyse_bloc();

	if (apparie(GenreLexeme::SINON)) {
		consomme();

		if (apparie(GenreLexeme::SI)) {
			noeud->bloc_si_faux = analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
		}
		else if (apparie(GenreLexeme::SAUFSI)) {
			noeud->bloc_si_faux = analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
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
	empile_etat("dans l'analyse de l'instruction #si", lexeme);

	auto noeud = m_tacheronne.assembleuse->cree_si_statique(lexeme);

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
	auto noeud = m_tacheronne.assembleuse->cree_tantque(lexeme_courant());
	consomme();

	noeud->condition = analyse_expression({}, GenreLexeme::TANTQUE, GenreLexeme::INCONNU);
	noeud->bloc = analyse_bloc();
	noeud->bloc->appartiens_a_boucle = noeud;

	return noeud;
}

NoeudExpression *Syntaxeuse::analyse_declaration_enum(NoeudExpression *gauche)
{
	auto lexeme = lexeme_courant();
	empile_etat("dans le syntaxage de l'énumération", lexeme);
	consomme();

	auto noeud_decl = m_tacheronne.assembleuse->cree_enum(gauche->lexeme);
	noeud_decl->bloc_parent->membres->ajoute(noeud_decl);

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

	auto lexeme_bloc = lexeme_courant();
	consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après 'énum'");

	auto bloc = m_tacheronne.assembleuse->empile_bloc(lexeme_bloc);

	auto expressions = dls::tablet<NoeudExpression *, 16>();

	while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
		if (apparie(GenreLexeme::POINT_VIRGULE)) {
			consomme();
			continue;
		}

		if (apparie_expression()) {
			auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

			if (noeud->est_ref_decl()) {
				expressions.ajoute(m_tacheronne.assembleuse->cree_declaration(noeud->comme_ref_decl()));
			}
			else {
				expressions.ajoute(noeud);
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

#include "biblinternes/outils/garde_portee.h"

NoeudDeclarationEnteteFonction *Syntaxeuse::analyse_declaration_fonction(Lexeme const *lexeme)
{
	auto lexeme_mot_cle = lexeme_courant();

	empile_etat("dans le syntaxage de la fonction", lexeme_mot_cle);
	consomme();

	auto noeud = m_tacheronne.assembleuse->cree_entete_fonction(lexeme);
	noeud->est_coroutine = lexeme_mot_cle->genre == GenreLexeme::COROUT;

	auto ancienne_fonction = fonction_courante;
	fonction_courante = noeud;

	DIFFERE {
		fonction_courante = ancienne_fonction;
	};

	// @concurrence critique, si nous avons plusieurs définitions
	if (noeud->ident == ID::principale) {
		if (m_unite->espace->fonction_principale) {
			::rapporte_erreur(m_unite->espace, noeud, "Redéfinition de la fonction principale pour cet espace.")
					.ajoute_message("La fonction principale fut déjà définie ici :\n")
					.ajoute_site(m_unite->espace->fonction_principale);
		}

		m_unite->espace->fonction_principale = noeud;
		noeud->drapeaux |= EST_RACINE;
	}
	else if (noeud->ident == ID::__point_d_entree_systeme) {
		m_unite->espace->fonction_point_d_entree = noeud;
		noeud->drapeaux |= EST_RACINE;
	}

	auto lexeme_bloc = lexeme_courant();
	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après le nom de la fonction");

	noeud->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(lexeme_bloc);
	noeud->bloc_parametres = m_tacheronne.assembleuse->empile_bloc(lexeme_bloc);

	/* analyse les paramètres de la fonction */
	auto params = dls::tablet<NoeudDeclarationVariable *, 16>();

	auto eu_declarations = false;

	while (!apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
		auto valeur_poly = false;

		if (apparie(GenreLexeme::DOLLAR)) {
			consomme();
			valeur_poly = true;
		}

		auto param = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::VIRGULE);

		if (param->est_decl_var()) {
			auto decl_var = static_cast<NoeudDeclarationVariable *>(param);
			if (valeur_poly) {
				decl_var->drapeaux |= EST_VALEUR_POLYMORPHIQUE;
				params.ajoute(decl_var);
				noeud->est_polymorphe = true;
			}
			else {
				decl_var->drapeaux |= EST_PARAMETRE;
				params.ajoute(decl_var);
			}

			eu_declarations = true;
		}
		else {
			// XXX - hack
			params.ajoute(static_cast<NoeudDeclarationVariable *>(param));
		}

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	copie_tablet_tableau(params, noeud->params);

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
			auto type_declare = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);
			noeud->params_sorties.ajoute(static_cast<NoeudDeclarationVariable *>(type_declare));

			if (!apparie(GenreLexeme::VIRGULE)) {
				break;
			}

			consomme();
		}

		// pousse les constantes polymorphiques de cette fonction dans ceux de la fonction-mère
		if (ancienne_fonction) {
			POUR (*noeud->bloc_constantes->membres.verrou_lecture()) {
				ancienne_fonction->bloc_constantes->membres->ajoute(it);
			}
		}

		consomme(GenreLexeme::PARENTHESE_FERMANTE, "attendu une parenthèse fermante");
	}
	else {
		noeud->bloc_parent->membres->ajoute(noeud);

		// nous avons la déclaration d'une fonction
		if (apparie(GenreLexeme::RETOUR_TYPE)) {
			consomme();

			auto eu_parenthese = false;
			if (apparie(GenreLexeme::PARENTHESE_OUVRANTE)) {
				consomme();
				eu_parenthese = true;
			}

			while (true) {
				auto decl_sortie = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);

				if (!decl_sortie->est_decl_var()) {
					auto ident = m_compilatrice.table_identifiants->identifiant_pour_nouvelle_chaine(enchaine("__ret", noeud->params_sorties.taille()));

					auto ref = m_tacheronne.assembleuse->cree_ref_decl(decl_sortie->lexeme);
					ref->ident = ident;

					auto decl = m_tacheronne.assembleuse->cree_declaration(ref);
					decl->expression_type = decl_sortie;
					decl->bloc_parent = decl_sortie->bloc_parent;

					decl_sortie = decl;
				}

				decl_sortie->drapeaux |= EST_PARAMETRE;

				noeud->params_sorties.ajoute(decl_sortie->comme_decl_var());

				if (!apparie(GenreLexeme::VIRGULE)) {
					break;
				}

				consomme();
			}

			if (noeud->params_sorties.taille() > 1) {
				auto ref = m_tacheronne.assembleuse->cree_ref_decl(noeud->params_sorties[0]->lexeme);
				/* il nous faut un identifiant valide */
				ref->ident = m_compilatrice.table_identifiants->identifiant_pour_nouvelle_chaine("valeur_de_retour");
				noeud->param_sortie = m_tacheronne.assembleuse->cree_declaration(ref);
			}
			else {
				noeud->param_sortie = noeud->params_sorties[0];
			}

			if (eu_parenthese) {
				consomme(GenreLexeme::PARENTHESE_FERMANTE, "attendu une parenthèse fermante après la liste des retours de la fonction");
			}
		}
		else {
			static const Lexeme lexeme_rien = { "rien", {}, GenreLexeme::RIEN, 0, 0, 0 };
			auto type_declare = m_tacheronne.assembleuse->cree_ref_type(&lexeme_rien);

			auto ident = m_compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");

			auto ref = m_tacheronne.assembleuse->cree_ref_decl(&lexeme_rien);
			ref->ident = ident;

			auto decl = m_tacheronne.assembleuse->cree_declaration(ref);
			decl->expression_type = type_declare;

			noeud->params_sorties.ajoute(decl);
			noeud->param_sortie = noeud->params_sorties[0];
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
				noeud->bloc_parametres->possede_contexte = false;
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

			if (noeud->params_sorties.taille() > 1) {
				lance_erreur("Ne peut avoir plusieurs valeur de retour pour une fonction externe");
			}

			/* ajoute un bloc même pour les fonctions externes, afin de stocker les paramètres */
			noeud->corps->bloc = m_tacheronne.assembleuse->empile_bloc(lexeme_courant());
			m_tacheronne.assembleuse->depile_bloc();
		}
		else {
			/* ignore les points-virgules implicites */
			if (apparie(GenreLexeme::POINT_VIRGULE)) {
				consomme();
			}

			auto noeud_corps = noeud->corps;

			auto ancien_est_dans_fonction = est_dans_fonction;
			est_dans_fonction = true;
			noeud_corps->bloc = analyse_bloc();
			est_dans_fonction = ancien_est_dans_fonction;

			while (apparie(GenreLexeme::AROBASE)) {
				consomme();

				if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
					lance_erreur("Attendu une chaine de caractère après '@'");
				}

				auto lexeme_annotation = lexeme_courant();
				noeud->annotations.ajoute(lexeme_annotation->chaine);

				consomme();
			}

			auto const doit_etre_type = noeud->possede_drapeau(EST_RACINE);
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
	empile_etat("dans le syntaxage de l'opérateur", lexeme_courant());
	consomme();

	auto lexeme = lexeme_courant();
	auto genre_operateur = lexeme->genre;

	if (!est_operateur_surchargeable(genre_operateur)) {
		lance_erreur("L'opérateur n'est pas surchargeable");
	}

	consomme();

	if (genre_operateur == GenreLexeme::CROCHET_OUVRANT) {
		consomme(GenreLexeme::CROCHET_FERMANT, "Attendu ']' après '[' pour la déclaration de l'opérateur");
	}

	// :: fonc
	consomme(GenreLexeme::DECLARATION_CONSTANTE, "Attendu :: après la déclaration de l'opérateur");
	consomme(GenreLexeme::FONC, "Attendu fonc après ::");

	auto noeud = m_tacheronne.assembleuse->cree_entete_fonction(lexeme);
	noeud->est_operateur = true;

	auto lexeme_bloc = lexeme_courant();
	consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après le nom de la fonction");

	noeud->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(lexeme_bloc);
	noeud->bloc_parametres = m_tacheronne.assembleuse->empile_bloc(lexeme_bloc);

	/* analyse les paramètres de la fonction */
	auto params = dls::tablet<NoeudDeclarationVariable *, 16>();

	while (!apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
		auto param = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::VIRGULE);

		if (!param->est_decl_var()) {
			rapporte_erreur(m_unite->espace, param, "Expression inattendue dans la déclaration des paramètres de l'opérateur");
		}

		auto decl_var = param->comme_decl_var();
		decl_var->drapeaux |= EST_PARAMETRE;
		params.ajoute(decl_var);

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	copie_tablet_tableau(params, noeud->params);

	if (noeud->params.taille() > 2) {
		erreur::lance_erreur(
					"La surcharge d'opérateur ne peut prendre au plus 2 paramètres",
					*m_unite->espace,
					noeud);
	}
	else if (noeud->params.taille() == 1) {
		if (genre_operateur == GenreLexeme::PLUS) {
			lexeme->genre = GenreLexeme::PLUS_UNAIRE;
		}
		else if (genre_operateur == GenreLexeme::MOINS) {
			lexeme->genre = GenreLexeme::MOINS_UNAIRE;
		}
		else if (!dls::outils::est_element(genre_operateur, GenreLexeme::TILDE, GenreLexeme::EXCLAMATION, GenreLexeme::PLUS_UNAIRE, GenreLexeme::MOINS_UNAIRE)) {
			erreur::lance_erreur(
						"La surcharge d'opérateur unaire n'est possible que pour '+', '-', '~', ou '!'",
						*m_unite->espace,
						noeud);
		}
	}

	consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' à la fin des paramètres de la fonction");

	/* analyse les types de retour de la fonction */
	consomme(GenreLexeme::RETOUR_TYPE, "Attendu un retour de type");

	while (true) {
		auto decl_sortie = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);

		if (!decl_sortie->est_decl_var()) {
			auto ident = m_compilatrice.table_identifiants->identifiant_pour_nouvelle_chaine(enchaine("__ret", noeud->params_sorties.taille()));

			auto ref = m_tacheronne.assembleuse->cree_ref_decl(decl_sortie->lexeme);
			ref->ident = ident;

			auto decl = m_tacheronne.assembleuse->cree_declaration(ref);
			decl->expression_type = decl_sortie;

			decl_sortie = decl;
		}

		decl_sortie->drapeaux |= EST_PARAMETRE;

		noeud->params_sorties.ajoute(decl_sortie->comme_decl_var());

		if (!apparie(GenreLexeme::VIRGULE)) {
			break;
		}

		consomme();
	}

	if (noeud->params_sorties.taille() > 1) {
		lance_erreur("Il est impossible d'avoir plusieurs de sortie pour un opérateur");
	}

	noeud->param_sortie = noeud->params_sorties[0];

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

	if (!dls::outils::possede_drapeau(noeud->drapeaux, FORCE_HORSLIGNE)) {
		noeud->drapeaux |= FORCE_ENLIGNE;
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

	while (apparie(GenreLexeme::AROBASE)) {
		consomme();

		if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
			lance_erreur("Attendu une chaine de caractère après '@'");
		}

		auto lexeme_annotation = lexeme_courant();
		noeud->annotations.ajoute(lexeme_annotation->chaine);

		consomme();
	}

	/* dépile le bloc des paramètres */
	m_tacheronne.assembleuse->depile_bloc();

	/* dépile le bloc des constantes */
	m_tacheronne.assembleuse->depile_bloc();

	depile_etat();

	return noeud;
}

template <typename  T>
static inline bool est_puissance_de_2(T x)
{
	return (x != 0) && (x & (x - 1)) == 0;
}

NoeudExpression *Syntaxeuse::analyse_declaration_structure(NoeudExpression *gauche)
{
	auto lexeme_mot_cle = lexeme_courant();
	empile_etat("dans le syntaxage de la structure", lexeme_mot_cle);
	consomme();

	auto noeud_decl = m_tacheronne.assembleuse->cree_struct(gauche->lexeme);
	noeud_decl->est_union = (lexeme_mot_cle->genre == GenreLexeme::UNION);
	noeud_decl->bloc_parent->membres->ajoute(noeud_decl);

	auto cree_tache = false;

	if (gauche->ident == ID::InfoType) {
		noeud_decl->type = m_unite->espace->typeuse.type_info_type_;
		auto type_info_type = m_unite->espace->typeuse.type_info_type_->comme_structure();
		type_info_type->decl = noeud_decl;
		type_info_type->nom = noeud_decl->ident;
	}
	else if (gauche->ident == ID::ContexteProgramme) {
		auto type_contexte = m_unite->espace->typeuse.type_contexte->comme_structure();
		noeud_decl->type = type_contexte;
		type_contexte->decl = noeud_decl;
		type_contexte->nom = noeud_decl->ident;
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
		noeud_decl->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(lexeme_courant());

		consomme();

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

			noeud_decl->params_polymorphiques.ajoute(decl_var);

			if (!apparie(GenreLexeme::VIRGULE)) {
				break;
			}

			consomme();
		}

		/* permet la déclaration de structures sans paramètres, pourtant ayant des parenthèse */
		if (noeud_decl->params_polymorphiques.taille() != 0) {
			noeud_decl->est_polymorphe = true;
			cree_tache = true;
		}

		consomme();
	}

	while (apparie(GenreLexeme::DIRECTIVE)) {
		consomme();

		auto ident_directive = lexeme_courant()->ident;

		if (ident_directive == ID::interface) {
			renseigne_type_interface(m_unite->espace->typeuse, noeud_decl->ident, noeud_decl->type);
			cree_tache = true;
		}
		else if (ident_directive == ID::externe) {
			noeud_decl->est_externe = true;
			// #externe implique nonsûr
			noeud_decl->est_nonsure = noeud_decl->est_union;
		}
		else if (ident_directive == ID::corps_texte) {
			noeud_decl->est_corps_texte = true;
		}
		else if (ident_directive == ID::compacte) {
			if (noeud_decl->est_union) {
				lance_erreur("Directive « compacte » impossible pour une union\n");
			}

			noeud_decl->est_compacte = true;
		}
		else if (ident_directive == ID::aligne) {
			consomme();

			if (!apparie(GenreLexeme::NOMBRE_ENTIER)) {
				lance_erreur("Un nombre entier est requis après « aligne »");
			}

			noeud_decl->alignement_desire = static_cast<uint32_t>(lexeme_courant()->valeur_entiere);

			if (!est_puissance_de_2(noeud_decl->alignement_desire)) {
				lance_erreur("Un alignement doit être une puissance de 2");
			}
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
		auto bloc = m_tacheronne.assembleuse->empile_bloc(lexeme_courant());
		consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après le nom de la structure");

		auto expressions = dls::tablet<NoeudExpression *, 16>();

		while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
			if (apparie(GenreLexeme::POINT_VIRGULE)) {
				consomme();
				continue;
			}

			if (apparie_expression()) {
				auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

				if (!noeud_decl->est_corps_texte && !noeud->est_declaration() && !noeud->est_assignation() && !noeud->est_empl()) {
					rapporte_erreur(m_unite->espace, noeud, "Attendu une déclaration ou une assignation dans le bloc de la structure");
				}

				expressions.ajoute(noeud);
			}
			else if (apparie_instruction()) {
				auto inst = analyse_instruction();
				expressions.ajoute(inst);
			}
			else {
				lance_erreur("attendu une expression ou une instruction");
			}
		}

		copie_tablet_tableau(expressions, *bloc->expressions.verrou_ecriture());

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

void Syntaxeuse::lance_erreur(const kuri::chaine &quoi, erreur::Genre genre)
{
	auto lexeme = lexeme_courant();
	auto fichier = m_unite->espace->fichier(lexeme->fichier);

	auto flux = dls::flux_chaine();

	flux << "\n";
	flux << fichier->chemin() << ':' << lexeme->ligne + 1 << " : erreur de syntaxage :\n";

	POUR (m_donnees_etat_syntaxage) {
		erreur::imprime_ligne_avec_message(flux, fichier, it.lexeme, it.message);
	}

	erreur::imprime_ligne_avec_message(flux, fichier, lexeme, quoi);

	throw erreur::frappe(flux.chn().c_str(), genre);
}

void Syntaxeuse::empile_etat(const char *message, Lexeme *lexeme)
{
	m_donnees_etat_syntaxage.ajoute({ lexeme, message });
}

void Syntaxeuse::depile_etat()
{
	m_donnees_etat_syntaxage.pop_back();
}
