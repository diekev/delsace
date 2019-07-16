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

#include "rendu_arbre.h"

#include <numeric>

#include "biblinternes/ego/outils.h"
#include "biblinternes/opengl/contexte_rendu.h"
#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/outils/fichier.hh"

#include "coeur/arbre.h"

/* ************************************************************************** */

TamponRendu *cree_tampon_arrete()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::contenu_fichier("nuanceurs/simple.vert"));

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::contenu_fichier("nuanceurs/simple.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");

	tampon->parametres_programme(parametre_programme);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", 0.0f, 0.0f, 0.0f, 1.0f);
	programme->desactive();

	return tampon;
}

TamponRendu *genere_tampon_arrete(Arbre *arbre)
{
	auto const nombre_arretes = arbre->arretes().taille();
	auto const nombre_elements = nombre_arretes * 2;
	auto tampon = cree_tampon_arrete();

	dls::tableau<dls::math::vec3f> sommets;
	sommets.reserve(nombre_elements);

	/* OpenGL ne travaille qu'avec des floats. */
	for (Arrete *arrete : arbre->arretes()) {
		sommets.pousse(arrete->s[0]->pos);
		sommets.pousse(arrete->s[1]->pos);
	}

	dls::tableau<unsigned int> indices(sommets.taille());
	std::iota(indices.debut(), indices.fin(), 0);

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.donnees();
	parametres_tampon.taille_octet_sommets = static_cast<size_t>(sommets.taille()) * sizeof(dls::math::vec3f);
	parametres_tampon.pointeur_index = indices.donnees();
	parametres_tampon.taille_octet_index = static_cast<size_t>(indices.taille()) * sizeof(unsigned int);
	parametres_tampon.elements = static_cast<size_t>(indices.taille());

	tampon->remplie_tampon(parametres_tampon);

	dls::ego::util::GPU_check_errors("Erreur lors de la création du tampon de sommets");

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);

	tampon->parametres_dessin(parametres_dessin);

	return tampon;
}

/* ************************************************************************** */

RenduArbre::RenduArbre(Arbre *arbre)
	: m_arbre(arbre)
{}

RenduArbre::~RenduArbre()
{
	delete m_tampon_arrete;
}

void RenduArbre::initialise()
{
	m_tampon_arrete = genere_tampon_arrete(m_arbre);
}

void RenduArbre::dessine(ContexteRendu const &contexte)
{
	m_tampon_arrete->dessine(contexte);
}
