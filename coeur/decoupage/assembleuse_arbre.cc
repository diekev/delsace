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

#include "arbre_syntactic.h"

assembleuse_arbre::~assembleuse_arbre()
{
	for (auto noeud : m_noeuds) {
		delete noeud;
	}
}

Noeud *assembleuse_arbre::ajoute_noeud(int type, const DonneesMorceaux &morceau, bool ajoute)
{
	auto noeud = cree_noeud(type, morceau);

	if (!m_pile.empty() && ajoute) {
		m_pile.top()->ajoute_noeud(noeud);
	}

	m_pile.push(noeud);

	return noeud;
}

void assembleuse_arbre::ajoute_noeud(Noeud *noeud)
{
	m_pile.top()->ajoute_noeud(noeud);
}

Noeud *assembleuse_arbre::cree_noeud(int type, const DonneesMorceaux &morceau)
{
	Noeud *noeud = nullptr;
	bool reutilise = false;

	switch (type) {
		case NOEUD_RACINE:
			m_memoire_utilisee += sizeof(NoeudRacine);
			noeud = new NoeudRacine(morceau);
			break;
		case NOEUD_APPEL_FONCTION:
			m_memoire_utilisee += sizeof(NoeudAppelFonction);
			noeud = new NoeudAppelFonction(morceau);
			break;
		case NOEUD_DECLARATION_FONCTION:
			m_memoire_utilisee += sizeof(NoeudDeclarationFonction);
			noeud = new NoeudDeclarationFonction(morceau);
			break;
		case NOEUD_EXPRESSION:
			m_memoire_utilisee += sizeof(NoeudExpression);
			noeud = new NoeudExpression(morceau);
			break;
		case NOEUD_ASSIGNATION_VARIABLE:
			m_memoire_utilisee += sizeof(NoeudAssignationVariable);
			noeud = new NoeudAssignationVariable(morceau);
			break;
		case NOEUD_DECLARATION_VARIABLE:
			m_memoire_utilisee += sizeof(NoeudDeclarationVariable);
			noeud = new NoeudDeclarationVariable(morceau);
			break;
		case NOEUD_VARIABLE:
			m_memoire_utilisee += sizeof(NoeudVariable);
			noeud = new NoeudVariable(morceau);
			break;
		case NOEUD_ACCES_MEMBRE:
			m_memoire_utilisee += sizeof(NoeudAccesMembre);
			noeud = new NoeudAccesMembre(morceau);
			break;
		case NOEUD_NOMBRE_ENTIER:
			if (!noeuds_entier_libres.empty()) {
				noeud = noeuds_entier_libres.back();
				noeuds_entier_libres.pop_back();
				*noeud = NoeudNombreEntier(morceau);
				reutilise = true;
			}
			else {
				m_memoire_utilisee += sizeof(NoeudNombreEntier);
				noeud = new NoeudNombreEntier(morceau);
			}
			break;
		case NOEUD_NOMBRE_REEL:
			if (!noeuds_reel_libres.empty()) {
				noeud = noeuds_reel_libres.back();
				noeuds_reel_libres.pop_back();
				*noeud = NoeudNombreReel(morceau);
				reutilise = true;
			}
			else {
				m_memoire_utilisee += sizeof(NoeudNombreReel);
				noeud = new NoeudNombreReel(morceau);
			}
			break;
		case NOEUD_OPERATION:
			if (!noeuds_op_libres.empty()) {
				noeud = noeuds_op_libres.back();
				noeuds_op_libres.pop_back();
				*noeud = NoeudOperation(morceau);
				reutilise = true;
			}
			else {
				m_memoire_utilisee += sizeof(NoeudOperation);
				noeud = new NoeudOperation(morceau);
			}
			break;
		case NOEUD_RETOUR:
			m_memoire_utilisee += sizeof(NoeudRetour);
			noeud = new NoeudRetour(morceau);
			break;
		case NOEUD_CONSTANTE:
			m_memoire_utilisee += sizeof(NoeudConstante);
			noeud = new NoeudConstante(morceau);
			break;
		case NOEUD_CHAINE_LITTERALE:
			m_memoire_utilisee += sizeof(NoeudChaineLitterale);
			noeud = new NoeudChaineLitterale(morceau);
			break;
		case NOEUD_BOOLEEN:
			m_memoire_utilisee += sizeof(NoeudBooleen);
			noeud = new NoeudBooleen(morceau);
			break;
	}

	if (!reutilise && noeud != nullptr) {
		m_noeuds.push_back(noeud);
	}

	return noeud;
}

void assembleuse_arbre::sors_noeud(int type)
{
	assert(m_pile.top()->type_noeud() == type);
	m_pile.pop();
}

void assembleuse_arbre::imprime_code(std::ostream &os)
{
	m_pile.top()->imprime_code(os, 0);
}

void assembleuse_arbre::genere_code_llvm(ContexteGenerationCode &contexte_generation)
{
	m_pile.top()->genere_code_llvm(contexte_generation);
}

void assembleuse_arbre::supprime_noeud(Noeud *noeud)
{
	switch (noeud->type_noeud()) {
		case NOEUD_NOMBRE_ENTIER:
			this->noeuds_entier_libres.push_back(dynamic_cast<NoeudNombreEntier *>(noeud));
			break;
		case NOEUD_NOMBRE_REEL:
			this->noeuds_reel_libres.push_back(dynamic_cast<NoeudNombreReel *>(noeud));
			break;
		case NOEUD_OPERATION:
			this->noeuds_op_libres.push_back(dynamic_cast<NoeudOperation *>(noeud));
			break;
	}
}

size_t assembleuse_arbre::memoire_utilisee() const
{
	return m_memoire_utilisee + m_noeuds.size() * sizeof(Noeud *);
}

void imprime_taille_memoire_noeud(std::ostream &os)
{
	os << "------------------------------------------------------------------\n";
	os << "NoeudRacine              : " << sizeof(NoeudRacine) << '\n';
	os << "NoeudAppelFonction       : " << sizeof(NoeudAppelFonction) << '\n';
	os << "NoeudDeclarationFonction : " << sizeof(NoeudDeclarationFonction) << '\n';
	os << "NoeudExpression          : " << sizeof(NoeudExpression) << '\n';
	os << "NoeudAssignationVariable : " << sizeof(NoeudAssignationVariable) << '\n';
	os << "NoeudVariable            : " << sizeof(NoeudVariable) << '\n';
	os << "NoeudNombreEntier        : " << sizeof(NoeudNombreEntier) << '\n';
	os << "NoeudNombreReel          : " << sizeof(NoeudNombreReel) << '\n';
	os << "NoeudOperation           : " << sizeof(NoeudOperation) << '\n';
	os << "NoeudRetour              : " << sizeof(NoeudRetour) << '\n';
	os << "NoeudConstante           : " << sizeof(NoeudConstante) << '\n';
	os << "NoeudChaineLitterale     : " << sizeof(NoeudChaineLitterale) << '\n';
	os << "NoeudBooleen             : " << sizeof(NoeudBooleen) << '\n';
	os << "------------------------------------------------------------------\n";
	os << "DonneesType              : " << sizeof(DonneesType) << '\n';
	os << "DonneesMorceaux          : " << sizeof(DonneesMorceaux) << '\n';
	os << "std::list<Noeud *>       : " << sizeof(std::list<Noeud *>) << '\n';
	os << "------------------------------------------------------------------\n";
}
