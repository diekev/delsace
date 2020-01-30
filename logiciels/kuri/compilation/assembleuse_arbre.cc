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
#include "coulisse_c.hh"

#ifdef AVEC_LLVM
#include "coulisse_llvm.hh"
#endif

#include "modules.hh"

assembleuse_arbre::assembleuse_arbre(ContexteGenerationCode &contexte)
{
	contexte.assembleuse = this;
	this->empile_noeud(type_noeud::RACINE, contexte, {});

	/* Pour fprintf dans les messages d'erreurs, nous incluons toujours "stdio.h". */
	this->ajoute_inclusion("stdio.h");
	/* Pour malloc/free, nous incluons toujours "stdlib.h". */
	this->ajoute_inclusion("stdlib.h");
	/* Pour strlen, nous incluons toujours "string.h". */
	this->ajoute_inclusion("string.h");
	/* Pour les coroutines nous incluons toujours pthread */
	this->ajoute_inclusion("pthread.h");
	this->bibliotheques.pousse("pthread");
	this->definitions.pousse("_REENTRANT");
}

assembleuse_arbre::~assembleuse_arbre()
{
	for (auto noeud : m_noeuds) {
		memoire::deloge("noeud_base", noeud);
	}
}

noeud::base *assembleuse_arbre::empile_noeud(type_noeud type, ContexteGenerationCode &contexte, DonneesLexeme const &morceau, bool ajoute)
{
	auto noeud = cree_noeud(type, contexte, morceau);

	if (!m_pile.est_vide() && ajoute) {
		this->ajoute_noeud(noeud);
	}

	m_pile.empile(noeud);

	return noeud;
}

void assembleuse_arbre::ajoute_noeud(noeud::base *noeud)
{
	m_pile.haut()->ajoute_noeud(noeud);
}

noeud::base *assembleuse_arbre::cree_noeud(
		type_noeud type,
		ContexteGenerationCode &contexte,
		DonneesLexeme const &morceau)
{
	auto noeud = memoire::loge<noeud::base>("noeud_base", contexte, morceau);
	m_memoire_utilisee += sizeof(noeud::base);

	/* À FAIRE : réutilise la mémoire des noeuds libérés. */

	if (noeud != nullptr) {
		noeud->type = type;

		if (type == type_noeud::APPEL_FONCTION) {
			/* requis pour pouvoir renseigner le noms de arguments depuis
			 * l'analyse. */
			noeud->valeur_calculee = dls::liste<dls::vue_chaine_compacte>{};

			/* requis pour déterminer le module dans le noeud d'accès point
			 * À FAIRE : trouver mieux pour accéder à cette information */
			noeud->module_appel = noeud->morceau.fichier;
		}

		m_noeuds.pousse(noeud);
	}

	return noeud;
}

void assembleuse_arbre::depile_noeud(type_noeud type)
{
	assert(m_pile.haut()->type == type);
	m_pile.depile();
	static_cast<void>(type);
}

void assembleuse_arbre::imprime_code(std::ostream &os)
{
	os << "------------------------------------------------------------------\n";
	m_pile.haut()->imprime_code(os, 0);
	os << "------------------------------------------------------------------\n";
}

void assembleuse_arbre::supprime_noeud(noeud::base *noeud)
{
	this->noeuds_libres.pousse(noeud);
}

size_t assembleuse_arbre::memoire_utilisee() const
{
	return m_memoire_utilisee + nombre_noeuds() * sizeof(noeud::base *);
}

size_t assembleuse_arbre::nombre_noeuds() const
{
	return static_cast<size_t>(m_noeuds.taille());
}

void assembleuse_arbre::ajoute_inclusion(const dls::vue_chaine_compacte &fichier)
{
	if (deja_inclus.trouve(fichier) != deja_inclus.fin()) {
		return;
	}

	deja_inclus.insere(fichier);
	inclusions.pousse(fichier);
}

noeud::base *assembleuse_arbre::racine() const
{
	return m_pile.haut();
}

void imprime_taille_memoire_noeud(std::ostream &os)
{
	os << "------------------------------------------------------------------\n";
	os << "noeud::base              : " << sizeof(noeud::base) << '\n';
	os << "DonneesTypeFinal         : " << sizeof(DonneesTypeFinal) << '\n';
	os << "DonneesLexeme          : " << sizeof(DonneesLexeme) << '\n';
	os << "dls::liste<noeud::base *> : " << sizeof(dls::liste<noeud::base *>) << '\n';
	os << "std::any                 : " << sizeof(std::any) << '\n';
	os << "------------------------------------------------------------------\n";
}
