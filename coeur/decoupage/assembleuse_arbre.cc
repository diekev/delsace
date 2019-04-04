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

#include "assembleuse_arbre.h"

#include "contexte_generation_code.h"

assembleuse_arbre::assembleuse_arbre(ContexteGenerationCode &contexte)
{
	contexte.assembleuse = this;
	this->empile_noeud(type_noeud::RACINE, contexte, {});
}

assembleuse_arbre::~assembleuse_arbre()
{
	for (auto noeud : m_noeuds) {
		delete noeud;
	}
}

noeud::base *assembleuse_arbre::empile_noeud(type_noeud type, ContexteGenerationCode &contexte, DonneesMorceaux const &morceau, bool ajoute)
{
	auto noeud = cree_noeud(type, contexte, morceau);

	if (!m_pile.empty() && ajoute) {
		this->ajoute_noeud(noeud);
	}

	m_pile.push(noeud);

	return noeud;
}

void assembleuse_arbre::ajoute_noeud(noeud::base *noeud)
{
	m_pile.top()->ajoute_noeud(noeud);
}

noeud::base *assembleuse_arbre::cree_noeud(
		type_noeud type,
		ContexteGenerationCode &contexte,
		DonneesMorceaux const &morceau)
{
	noeud::base *noeud = nullptr;
	bool reutilise = false;

	switch (type) {
		case type_noeud::RACINE:
			m_memoire_utilisee += sizeof(noeud::racine);
			noeud = new noeud::racine(contexte, morceau);
			break;
		case type_noeud::APPEL_FONCTION:
			m_memoire_utilisee += sizeof(noeud::appel_fonction);
			noeud = new noeud::appel_fonction(contexte, morceau);
			break;
		case type_noeud::DECLARATION_FONCTION:
			m_memoire_utilisee += sizeof(noeud::declaration_fonction);
			noeud = new noeud::declaration_fonction(contexte, morceau);
			break;
		case type_noeud::ASSIGNATION_VARIABLE:
			m_memoire_utilisee += sizeof(noeud::assignation_variable);
			noeud = new noeud::assignation_variable(contexte, morceau);
			break;
		case type_noeud::DECLARATION_VARIABLE:
			m_memoire_utilisee += sizeof(noeud::declaration_variable);
			noeud = new noeud::declaration_variable(contexte, morceau);
			break;
		case type_noeud::VARIABLE:
			m_memoire_utilisee += sizeof(noeud::variable);
			noeud = new noeud::variable(contexte, morceau);
			break;
		case type_noeud::ACCES_MEMBRE:
			m_memoire_utilisee += sizeof(noeud::acces_membre_de);
			noeud = new noeud::acces_membre_de(contexte, morceau);
			break;
		case type_noeud::CARACTERE:
			m_memoire_utilisee += sizeof(noeud::caractere);
			noeud = new noeud::caractere(contexte, morceau);
			break;
		case type_noeud::NOMBRE_ENTIER:
			if (!noeuds_entier_libres.empty()) {
				noeud = noeuds_entier_libres.back();
				noeuds_entier_libres.pop_back();
				/* Utilisation du 'placement new' pour construire à l'endroit du
				 * pointeur, car morceau est une référence, et nous ne pouvons
				 * appeler le constructeur de copie à cause de cette référence. */
				new(noeud) noeud::nombre_entier(contexte, morceau);
				reutilise = true;
			}
			else {
				m_memoire_utilisee += sizeof(noeud::nombre_entier);
				noeud = new noeud::nombre_entier(contexte, morceau);
			}
			break;
		case type_noeud::NOMBRE_REEL:
			if (!noeuds_reel_libres.empty()) {
				noeud = noeuds_reel_libres.back();
				noeuds_reel_libres.pop_back();
				/* Utilisation du 'placement new' pour construire à l'endroit du
				 * pointeur, car morceau est une référence, et nous ne pouvons
				 * appeler le constructeur de copie à cause de cette référence. */
				new(noeud) noeud::nombre_reel(contexte, morceau);
				reutilise = true;
			}
			else {
				m_memoire_utilisee += sizeof(noeud::nombre_reel);
				noeud = new noeud::nombre_reel(contexte, morceau);
			}
			break;
		case type_noeud::OPERATION_BINAIRE:
			if (!noeuds_op_libres.empty()) {
				noeud = noeuds_op_libres.back();
				noeuds_op_libres.pop_back();
				/* Utilisation du 'placement new' pour construire à l'endroit du
				 * pointeur, car morceau est une référence, et nous ne pouvons
				 * appeler le constructeur de copie à cause de cette référence. */
				new(noeud) noeud::operation_binaire(contexte, morceau);
				reutilise = true;
			}
			else {
				m_memoire_utilisee += sizeof(noeud::operation_binaire);
				noeud = new noeud::operation_binaire(contexte, morceau);
			}
			break;
		case type_noeud::OPERATION_UNAIRE:
			m_memoire_utilisee += sizeof(noeud::operation_unaire);
			noeud = new noeud::operation_unaire(contexte, morceau);
			break;
		case type_noeud::RETOUR:
			m_memoire_utilisee += sizeof(noeud::retourne);
			noeud = new noeud::retourne(contexte, morceau);
			break;
		case type_noeud::CONSTANTE:
			m_memoire_utilisee += sizeof(noeud::constante);
			noeud = new noeud::constante(contexte, morceau);
			break;
		case type_noeud::CHAINE_LITTERALE:
			m_memoire_utilisee += sizeof(noeud::chaine_litterale);
			noeud = new noeud::chaine_litterale(contexte, morceau);
			break;
		case type_noeud::BOOLEEN:
			m_memoire_utilisee += sizeof(noeud::booleen);
			noeud = new noeud::booleen(contexte, morceau);
			break;
		case type_noeud::SI:
			m_memoire_utilisee += sizeof(noeud::si);
			noeud = new noeud::si(contexte, morceau);
			break;
		case type_noeud::BLOC:
			m_memoire_utilisee += sizeof(noeud::bloc);
			noeud = new noeud::bloc(contexte, morceau);
			break;
		case type_noeud::POUR:
			m_memoire_utilisee += sizeof(noeud::pour);
			noeud = new noeud::pour(contexte, morceau);
			break;
		case type_noeud::CONTINUE_ARRETE:
			m_memoire_utilisee += sizeof(noeud::cont_arr);
			noeud = new noeud::cont_arr(contexte, morceau);
			break;
		case type_noeud::BOUCLE:
			m_memoire_utilisee += sizeof(noeud::boucle);
			noeud = new noeud::boucle(contexte, morceau);
			break;
		case type_noeud::TRANSTYPE:
			m_memoire_utilisee += sizeof(noeud::transtype);
			noeud = new noeud::transtype(contexte, morceau);
			break;
		case type_noeud::NUL:
			m_memoire_utilisee += sizeof(noeud::nul);
			noeud = new noeud::nul(contexte, morceau);
			break;
		case type_noeud::TAILLE_DE:
			m_memoire_utilisee += sizeof(noeud::taille_de);
			noeud = new noeud::taille_de(contexte, morceau);
			break;
		case type_noeud::PLAGE:
			m_memoire_utilisee += sizeof(noeud::plage);
			noeud = new noeud::plage(contexte, morceau);
			break;
		case type_noeud::ACCES_MEMBRE_POINT:
			m_memoire_utilisee += sizeof(noeud::acces_membre_point);
			noeud = new noeud::acces_membre_point(contexte, morceau);
			break;
		case type_noeud::DIFFERE:
			m_memoire_utilisee += sizeof(noeud::differe);
			noeud = new noeud::differe(contexte, morceau);
			break;
		case type_noeud::NONSUR:
			m_memoire_utilisee += sizeof(noeud::non_sur);
			noeud = new noeud::non_sur(contexte, morceau);
			break;
		case type_noeud::TABLEAU:
			m_memoire_utilisee += sizeof(noeud::tableau);
			noeud = new noeud::tableau(contexte, morceau);
			break;
	}

	if (!reutilise && noeud != nullptr) {
		m_noeuds.push_back(noeud);
	}

	return noeud;
}

void assembleuse_arbre::depile_noeud(type_noeud type)
{
	assert(m_pile.top()->type() == type);
	m_pile.pop();
	static_cast<void>(type);
}

void assembleuse_arbre::imprime_code(std::ostream &os)
{
	os << "------------------------------------------------------------------\n";
	m_pile.top()->imprime_code(os, 0);
	os << "------------------------------------------------------------------\n";
}

void assembleuse_arbre::genere_code_llvm(ContexteGenerationCode &contexte_generation)
{
	if (m_pile.empty()) {
		return;
	}

	m_pile.top()->genere_code_llvm(contexte_generation);
}

void assembleuse_arbre::supprime_noeud(noeud::base *noeud)
{
	switch (noeud->type()) {
		case type_noeud::NOMBRE_ENTIER:
			this->noeuds_entier_libres.push_back(dynamic_cast<noeud::nombre_entier *>(noeud));
			break;
		case type_noeud::NOMBRE_REEL:
			this->noeuds_reel_libres.push_back(dynamic_cast<noeud::nombre_reel *>(noeud));
			break;
		case type_noeud::OPERATION_BINAIRE:
			this->noeuds_op_libres.push_back(dynamic_cast<noeud::operation_binaire *>(noeud));
			break;
		default:
			break;
	}
}

size_t assembleuse_arbre::memoire_utilisee() const
{
	return m_memoire_utilisee + m_noeuds.size() * sizeof(noeud::base *);
}

size_t assembleuse_arbre::nombre_noeuds() const
{
	return m_noeuds.size();
}

void imprime_taille_memoire_noeud(std::ostream &os)
{
	os << "------------------------------------------------------------------\n";
	os << "noeud::racine               : " << sizeof(noeud::racine) << '\n';
	os << "noeud::appel_fonction       : " << sizeof(noeud::appel_fonction) << '\n';
	os << "noeud::declaration_fonction : " << sizeof(noeud::declaration_fonction) << '\n';
	os << "noeud::assignation_variable : " << sizeof(noeud::assignation_variable) << '\n';
	os << "noeud::variable             : " << sizeof(noeud::variable) << '\n';
	os << "noeud::nombre_entier        : " << sizeof(noeud::nombre_entier) << '\n';
	os << "noeud::nombre_reel          : " << sizeof(noeud::nombre_reel) << '\n';
	os << "noeud::operation_binaire    : " << sizeof(noeud::operation_binaire) << '\n';
	os << "noeud::operation_unaire     : " << sizeof(noeud::operation_unaire) << '\n';
	os << "noeud::retourne             : " << sizeof(noeud::retourne) << '\n';
	os << "noeud::constante            : " << sizeof(noeud::constante) << '\n';
	os << "noeud::chaine_litterale     : " << sizeof(noeud::chaine_litterale) << '\n';
	os << "noeud::booleen              : " << sizeof(noeud::booleen) << '\n';
	os << "noeud::caractere            : " << sizeof(noeud::caractere) << '\n';
	os << "noeud::si                   : " << sizeof(noeud::si) << '\n';
	os << "noeud::bloc                 : " << sizeof(noeud::bloc) << '\n';
	os << "noeud::pour                 : " << sizeof(noeud::pour) << '\n';
	os << "noeud::cont_arr             : " << sizeof(noeud::cont_arr) << '\n';
	os << "noeud::boucle               : " << sizeof(noeud::boucle) << '\n';
	os << "noeud::transtype            : " << sizeof(noeud::transtype) << '\n';
	os << "noeud::nul                  : " << sizeof(noeud::nul) << '\n';
	os << "noeud::taille_de            : " << sizeof(noeud::taille_de) << '\n';
	os << "noeud::plage                : " << sizeof(noeud::plage) << '\n';
	os << "noeud::acces_membre_point   : " << sizeof(noeud::acces_membre_point) << '\n';
	os << "noeud::differe              : " << sizeof(noeud::differe) << '\n';
	os << "noeud::non_sur              : " << sizeof(noeud::non_sur) << '\n';
	os << "noeud::tableau              : " << sizeof(noeud::tableau) << '\n';
	os << "------------------------------------------------------------------\n";
	os << "DonneesType                 : " << sizeof(DonneesType) << '\n';
	os << "DonneesMorceaux             : " << sizeof(DonneesMorceaux) << '\n';
	os << "std::list<noeud::base *>    : " << sizeof(std::list<noeud::base *>) << '\n';
	os << "std::any                    : " << sizeof(std::any) << '\n';
	os << "------------------------------------------------------------------\n";
}
