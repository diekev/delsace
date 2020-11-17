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

#include "erreur.h"

#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/numerique.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "arbre_syntaxique.hh"
#include "compilatrice.hh"
#include "lexemes.hh"
#include "validation_semantique.hh"

namespace erreur {

const char *chaine_erreur(Genre genre)
{
	switch (genre) {
#define ENUMERE_GENRE_ERREUR_EX(genre) case Genre::genre: return #genre;
		ENUMERE_GENRES_ERREUR
#undef ENUMERE_GENRE_ERREUR_EX
	}
	return "Ceci ne devrait pas s'afficher";
}

std::ostream &operator<<(std::ostream &os, Genre genre)
{
	os << chaine_erreur(genre);
	return os;
}

void imprime_ligne_avec_message(
		dls::flux_chaine &flux,
		Fichier *fichier,
		Lexeme const *lexeme,
		const char *message)
{
	flux << fichier->chemin() << ':' << lexeme->ligne + 1 << ':' << lexeme->colonne + 1 << " : ";
	flux << message << "\n";

	auto nc = dls::num::nombre_de_chiffres(lexeme->ligne + 1);

	for (auto i = 0; i < 5 - nc; ++i) {
		flux << ' ';
	}

	flux << lexeme->ligne + 1 << " | " << fichier->tampon()[lexeme->ligne];
	flux << "      | ";

	lng::erreur::imprime_caractere_vide(flux, lexeme->colonne, fichier->tampon()[lexeme->ligne]);
	flux << '^';
	lng::erreur::imprime_tilde(flux, lexeme->chaine);
	flux << '\n';
}

void lance_erreur(
		const dls::chaine &quoi,
		EspaceDeTravail const &espace,
		const Lexeme *lexeme,
		Genre type)
{
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto const identifiant = lexeme->genre;
	auto const &chaine = lexeme->chaine;

	auto ligne_courante = fichier->tampon()[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	ss << quoi;
	ss << ", obtenu : " << chaine << " (" << chaine_du_genre_de_lexeme(identifiant) << ')';

	throw erreur::frappe(ss.chn().c_str(), type);
}

void redefinition_fonction(
		EspaceDeTravail const &espace,
		Lexeme const *lexeme_redefinition,
		Lexeme const *lexeme_original)
{
	auto fichier = espace.fichier(lexeme_redefinition->fichier);
	auto pos = position_lexeme(*lexeme_redefinition);
	auto pos_mot = pos.pos;
	auto chaine = lexeme_redefinition->chaine;

	auto ligne_courante = fichier->tampon()[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	ss << "Redéfinition de la fonction !\n";
	ss << "La fonction fût déjà définie ici :\n";

	fichier = espace.fichier(lexeme_original->fichier);
	pos = position_lexeme(*lexeme_original);
	pos_mot = pos.pos;
	chaine = lexeme_original->chaine;
	ligne_courante = fichier->tampon()[pos.index_ligne];

	ss << fichier->chemin() << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;
	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::FONCTION_REDEFINIE);
}

void redefinition_symbole(EspaceDeTravail const &espace, const Lexeme *lexeme_redefinition, const Lexeme *lexeme_original)
{
	auto fichier = espace.fichier(lexeme_redefinition->fichier);
	auto pos = position_lexeme(*lexeme_redefinition);
	auto pos_mot = pos.pos;
	auto chaine = lexeme_redefinition->chaine;

	auto ligne_courante = fichier->tampon()[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	ss << "Redéfinition du symbole !\n";
	ss << "Le symbole fût déjà défini ici :\n";

	fichier = espace.fichier(lexeme_original->fichier);
	pos = position_lexeme(*lexeme_original);
	pos_mot = pos.pos;
	chaine = lexeme_original->chaine;
	ligne_courante = fichier->tampon()[pos.index_ligne];

	ss << fichier->chemin() << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;
	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::VARIABLE_REDEFINIE);
}

[[noreturn]] void lance_erreur_type_arguments(
		const Type *type_arg,
		const Type *type_enf,
		EspaceDeTravail const &espace,
		const Lexeme *lexeme_enfant,
		const Lexeme *lexeme)
{
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << ":\n";
	ss << "Dans l'appel de la fonction '" << lexeme->chaine << "':\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, lexeme_enfant->chaine);
	ss << '\n';

	ss << "Le type de l'argument '" << lexeme_enfant->chaine << "' ne correspond pas à celui requis !\n";
	ss << "Requiers : " << chaine_type(type_arg) << '\n';
	ss << "Obtenu   : " << chaine_type(type_enf) << '\n';
	ss << '\n';
	ss << "Astuce :\n";
	ss << "Vous pouvez convertir le type en utilisant l'opérateur 'transtype', comme ceci :\n";

	lng::erreur::imprime_ligne_entre(ss, ligne, 0, pos_mot);
	ss << "transtype(" << lexeme_enfant->chaine << " : " << chaine_type(type_arg) << ")";
	lng::erreur::imprime_ligne_entre(ss, ligne, pos_mot + lexeme_enfant->chaine.taille(), ligne.taille());
	ss << "\n----------------------------------------------------------------\n";

	throw frappe(ss.chn().c_str(), Genre::TYPE_ARGUMENT);
}

[[noreturn]] void lance_erreur_type_retour(
		const Type *type_arg,
		const Type *type_enf,
		EspaceDeTravail const &espace,
		NoeudExpression *racine)
{
	auto inst = static_cast<NoeudExpressionUnaire *>(racine);
	auto lexeme = inst->expr->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];
	auto etendue = calcule_etendue_noeud(inst->expr, fichier);
	auto chaine_expr = dls::vue_chaine_compacte(&ligne[etendue.pos_min], etendue.pos_max - etendue.pos_min);

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << ":\n";
	ss << "Dans l'expression de retour de la fonction :\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, etendue.pos_min, ligne);
	lng::erreur::imprime_tilde(ss, ligne, etendue.pos_min, pos_mot);
	ss << '^';
	lng::erreur::imprime_tilde(ss, ligne, pos_mot + 1, etendue.pos_max);
	ss << '\n';

	ss << '\n';
	ss << "Le type de '" << chaine_expr << "' ne correspond pas à celui requis !\n";
	ss << "Requiers : " << chaine_type(type_arg) << '\n';
	ss << "Obtenu   : " << chaine_type(type_enf) << '\n';
	ss << '\n';
	ss << "Astuce :\n";
	ss << "Vous pouvez convertir le type en utilisant l'opérateur 'transtype', comme ceci :\n";

	lng::erreur::imprime_ligne_entre(ss, ligne, 0, etendue.pos_min);
	ss << "transtype(" << chaine_expr << " : " << chaine_type(type_arg) << ")";
	lng::erreur::imprime_ligne_entre(ss, ligne, etendue.pos_max, ligne.taille());
	ss << "\n----------------------------------------------------------------\n";

	throw frappe(ss.chn().c_str(), Genre::TYPE_DIFFERENTS);
}

[[noreturn]] void lance_erreur_assignation_type_differents(
		const Type *type_gauche,
		const Type *type_droite,
		EspaceDeTravail const &espace,
		const Lexeme *lexeme)
{
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << ":\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, lexeme->chaine);
	ss << '\n';

	ss << "Ne peut pas assigner des types différents !\n";
	ss << "Type à gauche : " << chaine_type(type_gauche) << '\n';
	ss << "Type à droite : " << chaine_type(type_droite) << '\n';

	throw frappe(ss.chn().c_str(), Genre::ASSIGNATION_MAUVAIS_TYPE);
}

void lance_erreur_type_operation(
		const Type *type_gauche,
		const Type *type_droite,
		EspaceDeTravail const &espace,
		const Lexeme *lexeme)
{
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << ":\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, lexeme->chaine);
	ss << '\n';

	ss << "Les types de l'opération sont différents !\n";
	ss << "Type à gauche : " << chaine_type(type_gauche) << '\n';
	ss << "Type à droite : " << chaine_type(type_droite) << '\n';

	throw frappe(ss.chn().c_str(), Genre::TYPE_DIFFERENTS);
}

void type_indexage(
		EspaceDeTravail const &espace,
		const NoeudExpression *noeud)
{
	auto lexeme = noeud->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << ":\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, lexeme->chaine);
	ss << '\n';

	ss << "Le type de l'expression d'indexage est invalide !\n";
	ss << "Requiers : z64\n";
	ss << "Obtenu   : " << chaine_type(noeud->type) << '\n';

	throw frappe(ss.chn().c_str(), Genre::TYPE_DIFFERENTS);
}

void lance_erreur_fonction_inconnue(
		EspaceDeTravail const &espace,
		NoeudExpression *b,
		dls::tablet<DonneesCandidate, 10> const &candidates)
{
	auto const &lexeme = b->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << '\n';
	ss << "\nDans l'appel de la fonction '" << b->lexeme->chaine << "'\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, lexeme->chaine);
	ss << '\n';

	if (candidates.est_vide()) {
		ss << "\nFonction inconnue : aucune candidate trouvée\n";
		ss << "Vérifiez que la fonction existe bel et bien dans un fichier importé\n";

		throw erreur::frappe(ss.chn().c_str(), erreur::Genre::FONCTION_INCONNUE);
	}

	ss << "\nAucune candidate trouvée pour la fonction '" << b->lexeme->chaine << "'\n";

	auto type_erreur = erreur::Genre::FONCTION_INCONNUE;

	for (auto &dc : candidates) {
		auto decl = dc.noeud_decl;
		ss << "\nCandidate :";

		if (decl != nullptr) {
			auto const &lexeme_df = decl->lexeme;
			auto fichier_df = espace.fichier(lexeme_df->fichier);
			auto pos_df = position_lexeme(*lexeme_df);

			ss << ' ' << decl->ident->nom
			   << " (trouvée à " << fichier_df->chemin() << ':' << pos_df.numero_ligne << ")\n";
		}
		else {
			ss << '\n';
		}

		if (dc.raison == MECOMPTAGE_ARGS) {
			auto noeud_appel = static_cast<NoeudExpressionAppel *>(b);
			ss << "\tLe nombre d'arguments de la fonction est incorrect.\n";

			if (decl && decl->genre == GenreNoeud::DECLARATION_CORPS_FONCTION) {
				auto decl_fonc = decl->comme_entete_fonction();
				ss << "\tRequiers " << decl_fonc->params.taille << " arguments\n";
			}
			else if (decl && decl->genre == GenreNoeud::DECLARATION_STRUCTURE) {
				auto type_struct = static_cast<NoeudStruct const *>(decl)->type->comme_structure();
				ss << "\tRequiers " << type_struct->membres.taille << " arguments\n";
			}
			else {
				auto type_fonc = dc.type->comme_fonction();
				ss << "\tRequiers " << type_fonc->types_entrees.taille - dc.requiers_contexte << " arguments\n";
			}

			ss << "\tObtenu " << noeud_appel->params.taille << " arguments\n";
			type_erreur = erreur::Genre::NOMBRE_ARGUMENT;
		}

		if (dc.raison == MENOMMAGE_ARG) {
			imprime_ligne_avec_message(ss, fichier, dc.noeud_erreur->lexeme, "Argument inconnu");

			if (decl->genre == GenreNoeud::DECLARATION_CORPS_FONCTION) {
				auto decl_fonc = decl->comme_entete_fonction();
				ss << "\tLes arguments de la fonction sont : \n";

				for (auto i = 0; i < decl_fonc->params.taille; ++i) {
					auto param = decl_fonc->parametre_entree(i);
					ss << "\t\t" << param->ident->nom << '\n';
				}

				type_erreur = erreur::Genre::ARGUMENT_INCONNU;
			}
			else if (decl->genre == GenreNoeud::DECLARATION_STRUCTURE) {
				auto type_struct = static_cast<NoeudStruct const *>(decl)->type->comme_structure();
				ss << "\tLes membres de la structure sont : \n";

				POUR (type_struct->membres) {
					ss << "\t\t" << it.nom << '\n';
				}

				type_erreur = erreur::Genre::MEMBRE_INCONNU;
			}
		}

		if (dc.raison == RENOMMAGE_ARG) {
			type_erreur = erreur::Genre::ARGUMENT_REDEFINI;
			imprime_ligne_avec_message(ss, fichier, dc.noeud_erreur->lexeme, "L'argument a déjà été nommé");
		}

		if (dc.raison == MANQUE_NOM_APRES_VARIADIC) {
			type_erreur = erreur::Genre::ARGUMENT_INCONNU;
			imprime_ligne_avec_message(ss, fichier, dc.noeud_erreur->lexeme, "Nom d'argument manquant, les arguments doivent être nommés s'ils sont précédés d'arguments déjà nommés");
		}

		if (dc.raison == NOMMAGE_ARG_POINTEUR_FONCTION) {
			ss << "\tLes arguments d'un pointeur fonction ne peuvent être nommés\n";
			type_erreur = erreur::Genre::ARGUMENT_INCONNU;
		}

		if (dc.raison == TYPE_N_EST_PAS_FONCTION) {
			ss << "\tAppel d'une variable n'étant pas un pointeur de fonction\n";
			type_erreur = erreur::Genre::FONCTION_INCONNUE;
		}

		if (dc.raison == TROP_D_EXPRESSION_POUR_UNION) {
			ss << "\tOn ne peut initialiser qu'un seul membre d'une union à la fois\n";
			type_erreur = erreur::Genre::NORMAL;
		}

		if (dc.raison == EXPRESSION_MANQUANTE_POUR_UNION) {
			ss << "\tOn doit initialiser au moins un membre de l'union\n";
			type_erreur = erreur::Genre::NORMAL;
		}

		if (dc.raison == NOM_ARGUMENT_REQUIS) {
			ss << "\tLe nom de l'argument est requis pour les constructions de structures\n";
			type_erreur = erreur::Genre::MEMBRE_INCONNU;
		}

		if (dc.raison == CONTEXTE_MANQUANT) {
			ss << "\tNe peut appeler une fonction avec contexte dans un bloc n'ayant pas de contexte\n";
			type_erreur = erreur::Genre::NORMAL;
		}
		else if (dc.raison == EXPANSION_VARIADIQUE_FONCTION_EXTERNE) {
			ss << "\tImpossible d'utiliser une expansion variadique dans une fonction variadique externe\n";
			type_erreur = erreur::Genre::NORMAL;
		}
		else if (dc.raison == MULTIPLE_EXPANSIONS_VARIADIQUES) {
			ss << "\tPlusieurs expansions variadiques trouvées\n";
			type_erreur = erreur::Genre::NORMAL;
		}
		else if (dc.raison == EXPANSION_VARIADIQUE_APRES_ARGUMENTS_VARIADIQUES) {
			ss << "\tTentative d'utiliser une expansion d'arguments variadiques alors que d'autres arguments ont déjà été précisés\n";
			type_erreur = erreur::Genre::NORMAL;
		}
		else if (dc.raison == ARGUMENTS_VARIADIQEUS_APRES_EXPANSION_VARIAQUES) {
			ss << "\tTentative d'ajouter des arguments variadiques supplémentaire alors qu'une expansion est également utilisée\n";
			type_erreur = erreur::Genre::NORMAL;
		}
		else if (dc.raison == ARGUMENTS_MANQUANTS) {
			if (dc.arguments_manquants.taille() == 1) {
				ss << "\tUn argument est manquant :\n";
			}
			else {
				ss << "\tPlusieurs arguments sont manquants :\n";
			}

			for (auto ident : dc.arguments_manquants) {
				ss << "\t\t" << ident->nom << '\n';
			}
		}
		else if (dc.raison == IMPOSSIBLE_DE_DEFINIR_UN_TYPE_POLYMORPHIQUE) {
			ss << "\tImpossible de définir le type polymorphique " << dc.ident_poly_manquant->nom << '\n';
		}

		if (dc.raison == METYPAGE_ARG) {
			auto const &lexeme_enfant = dc.noeud_erreur->lexeme;

			ss << "\tLe type de l'argument '" << lexeme_enfant->chaine << "' ne correspond pas à celui requis !\n";
			ss << "\tRequiers : " << chaine_type(dc.type_attendu) << '\n';
			ss << "\tObtenu   : " << chaine_type(dc.type_obtenu) << '\n';
			type_erreur = erreur::Genre::TYPE_ARGUMENT;
		}
	}
	ss << "\n----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), type_erreur);
}

void lance_erreur_fonction_nulctx(
			EspaceDeTravail const &espace,
			NoeudExpression const *appl_fonc,
			NoeudExpression const *decl_fonc,
			NoeudExpression const *decl_appel)
{
	auto const &lexeme = appl_fonc->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << '\n';
	ss << "\nDans l'appel de la fonction « " << appl_fonc->lexeme->chaine << " »\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, lexeme->chaine);
	ss << '\n';

	ss << "Ne peut appeler une fonction avec contexte dans une fonction sans contexte !\n";

	ss << "Note : la fonction est appelée dans « " << decl_fonc->ident->nom << " » "
	   << " qui a été déclarée sans contexte via #!nulctx.\n";

	ss << "\n« " << decl_fonc->ident->nom << " » est déclarée ici :\n";
	fichier = espace.fichier(decl_fonc->lexeme->fichier);
	auto pos_decl = position_lexeme(*decl_fonc->lexeme);
	ss << fichier->chemin() << ':' << pos_decl.numero_ligne << '\n' << '\n';
	ss << fichier->tampon()[pos_decl.index_ligne];

	ss << "\n« " << decl_appel->ident->nom << " » est déclarée ici :\n";
	fichier = espace.fichier(decl_appel->lexeme->fichier);
	auto pos_appel = position_lexeme(*decl_appel->lexeme);
	ss << fichier->chemin() << ':' << pos_appel.numero_ligne << '\n' << '\n';
	ss << fichier->tampon()[pos_appel.index_ligne];

	ss << "\n----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::APPEL_INVALIDE);
}

void lance_erreur_acces_hors_limites(
			EspaceDeTravail const &espace,
			NoeudExpression *b,
			long taille_tableau,
			Type *type_tableau,
			long index_acces)
{
	auto const &lexeme = b->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << '\n';
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, lexeme->chaine);
	ss << '\n';

	ss << "Accès au tableau hors de ses limites !\n";

	ss << "\tLe tableau a une taille de " << taille_tableau << " (de type : "
	   << chaine_type(type_tableau) << ").\n";
	ss << "\tL'accès se fait à l'index " << index_acces << " (index maximal : " << taille_tableau - 1 << ").\n";

	ss << "\n----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::NORMAL);
}

void lance_erreur_type_operation(
			EspaceDeTravail const &espace,
			NoeudExpression *b)
{
	// soit l'opérateur n'a pas de surcharge (le typage n'est pas bon)
	// soit l'opérateur n'est pas commutatif
	// soit l'opérateur n'est pas défini pour le type

	auto const &lexeme = b->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	auto etendue = calcule_etendue_noeud(b, fichier);

	auto inst = static_cast<NoeudExpressionBinaire *>(b);
	auto enfant_gauche = inst->expr1;
	auto enfant_droite = inst->expr2;

	auto const &type_gauche = enfant_gauche->type;
	auto const &type_droite = enfant_droite->type;

	auto etendue_gauche = calcule_etendue_noeud(enfant_gauche, fichier);
	auto etendue_droite = calcule_etendue_noeud(enfant_droite, fichier);

	auto expr_gauche = dls::vue_chaine_compacte(&ligne[etendue_gauche.pos_min], etendue_gauche.pos_max - etendue_gauche.pos_min);
	auto expr_droite = dls::vue_chaine_compacte(&ligne[etendue_droite.pos_min], etendue_droite.pos_max - etendue_droite.pos_min);

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << '\n';
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, etendue.pos_min, ligne);
	lng::erreur::imprime_tilde(ss, ligne, etendue.pos_min, pos_mot);
	ss << '^';
	lng::erreur::imprime_tilde(ss, ligne, pos_mot + 1, etendue.pos_max);
	ss << '\n';

	ss << "Aucun opérateur trouvé pour l'opération !\n";

	if (type_droite != type_gauche) {
		ss << '\n';
		ss << "Veuillez vous assurer que les types correspondent.\n";
		ss << '\n';
		ss << "L'expression à gauche de l'opérateur, " << expr_gauche << ", est de type : ";
		ss << chaine_type(type_gauche) << '\n';
		ss << "L'expression à droite de l'opérateur, " << expr_droite << ", est de type : ";
		ss << chaine_type(type_droite) << '\n';
		ss << '\n';
		ss << "Pour résoudre ce problème, vous pouvez par exemple transtyper l'une des deux expressions :\n";
		ss << "transtype(" << expr_gauche << " : " << chaine_type(type_droite) << ")\n";
		ss << "ou\n";
		ss << "transtype(" << expr_droite << " : " << chaine_type(type_gauche) << ")\n";
	}
	else {
		ss << "Note : les variables sont de type : " << chaine_type(type_droite) << '\n';
	}

	ss << "----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::TYPE_DIFFERENTS);
}

void lance_erreur_type_operation_unaire(
			EspaceDeTravail const &espace,
			NoeudExpression *b)
{
	// soit l'opérateur n'a pas de surcharge (le typage n'est pas bon)
	// soit l'opérateur n'est pas commutatif
	// soit l'opérateur n'est pas défini pour le type

	auto const &lexeme = b->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	auto inst = static_cast<NoeudExpressionUnaire *>(b);
	auto etendue = calcule_etendue_noeud(inst, fichier);

	auto enfant_droite = inst->expr;
	auto const &type_droite = enfant_droite->type;
	auto etendue_droite = calcule_etendue_noeud(enfant_droite, fichier);
	auto expr_droite = dls::vue_chaine_compacte(&ligne[etendue_droite.pos_min], etendue_droite.pos_max - etendue_droite.pos_min);

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << '\n';
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, ligne, pos_mot + 1, etendue.pos_max);
	ss << '\n';

	ss << "Aucun opérateur trouvé pour l'opération !\n";
	ss << "Veuillez vous assurer que les types correspondent.\n";
	ss << '\n';
	ss << "L'expression à droite de l'opérateur, " << expr_droite << ", est de type : ";
	ss << chaine_type(type_droite) << '\n';
	ss << "----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::TYPE_DIFFERENTS);
}

struct CandidatMembre {
	long distance = 0;
	dls::vue_chaine_compacte chaine = "";
};

static auto trouve_candidat(
			dls::ensemble<dls::vue_chaine_compacte> const &membres,
			dls::vue_chaine_compacte const &nom_donne)
{
	auto candidat = CandidatMembre{};
	candidat.distance = 1000;

	for (auto const &nom_membre : membres) {
		auto candidat_possible = CandidatMembre();
		candidat_possible.distance = distance_levenshtein(nom_donne, nom_membre);
		candidat_possible.chaine = nom_membre;

		if (candidat_possible.distance < candidat.distance) {
			candidat = candidat_possible;
		}
	}

	return candidat;
}

[[noreturn]] static void genere_erreur_membre_inconnu(
			EspaceDeTravail const &espace,
			NoeudExpression *acces,
			NoeudExpression *structure,
			NoeudExpression *membre,
			dls::ensemble<dls::vue_chaine_compacte> const &membres,
			const char *chaine_structure)
{
	auto candidat = trouve_candidat(membres, membre->ident->nom);

	auto const &lexeme = acces->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	auto etendue = calcule_etendue_noeud(acces, fichier);

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << '\n' << '\n';

	if (structure->genre == GenreNoeud::EXPRESSION_REFERENCE_MEMBRE) {
		auto noeud = static_cast<NoeudExpressionMembre *>(structure);
		structure = noeud->membre;
	}

	ss << "Dans l'expression d'accès de membre :\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, etendue.pos_min, ligne);
	lng::erreur::imprime_tilde(ss, ligne, etendue.pos_min, pos_mot);
	ss << '^';
	lng::erreur::imprime_tilde(ss, ligne, pos_mot + 1, etendue.pos_max);
	ss << '\n';

	ss << '\n';
	ss << "Le membre « " << membre->ident->nom << " » est inconnu !\n";

	ss << '\n';
	ss << "Les membres " << chaine_structure << " sont :\n";

	for (auto nom : membres) {
		ss << '\t' << nom << '\n';
	}

	ss << '\n';
	ss << "Candidat possible : " << candidat.chaine << '\n';
	ss << "----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::MEMBRE_INCONNU);
}

void membre_inconnu(
			EspaceDeTravail const &espace,
			NoeudExpression *acces,
			NoeudExpression *structure,
			NoeudExpression *membre,
			TypeCompose *type)
{
	auto membres = dls::ensemble<dls::vue_chaine_compacte>();

	POUR (type->membres) {
		membres.insere(it.nom);
	}

	const char *message;

	if (type->genre == GenreType::ENUM) {
		message = "de l'énumération";
	}
	else if (type->genre == GenreType::UNION) {
		message = "de l'union";
	}
	else if (type->genre == GenreType::ERREUR) {
		message = "de l'erreur";
	}
	else {
		message = "de la structure";
	}

	genere_erreur_membre_inconnu(espace, acces, structure, membre, membres, message);
}

void membre_inactif(
			EspaceDeTravail const &espace,
			ContexteValidationCode &contexte,
			NoeudExpression *acces,
			NoeudExpression *structure,
			NoeudExpression *membre)
{
	auto const &lexeme = acces->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	auto etendue = calcule_etendue_noeud(acces, fichier);

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << '\n' << '\n';
	ss << "Dans l'accès à « " << structure->ident->nom << " » :\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, etendue.pos_min, ligne);
	lng::erreur::imprime_tilde(ss, ligne, etendue.pos_min, pos_mot);
	ss << '^';
	lng::erreur::imprime_tilde(ss, ligne, pos_mot + 1, etendue.pos_max);
	ss << '\n';

	ss << '\n';
	ss << "Le membre « " << membre->ident->nom << " » est inactif dans ce contexte !\n";
	ss << "Le membre actif dans ce contexte est « " << contexte.trouve_membre_actif(structure->ident->nom) << " ».\n";
	ss << "----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::MEMBRE_INACTIF);
}

void valeur_manquante_discr(
			EspaceDeTravail const &espace,
			NoeudExpression *expression,
			dls::ensemble<dls::vue_chaine_compacte> const &valeurs_manquantes)
{
	auto const &lexeme = expression->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];

	auto etendue = calcule_etendue_noeud(expression, fichier);

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin() << ':' << pos.numero_ligne << '\n' << '\n';
	ss << "Dans la discrimination de « ";
	ss << dls::vue_chaine(ligne.begin() + etendue.pos_min, etendue.pos_max - etendue.pos_min);
	ss << " » :\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, etendue.pos_min, ligne);
	lng::erreur::imprime_tilde(ss, ligne, etendue.pos_min, pos_mot);
	ss << '^';
	lng::erreur::imprime_tilde(ss, ligne, pos_mot + 1, etendue.pos_max);
	ss << '\n';

	ss << '\n';

	if (valeurs_manquantes.taille() == 1) {
		ss << "Une valeur n'est pas prise en compte :\n";
	}
	else {
		ss << "Plusieurs valeurs ne sont pas prises en compte :\n";
	}

	for (auto const &valeur : valeurs_manquantes) {
		ss << '\t' << valeur << '\n';
	}

	ss << "----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::MEMBRE_INACTIF);
}

void fonction_principale_manquante(EspaceDeTravail const &espace)
{
	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Dans l'espace de travail « " << espace.nom << " » :\n";
	ss << "Erreur : impossible de trouver la fonction principale\n";
	ss << "Veuillez vérifier qu'elle soit bien présente dans un module\n";
	ss << "\n----------------------------------------------------------------\n";
	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::MEMBRE_INACTIF);
}

}

/* ************************************************************************** */

Erreur::Erreur(EspaceDeTravail *espace_)
	: espace(espace_)
{}

Erreur::~Erreur() noexcept(false)
{
	throw erreur::frappe(message.c_str(), erreur::Genre::NORMAL);
}

Erreur &Erreur::ajoute_message(const dls::chaine &m)
{
	message += m;
	return *this;
}

Erreur &Erreur::ajoute_site(NoeudExpression *site)
{
	assert(espace);

	auto fichier = espace->fichier(site->lexeme->fichier);
	auto flux = dls::flux_chaine();
	flux << message;

	erreur::imprime_ligne_avec_message(flux, fichier, site->lexeme, "");
	flux << '\n';

	message = flux.chn();
	return *this;
}

Erreur &Erreur::ajoute_conseil(const dls::chaine &c)
{
	auto flux = dls::flux_chaine();
	flux << message;
	flux << "\033[4mConseil\033[00m : " << c;

	message = flux.chn();
	return *this;
}

static dls::chaine chaine_pour_erreur(erreur::Genre genre)
{
	switch (genre) {
		default:
		{
			return "ERREUR";
		}
		case erreur::Genre::LEXAGE:
		{
			return "ERREUR DE LEXAGE";
		}
		case erreur::Genre::SYNTAXAGE:
		{
			return "ERREUR DE LEXAGE";
		}
		case erreur::Genre::TYPE_INCONNU:
		case erreur::Genre::TYPE_DIFFERENTS:
		case erreur::Genre::TYPE_ARGUMENT:
		{
			return "ERREUR DE TYPAGE";
		}
	}

	return "ERREUR";
}

#define COULEUR_NORMALE "\033[0m"
#define COULEUR_CYAN_GRAS "\033[1;36m"

Erreur rapporte_erreur(EspaceDeTravail *espace, NoeudExpression *site, const dls::chaine &message, erreur::Genre genre)
{
	auto fichier = espace->fichier(site->lexeme->fichier);

	auto flux = dls::flux_chaine();
	flux << COULEUR_CYAN_GRAS << "-- ";

	auto chaine_erreur = chaine_pour_erreur(genre);
	flux << chaine_erreur << ' ';

	for (auto i = 0; i < 76 - chaine_erreur.taille(); ++i) {
		flux << '-';
	}

	flux << "\n\n" << COULEUR_NORMALE;

	flux << "Dans l'espace de travail \"" << espace->nom << "\" :\n";
	flux << "\nErreur : ";
	erreur::imprime_ligne_avec_message(flux, fichier, site->lexeme, "");
	flux << '\n';
	flux << message;
	flux << '\n';
	flux << '\n';

	auto erreur = Erreur(espace);
	erreur.message = flux.chn();
	return erreur;
}

Erreur rapporte_erreur_sans_site(EspaceDeTravail *espace, const dls::chaine &message, erreur::Genre genre)
{
	auto flux = dls::flux_chaine();
	flux << COULEUR_CYAN_GRAS << "-- ";

	auto chaine_erreur = chaine_pour_erreur(genre);
	flux << chaine_erreur << ' ';

	for (auto i = 0; i < 76 - chaine_erreur.taille(); ++i) {
		flux << '-';
	}

	flux << "\n\n" << COULEUR_NORMALE;

	flux << "Dans l'espace de travail \"" << espace->nom << "\" :\n";
	flux << "\nErreur : ";
	flux << message;
	flux << '\n';
	flux << '\n';

	auto erreur = Erreur(espace);
	erreur.message = flux.chn();
	return erreur;
}

Erreur rapporte_erreur(EspaceDeTravail *espace, kuri::chaine fichier, int ligne, kuri::chaine message)
{
	auto flux = dls::flux_chaine();
	flux << COULEUR_CYAN_GRAS << "-- ";

	auto chaine_erreur = dls::chaine("ERREUR");
	flux << chaine_erreur << ' ';

	for (auto i = 0; i < 76 - chaine_erreur.taille(); ++i) {
		flux << '-';
	}

	flux << "\n\n" << COULEUR_NORMALE;

	const Fichier *f = espace->fichier({ fichier.pointeur, fichier.taille });

	flux << "Dans l'espace de travail \"" << espace->nom << "\" :\n";
	flux << "\nErreur : " << f->chemin() << ":" << ligne << ":\n";
	flux << f->tampon()[ligne - 1];
	flux << '\n';
	flux << message;
	flux << '\n';
	flux << '\n';

	auto erreur = Erreur(espace);
	erreur.message = flux.chn();
	return erreur;
}
