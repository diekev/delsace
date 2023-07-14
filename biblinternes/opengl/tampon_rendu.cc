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

#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/fichier.hh"

#include "atlas_texture.h"
#include "contexte_rendu.h"

/* ------------------------------------------------------------------------- */
/** \name SourcesGLSL
 * \{ */

std::optional<SourcesGLSL> crée_sources_glsl_depuis_texte(dls::chaine const &vertex,
                                                          dls::chaine const &fragment)
{
    if (vertex.est_vide()) {
        return {};
    }

    if (fragment.est_vide()) {
        return {};
    }

    return SourcesGLSL{vertex, fragment};
}

std::optional<SourcesGLSL> crée_sources_glsl_depuis_fichier(dls::chaine const &fichier_vertex,
                                                            dls::chaine const &fichier_fragment)
{
    auto vertex = dls::contenu_fichier(fichier_vertex);
    if (vertex.est_vide()) {
        return {};
    }

    auto fragment = dls::contenu_fichier(fichier_fragment);
    if (fragment.est_vide()) {
        return {};
    }

    return SourcesGLSL{vertex, fragment};
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Parsage Sources GLSL
 * \{ */


static dls::chaine extrait_nom_attribut_ou_uniforme(dls::chaine const &ligne)
{
    auto dernière_espace = ligne.trouve_dernier_de(' ');
    auto point_virgule = ligne.trouve_dernier_de(';');
    return ligne.sous_chaine(dernière_espace + 1, point_virgule - dernière_espace - 1);
}

static void parse_attributs_et_uniformes_depuis(dls::chaine const &source,
                                                ParametresProgramme &résultat)
{
    auto lignes = dls::morcelle(source, '\n');

    // À FAIRE : in/out
    // À FAIRE : location
    // À FAIRE : déduplique les attributs/uniformes
    POUR (lignes) {
        if (it.sous_chaine(0, 6) == "layout") {
            auto attribut = extrait_nom_attribut_ou_uniforme(it);
            résultat.ajoute_attribut(attribut);
            // std::cerr << "attribut : '" << attribut << "'\n";
        }
        else if (it.sous_chaine(0, 7) == "uniform") {
            auto uniform = extrait_nom_attribut_ou_uniforme(it);
            résultat.ajoute_uniforme(uniform);
            // std::cerr << "uniform : '" << uniform << "'\n";
        }
    }
}

static ParametresProgramme parse_sources_glsl(SourcesGLSL const &sources)
{
    // À FAIRE : valide que tout est là
    ParametresProgramme résultat;
    parse_attributs_et_uniformes_depuis(sources.vertex, résultat);
    parse_attributs_et_uniformes_depuis(sources.fragment, résultat);

    // À FAIRE : nous n'avons pas tout le temps besoin de ça.
    résultat.ajoute_uniforme("matrice");
    résultat.ajoute_uniforme("MVP");
    return résultat;
}

/** \} */

/* ************************************************************************** */

void ParametresProgramme::ajoute_attribut(dls::chaine const &nom)
{
	m_attributs.ajoute(nom);
}

void ParametresProgramme::ajoute_uniforme(dls::chaine const &nom)
{
	m_uniformes.ajoute(nom);
}

dls::tableau<dls::chaine> const &ParametresProgramme::attributs() const
{
	return m_attributs;
}

dls::tableau<dls::chaine> const &ParametresProgramme::uniformes() const
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

std::unique_ptr<TamponRendu> TamponRendu::crée_unique(SourcesGLSL const &sources)
{
    auto résultat = std::make_unique<TamponRendu>();

    if (!sources.vertex.est_vide()) {
        résultat->charge_source_programme(dls::ego::Nuanceur::VERTEX, sources.vertex);
    }

    if (!sources.fragment.est_vide()) {
        résultat->charge_source_programme(dls::ego::Nuanceur::FRAGMENT, sources.fragment);
    }

    résultat->finalise_programme();

    ParametresProgramme parametre_programme = parse_sources_glsl(sources);
    résultat->parametres_programme(parametre_programme);

    return résultat;
}

void TamponRendu::charge_source_programme(dls::ego::Nuanceur type_programme, dls::chaine const &source, std::ostream &os)
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

void TamponRendu::remplie_tampon_matrices_instance(const ParametresTampon &parametres)
{
	initialise_tampon();

	m_donnees_tampon->attache();

	m_instance = true;
	m_nombre_instances = parametres.nombre_instances;

	m_donnees_tampon->genere_tampon_extra(
				parametres.pointeur_donnees_extra,
				static_cast<long>(parametres.taille_octet_donnees_extra));
	dls::ego::util::GPU_check_errors("Erreur lors de la génération du tampon extra");

	auto idx_attr = static_cast<unsigned>(m_programme[parametres.attribut]);
	auto dim_attr = parametres.dimension_attribut;

	auto taille_vec4 = static_cast<int>(sizeof(dls::math::vec4f));

	m_donnees_tampon->pointeur_attribut(idx_attr    , dim_attr, 4 * taille_vec4, nullptr);
	m_donnees_tampon->pointeur_attribut(idx_attr + 1, dim_attr, 4 * taille_vec4, reinterpret_cast<void *>(taille_vec4));
	m_donnees_tampon->pointeur_attribut(idx_attr + 2, dim_attr, 4 * taille_vec4, reinterpret_cast<void *>(2 * taille_vec4));
	m_donnees_tampon->pointeur_attribut(idx_attr + 3, dim_attr, 4 * taille_vec4, reinterpret_cast<void *>(3 * taille_vec4));

	glVertexAttribDivisor(idx_attr    , 1);
	glVertexAttribDivisor(idx_attr + 1, 1);
	glVertexAttribDivisor(idx_attr + 2, 1);
	glVertexAttribDivisor(idx_attr + 3, 1);

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

	if (m_instance) {
		if (m_dessin_indexe) {
			glDrawElementsInstanced(
						m_paramatres_dessin.type_dessin(),
						static_cast<int>(m_elements),
						m_paramatres_dessin.type_donnees(),
						nullptr,
						static_cast<int>(m_nombre_instances));
		}
		else {
			glDrawArraysInstanced(
						m_paramatres_dessin.type_dessin(),
						0,
						static_cast<int>(m_elements),
						static_cast<int>(m_nombre_instances));
		}
	}
	else {
		if (m_dessin_indexe) {
			glDrawElements(m_paramatres_dessin.type_dessin(), static_cast<int>(m_elements), m_paramatres_dessin.type_donnees(), nullptr);
		}
		else {
			glDrawArrays(m_paramatres_dessin.type_dessin(), 0, static_cast<int>(m_elements));
		}
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

void remplis_tampon_instances(TamponRendu *tampon,
                              dls::chaine const &nom,
                              dls::tableau<dls::math::mat4x4f> &matrices)
{
    auto parametres_tampon_instance = ParametresTampon{};
    parametres_tampon_instance.attribut = nom;
    parametres_tampon_instance.dimension_attribut = 4;
    parametres_tampon_instance.pointeur_donnees_extra = matrices.donnees();
    parametres_tampon_instance.taille_octet_donnees_extra = static_cast<size_t>(
                                                                matrices.taille()) *
                                                            sizeof(dls::math::mat4x4f);
    parametres_tampon_instance.nombre_instances = static_cast<size_t>(matrices.taille());

    tampon->remplie_tampon_matrices_instance(parametres_tampon_instance);
}
