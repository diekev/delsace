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

#include "arbre_syntactic.hh"

#include "biblinternes/structures/flux_chaine.hh"

#include "../tori/objet.hh"

static void imprime_tab(std::ostream &os, int tab)
{
	for (int i = 0; i < tab; ++i) {
		os << ' ' << ' ';
	}
}

/* ************************************************************************** */

Noeud::Noeud(const DonneesMorceaux &donnees)
	: donnees_morceaux(donnees)
{}

void Noeud::ajoute_enfant(Noeud *noeud)
{
	enfants.pousse(noeud);
}

/* ************************************************************************** */

NoeudChaineCaractere::NoeudChaineCaractere(const DonneesMorceaux &donnees)
	: Noeud(donnees)
{}

type_noeud NoeudChaineCaractere::type() const
{
	return type_noeud::CHAINE_CARACTERE;
}

void NoeudChaineCaractere::imprime_arbre(std::ostream &os, int tab) const
{
	imprime_tab(os, tab);
	os << "Chaîne Caractère\n";
}

void NoeudChaineCaractere::genere_code(dls::chaine &tampon, tori::ObjetDictionnaire &/*objet*/) const
{
	tampon += donnees_morceaux.chaine;
}

/* ************************************************************************** */

NoeudVariable::NoeudVariable(const DonneesMorceaux &donnees)
	: Noeud(donnees)
{}

type_noeud NoeudVariable::type() const
{
	return type_noeud::VARIABLE;
}

void NoeudVariable::imprime_arbre(std::ostream &os, int tab) const
{
	imprime_tab(os, tab);
	os << "Variable\n";
}

void NoeudVariable::genere_code(dls::chaine &tampon, tori::ObjetDictionnaire &objet) const
{
	auto variable = this->donnees_morceaux.chaine;
	auto iter = objet.valeur.trouve(dls::chaine(variable));

	if (iter != objet.valeur.fin()) {
		auto const &objet_variable = iter->second;

		switch (objet_variable->type) {
			case tori::type_objet::NUL:
			{
				tampon += "nul";
				break;
			}
			case tori::type_objet::CHAINE:
			{
				auto chaine = static_cast<tori::ObjetChaine *>(objet_variable.get());
				tampon += chaine->valeur;
				break;
			}
			case tori::type_objet::NOMBRE_ENTIER:
			{
				auto nombre = static_cast<tori::ObjetNombreEntier *>(objet_variable.get());
				tampon += std::to_string(nombre->valeur);
				break;
			}
			case tori::type_objet::NOMBRE_REEL:
			{
				auto nombre = static_cast<tori::ObjetNombreReel *>(objet_variable.get());
				tampon += std::to_string(nombre->valeur);
				break;
			}
			case tori::type_objet::TABLEAU:
			case tori::type_objet::DICTIONNAIRE:
			{
				dls::flux_chaine ss;
				ss << std::hex << objet_variable.get();

				tampon += "objet à " + ss.chn();
				break;
			}
		}
	}
}

/* ************************************************************************** */

NoeudBloc::NoeudBloc(const DonneesMorceaux &donnees)
	: Noeud(donnees)
{}

type_noeud NoeudBloc::type() const
{
	return type_noeud::BLOC;
}

void NoeudBloc::imprime_arbre(std::ostream &os, int tab) const
{
	imprime_tab(os, tab);
	os << "Bloc :\n";

	for (auto enfant : enfants) {
		enfant->imprime_arbre(os, tab + 1);
	}
}

void NoeudBloc::genere_code(dls::chaine &tampon, tori::ObjetDictionnaire &objet) const
{
	for (auto enfant : enfants) {
		enfant->genere_code(tampon, objet);
	}
}

/* ************************************************************************** */

NoeudSi::NoeudSi(const DonneesMorceaux &donnees)
	: Noeud(donnees)
{}

type_noeud NoeudSi::type() const
{
	return type_noeud::SI;
}

void NoeudSi::imprime_arbre(std::ostream &os, int tab) const
{
	imprime_tab(os, tab);
	os << "Si :\n";

	for (auto enfant : enfants) {
		enfant->imprime_arbre(os, tab + 1);
	}
}

void NoeudSi::genere_code(dls::chaine &tampon, tori::ObjetDictionnaire &objet) const
{
	auto variable = enfants[0]->donnees_morceaux.chaine;

	if (objet.valeur.trouve(dls::chaine(variable)) != objet.valeur.fin()) {
		enfants[1]->genere_code(tampon, objet);
	}
	else {
		if (enfants.taille() > 2) {
			enfants[2]->genere_code(tampon, objet);
		}
	}
}

/* ************************************************************************** */

NoeudPour::NoeudPour(const DonneesMorceaux &donnees)
	: Noeud(donnees)
{}

type_noeud NoeudPour::type() const
{
	return type_noeud::POUR;
}

void NoeudPour::imprime_arbre(std::ostream &os, int tab) const
{
	imprime_tab(os, tab);
	os << "Pour :\n";

	for (auto enfant : enfants) {
		enfant->imprime_arbre(os, tab + 1);
	}
}

void NoeudPour::genere_code(dls::chaine &tampon, tori::ObjetDictionnaire &objet) const
{
	auto propriete = enfants[1]->donnees_morceaux.chaine;
	auto iter = objet.valeur.trouve(dls::chaine(propriete));

	if (iter == objet.valeur.fin()) {
		return;
	}

	auto objet_iter = iter->second;

	auto variable = enfants[0]->donnees_morceaux.chaine;

	switch (objet_iter->type) {
		case tori::type_objet::NUL:
		{
			throw "Objet nul n'est pas itérable !";
		}
		case tori::type_objet::CHAINE:
		{
			throw "Objet chaine n'est pas itérable !";
		}
		case tori::type_objet::NOMBRE_ENTIER:
		{
			throw "Objet nombre entier n'est pas itérable !";
		}
		case tori::type_objet::NOMBRE_REEL:
		{
			throw "Objet nombre réel n'est pas itérable !";
		}
		case tori::type_objet::TABLEAU:
		{
			auto tableau = static_cast<tori::ObjetTableau *>(objet_iter.get());

			for (auto const &objet_tableau : tableau->valeur) {
				objet.valeur[dls::chaine(variable)] = objet_tableau;

				enfants[2]->genere_code(tampon, objet);
			}

			break;
		}
		case tori::type_objet::DICTIONNAIRE:
		{
			auto dictionnaire = static_cast<tori::ObjetDictionnaire *>(objet_iter.get());

			for (auto const &paire_iter : dictionnaire->valeur) {
				auto objet_nom = std::make_shared<tori::ObjetChaine>();
				objet_nom->valeur = paire_iter.first;

				objet.valeur[dls::chaine(variable)] = objet_nom;

				enfants[2]->genere_code(tampon, objet);
			}

			break;
		}
	}
}
