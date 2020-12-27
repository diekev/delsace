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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "logeuse_memoire.hh"

#include <iostream>
#include <sstream>

namespace memoire {

logeuse_memoire logeuse_memoire::m_instance = logeuse_memoire{};

logeuse_memoire::~logeuse_memoire()
{
	if (memoire_allouee != 0) {
		std::cerr << "Fuite de mémoire ou désynchronisation : "
				  << formate_taille(memoire_allouee) << '\n';

#ifdef DEBOGUE_MEMOIRE
		for (auto &alveole : tableau_allocation) {
			if (alveole.second != 0) {
				std::cerr << '\t' << alveole.first << " : " << formate_taille(alveole.second) << '\n';
			}
		}
#endif
	}
}

logeuse_memoire &logeuse_memoire::instance()
{
	return m_instance;
}

void logeuse_memoire::ajoute_memoire(const char *message, long taille)
{
#ifdef DEBOGUE_MEMOIRE
	tableau_allocation[message] += taille;
#else
	static_cast<void>(message);
#endif

	this->memoire_allouee += taille;
	this->memoire_consommee = std::max(this->memoire_allouee.load(), this->memoire_consommee.load());
}

void logeuse_memoire::enleve_memoire(const char *message, long taille)
{
#ifdef DEBOGUE_MEMOIRE
	tableau_allocation[message] -= taille;
#else
	static_cast<void>(message);
#endif

	this->memoire_allouee -= taille;
}

/* ************************************************************************** */

#define PROTEGE_MEMOIRE

inline long taille_alignee(long taille)
{
	return (taille + 3) & (~3);
}

struct EnteteMemoire {
	const char *message = nullptr;
	long taille = 0;
};

inline void *pointeur_depuis_entete(EnteteMemoire *entete)
{
	return entete + 1;
}

inline EnteteMemoire *entete_depuis_pointeur(void *pointeur)
{
	/* le pointeur pointe sur le bloc, recule d'une EnteteMemoire pour retrouvre l'entête */
	return static_cast<EnteteMemoire *>(pointeur) - 1;
}

void *logeuse_memoire::loge_generique(const char *message, long taille)
{
#ifndef PROTEGE_MEMOIRE
	return malloc(static_cast<size_t>(taille));
#else
	/* aligne la taille désirée */
	taille = taille_alignee(taille);

	EnteteMemoire *entete = static_cast<EnteteMemoire *>(malloc(static_cast<size_t>(taille) + sizeof(EnteteMemoire)));
	entete->message = message;
	entete->taille = taille;

	ajoute_memoire(message, taille);
	nombre_allocations += 1;

	/* saute l'entête et retourne le bloc de la taille désirée */
	return pointeur_depuis_entete(entete);
#endif
}

void *logeuse_memoire::reloge_generique(const char *message, void *ptr, long ancienne_taille, long nouvelle_taille)
{
#ifndef PROTEGE_MEMOIRE
	return realloc(ptr, static_cast<size_t>(nouvelle_taille));
#else

	assert(ptr || ancienne_taille == 0);

	/* calcule la taille alignée correspondante à l'allocation */
	ancienne_taille = taille_alignee(ancienne_taille);

	/* calcule la taille alignée correspondante à l'allocation */
	nouvelle_taille = taille_alignee(nouvelle_taille);

	EnteteMemoire *entete = static_cast<EnteteMemoire *>(ptr);

	/* il est possible d'utiliser reloge avec un pointeur nul, ce qui agit comme un loge */
	if (entete) {
		/* le pointeur pointe sur le bloc, recule d'une EnteteMemoire pour retrouvre l'entête */
		entete = entete_depuis_pointeur(entete);

		if (entete->taille != ancienne_taille) {
			std::cerr << "Désynchronisation pour le bloc '" << entete->message << "' ('" << message << "') lors du relogement !\n";
			std::cerr << "La taille du logement était de " << entete->taille << ", mais la taille reçue est de " << ancienne_taille << " !\n";
		}
	}

	entete = static_cast<EnteteMemoire *>(realloc(entete, sizeof(EnteteMemoire) + static_cast<size_t>(nouvelle_taille)));
	entete->taille = nouvelle_taille;
	entete->message = message;

	ajoute_memoire(message, nouvelle_taille - ancienne_taille);
	nombre_allocations += 1;
	nombre_reallocations += 1;

	/* saute l'entête et retourne le bloc de la taille désirée */
	return pointeur_depuis_entete(entete);
#endif
}

void logeuse_memoire::deloge_generique(const char *message, void *ptr, long taille)
{
#ifndef PROTEGE_MEMOIRE
	free(ptr);
#else
	if (ptr == nullptr) {
		return;
	}

	/* calcule la taille alignée correspondante à l'allocation */
	taille = taille_alignee(taille);

	EnteteMemoire *entete = entete_depuis_pointeur(ptr);

	if (entete->taille != taille) {
		std::cerr << "Désynchronisation pour le bloc '" << entete->message << "' ('" << message << "') lors du délogement !\n";
		std::cerr << "La taille du logement était de " << entete->taille << ", mais la taille reçue est de " << taille << " !\n";
	}

	free(entete);

	enleve_memoire(message, taille);
	nombre_deallocations += 1;
#endif
}

/* ************************************************************************** */

long allouee()
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.memoire_allouee.load();
}

long consommee()
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.memoire_consommee.load();
}

long nombre_allocations()
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.nombre_allocations.load();
}

long nombre_reallocations()
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.nombre_reallocations.load();
}

long nombre_deallocations()
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.nombre_deallocations.load();
}

std::string formate_taille(long octets)
{
	std::stringstream ss;

	if (octets < 0) {
		octets = -octets;
		ss << "-";
	}

	if (octets < 1024) {
		ss << octets << " o";
	}
	else if (octets < (1024 * 1024)) {
		ss << octets / (1024) << " Ko";
	}
	else if (octets < (1024 * 1024 * 1024)) {
		ss << octets / (1024 * 1024) << " Mo";
	}
	else {
		ss << octets / (1024 * 1024 * 1024) << " Go";
	}

	return ss.str();
}

}  /* namespace memoire */
