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

#include "arbre_syntactic.h"
#include "contexte_generation_code.h"
#include "morceaux.hh"

namespace lng::erreur {

static void imprime_tilde(dls::flux_chaine &ss, dls::vue_chaine_compacte chaine)
{
	imprime_tilde(ss, dls::vue_chaine(chaine.pointeur(), chaine.taille()));
}

}

namespace erreur {

struct PositionMorceau {
	long ligne = 0;
	long pos = 0;
};

static auto trouve_position(DonneesMorceau const &morceau, DonneesModule *module)
{
	auto ptr = morceau.chaine.pointeur();
	auto pos = PositionMorceau{};

	for (auto i = 0ul; i < module->tampon.nombre_lignes() - 1; ++i) {
		auto l0 = module->tampon[static_cast<long>(i)];
		auto l1 = module->tampon[static_cast<long>(i + 1)];

		if (ptr >= l0.begin() && ptr < l1.begin()) {
			pos.ligne = static_cast<long>(i);
			pos.pos = ptr - l0.begin();
			break;
		}
	}

	return pos;
}

void lance_erreur(
		const dls::chaine &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesMorceau &morceau,
		type_erreur type)
{
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto pos = trouve_position(morceau, module);
	auto const ligne = pos.ligne;
	auto const pos_mot = pos.pos;
	auto const identifiant = morceau.identifiant;
	auto const &chaine = morceau.chaine;

	auto ligne_courante = module->tampon[ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << module->chemin << ':' << ligne + 1 << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	ss << quoi;
	ss << ", obtenu : " << chaine << " (" << chaine_identifiant(identifiant) << ')';

	throw erreur::frappe(ss.chn().c_str(), type);
}

void lance_erreur_plage(
		const dls::chaine &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesMorceau &premier_morceau,
		const DonneesMorceau &dernier_morceau,
		type_erreur type)
{
	auto module = contexte.module(static_cast<size_t>(premier_morceau.module));
	auto pos = trouve_position(premier_morceau, module);
	auto const ligne = pos.ligne;
	auto const pos_premier = pos.pos;

	auto module_dernier = contexte.module(static_cast<size_t>(dernier_morceau.module));
	auto const pos_dernier = trouve_position(premier_morceau, module_dernier).pos;

	auto ligne_courante = module->tampon[ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << module->chemin << ':' << ligne + 1 << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_premier, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, ligne_courante, pos_premier, pos_dernier + 1);
	ss << '\n';

	ss << quoi;

	throw erreur::frappe(ss.chn().c_str(), type);
}

[[noreturn]] void lance_erreur_type_arguments(
		const DonneesTypeFinal &type_arg,
		const DonneesTypeFinal &type_enf,
		const ContexteGenerationCode &contexte,
		const DonneesMorceau &morceau_enfant,
		const DonneesMorceau &morceau)
{
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto pos = trouve_position(morceau, module);
	auto const numero_ligne = pos.ligne;
	auto const pos_mot = pos.pos;
	auto ligne = module->tampon[numero_ligne];

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << "Dans l'appel de la fonction '" << morceau.chaine << "':\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, morceau_enfant.chaine);
	ss << '\n';

	ss << "Le type de l'argument '" << morceau_enfant.chaine << "' ne correspond pas à celui requis !\n";
	ss << "Requiers : " << chaine_type(type_arg, contexte) << '\n';
	ss << "Obtenu   : " << chaine_type(type_enf, contexte) << '\n';
	ss << '\n';
	ss << "Astuce :\n";
	ss << "Vous pouvez convertir le type en utilisant l'opérateur 'transtype', comme ceci :\n";

	lng::erreur::imprime_ligne_entre(ss, ligne, 0, pos_mot);
	ss << "transtype(" << morceau_enfant.chaine << " : " << chaine_type(type_arg, contexte) << ")";
	lng::erreur::imprime_ligne_entre(ss, ligne, pos_mot + morceau_enfant.chaine.taille(), ligne.taille());
	ss << "\n----------------------------------------------------------------\n";

	throw frappe(ss.chn().c_str(), type_erreur::TYPE_ARGUMENT);
}

[[noreturn]] void lance_erreur_type_retour(
		const DonneesTypeFinal &type_arg,
		const DonneesTypeFinal &type_enf,
		const ContexteGenerationCode &contexte,
		const DonneesMorceau &morceau_enfant,
		const DonneesMorceau &morceau)
{
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto pos = trouve_position(morceau, module);
	auto const numero_ligne = pos.ligne;
	auto const pos_mot = pos.pos;
	auto ligne = module->tampon[numero_ligne];

	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << "Dans l'expression de '" << morceau.chaine << "':\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, morceau_enfant.chaine);
	ss << '\n';

	ss << "Le type de '" << morceau_enfant.chaine << "' ne correspond pas à celui requis !\n";
	ss << "Requiers : " << chaine_type(type_arg, contexte) << '\n';
	ss << "Obtenu   : " << chaine_type(type_enf, contexte) << '\n';
	ss << '\n';
	ss << "Astuce :\n";
	ss << "Vous pouvez convertir le type en utilisant l'opérateur 'transtype', comme ceci :\n";

	lng::erreur::imprime_ligne_entre(ss, ligne, 0, pos_mot);
	ss << "transtype(" << morceau_enfant.chaine << " : " << chaine_type(type_arg, contexte) << ")";
	lng::erreur::imprime_ligne_entre(ss, ligne, pos_mot + morceau_enfant.chaine.taille(), ligne.taille());
	ss << "\n----------------------------------------------------------------\n";

	throw frappe(ss.chn().c_str(), type_erreur::TYPE_DIFFERENTS);
}

[[noreturn]] void lance_erreur_assignation_type_differents(
		const DonneesTypeFinal &type_gauche,
		const DonneesTypeFinal &type_droite,
		const ContexteGenerationCode &contexte,
		const DonneesMorceau &morceau)
{
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto pos = trouve_position(morceau, module);
	auto const numero_ligne = pos.ligne;
	auto const pos_mot = pos.pos;
	auto ligne = module->tampon[numero_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Ne peut pas assigner des types différents !\n";
	ss << "Type à gauche : " << chaine_type(type_gauche, contexte) << '\n';
	ss << "Type à droite : " << chaine_type(type_droite, contexte) << '\n';

	throw frappe(ss.chn().c_str(), type_erreur::ASSIGNATION_MAUVAIS_TYPE);
}

void lance_erreur_type_operation(
		const DonneesTypeFinal &type_gauche,
		const DonneesTypeFinal &type_droite,
		const ContexteGenerationCode &contexte,
		const DonneesMorceau &morceau)
{
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto pos = trouve_position(morceau, module);
	auto const numero_ligne = pos.ligne;
	auto const pos_mot = pos.pos;
	auto ligne = module->tampon[numero_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Les types de l'opération sont différents !\n";
	ss << "Type à gauche : " << chaine_type(type_gauche, contexte) << '\n';
	ss << "Type à droite : " << chaine_type(type_droite, contexte) << '\n';

	throw frappe(ss.chn().c_str(), type_erreur::TYPE_DIFFERENTS);
}

void lance_erreur_fonction_inconnue(
		ContexteGenerationCode const &contexte,
		noeud::base *b,
		dls::tableau<DonneesCandidate> const &candidates)
{
	auto const &morceau = b->morceau;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto pos = trouve_position(morceau, module);
	auto const numero_ligne = pos.ligne;
	auto const pos_mot = pos.pos;
	auto ligne = module->tampon[numero_ligne];

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << module->chemin << ':' << pos_mot << '\n';
	ss << "\nDans l'appel de la fonction '" << b->morceau.chaine << "'\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	if (candidates.est_vide()) {
		ss << "\nFonction inconnue : aucune candidate trouvée\n";
		ss << "Vérifiez que la fonction existe bel et bien dans un module importé\n";

		throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::FONCTION_INCONNUE);
	}

	ss << "\nAucune candidate trouvée pour la fonction '" << b->morceau.chaine << "'\n";

	auto type_erreur = erreur::type_erreur::FONCTION_INCONNUE;

	for (auto &dc : candidates) {
		auto df = dc.df;
		ss << "\nCandidate :";

		if (df != nullptr) {
			auto noeud_decl = df->noeud_decl;

			auto const &morceau_df = noeud_decl->morceau;
			auto module_df = contexte.module(static_cast<size_t>(morceau_df.module));
			auto pos_df = trouve_position(morceau_df, module_df);
			auto const numero_ligne_df = pos_df.ligne;

			ss << ' ' << noeud_decl->chaine()
			   << " (trouvée à " << module_df->chemin << ':' << numero_ligne_df + 1 << ")\n";
		}
		else {
			ss << '\n';
		}

		if (dc.raison == MECOMPTAGE_ARGS) {
			ss << "\tLe nombre d'arguments de la fonction est incorrect.\n";
			ss << "\tRequiers " << df->args.taille() << " arguments\n";
			ss << "\tObtenu " << b->enfants.taille() << " arguments\n";
			type_erreur = erreur::type_erreur::NOMBRE_ARGUMENT;
		}

		if (dc.raison == MENOMMAGE_ARG) {
			/* À FAIRE : trouve le morceau correspondant à l'argument. */
			ss << "\tArgument '" << dc.nom_arg << "' inconnu !\n";
			ss << "\tLes arguments de la fonction sont : \n";

			for (auto const &argument : df->args) {
				ss << "\t\t" << argument.nom << '\n';
			}

			type_erreur = erreur::type_erreur::ARGUMENT_INCONNU;
		}

		if (dc.raison == RENOMMAGE_ARG) {
			/* À FAIRE : trouve le morceau correspondant à l'argument. */
			ss << "\tL'argument '" << dc.nom_arg << "' a déjà été nommé\n";
			type_erreur = erreur::type_erreur::ARGUMENT_REDEFINI;
		}

		if (dc.raison == MANQUE_NOM_APRES_VARIADIC) {
			/* À FAIRE : trouve le morceau correspondant à l'argument. */
			ss << "\tNom d'argument manquant\n";
			ss << "\tLes arguments doivent être nommés s'ils sont précédés d'arguments déjà nommés\n";
			type_erreur = erreur::type_erreur::ARGUMENT_INCONNU;
		}

		if (dc.raison == METYPAGE_ARG) {
			auto const &morceau_enfant = dc.noeud_decl->morceau;

			ss << "\tLe type de l'argument '" << morceau_enfant.chaine << "' ne correspond pas à celui requis !\n";
			ss << "\tRequiers : " << chaine_type(dc.type1, contexte) << '\n';
			ss << "\tObtenu   : " << chaine_type(dc.type2, contexte) << '\n';
			/* À FAIRE */
//			ss << '\n';
//			ss << "Astuce :\n";
//			ss << "Vous pouvez convertir le type en utilisant l'opérateur 'transtype', comme ceci :\n";

//			imprime_ligne_entre(ss, ligne, 0, pos_mot);
//			ss << "transtype(" << morceau_enfant.chaine << " : " << dc.type1 << ")";
//			imprime_ligne_entre(ss, ligne, pos_mot + morceau_enfant.chaine.taille(), ligne.taille());
			type_erreur = erreur::type_erreur::TYPE_ARGUMENT;
		}

#ifdef NON_SUR
		if (candidate->arg_pointeur && !contexte.non_sur()) {
			/* À FAIRE : trouve le morceau correspondant à l'argument. */
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
			noeud::base *appl_fonc,
			noeud::base *decl_fonc,
			noeud::base *decl_appel)
{
	auto const &morceau = appl_fonc->morceau;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto pos = trouve_position(morceau, module);
	auto const numero_ligne = pos.ligne;
	auto const pos_mot = pos.pos;
	auto ligne = module->tampon[numero_ligne];

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << module->chemin << ':' << numero_ligne << '\n';
	ss << "\nDans l'appel de la fonction « " << appl_fonc->morceau.chaine << " »\n";
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Ne peut appeler une fonction avec contexte dans une fonction sans contexte !\n";

	ss << "Note : la fonction est appelée dans « " << decl_fonc->chaine() << " » "
	   << " qui a été déclarée sans contexte via #!nulctx.\n";

	ss << "\n« " << decl_fonc->chaine() << " » est déclarée ici :\n";
	module = contexte.module(static_cast<size_t>(decl_fonc->morceau.module));
	auto pos_decl = trouve_position(decl_fonc->morceau, module);
	ss << module->chemin << ':' << pos_decl.ligne << '\n' << '\n';
	ss << module->tampon[pos_decl.ligne];

	ss << "\n« " << appl_fonc->chaine() << " » est déclarée ici :\n";
	module = contexte.module(static_cast<size_t>(decl_appel->morceau.module));
	auto pos_appel = trouve_position(decl_appel->morceau, module);
	ss << module->chemin << ':' << pos_appel.ligne << '\n' << '\n';
	ss << module->tampon[pos_appel.ligne];

	ss << "\n----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::APPEL_INVALIDE);
}

void lance_erreur_acces_hors_limites(
			ContexteGenerationCode const &contexte,
			noeud::base *b,
			int taille_tableau,
			DonneesTypeFinal const &type_tableau,
			long index_acces)
{
	auto const &morceau = b->morceau;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto pos = trouve_position(morceau, module);
	auto const numero_ligne = pos.ligne;
	auto const pos_mot = pos.pos;
	auto ligne = module->tampon[numero_ligne];

	dls::flux_chaine ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << module->chemin << ':' << numero_ligne << '\n';
	ss << ligne;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	lng::erreur::imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Accès au tableau hors de ses limites !\n";

	ss << "\tLe tableau a une taille de " << taille_tableau << " (de type : "
	   << chaine_type(type_tableau, contexte) << ").\n";
	ss << "\tL'accès se fait à l'index " << index_acces << " (index maximal : " << taille_tableau - 1 << ").\n";

	ss << "\n----------------------------------------------------------------\n";

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::NORMAL);
}

}
