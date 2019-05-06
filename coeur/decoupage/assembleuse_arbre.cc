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

#include <sstream>

#include "broyage.hh"
#include "contexte_generation_code.h"
#include "coulisse_c.hh"
#include "coulisse_llvm.hh"

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
	auto noeud = new noeud::base(contexte, morceau);
	m_memoire_utilisee += sizeof(noeud::base);

	/* À FAIRE : réutilise la mémoire des noeuds libérés. */

	if (noeud != nullptr) {
		noeud->type = type;

		if (type == type_noeud::APPEL_FONCTION) {
			/* requis pour pouvoir renseigner le noms de arguments depuis
			 * l'analyse. */
			noeud->valeur_calculee = std::list<std::string_view>{};

			/* requis pour déterminer le module dans le noeud d'accès point
			 * À FAIRE : trouver mieux pour accéder à cette information */
			noeud->module_appel = noeud->morceau.module;
		}

		m_noeuds.push_back(noeud);
	}

	return noeud;
}

void assembleuse_arbre::depile_noeud(type_noeud type)
{
	assert(m_pile.top()->type == type);
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

	noeud::genere_code_llvm(m_pile.top(), contexte_generation, false);
}

void assembleuse_arbre::genere_code_C(
		ContexteGenerationCode &contexte_generation,
		std::ostream &os)
{
	if (m_pile.empty()) {
		return;
	}

	/* Pour malloc/free, nous incluons toujours "stdlib.h". */
	os << "#include <stdlib.h>\n";

	for (auto const &inc : this->inclusions) {
		os << "#include <" << inc << ">\n";
	}

	os << "\n";

	os << "static long __VG_memoire_utilisee__ = 0;";
	os << "static long ";
	os << broye_nom_fonction("mémoire_utilisée", "");
	os << "() { return __VG_memoire_utilisee__; }";

	/* NOTE : les initiliaseurs des infos types doivent être valide pour toute
	 * la durée du programme, donc nous les mettons dans la fonction principale.
	 */
	std::stringstream ss_infos_types;

	noeud::genere_code_C(m_pile.top(), contexte_generation, false, os, ss_infos_types);

	auto debut_main =
R"(
int main(int argc, char **argv)
{
	Tableau_char_ptr_ tabl_args;
	tabl_args.pointeur = argv;
	tabl_args.taille = argc;
)";

	auto fin_main =
R"(
	return principale(tabl_args);
}
)";

	os << debut_main;
	os << ss_infos_types.str();
	os << fin_main;
}

void assembleuse_arbre::supprime_noeud(noeud::base *noeud)
{
	this->noeuds_libres.push_back(noeud);
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
	os << "noeud::base              : " << sizeof(noeud::base) << '\n';
	os << "DonneesType              : " << sizeof(DonneesType) << '\n';
	os << "DonneesMorceaux          : " << sizeof(DonneesMorceaux) << '\n';
	os << "std::list<noeud::base *> : " << sizeof(std::list<noeud::base *>) << '\n';
	os << "std::any                 : " << sizeof(std::any) << '\n';
	os << "------------------------------------------------------------------\n";
}
