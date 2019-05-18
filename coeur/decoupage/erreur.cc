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

#include <sstream>

#include <delsace/langage/unicode.hh>

#include "arbre_syntactic.h"
#include "contexte_generation_code.h"
#include "donnees_type.h"
#include "modules.hh"
#include "morceaux.h"

namespace erreur {

frappe::frappe(const char *message, type_erreur type)
	: m_message(message)
	, m_type(type)
{}

type_erreur frappe::type() const
{
	return m_type;
}

const char *frappe::message() const
{
	return m_message.c_str();
}

static void imprime_caractere_vide(std::ostream &os, const size_t nombre, const std::string_view &chaine)
{
	/* Le 'nombre' est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (size_t i = 0; i < std::min(nombre, chaine.size());) {
		if (chaine[i] == '\t') {
			os << '\t';
		}
		else {
			os << ' ';
		}

		i += lng::decalage_pour_caractere(chaine, i);
	}
}

static void imprime_tilde(std::ostream &os, const std::string_view &chaine)
{
	for (size_t i = 0; i < chaine.size() - 1;) {
		os << '~';
		i += lng::decalage_pour_caractere(chaine, i);
	}
}

static void imprime_tilde(std::ostream &os, const std::string_view &chaine, size_t debut, size_t fin)
{
	for (size_t i = debut; i < fin;) {
		os << '~';
		i += lng::decalage_pour_caractere(chaine, i);
	}
}

static void imprime_ligne_entre(
		std::ostream &os,
		const std::string_view &chaine,
		size_t debut,
		size_t fin)
{
	for (auto i = debut; i < fin; ++i) {
		os << chaine[i];
	}
}

void lance_erreur(
		const std::string &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau,
		type_erreur type)
{
	auto const ligne = morceau.ligne_pos >> 32;
	auto const pos_mot = morceau.ligne_pos & 0xffffffff;
	auto const identifiant = morceau.identifiant;
	auto const &chaine = morceau.chaine;

	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto ligne_courante = module->tampon[ligne];

	std::stringstream ss;
	ss << "Erreur : " << module->chemin << ':' << ligne + 1 << ":\n";
	ss << ligne_courante;

	imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	imprime_tilde(ss, chaine);
	ss << '\n';

	ss << quoi;
	ss << ", obtenu : " << chaine << " (" << chaine_identifiant(identifiant) << ')';

	throw erreur::frappe(ss.str().c_str(), type);
}

void lance_erreur_plage(
		const std::string &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &premier_morceau,
		const DonneesMorceaux &dernier_morceau,
		type_erreur type)
{
	auto const ligne = premier_morceau.ligne_pos >> 32;
	auto const pos_premier = premier_morceau.ligne_pos & 0xffffffff;
	auto const pos_dernier = dernier_morceau.ligne_pos & 0xffffffff;

	auto module = contexte.module(static_cast<size_t>(premier_morceau.module));
	auto ligne_courante = module->tampon[ligne];

	std::stringstream ss;
	ss << "Erreur : " << module->chemin << ':' << ligne + 1 << ":\n";
	ss << ligne_courante;

	imprime_caractere_vide(ss, pos_premier, ligne_courante);
	ss << '^';
	imprime_tilde(ss, ligne_courante, pos_premier, pos_dernier + 1);
	ss << '\n';

	ss << quoi;

	throw erreur::frappe(ss.str().c_str(), type);
}

[[noreturn]] void lance_erreur_nombre_arguments(
		const size_t nombre_arguments,
		const size_t nombre_recus,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau)
{
	auto const numero_ligne = morceau.ligne_pos >> 32;
	auto const pos_mot = morceau.ligne_pos & 0xffffffff;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto ligne = module->tampon[numero_ligne];

	std::stringstream ss;
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Le nombre d'arguments de la fonction est incorrect.\n";
	ss << "Requiers : " << nombre_arguments << '\n';
	ss << "Obtenu : " << nombre_recus << '\n';

	throw frappe(ss.str().c_str(), type_erreur::NOMBRE_ARGUMENT);
}

[[noreturn]] void lance_erreur_type_arguments(
		const DonneesType &type_arg,
		const DonneesType &type_enf,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau_enfant,
		const DonneesMorceaux &morceau)
{
	auto const numero_ligne = morceau.ligne_pos >> 32;
	auto const pos_mot = morceau_enfant.ligne_pos & 0xffffffff;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto ligne = module->tampon[numero_ligne];

	std::stringstream ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << "Dans l'appel de la fonction '" << morceau.chaine << "':\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau_enfant.chaine);
	ss << '\n';

	ss << "Le type de l'argument '" << morceau_enfant.chaine << "' ne correspond pas à celui requis !\n";
	ss << "Requiers : " << chaine_type(type_arg, contexte) << '\n';
	ss << "Obtenu   : " << chaine_type(type_enf, contexte) << '\n';
	ss << '\n';
	ss << "Astuce :\n";
	ss << "Vous pouvez convertir le type en utilisant l'opérateur 'transtype', comme ceci :\n";

	imprime_ligne_entre(ss, ligne, 0, pos_mot);
	ss << "transtype(" << morceau_enfant.chaine << " : " << chaine_type(type_arg, contexte) << ")";
	imprime_ligne_entre(ss, ligne, pos_mot + morceau_enfant.chaine.size(), ligne.size());
	ss << "\n----------------------------------------------------------------\n";

	throw frappe(ss.str().c_str(), type_erreur::TYPE_ARGUMENT);
}

[[noreturn]] void lance_erreur_type_retour(
		const DonneesType &type_arg,
		const DonneesType &type_enf,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau_enfant,
		const DonneesMorceaux &morceau)
{
	auto const numero_ligne = morceau.ligne_pos >> 32;
	auto const pos_mot = morceau_enfant.ligne_pos & 0xffffffff;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto ligne = module->tampon[numero_ligne];

	std::stringstream ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << "Dans l'expression de '" << morceau.chaine << "':\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau_enfant.chaine);
	ss << '\n';

	ss << "Le type de '" << morceau_enfant.chaine << "' ne correspond pas à celui requis !\n";
	ss << "Requiers : " << chaine_type(type_arg, contexte) << '\n';
	ss << "Obtenu   : " << chaine_type(type_enf, contexte) << '\n';
	ss << '\n';
	ss << "Astuce :\n";
	ss << "Vous pouvez convertir le type en utilisant l'opérateur 'transtype', comme ceci :\n";

	imprime_ligne_entre(ss, ligne, 0, pos_mot);
	ss << "transtype(" << morceau_enfant.chaine << " : " << chaine_type(type_arg, contexte) << ")";
	imprime_ligne_entre(ss, ligne, pos_mot + morceau_enfant.chaine.size(), ligne.size());
	ss << "\n----------------------------------------------------------------\n";

	throw frappe(ss.str().c_str(), type_erreur::TYPE_ARGUMENT);
}

[[noreturn]] void lance_erreur_argument_inconnu(
		const std::string_view &nom_arg,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau)
{
	auto const numero_ligne = morceau.ligne_pos >> 32;
	auto const pos_mot = morceau.ligne_pos & 0xffffffff;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto ligne = module->tampon[numero_ligne];

	std::stringstream ss;
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Fonction : '" << morceau.chaine
	   << "', argument nommé '" << nom_arg << "' inconnu !\n";

	throw frappe(ss.str().c_str(), type_erreur::ARGUMENT_INCONNU);
}

[[noreturn]] void lance_erreur_redeclaration_argument(
		const std::string_view &nom_arg,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau)
{
	auto const numero_ligne = morceau.ligne_pos >> 32;
	auto const pos_mot = morceau.ligne_pos & 0xffffffff;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto ligne = module->tampon[numero_ligne];

	std::stringstream ss;
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Fonction : '" << morceau.chaine
	   << "', redéclaration de l'argument '" << nom_arg << "' !\n";

	throw frappe(ss.str().c_str(), type_erreur::ARGUMENT_REDEFINI);
}

[[noreturn]] void lance_erreur_assignation_type_differents(
		const DonneesType &type_gauche,
		const DonneesType &type_droite,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau)
{
	auto const numero_ligne = morceau.ligne_pos >> 32;
	auto const pos_mot = morceau.ligne_pos & 0xffffffff;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto ligne = module->tampon[numero_ligne];

	std::stringstream ss;
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Ne peut pas assigner des types différents !\n";
	ss << "Type à gauche : " << chaine_type(type_gauche, contexte) << '\n';
	ss << "Type à droite : " << chaine_type(type_droite, contexte) << '\n';

	throw frappe(ss.str().c_str(), type_erreur::ASSIGNATION_MAUVAIS_TYPE);
}

void lance_erreur_type_operation(
		const DonneesType &type_gauche,
		const DonneesType &type_droite,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau)
{
	auto const numero_ligne = morceau.ligne_pos >> 32;
	auto const pos_mot = morceau.ligne_pos & 0xffffffff;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto ligne = module->tampon[numero_ligne];

	std::stringstream ss;
	ss << "Erreur : " << module->chemin << ':' << numero_ligne + 1 << ":\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Les types de l'opération sont différents !\n";
	ss << "Type à gauche : " << chaine_type(type_gauche, contexte) << '\n';
	ss << "Type à droite : " << chaine_type(type_droite, contexte) << '\n';

	throw frappe(ss.str().c_str(), type_erreur::TYPE_DIFFERENTS);
}

void lance_erreur_fonction_inconnue(
		ContexteGenerationCode const &contexte,
		noeud::base *b,
		std::vector<DonneesCandidate> const &candidates)
{
	auto const &morceau = b->morceau;
	auto const numero_ligne = morceau.ligne_pos >> 32;
	auto const pos_mot = morceau.ligne_pos & 0xffffffff;
	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto ligne = module->tampon[numero_ligne];

	std::stringstream ss;

	ss << "\n----------------------------------------------------------------\n";
	ss << "Erreur : " << module->chemin << ':' << (b->morceau.ligne_pos >> 32) << '\n';
	ss << "\nDans l'appel de la fonction '" << b->morceau.chaine << "'\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	if (candidates.empty()) {
		ss << "\nFonction inconnue : aucune candidate trouvée\n";
		ss << "Vérifiez que la fonction existe bel et bien dans un module importé\n";

		throw erreur::frappe(ss.str().c_str(), erreur::type_erreur::FONCTION_INCONNUE);
	}

	ss << "\nAucune candidate trouvée pour la fonction '" << b->morceau.chaine << "'\n";

	auto type_erreur = erreur::type_erreur::FONCTION_INCONNUE;

	for (auto &dc : candidates) {
		auto df = dc.df;
		ss << "\nCandidate :";

		if (df != nullptr) {
			auto noeud_decl = df->noeud_decl;

			auto const &morceau_df = noeud_decl->morceau;
			auto const numero_ligne_df = morceau_df.ligne_pos >> 32;
			auto module_df = contexte.module(static_cast<size_t>(morceau_df.module));

			ss << ' ' << noeud_decl->chaine()
			   << " (trouvée à " << module_df->chemin << ':' << numero_ligne_df + 1 << ")\n";
		}
		else {
			ss << '\n';
		}

		if (dc.raison == MECOMPTAGE_ARGS) {
			ss << "\tLe nombre d'arguments de la fonction est incorrect.\n";
			ss << "\tRequiers " << df->args.size() << " arguments\n";
			ss << "\tObtenu " << b->enfants.size() << " arguments\n";
			type_erreur = erreur::type_erreur::NOMBRE_ARGUMENT;
		}

		if (dc.raison == MENOMMAGE_ARG) {
			/* À FAIRE : trouve le morceau correspondant à l'argument. */
			ss << "\tArgument '" << dc.nom_arg << "' inconnu !\n";
			ss << "\tLes arguments de la fonction sont : \n";

			for (auto const &nom_arg : df->nom_args) {
				ss << "\t\t" << nom_arg << '\n';
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
//			imprime_ligne_entre(ss, ligne, pos_mot + morceau_enfant.chaine.size(), ligne.size());
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

	throw erreur::frappe(ss.str().c_str(), type_erreur);
}

}
