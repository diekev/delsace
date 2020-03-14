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
#include "biblinternes/structures/flux_chaine.hh"

#include "arbre_syntactic.h"
#include "contexte_generation_code.h"
#include "lexemes.hh"

namespace lng::erreur {

static void imprime_tilde(dls::flux_chaine &ss, dls::vue_chaine_compacte chaine)
{
	imprime_tilde(ss, dls::vue_chaine(chaine.pointeur(), chaine.taille()));
}

static void imprime_tilde(
		dls::flux_chaine &ss,
		dls::vue_chaine_compacte chaine,
		long debut,
		long fin)
{
	imprime_tilde(ss, dls::vue_chaine(chaine.pointeur(), chaine.taille()), debut, fin);
}

}

namespace erreur {

void lance_erreur(
		const dls::chaine &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme *lexeme,
		type_erreur type)
{
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto const identifiant = lexeme->genre;
	auto const &chaine = lexeme->chaine;

	auto ligne_courante = fichier->tampon[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	ss << quoi;
	ss << ", obtenu : " << chaine << " (" << chaine_identifiant(identifiant) << ')';

	throw erreur::frappe(ss.chn().c_str(), type);
}

void redefinition_fonction(
		const ContexteGenerationCode &contexte,
		DonneesLexeme const *lexeme_redefition,
		DonneesLexeme const *lexeme_original)
{
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme_redefition->fichier));
	auto pos = trouve_position(*lexeme_redefition, fichier);
	auto pos_mot = pos.pos;
	auto chaine = lexeme_redefition->chaine;

	auto ligne_courante = fichier->tampon[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	ss << "Redéfinition de la fonction !\n";
	ss << "La fonction fût déjà définie ici :\n";

	fichier = contexte.fichier(static_cast<size_t>(lexeme_original->fichier));
	pos = trouve_position(*lexeme_original, fichier);
	pos_mot = pos.pos;
	chaine = lexeme_original->chaine;
	ligne_courante = fichier->tampon[pos.index_ligne];

	ss << fichier->chemin << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;
	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::FONCTION_REDEFINIE);
}

void redefinition_symbole(const ContexteGenerationCode &contexte, const DonneesLexeme *lexeme_redefition, const DonneesLexeme *lexeme_original)
{
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme_redefition->fichier));
	auto pos = trouve_position(*lexeme_redefition, fichier);
	auto pos_mot = pos.pos;
	auto chaine = lexeme_redefition->chaine;

	auto ligne_courante = fichier->tampon[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	ss << "Redéfinition du symbole !\n";
	ss << "Le symbole fût déjà défini ici :\n";

	fichier = contexte.fichier(static_cast<size_t>(lexeme_original->fichier));
	pos = trouve_position(*lexeme_original, fichier);
	pos_mot = pos.pos;
	chaine = lexeme_original->chaine;
	ligne_courante = fichier->tampon[pos.index_ligne];

	ss << fichier->chemin << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;
	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::VARIABLE_REDEFINIE);
}

void lance_erreur_plage(
		const dls::chaine &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme *premier_lexeme,
		const DonneesLexeme *dernier_lexeme,
		type_erreur type)
{
	auto fichier = contexte.fichier(static_cast<size_t>(premier_lexeme->fichier));
	auto pos = trouve_position(*premier_lexeme, fichier);
	auto const pos_premier = pos.pos;

	auto fichier_dernier = contexte.fichier(static_cast<size_t>(dernier_lexeme->fichier));
	auto const pos_dernier = trouve_position(*premier_lexeme, fichier_dernier).pos;

	auto ligne_courante = fichier->tampon[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_premier, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, ligne_courante, pos_premier, pos_dernier + 1);
	ss << '\n';

	ss << quoi;

	throw erreur::frappe(ss.chn().c_str(), type);
}

[[noreturn]] void lance_erreur_type_arguments(
		const Type *type_arg,
		const Type *type_enf,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme *lexeme_enfant,
		const DonneesLexeme *lexeme)
{
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << ":\n";
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

	throw frappe(ss.chn().c_str(), type_erreur::TYPE_ARGUMENT);
}

[[noreturn]] void lance_erreur_type_retour(
		const Type *type_arg,
		const Type *type_enf,
		const ContexteGenerationCode &contexte,
		NoeudBase *racine)
{
	auto inst = static_cast<NoeudExpressionUnaire *>(racine);
	auto lexeme = inst->expr->lexeme;
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];
	auto etendue = calcule_etendue_noeud(inst->expr, fichier);
	auto chaine_expr = dls::vue_chaine_compacte(&ligne[etendue.pos_min], etendue.pos_max - etendue.pos_min);

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << ":\n";
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

	throw frappe(ss.chn().c_str(), type_erreur::TYPE_DIFFERENTS);
}

[[noreturn]] void lance_erreur_assignation_type_differents(
		const Type *type_gauche,
		const Type *type_droite,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme *lexeme)
{
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << ":\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, lexeme->chaine);
	ss << '\n';

	ss << "Ne peut pas assigner des types différents !\n";
	ss << "Type à gauche : " << chaine_type(type_gauche) << '\n';
	ss << "Type à droite : " << chaine_type(type_droite) << '\n';

	throw frappe(ss.chn().c_str(), type_erreur::ASSIGNATION_MAUVAIS_TYPE);
}

void lance_erreur_type_operation(
		const Type *type_gauche,
		const Type *type_droite,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme *lexeme)
{
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << ":\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, lexeme->chaine);
	ss << '\n';

	ss << "Les types de l'opération sont différents !\n";
	ss << "Type à gauche : " << chaine_type(type_gauche) << '\n';
	ss << "Type à droite : " << chaine_type(type_droite) << '\n';

	throw frappe(ss.chn().c_str(), type_erreur::TYPE_DIFFERENTS);
}

void lance_erreur_fonction_inconnue(
		ContexteGenerationCode const &contexte,
		NoeudBase *b,
		dls::tablet<DonneesCandidate, 10> const &candidates)
{
	auto const &lexeme = b->lexeme;
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << '\n';
	ss << "\nDans l'appel de la fonction '" << b->lexeme->chaine << "'\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, lexeme->chaine);
	ss << '\n';

	if (candidates.est_vide()) {
		ss << "\nFonction inconnue : aucune candidate trouvée\n";
		ss << "Vérifiez que la fonction existe bel et bien dans un fichier importé\n";

		throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::FONCTION_INCONNUE);
	}

	ss << "\nAucune candidate trouvée pour la fonction '" << b->lexeme->chaine << "'\n";

	auto type_erreur = erreur::type_erreur::FONCTION_INCONNUE;

	for (auto &dc : candidates) {
		auto decl_fonc = dc.decl_fonc;
		ss << "\nCandidate :";

		if (decl_fonc != nullptr) {
			auto const &lexeme_df = decl_fonc->lexeme;
			auto fichier_df = contexte.fichier(static_cast<size_t>(lexeme_df->fichier));
			auto pos_df = trouve_position(*lexeme_df, fichier_df);

			ss << ' ' << decl_fonc->ident->nom
			   << " (trouvée à " << fichier_df->chemin << ':' << pos_df.numero_ligne << ")\n";
		}
		else {
			ss << '\n';
		}

		if (dc.raison == MECOMPTAGE_ARGS) {
			auto noeud_appel = static_cast<NoeudExpressionAppel *>(b);
			ss << "\tLe nombre d'arguments de la fonction est incorrect.\n";
			ss << "\tRequiers " << decl_fonc->params.taille << " arguments\n";
			ss << "\tObtenu " << noeud_appel->params.taille << " arguments\n";
			type_erreur = erreur::type_erreur::NOMBRE_ARGUMENT;
		}

		if (dc.raison == MENOMMAGE_ARG) {
			/* À FAIRE : trouve le lexeme correspondant à l'argument. */
			ss << "\tArgument '" << dc.nom_arg << "' inconnu !\n";
			ss << "\tLes arguments de la fonction sont : \n";

			POUR (decl_fonc->params) {
				ss << "\t\t" << it->ident->nom << '\n';
			}

			type_erreur = erreur::type_erreur::ARGUMENT_INCONNU;
		}

		if (dc.raison == RENOMMAGE_ARG) {
			/* À FAIRE : trouve le lexeme correspondant à l'argument. */
			ss << "\tL'argument '" << dc.nom_arg << "' a déjà été nommé\n";
			type_erreur = erreur::type_erreur::ARGUMENT_REDEFINI;
		}

		if (dc.raison == MANQUE_NOM_APRES_VARIADIC) {
			/* À FAIRE : trouve le lexeme correspondant à l'argument. */
			ss << "\tNom d'argument manquant\n";
			ss << "\tLes arguments doivent être nommés s'ils sont précédés d'arguments déjà nommés\n";
			type_erreur = erreur::type_erreur::ARGUMENT_INCONNU;
		}

		if (dc.raison == METYPAGE_ARG) {
			auto const &lexeme_enfant = dc.noeud_decl->lexeme;

			ss << "\tLe type de l'argument '" << lexeme_enfant->chaine << "' ne correspond pas à celui requis !\n";
			ss << "\tRequiers : " << chaine_type(dc.type1) << '\n';
			ss << "\tObtenu   : " << chaine_type(dc.type2) << '\n';
			/* À FAIRE */
//			ss << '\n';
//			ss << "Astuce :\n";
//			ss << "Vous pouvez convertir le type en utilisant l'opérateur 'transtype', comme ceci :\n";

//			imprime_ligne_entre(ss, ligne, 0, pos_mot);
//			ss << "transtype(" << lexeme_enfant->chaine << " : " << dc.type1 << ")";
//			imprime_ligne_entre(ss, ligne, pos_mot + lexeme_enfant->chaine.taille(), ligne.taille());
			type_erreur = erreur::type_erreur::TYPE_ARGUMENT;
		}

#ifdef NON_SUR
		if (candidate->arg_pointeur && !contexte.non_sur()) {
			/* À FAIRE : trouve le lexeme correspondant à l'argument. */
			ss << "\tNe peut appeler une fonction avec un argument pointé hors d'un bloc 'nonsûr'\n"
			type_erreur = erreur::type_erreur::APPEL_INVALIDE
		}

#endif
	}
	ss << "\n----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), type_erreur);
}

void lance_erreur_fonction_nulctx(
			ContexteGenerationCode const &contexte,
			NoeudBase *appl_fonc,
			NoeudBase *decl_fonc,
			NoeudBase *decl_appel)
{
	auto const &lexeme = appl_fonc->lexeme;
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << '\n';
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
	fichier = contexte.fichier(static_cast<size_t>(decl_fonc->lexeme->fichier));
	auto pos_decl = trouve_position(*decl_fonc->lexeme, fichier);
	ss << fichier->chemin << ':' << pos_decl.numero_ligne << '\n' << '\n';
	ss << fichier->tampon[pos_decl.index_ligne];

	ss << "\n« " << appl_fonc->ident->nom << " » est déclarée ici :\n";
	fichier = contexte.fichier(static_cast<size_t>(decl_appel->lexeme->fichier));
	auto pos_appel = trouve_position(*decl_appel->lexeme, fichier);
	ss << fichier->chemin << ':' << pos_appel.numero_ligne << '\n' << '\n';
	ss << fichier->tampon[pos_appel.index_ligne];

	ss << "\n----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::APPEL_INVALIDE);
}

void lance_erreur_acces_hors_limites(
			ContexteGenerationCode const &contexte,
			NoeudBase *b,
			long taille_tableau,
			Type *type_tableau,
			long index_acces)
{
	auto const &lexeme = b->lexeme;
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << '\n';
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

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::NORMAL);
}

void lance_erreur_type_operation(
			ContexteGenerationCode const &contexte,
			NoeudBase *b)
{
	// soit l'opérateur n'a pas de surcharge (le typage n'est pas bon)
	// soit l'opérateur n'est pas commutatif
	// soit l'opérateur n'est pas défini pour le type

	auto const &lexeme = b->lexeme;
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	auto etendue = calcule_etendue_noeud(static_cast<NoeudExpression *>(b), fichier);

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
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << '\n';
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

	ss << "----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::TYPE_DIFFERENTS);
}

void lance_erreur_type_operation_unaire(
			ContexteGenerationCode const &contexte,
			NoeudBase *b)
{
	// soit l'opérateur n'a pas de surcharge (le typage n'est pas bon)
	// soit l'opérateur n'est pas commutatif
	// soit l'opérateur n'est pas défini pour le type

	auto const &lexeme = b->lexeme;
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	auto inst = static_cast<NoeudExpressionUnaire *>(b);
	auto etendue = calcule_etendue_noeud(inst, fichier);

	auto enfant_droite = inst->expr;
	auto const &type_droite = enfant_droite->type;
	auto etendue_droite = calcule_etendue_noeud(enfant_droite, fichier);
	auto expr_droite = dls::vue_chaine_compacte(&ligne[etendue_droite.pos_min], etendue_droite.pos_max - etendue_droite.pos_min);

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << '\n';
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

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::TYPE_DIFFERENTS);
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
			ContexteGenerationCode &contexte,
			NoeudBase *acces,
			NoeudBase *structure,
			NoeudBase *membre,
			dls::ensemble<dls::vue_chaine_compacte> const &membres,
			const char *chaine_structure)
{
	auto candidat = trouve_candidat(membres, membre->ident->nom);

	auto const &lexeme = acces->lexeme;
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	auto etendue = calcule_etendue_noeud(static_cast<NoeudExpression *>(acces), fichier);

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << '\n' << '\n';

	if (structure->genre == GenreNoeud::EXPRESSION_REFERENCE_MEMBRE) {
		auto noeud = static_cast<NoeudExpressionBinaire *>(structure);
		structure = noeud->expr2;
	}

	ss << "Dans l'accès à « " << structure->ident->nom << " » :\n";
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

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::MEMBRE_INCONNU);
}

void membre_inconnu(
			ContexteGenerationCode &contexte,
			NoeudBloc *bloc,
			NoeudBase *acces,
			NoeudBase *structure,
			NoeudBase *membre,
			Type *type)
{
	auto membres = dls::ensemble<dls::vue_chaine_compacte>();

	POUR (bloc->membres) {
		membres.insere(it->ident->nom);
	}

	const char *message;

	if (type->genre == GenreType::ENUM) {
		message = "de l'énumération";
	}
	else if (type->genre == GenreType::UNION) {
		message = "de l'union";
	}
	else {
		message = "de la structure";
	}

	genere_erreur_membre_inconnu(contexte, acces, structure, membre, membres, message);
}

void membre_inconnu_tableau(
			ContexteGenerationCode &contexte,
			NoeudBase *acces,
			NoeudBase *structure,
			NoeudBase *membre)
{
	auto membres = dls::ensemble<dls::vue_chaine_compacte>();
	membres.insere("taille");
	membres.insere("pointeur");

	genere_erreur_membre_inconnu(contexte, acces, structure, membre, membres, "du tableau");
}

void membre_inconnu_chaine(
			ContexteGenerationCode &contexte,
			NoeudBase *acces,
			NoeudBase *structure,
			NoeudBase *membre)
{
	auto membres = dls::ensemble<dls::vue_chaine_compacte>();
	membres.insere("taille");
	membres.insere("pointeur");

	genere_erreur_membre_inconnu(contexte, acces, structure, membre, membres, "de la chaine");
}

void membre_inconnu_eini(
			ContexteGenerationCode &contexte,
			NoeudBase *acces,
			NoeudBase *structure,
			NoeudBase *membre)
{
	auto membres = dls::ensemble<dls::vue_chaine_compacte>();
	membres.insere("info");
	membres.insere("pointeur");

	genere_erreur_membre_inconnu(contexte, acces, structure, membre, membres, "de la chaine");
}

void membre_inactif(
			ContexteGenerationCode &contexte,
			NoeudBase *acces,
			NoeudBase *structure,
			NoeudBase *membre)
{
	auto const &lexeme = acces->lexeme;
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	auto etendue = calcule_etendue_noeud(static_cast<NoeudExpression *>(acces), fichier);

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << '\n' << '\n';
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

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::MEMBRE_INACTIF);
}

void valeur_manquante_discr(
			ContexteGenerationCode &contexte,
			NoeudBase *expression,
			dls::ensemble<dls::vue_chaine_compacte> const &valeurs_manquantes)
{
	auto const &lexeme = expression->lexeme;
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme->fichier));
	auto pos = trouve_position(*lexeme, fichier);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon[pos.index_ligne];

	auto etendue = calcule_etendue_noeud(static_cast<NoeudExpression *>(expression), fichier);

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << fichier->chemin << ':' << pos.numero_ligne << '\n' << '\n';
	ss << "Dans la discrimination de « " << expression->ident->nom << " » :\n";
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

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::MEMBRE_INACTIF);
}

void fonction_principale_manquante()
{
	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : impossible de trouver la fonction principale\n";
	ss << "Veuillez vérifier qu'elle soit bien présente dans un module\n";
	ss << "\n----------------------------------------------------------------\n";
	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::MEMBRE_INACTIF);
}

}
