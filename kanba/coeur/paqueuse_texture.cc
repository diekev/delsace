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

#include "paqueuse_texture.h"

#include "maillage.h"

#undef LOGUE_EMPATQUETTAGE

#undef QUEUE_PRIORITE

#ifdef LOGUE_EMPATQUETTAGE
#	include <iostream>
#	define LOG std::cerr
#else
struct LoggeuseEmpaquettage {};

template <typename T>
LoggeuseEmpaquettage &operator<<(LoggeuseEmpaquettage &le, const T&)
{
	return le;
}

static LoggeuseEmpaquettage loggeuse_empaquettage;
#	define LOG loggeuse_empaquettage
#endif

uint64_t empreinte_xy(unsigned x, unsigned y)
{
	return uint64_t(x) << 32ul | uint64_t(y);
}

static std::map<uint64_t, Polygone *> tableau_coord_polygones;

PaqueuseTexture::Noeud::~Noeud()
{
	if (droite) {
		delete droite;
	}

	if (gauche) {
		delete gauche;
	}
}

PaqueuseTexture::PaqueuseTexture()
	: m_racine(new Noeud())
{}

PaqueuseTexture::~PaqueuseTexture()
{
	delete m_racine;
}

void PaqueuseTexture::empaquete(const std::vector<Polygone *> &polygones)
{
	LOG << "Début de la création de la texture paquetée...\n";

	const auto largeur_max = polygones[0]->res_u * polygones.size();
	const auto hauteur_max = polygones[0]->res_v * polygones.size();
	m_racine->largeur = 16384;
	m_racine->hauteur = 16384;

	LOG << "Taille noeud racine : "
		<< m_racine->largeur << 'x' << m_racine->hauteur << "...\n";

#ifdef QUEUE_PRIORITE
	m_queue_priorite.push(m_racine);
#endif

	Noeud *noeud;

	for (const auto &polygone : polygones) {
#ifdef QUEUE_PRIORITE
		while (!m_queue_priorite.empty()) {
			noeud = m_queue_priorite.top();

			if (polygone->res_u <= noeud->largeur && polygone->res_v <= noeud->hauteur) {
				noeud = brise_noeud(noeud, polygone->res_u, polygone->res_v);
				polygone->x = noeud->x;
				polygone->y = noeud->y;

				LOG << "Assignation des coordonnées "
					<< polygone->x << ',' << polygone->y
					<< " au polygone " << polygone->index
					<< " (" << polygone->res_u << 'x' << polygone->res_v << ')' << '\n';

				break;
			}
		}
#else
		noeud = trouve_noeud(m_racine, polygone->res_u, polygone->res_v);

		if (noeud != nullptr) {
			noeud = brise_noeud(noeud, polygone->res_u, polygone->res_v);
		}
		else {
			noeud = elargi_noeud(static_cast<unsigned>(largeur_max), static_cast<unsigned>(hauteur_max));

			if (!noeud) {
				LOG << "N'arrive pas à élargir texture "
					<< m_racine->largeur << 'x' << m_racine->hauteur
					<< " pour un polygone de taille "
					<< polygone->res_u << 'x' << polygone->res_v << '\n';
			}
		}

		polygone->x = noeud->x;
		polygone->y = noeud->y;

		const auto empreinte = empreinte_xy(polygone->x, polygone->y);

		if (tableau_coord_polygones.find(empreinte) != tableau_coord_polygones.end()) {
			//std::cerr << "Plusieurs polygones ont les mêmes coordonnées !\n";
		}
		else {
			tableau_coord_polygones.insert({ empreinte, polygone });
		}

		LOG << "Assignation des coordonnées "
			<< polygone->x << ',' << polygone->y
			<< " au polygone " << polygone->index
			<< " (" << polygone->res_u << 'x' << polygone->res_v << ')' << '\n';

		max_x = std::max(max_x, polygone->x + polygone->res_u);
		max_y = std::max(max_y, polygone->y + polygone->res_v);
#endif
	}

	LOG << "Taille texture finale : "
		<< largeur() << 'x' << hauteur() << "...\n";
}

unsigned int PaqueuseTexture::largeur() const
{
	return max_x; //m_racine->largeur;
}

unsigned int PaqueuseTexture::hauteur() const
{
	return max_y; //m_racine->hauteur;
}

PaqueuseTexture::Noeud *PaqueuseTexture::trouve_noeud(
		Noeud *racine,
		unsigned largeur,
		unsigned hauteur)
{
	//LOG << "Recherche d'un noeud de taille " << largeur << 'x' << hauteur << "...\n";

	if (racine->utilise) {
		Noeud *noeud = nullptr;

		if (racine->droite) {
			noeud = trouve_noeud(racine->droite, largeur, hauteur);
		}

		if (noeud == nullptr && racine->gauche) {
			noeud = trouve_noeud(racine->gauche, largeur, hauteur);
		}

		return noeud;
	}

	if (largeur <= racine->largeur && hauteur <= racine->hauteur) {
		return racine;
	}

	return nullptr;
}

PaqueuseTexture::Noeud *PaqueuseTexture::brise_noeud(
		Noeud *noeud,
		unsigned largeur,
		unsigned hauteur)
{
	noeud->utilise = true;

	if (noeud->hauteur > hauteur) {
		noeud->gauche = new Noeud();
		noeud->gauche->x = noeud->x;
		noeud->gauche->y = noeud->y + hauteur;
		noeud->gauche->largeur = noeud->largeur;
		noeud->gauche->hauteur = noeud->hauteur - hauteur;

#ifdef QUEUE_PRIORITE
		m_queue_priorite.push(noeud->gauche);
#endif
	}

	if (noeud->largeur > largeur) {
		noeud->droite = new Noeud();
		noeud->droite->x = noeud->x + largeur;
		noeud->droite->y = noeud->y;
		noeud->droite->largeur = noeud->largeur - largeur;
		noeud->droite->hauteur = noeud->hauteur;

#ifdef QUEUE_PRIORITE
		m_queue_priorite.push(noeud->droite);
#endif
	}

	return noeud;
}

PaqueuseTexture::Noeud *PaqueuseTexture::elargi_noeud(unsigned largeur, unsigned hauteur)
{
	LOG << __func__ << '\n';
#if 1
	if (m_racine->largeur < m_racine->hauteur) {
		return elargi_largeur(largeur, hauteur);
	}

	return elargi_hauteur(largeur, hauteur);
#else
	const auto peut_elargir_hauteur = largeur <= m_racine->largeur;
	const auto peut_elargir_largeur = hauteur <= m_racine->hauteur;

	/* Essaie de rester carrée en élargissant la largeur quand la hauteur
		 * est bien plus grande que la largeur. */
	const auto doit_elargir_largeur = peut_elargir_largeur
									  && (m_racine->hauteur >= (m_racine->largeur + largeur));

	/* Essaie de rester carrée en élargissant la hauteur quand la largeur
		 * est bien plus grande que la hauteur. */
	const auto doit_elargir_hauteur = peut_elargir_hauteur
									  && (m_racine->largeur >= (m_racine->hauteur + hauteur));

	if (doit_elargir_largeur) {
		return elargi_largeur(largeur, hauteur);
	}

	if (doit_elargir_hauteur) {
		return elargi_hauteur(largeur, hauteur);
	}

	if (peut_elargir_largeur) {
		return elargi_largeur(largeur, hauteur);
	}

	if (peut_elargir_hauteur) {
		return elargi_hauteur(largeur, hauteur);
	}

	/* Il faut choisir une bonne taille de base pour éviter de retourner ici. */
	return nullptr;
#endif
}

PaqueuseTexture::Noeud *PaqueuseTexture::elargi_largeur(unsigned largeur, unsigned hauteur)
{
	LOG << __func__ << '\n';

	auto racine = new Noeud();
	racine->utilise = true;
	racine->x = 0;
	racine->y = 0;
	racine->hauteur = m_racine->hauteur;
	racine->largeur = m_racine->largeur + largeur;
	racine->gauche = m_racine;
	racine->droite = new Noeud();
	racine->droite->x = m_racine->largeur;
	racine->droite->y = 0;
	racine->droite->largeur = largeur;
	racine->droite->hauteur = m_racine->largeur;

	m_racine = racine;

	auto noeud = trouve_noeud(m_racine, largeur, hauteur);

	if (noeud) {
		return brise_noeud(noeud, largeur, hauteur);
	}

	return nullptr;
}

PaqueuseTexture::Noeud *PaqueuseTexture::elargi_hauteur(unsigned largeur, unsigned hauteur)
{
	LOG << __func__ << '\n';

	auto racine = new Noeud();
	racine->utilise = true;
	racine->x = 0;
	racine->y = 0;
	racine->hauteur = m_racine->hauteur + hauteur;
	racine->largeur = m_racine->largeur;
	racine->droite = m_racine;
	racine->gauche = new Noeud();
	racine->gauche->x = 0;
	racine->gauche->y = m_racine->hauteur;
	racine->gauche->largeur = m_racine->largeur;
	racine->gauche->hauteur = largeur;

	m_racine = racine;

	auto noeud = trouve_noeud(m_racine, largeur, hauteur);

	if (noeud) {
		return brise_noeud(noeud, largeur, hauteur);
	}

	return nullptr;
}
