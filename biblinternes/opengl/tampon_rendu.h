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

#pragma once

#include <optional>

#include "biblinternes/ego/tampon_objet.h"
#include "biblinternes/ego/programme.h"
#include "biblinternes/ego/texture.h"

#include "biblinternes/math/matrice.hh"
#include "biblinternes/math/vecteur.hh"
#include "biblinternes/phys/couleur.hh"

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

class AtlasTexture;
class ContexteRendu;

/* ------------------------------------------------------------------------- */
/** \name SourcesGLSL
 * \{ */

struct SourcesGLSL {
    dls::chaine vertex{};
    dls::chaine fragment{};
};

std::optional<SourcesGLSL> crée_sources_glsl_depuis_texte(dls::chaine const &vertex, dls::chaine const &fragment);

std::optional<SourcesGLSL> crée_sources_glsl_depuis_fichier(dls::chaine const &fichier_vertex,
                                                            dls::chaine const &fichier_fragment);

/** \} */

/* ************************************************************************** */

/**
 * La struct ParametresTampon gère les données nécessaires à l'élaboration de
 * tampon sommet ou d'autres attributs.
 */
struct ParametresTampon {
	/* Le nom de l'attribut. */
	dls::chaine attribut = "";

	/* Le nombre de dimension d'un élément de l'attribut (généralement 2 ou 3). */
	int dimension_attribut = 0;

	/* Le pointeur vers le début du tableau de sommets. */
	void *pointeur_sommets = nullptr;

	/* La taille en octet des sommets. */
	size_t taille_octet_sommets = 0ul;

	/* Le pointeur vers le début du tableau d'index des sommets. */
	void *pointeur_index = nullptr;

	/* Le taille en octet des index des sommets. */
	size_t taille_octet_index = 0ul;

	/* Le nombre d'index. */
	size_t elements = 0ul;

	/* Le pointeur vers le début du tableau des données extra. */
	void *pointeur_donnees_extra = nullptr;

	/* Le taille en octet des données extra. */
	size_t taille_octet_donnees_extra = 0ul;

	/* Le nombre d'instances à dessiner */
	size_t nombre_instances = 0ul;
};

/* ************************************************************************** */

/**
 * La classe ParametresProgramme gère les noms des attributs et des valeurs
 * uniformes pour un TamponRendu.
 */
class ParametresProgramme {
	dls::tableau<dls::chaine> m_attributs = {};
	dls::tableau<dls::chaine> m_uniformes = {};

public:
	/**
	 * Construit une instance de ParametresProgramme par défaut.
	 */
	ParametresProgramme() = default;

	/**
	 * Ajoute le nom d'un nouvel attribut dans la liste des attributs.
	 */
	void ajoute_attribut(dls::chaine const &nom);

	/**
	 * Ajoute le nom d'une nouvelle valeur uniforme dans la liste des uniformes.
	 */
	void ajoute_uniforme(dls::chaine const &nom);

	/**
	 * Retourne la liste des attributs.
	 */
	dls::tableau<dls::chaine> const &attributs() const;

	/**
	 * Retourne la liste des uniformes.
	 */
	dls::tableau<dls::chaine> const &uniformes() const;
};

/* ************************************************************************** */

/**
 * La classe ParametresDessin gère les différents paramètres pouvant être
 * utilisé pour dessiner un programme (GL_TRIANGLES, GL_POINTS, taille des
 * lignes ou points, etc...)
 */
class ParametresDessin {
	unsigned int m_type_dessin = 0x0004; /* GL_TRIANGLES */
	unsigned int m_type_donnees = 0x1405; /* GL_UNSIGNED_INT */
	float m_taille_ligne = 1.0f;
	float m_taille_point = 1.0f;

public:
	ParametresDessin() = default;

	/**
	 * Change le type de dessin selon le type spécifié en paramètre.
	 */
	void type_dessin(unsigned int type);

	/**
	 * Retourne le type de dessin utilisé, par défaut GL_TRIANGLES.
	 */
	unsigned int type_dessin() const;

	/**
	 * Change le type de données selon le type spécifié en paramètre.
	 */
	void type_donnees(unsigned int type);

	/**
	 * Retourne le type de données utilisé, par défaut GL_UNSIGNED_INT.
	 */
	unsigned int type_donnees() const;

	/**
	 * Change la taille en pixel des lignes à dessiner.
	 */
	void taille_ligne(float taille);

	/**
	 * Retourne la taille en pixel des lignes à dessiner, par défaut 1.0f.
	 */
	float taille_ligne() const;

	/**
	 * Change la taille en pixel des points à dessiner.
	 */
	void taille_point(float taille);

	/**
	 * Retourne la taille en pixel des points à dessiner, par défaut 1.0f.
	 */
	float taille_point() const;
};

/* ************************************************************************** */

/**
 * La classe TamponRendu gère les données et les paramètres d'un BufferObject.
 */
class TamponRendu {
	dls::ego::TamponObjet::Ptr m_donnees_tampon = nullptr;
	dls::ego::Programme m_programme{};
	size_t m_elements = 0;
	size_t m_nombre_instances = 0;

	dls::ego::Texture2D::Ptr m_texture = nullptr;
	dls::ego::Texture3D::Ptr m_texture_3d = nullptr;

	AtlasTexture *m_atlas = nullptr;

	ParametresDessin m_paramatres_dessin{};

	bool m_requiers_normal = false;
	bool m_peut_surligner = false;
	bool m_dessin_indexe = false;
	bool m_instance = false;

public:
	~TamponRendu();

    static std::unique_ptr<TamponRendu> crée_unique(SourcesGLSL const &sources);

	/**
	 * Charge les sources du programme de tampon pour le type de programme
	 * spécifié (VERTEX, FRAGMENT, GEOMETRY, etc...).
	 *
	 * Les sources doivent être un texte et non un chemin vers un fichier ! Il
	 * est attendu que la fonction qui appèle cette méthode se charge d'ouvrir
	 * les fichiers et de mettre leurs contenus dans des dls::chaine.
	 */
	void charge_source_programme(
			dls::ego::Nuanceur type_programme,
			dls::chaine const &source,
			std::ostream &os = std::cerr);

	/**
	 * Mets en place les paramètres du programme de ce tampon.
	 */
	void parametres_programme(ParametresProgramme const &parametres);

	/**
	 * Mets en place les paramètres du dessin.
	 */
	void parametres_dessin(ParametresDessin const &parametres);

	ParametresDessin const &parametres_dessin() const
	{
		return m_paramatres_dessin;
	}

	/**
	 * Finalise la construction du programme. Cette fonction doit
	 * obligatoirement être appelée après avoir chargé les sources du programme
	 * sans quoi le programme sera invalide et ne pourra pas être dessiné !
	 */
	void finalise_programme(std::ostream &os = std::cerr);

	/**
	 * Définie si oui ou non le programme peut surligner, c'est-à-dire être
	 * appelé une deuxième fois pour dessiner le contour ou surlignage d'un
	 * objet.
	 */
	void peut_surligner(bool ouinon);

	/**
	 * Remplie le tampon des sommets selon les paramètres spécifiés.
	 */
	void remplie_tampon(ParametresTampon const &parametres);

	/**
	 * Remplie un tampon extra selon les paramètres spécifiés.
	 */
	void remplie_tampon_extra(ParametresTampon const &parametres);

	/**
	 * Remplie un tampon extra selon les paramètres spécifiés.
	 */
	void remplie_tampon_matrices_instance(ParametresTampon const &parametres);

	/**
	 * Dessine ce tampon selon le dans le contexte spécifié.
	 */
	void dessine(ContexteRendu const &contexte);

	/**
	 * Retourne un pointeur vers le programme de ce tampon.
	 */
	dls::ego::Programme *programme();

	/**
	 * Ajoute une texture dans ce tampon.
	 */
	void ajoute_texture();

	/**
	 * Ajoute une texture dans ce tampon.
	 */
	void ajoute_texture_3d();

	/**
	 * Ajoute un atlas texture dans ce tampon.
	 */
	void ajoute_atlas();

	/**
	 * Retourne un pointeur vers la texture de ce tampon.
	 */
	dls::ego::Texture2D *texture();

	/**
	 * Retourne un pointeur vers la texture de ce tampon.
	 */
	dls::ego::Texture3D *texture_3d();

	/**
	 * Retourne un pointeur vers l'atlas texture de ce tampon.
	 */
	AtlasTexture *atlas();

	dls::ego::TamponObjet::Ptr &donnees()
	{
		return m_donnees_tampon;
	}

private:
	/**
	 * Initialise le BufferObject de ce tampon en le créant s'il n'a pas déjà
	 * été créer.
	 */
	void initialise_tampon();
};

/**
 * Ajoute le tampon spécifié dans une liste pour être supprimé plus tard avec
 * un appel à la fonction `purge_tous_les_tampons`.
 *
 * Cette fonction est à utiliser pour supprimer des tampons depuis des threads
 * différents de celui où le contexte OpenGL à été créé.
 */
void supprime_tampon_rendu(TamponRendu *tampon);

/**
 * Supprime tous les tampons qui ont été rammassés par la fonction
 * `supprime_tampon_rendu`.
 *
 * Cette fonction est à appeler dans le thread qui a créé le contexte OpenGL qui
 * a été utilisé pour créer les tampons en premier lieu.
 */
void purge_tous_les_tampons();

/* ------------------------------------------------------------------------- */
/** \name Remplissage de TamponRendu.
 * \{ */

template <typename T>
struct nombre_de_dimensions;

template <>
struct nombre_de_dimensions<dls::math::vec2f> {
    static constexpr int valeur = 2;
};

template <>
struct nombre_de_dimensions<dls::math::vec3f> {
    static constexpr int valeur = 3;
};

template <>
struct nombre_de_dimensions<dls::math::vec4f> {
    static constexpr int valeur = 4;
};

template <>
struct nombre_de_dimensions<dls::phys::couleur32> {
    static constexpr int valeur = 4;
};

template <typename T>
static void remplis_tampon_principal(TamponRendu *tampon,
                                     dls::chaine const &nom,
                                     dls::tableau<T> &valeurs)
{
    ParametresTampon parametres_tampon;
    parametres_tampon.attribut = nom;
    parametres_tampon.dimension_attribut = nombre_de_dimensions<T>::valeur;
    parametres_tampon.pointeur_sommets = valeurs.donnees();
    parametres_tampon.taille_octet_sommets = static_cast<size_t>(valeurs.taille()) * sizeof(T);
    parametres_tampon.elements = static_cast<size_t>(valeurs.taille());

    tampon->remplie_tampon(parametres_tampon);
}

template <typename T, typename TypeIndex>
static void remplis_tampon_principal(TamponRendu *tampon,
                                     dls::chaine const &nom,
                                     dls::tableau<T> &valeurs,
                                     dls::tableau<TypeIndex> &index)
{
    ParametresTampon parametres_tampon;
    parametres_tampon.attribut = nom;
    parametres_tampon.dimension_attribut = nombre_de_dimensions<T>::valeur;
    parametres_tampon.pointeur_sommets = valeurs.donnees();
    parametres_tampon.taille_octet_sommets = static_cast<size_t>(valeurs.taille()) * sizeof(T);

    if (index.est_vide()) {
        parametres_tampon.elements = static_cast<size_t>(valeurs.taille());
    }
    else {
        parametres_tampon.pointeur_index = index.donnees();
        parametres_tampon.taille_octet_index = static_cast<size_t>(index.taille()) *
                                               sizeof(TypeIndex);
        parametres_tampon.elements = static_cast<size_t>(index.taille());
    }

    tampon->remplie_tampon(parametres_tampon);
}

template <typename T>
static void remplis_tampon_extra(TamponRendu *tampon,
                                 dls::chaine const &nom,
                                 dls::tableau<T> &valeurs)
{
    ParametresTampon parametres_tampon;
    parametres_tampon.attribut = nom;
    parametres_tampon.dimension_attribut = nombre_de_dimensions<T>::valeur;
    parametres_tampon.pointeur_donnees_extra = valeurs.donnees();
    parametres_tampon.taille_octet_donnees_extra = static_cast<size_t>(valeurs.taille()) *
                                                   sizeof(T);
    parametres_tampon.elements = static_cast<size_t>(valeurs.taille());

    tampon->remplie_tampon_extra(parametres_tampon);
}

void remplis_tampon_instances(TamponRendu *tampon,
                              dls::chaine const &nom,
                              dls::tableau<dls::math::mat4x4f> &matrices);

/** \} */
