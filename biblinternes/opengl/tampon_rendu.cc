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

#include "tampon_rendu.h"

#include "biblinternes/ego/outils.h"
#include <GL/glew.h>
#include <mutex>
#include <tbb/concurrent_vector.h>

#include "atlas_texture.h"
#include "contexte_rendu.h"

/* ************************************************************************** */

void ParametresProgramme::ajoute_attribut(std::string const &nom)
{
	m_attributs.push_back(nom);
}

void ParametresProgramme::ajoute_uniforme(std::string const &nom)
{
	m_uniformes.push_back(nom);
}

std::vector<std::string> const &ParametresProgramme::attributs() const
{
	return m_attributs;
}

std::vector<std::string> const &ParametresProgramme::uniformes() const
{
	return m_uniformes;
}

/* ************************************************************************** */

void ParametresDessin::type_dessin(unsigned int type)
{
	m_type_dessin = type;
}

unsigned int ParametresDessin::type_dessin() const
{
	return m_type_dessin;
}

void ParametresDessin::type_donnees(unsigned int type)
{
	m_type_donnees = type;
}

unsigned int ParametresDessin::type_donnees() const
{
	return m_type_donnees;
}

float ParametresDessin::taille_ligne() const
{
	return m_taille_ligne;
}

void ParametresDessin::taille_ligne(float taille)
{
	m_taille_ligne = taille;
}

float ParametresDessin::taille_point() const
{
	return m_taille_point;
}

void ParametresDessin::taille_point(float taille)
{
	m_taille_point = taille;
}

/* ************************************************************************** */

TamponRendu::~TamponRendu()
{
	delete m_atlas;
}

void TamponRendu::charge_source_programme(dls::ego::Nuanceur type_programme, std::string const &source, std::ostream &os)
{
	m_programme.charge(type_programme, source, os);
}

void TamponRendu::parametres_programme(ParametresProgramme const &parametres)
{
	m_programme.active();

	for (auto const &attribut : parametres.attributs()) {
		m_programme.ajoute_attribut(attribut);
	}

	for (auto const &uniform : parametres.uniformes()) {
		m_programme.ajoute_uniforme(uniform);
	}

	m_programme.desactive();
}

void TamponRendu::parametres_dessin(ParametresDessin const &parametres)
{
	m_paramatres_dessin = parametres;
}

void TamponRendu::finalise_programme(std::ostream &os)
{
	m_programme.cree_et_lie_programme(os);
}

void TamponRendu::peut_surligner(bool ouinon)
{
	m_peut_surligner = ouinon;
}

void TamponRendu::initialise_tampon()
{
	if (m_donnees_tampon == nullptr) {
		m_donnees_tampon = dls::ego::TamponObjet::cree_unique();
	}
}

void TamponRendu::remplie_tampon(ParametresTampon const &parametres)
{
	initialise_tampon();

	m_elements = parametres.elements;

	m_donnees_tampon->attache();

	m_donnees_tampon->genere_tampon_sommet(
				parametres.pointeur_sommets,
				static_cast<long>(parametres.taille_octet_sommets));

	dls::ego::util::GPU_check_errors("Erreur lors du tampon sommet");

	if (parametres.pointeur_index) {
		m_donnees_tampon->genere_tampon_index(
					parametres.pointeur_index,
					static_cast<long>(parametres.taille_octet_index));

		dls::ego::util::GPU_check_errors("Erreur lors du tampon index");

		m_dessin_indexe = true;
	}

	m_donnees_tampon->pointeur_attribut(
				static_cast<unsigned>(m_programme[parametres.attribut]),
				parametres.dimension_attribut);

	dls::ego::util::GPU_check_errors("Erreur lors de la mise en place de l'attribut");

	m_donnees_tampon->detache();
}

void TamponRendu::remplie_tampon_extra(ParametresTampon const &parametres)
{
	initialise_tampon();

	m_donnees_tampon->attache();

	m_donnees_tampon->genere_tampon_extra(
				parametres.pointeur_donnees_extra,
				static_cast<long>(parametres.taille_octet_donnees_extra));
	dls::ego::util::GPU_check_errors("Erreur lors de la génération du tampon extra");

	m_donnees_tampon->pointeur_attribut(
				static_cast<unsigned>(m_programme[parametres.attribut]),
				parametres.dimension_attribut);

	dls::ego::util::GPU_check_errors("Erreur lors de la mise en place du pointeur");

	m_donnees_tampon->detache();

	if (parametres.attribut == "normal" || parametres.attribut == "normaux") {
		m_requiers_normal = true;
	}
}

void TamponRendu::dessine(ContexteRendu const &contexte)
{
	if (!m_programme.est_valide()) {
		std::cerr << "Programme invalide !\n";
		return;
	}

	if (m_paramatres_dessin.type_dessin() == GL_POINTS) {
		glPointSize(m_paramatres_dessin.taille_point());
	}
	else if (m_paramatres_dessin.type_dessin() == GL_LINES) {
		glLineWidth(m_paramatres_dessin.taille_ligne());
	}

	dls::ego::util::GPU_check_errors("Erreur lors du rendu du tampon avant utilisation programme");

	m_programme.active();
	m_donnees_tampon->attache();

	if (m_texture) {
		m_texture->attache();
	}

	if (m_texture_3d) {
		m_texture_3d->attache();
	}

	if (m_atlas) {
		m_atlas->attache();
	}

	dls::ego::util::GPU_check_errors("Erreur lors du rendu du tampon après attache texture");

	glUniformMatrix4fv(m_programme("matrice"), 1, GL_FALSE, &contexte.matrice_objet()[0][0]);
	dls::ego::util::GPU_check_errors("Erreur lors du passage de la matrice objet");
	glUniformMatrix4fv(m_programme("MVP"), 1, GL_FALSE, &contexte.MVP()[0][0]);
	dls::ego::util::GPU_check_errors("Erreur lors du passage de la matrice MVP");

	if (m_requiers_normal) {
		glUniformMatrix3fv(m_programme("N"), 1, GL_FALSE, &contexte.normal()[0][0]);
		dls::ego::util::GPU_check_errors("Erreur lors du passage de la matrice N");
	}

	if (m_peut_surligner) {
		glUniform1i(m_programme("pour_surlignage"), contexte.pour_surlignage());
		dls::ego::util::GPU_check_errors("Erreur lors du passage pour_surlignage");
	}

	if (m_dessin_indexe) {
		glDrawElements(m_paramatres_dessin.type_dessin(), static_cast<int>(m_elements), m_paramatres_dessin.type_donnees(), nullptr);
	}
	else {
		glDrawArrays(m_paramatres_dessin.type_dessin(), 0, static_cast<int>(m_elements));
	}

	dls::ego::util::GPU_check_errors("Erreur lors du rendu du tampon après dessin indexé");

	if (m_atlas) {
		m_atlas->detache();
	}

	if (m_texture) {
		m_texture->detache();
	}

	if (m_texture_3d) {
		m_texture_3d->detache();
	}

	m_donnees_tampon->detache();
	m_programme.desactive();

	if (m_paramatres_dessin.type_dessin() == GL_POINTS) {
		glPointSize(1.0f);
	}
	else if (m_paramatres_dessin.type_dessin() == GL_LINES) {
		glLineWidth(1.0f);
	}
}

dls::ego::Programme *TamponRendu::programme()
{
	return &m_programme;
}

void TamponRendu::ajoute_atlas()
{
	if (m_atlas == nullptr) {
		m_atlas = new AtlasTexture(0);
	}
}

void TamponRendu::ajoute_texture()
{
	m_texture = dls::ego::Texture2D::cree_unique(0);
}

void TamponRendu::ajoute_texture_3d()
{
	m_texture_3d = dls::ego::Texture3D::cree_unique(0);
}

dls::ego::Texture2D *TamponRendu::texture()
{
	return m_texture.get();
}

dls::ego::Texture3D *TamponRendu::texture_3d()
{
	return m_texture_3d.get();
}

AtlasTexture *TamponRendu::atlas()
{
	return m_atlas;
}

/* ************************************************************************** */

static tbb::concurrent_vector<TamponRendu *> poubelle_tampon;
static std::mutex mutex_poubelle;

void supprime_tampon_rendu(TamponRendu *tampon)
{
	poubelle_tampon.push_back(tampon);
}

void purge_tous_les_tampons()
{
	{
		std::unique_lock<std::mutex> lock(mutex_poubelle);

		for (TamponRendu *tampon : poubelle_tampon) {
			delete tampon;
		}
	}

	poubelle_tampon.clear();
}
