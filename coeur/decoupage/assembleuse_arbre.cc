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

assembleuse_arbre::~assembleuse_arbre()
{
	for (auto noeud : m_noeuds) {
		delete noeud;
	}
}

Noeud *assembleuse_arbre::empile_noeud(type_noeud type, const DonneesMorceaux &morceau, bool ajoute)
{
	auto noeud = cree_noeud(type, morceau);

	if (!m_pile.empty() && ajoute) {
		this->ajoute_noeud(noeud);
	}

	m_pile.push(noeud);

	return noeud;
}

void assembleuse_arbre::ajoute_noeud(Noeud *noeud)
{
	m_pile.top()->ajoute_noeud(noeud);
}

Noeud *assembleuse_arbre::cree_noeud(type_noeud type, const DonneesMorceaux &morceau)
{
	Noeud *noeud = nullptr;
	bool reutilise = false;

	switch (type) {
		case type_noeud::RACINE:
			m_memoire_utilisee += sizeof(NoeudRacine);
			noeud = new NoeudRacine(morceau);
			break;
		case type_noeud::APPEL_FONCTION:
			m_memoire_utilisee += sizeof(NoeudAppelFonction);
			noeud = new NoeudAppelFonction(morceau);
			break;
		case type_noeud::DECLARATION_FONCTION:
			m_memoire_utilisee += sizeof(NoeudDeclarationFonction);
			noeud = new NoeudDeclarationFonction(morceau);
			break;
		case type_noeud::ASSIGNATION_VARIABLE:
			m_memoire_utilisee += sizeof(NoeudAssignationVariable);
			noeud = new NoeudAssignationVariable(morceau);
			break;
		case type_noeud::DECLARATION_VARIABLE:
			m_memoire_utilisee += sizeof(NoeudDeclarationVariable);
			noeud = new NoeudDeclarationVariable(morceau);
			break;
		case type_noeud::VARIABLE:
			m_memoire_utilisee += sizeof(NoeudVariable);
			noeud = new NoeudVariable(morceau);
			break;
		case type_noeud::ACCES_MEMBRE:
			m_memoire_utilisee += sizeof(NoeudAccesMembre);
			noeud = new NoeudAccesMembre(morceau);
			break;
		case type_noeud::CARACTERE:
			m_memoire_utilisee += sizeof(NoeudCaractere);
			noeud = new NoeudCaractere(morceau);
			break;
		case type_noeud::NOMBRE_ENTIER:
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
		case type_noeud::NOMBRE_REEL:
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
		case type_noeud::OPERATION_BINAIRE:
			if (!noeuds_op_libres.empty()) {
				noeud = noeuds_op_libres.back();
				noeuds_op_libres.pop_back();
				*noeud = NoeudOperationBinaire(morceau);
				reutilise = true;
			}
			else {
				m_memoire_utilisee += sizeof(NoeudOperationBinaire);
				noeud = new NoeudOperationBinaire(morceau);
			}
			break;
		case type_noeud::OPERATION_UNAIRE:
			m_memoire_utilisee += sizeof(NoeudOperationUnaire);
			noeud = new NoeudOperationUnaire(morceau);
			break;
		case type_noeud::RETOUR:
			m_memoire_utilisee += sizeof(NoeudRetour);
			noeud = new NoeudRetour(morceau);
			break;
		case type_noeud::CONSTANTE:
			m_memoire_utilisee += sizeof(NoeudConstante);
			noeud = new NoeudConstante(morceau);
			break;
		case type_noeud::CHAINE_LITTERALE:
			m_memoire_utilisee += sizeof(NoeudChaineLitterale);
			noeud = new NoeudChaineLitterale(morceau);
			break;
		case type_noeud::BOOLEEN:
			m_memoire_utilisee += sizeof(NoeudBooleen);
			noeud = new NoeudBooleen(morceau);
			break;
		case type_noeud::SI:
			m_memoire_utilisee += sizeof(NoeudSi);
			noeud = new NoeudSi(morceau);
			break;
		case type_noeud::BLOC:
			m_memoire_utilisee += sizeof(NoeudBloc);
			noeud = new NoeudBloc(morceau);
			break;
		case type_noeud::POUR:
			m_memoire_utilisee += sizeof(NoeudPour);
			noeud = new NoeudPour(morceau);
			break;
		case type_noeud::CONTINUE_ARRETE:
			m_memoire_utilisee += sizeof(NoeudContArr);
			noeud = new NoeudContArr(morceau);
			break;
		case type_noeud::BOUCLE:
			m_memoire_utilisee += sizeof(NoeudBoucle);
			noeud = new NoeudBoucle(morceau);
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
	m_pile.top()->genere_code_llvm(contexte_generation);
}

void assembleuse_arbre::supprime_noeud(Noeud *noeud)
{
	switch (noeud->type()) {
		case type_noeud::NOMBRE_ENTIER:
			this->noeuds_entier_libres.push_back(dynamic_cast<NoeudNombreEntier *>(noeud));
			break;
		case type_noeud::NOMBRE_REEL:
			this->noeuds_reel_libres.push_back(dynamic_cast<NoeudNombreReel *>(noeud));
			break;
		case type_noeud::OPERATION_BINAIRE:
			this->noeuds_op_libres.push_back(dynamic_cast<NoeudOperationBinaire *>(noeud));
			break;
		default:
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
	os << "NoeudAssignationVariable : " << sizeof(NoeudAssignationVariable) << '\n';
	os << "NoeudVariable            : " << sizeof(NoeudVariable) << '\n';
	os << "NoeudNombreEntier        : " << sizeof(NoeudNombreEntier) << '\n';
	os << "NoeudNombreReel          : " << sizeof(NoeudNombreReel) << '\n';
	os << "NoeudOperationBinaire    : " << sizeof(NoeudOperationBinaire) << '\n';
	os << "NoeudOperationUnaire     : " << sizeof(NoeudOperationUnaire) << '\n';
	os << "NoeudRetour              : " << sizeof(NoeudRetour) << '\n';
	os << "NoeudConstante           : " << sizeof(NoeudConstante) << '\n';
	os << "NoeudChaineLitterale     : " << sizeof(NoeudChaineLitterale) << '\n';
	os << "NoeudBooleen             : " << sizeof(NoeudBooleen) << '\n';
	os << "NoeudCaractere           : " << sizeof(NoeudCaractere) << '\n';
	os << "NoeudSi                  : " << sizeof(NoeudSi) << '\n';
	os << "NoeudBloc                : " << sizeof(NoeudBloc) << '\n';
	os << "NoeudPour                : " << sizeof(NoeudPour) << '\n';
	os << "NoeudContArr             : " << sizeof(NoeudContArr) << '\n';
	os << "NoeudBoucle              : " << sizeof(NoeudBoucle) << '\n';
	os << "------------------------------------------------------------------\n";
	os << "DonneesType              : " << sizeof(DonneesType) << '\n';
	os << "DonneesMorceaux          : " << sizeof(DonneesMorceaux) << '\n';
	os << "std::list<Noeud *>       : " << sizeof(std::list<Noeud *>) << '\n';
	os << "std::any                 : " << sizeof(std::any) << '\n';
	os << "------------------------------------------------------------------\n";
}
