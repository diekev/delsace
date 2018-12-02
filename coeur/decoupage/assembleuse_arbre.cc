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

Noeud *assembleuse_arbre::empile_noeud(type_noeud type, ContexteGenerationCode &contexte, DonneesMorceaux const &morceau, bool ajoute)
{
	auto noeud = cree_noeud(type, contexte, morceau);

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

Noeud *assembleuse_arbre::cree_noeud(type_noeud type, ContexteGenerationCode &contexte, DonneesMorceaux const &morceau)
{
	Noeud *noeud = nullptr;
	bool reutilise = false;

	switch (type) {
		case type_noeud::RACINE:
			m_memoire_utilisee += sizeof(NoeudRacine);
			noeud = new NoeudRacine(contexte, morceau);
			break;
		case type_noeud::APPEL_FONCTION:
			m_memoire_utilisee += sizeof(NoeudAppelFonction);
			noeud = new NoeudAppelFonction(contexte, morceau);
			break;
		case type_noeud::DECLARATION_FONCTION:
			m_memoire_utilisee += sizeof(NoeudDeclarationFonction);
			noeud = new NoeudDeclarationFonction(contexte, morceau);
			break;
		case type_noeud::ASSIGNATION_VARIABLE:
			m_memoire_utilisee += sizeof(NoeudAssignationVariable);
			noeud = new NoeudAssignationVariable(contexte, morceau);
			break;
		case type_noeud::DECLARATION_VARIABLE:
			m_memoire_utilisee += sizeof(NoeudDeclarationVariable);
			noeud = new NoeudDeclarationVariable(contexte, morceau);
			break;
		case type_noeud::VARIABLE:
			m_memoire_utilisee += sizeof(NoeudVariable);
			noeud = new NoeudVariable(contexte, morceau);
			break;
		case type_noeud::ACCES_MEMBRE:
			m_memoire_utilisee += sizeof(NoeudAccesMembre);
			noeud = new NoeudAccesMembre(contexte, morceau);
			break;
		case type_noeud::CARACTERE:
			m_memoire_utilisee += sizeof(NoeudCaractere);
			noeud = new NoeudCaractere(contexte, morceau);
			break;
		case type_noeud::NOMBRE_ENTIER:
			if (!noeuds_entier_libres.empty()) {
				noeud = noeuds_entier_libres.back();
				noeuds_entier_libres.pop_back();
				/* Utilisation du 'placement new' pour construire à l'endroit du
				 * pointeur, car morceau est une référence, et nous ne pouvons
				 * appeler le constructeur de copie à cause de cette référence. */
				new(noeud) NoeudNombreEntier(contexte, morceau);
				reutilise = true;
			}
			else {
				m_memoire_utilisee += sizeof(NoeudNombreEntier);
				noeud = new NoeudNombreEntier(contexte, morceau);
			}
			break;
		case type_noeud::NOMBRE_REEL:
			if (!noeuds_reel_libres.empty()) {
				noeud = noeuds_reel_libres.back();
				noeuds_reel_libres.pop_back();
				/* Utilisation du 'placement new' pour construire à l'endroit du
				 * pointeur, car morceau est une référence, et nous ne pouvons
				 * appeler le constructeur de copie à cause de cette référence. */
				new(noeud) NoeudNombreReel(contexte, morceau);
				reutilise = true;
			}
			else {
				m_memoire_utilisee += sizeof(NoeudNombreReel);
				noeud = new NoeudNombreReel(contexte, morceau);
			}
			break;
		case type_noeud::OPERATION_BINAIRE:
			if (!noeuds_op_libres.empty()) {
				noeud = noeuds_op_libres.back();
				noeuds_op_libres.pop_back();
				/* Utilisation du 'placement new' pour construire à l'endroit du
				 * pointeur, car morceau est une référence, et nous ne pouvons
				 * appeler le constructeur de copie à cause de cette référence. */
				new(noeud) NoeudOperationBinaire(contexte, morceau);
				reutilise = true;
			}
			else {
				m_memoire_utilisee += sizeof(NoeudOperationBinaire);
				noeud = new NoeudOperationBinaire(contexte, morceau);
			}
			break;
		case type_noeud::OPERATION_UNAIRE:
			m_memoire_utilisee += sizeof(NoeudOperationUnaire);
			noeud = new NoeudOperationUnaire(contexte, morceau);
			break;
		case type_noeud::RETOUR:
			m_memoire_utilisee += sizeof(NoeudRetour);
			noeud = new NoeudRetour(contexte, morceau);
			break;
		case type_noeud::CONSTANTE:
			m_memoire_utilisee += sizeof(NoeudConstante);
			noeud = new NoeudConstante(contexte, morceau);
			break;
		case type_noeud::CHAINE_LITTERALE:
			m_memoire_utilisee += sizeof(NoeudChaineLitterale);
			noeud = new NoeudChaineLitterale(contexte, morceau);
			break;
		case type_noeud::BOOLEEN:
			m_memoire_utilisee += sizeof(NoeudBooleen);
			noeud = new NoeudBooleen(contexte, morceau);
			break;
		case type_noeud::SI:
			m_memoire_utilisee += sizeof(NoeudSi);
			noeud = new NoeudSi(contexte, morceau);
			break;
		case type_noeud::BLOC:
			m_memoire_utilisee += sizeof(NoeudBloc);
			noeud = new NoeudBloc(contexte, morceau);
			break;
		case type_noeud::POUR:
			m_memoire_utilisee += sizeof(NoeudPour);
			noeud = new NoeudPour(contexte, morceau);
			break;
		case type_noeud::CONTINUE_ARRETE:
			m_memoire_utilisee += sizeof(NoeudContArr);
			noeud = new NoeudContArr(contexte, morceau);
			break;
		case type_noeud::BOUCLE:
			m_memoire_utilisee += sizeof(NoeudBoucle);
			noeud = new NoeudBoucle(contexte, morceau);
			break;
		case type_noeud::TRANSTYPE:
			m_memoire_utilisee += sizeof(NoeudTranstype);
			noeud = new NoeudTranstype(contexte, morceau);
			break;
		case type_noeud::NUL:
			m_memoire_utilisee += sizeof(NoeudNul);
			noeud = new NoeudNul(contexte, morceau);
			break;
		case type_noeud::TAILLE_DE:
			m_memoire_utilisee += sizeof(NoeudTailleDe);
			noeud = new NoeudTailleDe(contexte, morceau);
			break;
		case type_noeud::PLAGE:
			m_memoire_utilisee += sizeof(NoeudPlage);
			noeud = new NoeudPlage(contexte, morceau);
			break;
		case type_noeud::ACCES_MEMBRE_POINT:
			m_memoire_utilisee += sizeof(NoeudAccesMembrePoint);
			noeud = new NoeudAccesMembrePoint(contexte, morceau);
			break;
		case type_noeud::DIFFERE:
			m_memoire_utilisee += sizeof(NoeudDefere);
			noeud = new NoeudDefere(contexte, morceau);
			break;
		case type_noeud::NONSUR:
			m_memoire_utilisee += sizeof(NoeudNonSur);
			noeud = new NoeudNonSur(contexte, morceau);
			break;
		case type_noeud::TABLEAU:
			m_memoire_utilisee += sizeof(NoeudTableau);
			noeud = new NoeudTableau(contexte, morceau);
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

size_t assembleuse_arbre::nombre_noeuds() const
{
	return m_noeuds.size();
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
	os << "NoeudTranstype           : " << sizeof(NoeudTranstype) << '\n';
	os << "NoeudNul                 : " << sizeof(NoeudNul) << '\n';
	os << "NoeudTailleDe            : " << sizeof(NoeudTailleDe) << '\n';
	os << "NoeudPlage               : " << sizeof(NoeudPlage) << '\n';
	os << "NoeudAccesMembrePoint    : " << sizeof(NoeudAccesMembrePoint) << '\n';
	os << "NoeudDefere              : " << sizeof(NoeudDefere) << '\n';
	os << "NoeudNonSur              : " << sizeof(NoeudNonSur) << '\n';
	os << "NoeudTableau             : " << sizeof(NoeudTableau) << '\n';
	os << "------------------------------------------------------------------\n";
	os << "DonneesType              : " << sizeof(DonneesType) << '\n';
	os << "DonneesMorceaux          : " << sizeof(DonneesMorceaux) << '\n';
	os << "std::list<Noeud *>       : " << sizeof(std::list<Noeud *>) << '\n';
	os << "std::any                 : " << sizeof(std::any) << '\n';
	os << "------------------------------------------------------------------\n";
}
