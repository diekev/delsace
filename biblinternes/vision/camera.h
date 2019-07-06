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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#pragma once

#include "biblinternes/math/matrice.hh"
#include "biblinternes/math/vecteur.hh"

namespace vision {

enum class TypeProjection {
	PERSPECTIVE,
	ORTHOGRAPHIQUE,
};

struct EchantillonCamera {
	double x = 0.0;
	double y = 0.0;
	double temps = 0.0;
};

#undef NOUVELLE_CAMERA

/* ************************************************************************** */
#ifndef NOUVELLE_CAMERA

/**
 * La classe Camera gère les propriétés de la caméra permettant de voir
 * une scène 3D.
 *
 * À FAIRE : remplacer avec la classe Camera du moteur de rendu.
 */
class Camera3D {
	int m_largeur = 0;
	int m_hauteur = 0;
	float m_aspect = 0.0f;
	float m_tete = 0.0f;

	/* Inclinaison sur l'axe des x local de la caméra définissant si elle
	 * regarde vers le haut ou vers le bas. */
	float m_inclinaison = 0.0f;

	float m_proche = 0.0f;
	float m_eloigne = 0.0f;
	float m_distance = 0.0f;

	/* À FAIRE : m_hauteur_senseur + préréglages de tailles de senseurs connues
	 *  champs_de_vue horizontal = 2atan(0.5 * largeur_senseur / longueur_focale) */
	float m_largeur_senseur = 0.0f;

	float m_longueur_focale = 0.0f;

	/* Vitesse de déplacement avant-arrière (Z local). */
	float m_vitesse_zoom = 0.0f;

	/* Vitesse de déplacement haut-bas (Y local). */
	float m_vitesse_chute = 0.0f;

	/* Vitesse de déplacement gauche-droite (X local). */
	float m_vitesse_laterale = 0.0f;

	dls::math::mat4x4f m_camera_vers_monde = dls::math::mat4x4f(1.0f);
	dls::math::mat4x4f m_monde_vers_camera = dls::math::mat4x4f(1.0f);
	dls::math::mat4x4f m_projection = dls::math::mat4x4f(1.0f);
	dls::math::vec3f m_position = dls::math::vec3f(0.0f);
	dls::math::vec3f m_rotation = dls::math::vec3f(0.0f);
	dls::math::vec3f m_direction = dls::math::vec3f(0.0f);
	dls::math::vec3f m_cible = dls::math::vec3f(0.0f);
	dls::math::vec3f m_droite = dls::math::vec3f(0.0f);
	dls::math::vec3f m_haut = dls::math::vec3f(0.0f);

	bool m_besoin_ajournement = true;

	TypeProjection m_type_projection = TypeProjection::ORTHOGRAPHIQUE;

public:
	/**
	 * Construit une caméra par défaut avec les dimensions d'écran spécifiées.
	 */
	Camera3D(int largeur, int hauteur);

	/**
	 * Détruit la caméra.
	 */
	~Camera3D() = default;

	/**
	 * Ajourne les différentes matrices de la caméra.
	 */
	void ajourne();

	/**
	 * Retourne la direction vers la laquelle la caméra pointe.
	 */
	dls::math::vec3f dir() const;

	/**
	 * Retourne la matrice de modèle-vue.
	 */
	dls::math::mat4x4f MV() const;

	/**
	 * Retourne la matrice de projection.
	 */
	dls::math::mat4x4f P() const;

	/**
	 * Retourne la position de la caméra.
	 */
	dls::math::vec3f pos() const;

	/**
	 * Redimensionne la vue de la caméra. En effet, cette méthode ne fait que
	 * changer l'aspect de la caméra tout en mettant à jour la matrice de
	 * projection.
	 */
	void redimensionne(int largeur, int hauteur);

	/**
	 * Ajuste la vitesse à laquelle la caméra bouge sur ses axes locaux.
	 *
	 * Le zoom définit la vitesse de motion sur l'axe local Z (avant-arrière).
	 * Le latéral définit la vitesse de motion sur l'axe local X (gauche-droite).
	 * La chute définit la vitesse de motion sur l'axe local Y (haut-bas).
	 */
	void ajuste_vitesse(
			const float zoom = 0.1f,
			const float laterale = 0.002f,
			const float chute = 0.02f);

	float distance() const;

	void distance(float d);

	float vitesse_zoom() const;

	float vitesse_chute() const;

	float vitesse_laterale() const;

	float inclinaison() const;

	void inclinaison(float x);

	float tete() const;

	void tete(float x);

	dls::math::vec3f const &cible() const;

	void cible(dls::math::vec3f const &x);

	dls::math::vec3f const &haut() const;

	dls::math::vec3f const &droite() const;

	void besoin_ajournement(bool ouinon);

	int hauteur() const;

	int largeur() const;

	dls::math::point2f pos_ecran(dls::math::point3f const &pos);
	dls::math::point3f pos_monde(dls::math::point3f const &pos);

	void projection(TypeProjection proj);

	void largeur_senseur(float l);

	void longueur_focale(float l);

	void profondeur(float proche, float eloigne);

	void position(dls::math::vec3f const &p);

	void rotation(dls::math::vec3f const &r);

	void ajourne_pour_operatrice();

	float eloigne() const;
	dls::math::mat4x4f camera_vers_monde() const;

private:
	void ajourne_projection();
};
#else
class Camera {
	double m_largeur_inverse = 0.0;
	double m_hauteur_inverse = 0.0;
	double m_aspect = 0.0;

protected:
	Transformation m_camera_vers_monde;

	double m_obturateur_ouvert;
	double m_obturateur_ferme;

	Pellicule *m_pellicule;

	dls::math::vec3d m_position;
	dls::math::vec3d m_rotation;

public:
	Camera() = default;

	Rayon genere_rayon(EchantillonCamera echantillon) const;

	virtual Rayon genere_rayon(EchantillonCamera const &echantillon) const = 0;

	void ouverture_obturateur(double valeur);
	double ouverture_obturateur();

	void fermeture_obturateur(double valeur);
	double fermeture_obturateur();

	void position(dls::math::vec3d const &valeur);
	dls::math::vec3d position();

	void rotation(dls::math::vec3d const &valeur);
	dls::math::vec3d rotation();

	virtual void ajourne();

	void pellicule(Pellicule *pellicule);

	Transformation const &camera_vers_monde() const;
};

/* ************************************************************************** */

class ProjectiveCamera : public Camera {
protected:
	Transformation m_camera_vers_ecran;
	Transformation m_rateau_vers_camera;
	Transformation m_ecran_vers_rateau;
	Transformation m_rateau_vers_ecran;

	Transformation m_projection;

	double m_rayon_lentille = 0.0;
	double m_distance_focale = 0.0;
	double m_champs_de_vue = 0.0;

	double m_fenetre_ecran[4] = {
		-1.0, 1.0,
		-1.0, 1.0
	};

public:
	ProjectiveCamera();

	void fenetre_ecran(double valeur[4]);

	void rayon_lentille(double valeur);

	double rayon_lentille();

	void champs_de_vue(double valeur);

	double champs_de_vue();

	void distance_focale(double valeur);

	double distance_focale();

	void ajourne() override;
};

/* ************************************************************************** */

Transformation orthogaphique(double znear, double zfar);

class CameraOrthographique final : public ProjectiveCamera {
private:
	dls::math::vec3d m_camera_dx;
	dls::math::vec3d m_camera_dy;

public:
	CameraOrthographique();

	Rayon genere_rayon(EchantillonCamera const &echantillon) const override;

	void ajourne() override;
};

/* ************************************************************************** */

Transformation perspective(double fov, double n, double f);

class CameraPerspective final : public ProjectiveCamera {
private:
	dls::math::vec3d m_camera_dx;
	dls::math::vec3d m_camera_dy;

public:
	CameraPerspective();

	Rayon genere_rayon(EchantillonCamera const &echantillon) const override;

	void ajourne() override;
};
#endif

}  /* namespace vision */
