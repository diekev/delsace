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

#include "camera.h"

#include <iostream>

#include "../outils/constantes.h"

namespace vision {

/* retourne deux solutions, pour pouvoir choisir la meilleure */
void angles_euler_depuis_matrice(const dls::math::mat4x4f &matrice, dls::math::vec3f &angle1, dls::math::vec3f &angle2)
{
	const auto sy = std::hypot(matrice[0][0], matrice[0][1]);

	if (sy > 1e-6f) {
		angle1.x = std::atan2( matrice[1][2], matrice[2][2]);
		angle1.y = std::atan2(-matrice[0][2], sy);
		angle1.z = std::atan2( matrice[0][1], matrice[0][0]);

		angle2.x = std::atan2(-matrice[1][2], -matrice[2][2]);
		angle2.y = std::atan2(-matrice[0][2], -sy);
		angle2.z = std::atan2(-matrice[0][1], -matrice[0][0]);
	}
	else {
		angle1.x = angle2.x = std::atan2(-matrice[2][1], matrice[1][1]);
		angle1.y = angle2.y = std::atan2(-matrice[0][2], sy);
		angle1.z = angle2.z = 0.0f;
	}
}

void angles_euler_depuis_matrice(const dls::math::mat4x4f &matrice, dls::math::vec3f &angle)
{
	dls::math::vec3f angle1;
	dls::math::vec3f angle2;
	angles_euler_depuis_matrice(matrice, angle1, angle2);

	const auto a = std::abs(angle1.x) + std::abs(angle1.y) + std::abs(angle1.z);
	const auto b = std::abs(angle2.x) + std::abs(angle2.y) + std::abs(angle2.z);

	/* retourne la meilleure, qui est celle avec les valeurs les plus basses */
	if (a > b) {
		angle = angle2;
	}
	else {
		angle = angle1;
	}
}

#if 1
Camera3D::Camera3D(int largeur, int hauteur)
	: m_largeur(largeur)
	, m_hauteur(hauteur)
	, m_aspect(float(largeur) / hauteur)
	, m_tete(30.0f)
	, m_inclinaison(45.0f)
	, m_proche(0.1f)
	, m_eloigne(1000.0f)
	, m_distance(25.0f)
	, m_largeur_senseur(22.0f)
	, m_longueur_focale(35.0f)
	, m_vitesse_zoom(0.2f)
	, m_vitesse_chute(0.5f)
	, m_vitesse_laterale(0.05f)
	, m_position(dls::math::vec3f(0.0f, 0.0f, -1.0f))
	, m_direction(dls::math::vec3f(0.0f, 0.0f, 1.0f))
	, m_cible(dls::math::vec3f(0.0f))
	, m_droite(dls::math::vec3f(1.0f, 0.0f, 0.0f))
	, m_haut(dls::math::vec3f(0.0f, 1.0f, 0.0f))
	, m_besoin_ajournement(true)
{}

void Camera3D::ajuste_vitesse(const float zoom, const float laterale, const float chute)
{
	m_vitesse_zoom = std::max(0.0001f, m_distance * zoom);
	m_vitesse_laterale = std::max(0.0001f, m_distance * laterale);
	m_vitesse_chute = std::max(0.2f, std::min(1.0f, m_distance * chute));
}

float Camera3D::distance() const
{
	return m_distance;
}

void Camera3D::distance(float d)
{
	m_distance = d;
}

float Camera3D::vitesse_zoom() const
{
	return m_vitesse_zoom;
}

float Camera3D::vitesse_chute() const
{
	return m_vitesse_chute;
}

float Camera3D::vitesse_laterale() const
{
	return m_vitesse_laterale;
}

float Camera3D::inclinaison() const
{
	return m_inclinaison;
}

void Camera3D::inclinaison(float x)
{
	m_inclinaison = x;
}

float Camera3D::tete() const
{
	return m_tete;
}

void Camera3D::tete(float x)
{
	m_tete = x;
}

const dls::math::vec3f &Camera3D::cible() const
{
	return m_cible;
}

void Camera3D::cible(const dls::math::vec3f &x)
{
	m_cible = x;
}

const dls::math::vec3f &Camera3D::haut() const
{
	return m_haut;
}

const dls::math::vec3f &Camera3D::droite() const
{
	return m_droite;
}

void Camera3D::besoin_ajournement(bool ouinon)
{
	m_besoin_ajournement = ouinon;
}

int Camera3D::hauteur() const
{
	return m_hauteur;
}

int Camera3D::largeur() const
{
	return m_largeur;
}

void Camera3D::redimensionne(int largeur, int hauteur)
{
	m_largeur = largeur;
	m_hauteur = hauteur;
	m_aspect = static_cast<float>(largeur) / hauteur;

	ajourne_projection();

	m_besoin_ajournement = true;
}

void Camera3D::ajourne()
{
	if (!m_besoin_ajournement) {
		return;
	}

	if (m_type_projection == TypeProjection::ORTHOGRAPHIQUE) {
		ajourne_projection();
	}

	const float tete = dls::math::degrees_vers_radians(m_tete);
	const float inclinaison = dls::math::degrees_vers_radians(m_inclinaison);

	m_position[0] = m_cible[0] + m_distance * std::cos(tete) * std::cos(inclinaison);
	m_position[1] = m_cible[1] + m_distance * std::sin(tete);
	m_position[2] = m_cible[2] + m_distance * std::cos(tete) * std::sin(inclinaison);

	m_direction = dls::math::normalise(m_cible - m_position);

	m_haut[1] = (std::cos(tete)) > 0 ? 1.0f : -1.0f;
	m_droite = dls::math::produit_croix(m_direction, m_haut);

	m_monde_vers_camera = dls::math::mire(m_position, m_cible, m_haut);
	m_camera_vers_monde = dls::math::inverse(m_monde_vers_camera);

	m_besoin_ajournement = false;

#if 0
	dls::math::vec3f rot;
	angles_euler_depuis_matrice(m_camera_vers_monde, rot);

	std::cerr << "-------------------------------------------------------\n";
	std::cerr << "données reconstruites\n";
	std::cerr << "Position : " << m_camera_vers_monde[3][0] << ',' << m_camera_vers_monde[3][1] << ',' << m_camera_vers_monde[3][2] << '\n';
	std::cerr << "Rotation X : " << (rot.x * POIDS_RAD_DEG) << '\n';
	std::cerr << "Rotation Y : " << (rot.y * POIDS_RAD_DEG) << '\n';
	std::cerr << "Rotation Z : " << (rot.z * POIDS_RAD_DEG) << '\n';
#endif
}

dls::math::vec3f Camera3D::dir() const
{
	return m_direction;
}

dls::math::mat4x4f Camera3D::MV() const
{
	return m_monde_vers_camera;
}

dls::math::mat4x4f Camera3D::camera_vers_monde() const
{
	return m_camera_vers_monde;
}

dls::math::mat4x4f Camera3D::P() const
{
	return m_projection;
}

dls::math::vec3f Camera3D::pos() const
{
	return m_position;
}

dls::math::point2f Camera3D::pos_ecran(const dls::math::point3f &pos)
{
	const auto &point = dls::math::projette(
							dls::math::vec3f(pos.x, pos.y, pos.z),
							MV(),
							P(),
							dls::math::vec4f(0, 0, largeur(), hauteur()));

	return dls::math::point2f(point.x, hauteur() - point.y);
}

dls::math::point3f Camera3D::pos_monde(const dls::math::point3f &pos)
{
	return dls::math::deprojette(
				dls::math::vec3f(pos.x * largeur(), pos.y * hauteur(), pos.z),
				MV(),
				P(),
				dls::math::vec4f(0, 0, largeur(), hauteur()));
}

void Camera3D::projection(TypeProjection proj)
{
	m_type_projection = proj;
}

void Camera3D::largeur_senseur(float l)
{
	m_largeur_senseur = l;
}

void Camera3D::longueur_focale(float l)
{
	m_longueur_focale = l;
}

void Camera3D::profondeur(float proche, float eloigne)
{
	m_proche = proche;
	m_eloigne = eloigne;
}

void Camera3D::position(const dls::math::vec3f &p)
{
	m_position = p;
}

void Camera3D::rotation(const dls::math::vec3f &r)
{
	m_rotation = r;
}

void Camera3D::ajourne_projection()
{
	if (m_type_projection == TypeProjection::ORTHOGRAPHIQUE) {
		const auto largeur = m_distance * 0.5f;
		const auto hauteur = largeur / m_aspect;

		m_projection = dls::math::ortho(-largeur, largeur, -hauteur, hauteur, m_proche, m_eloigne);
	}
	else {
		/* À FAIRE : perspective prend des degrées mais devrait prendre des radians. */
		const auto champs_de_vue = dls::math::radians_vers_degrees(2.0f * std::atan(0.5f * m_largeur_senseur / m_longueur_focale));
		m_projection = dls::math::perspective(champs_de_vue, m_aspect, m_proche, m_eloigne);
	}
}

void Camera3D::ajourne_pour_operatrice()
{
	ajourne_projection();

	m_camera_vers_monde = dls::math::mat4x4f(1.0);
	m_camera_vers_monde = dls::math::translation(m_camera_vers_monde, m_position);
	m_camera_vers_monde = dls::math::rotation(m_camera_vers_monde, m_rotation.x, dls::math::vec3f(1.0, 0.0, 0.0));
	m_camera_vers_monde = dls::math::rotation(m_camera_vers_monde, m_rotation.y, dls::math::vec3f(0.0, 1.0, 0.0));
	m_camera_vers_monde = dls::math::rotation(m_camera_vers_monde, m_rotation.z, dls::math::vec3f(0.0, 0.0, 1.0));

	m_monde_vers_camera = dls::math::inverse(m_camera_vers_monde);

#if 0
	dls::math::vec3f rot;
	angles_euler_depuis_matrice(m_camera_vers_monde, rot);

	std::cerr << "-------------------------------------------------------\n";
	std::cerr << "données\n";
	std::cerr << "Position : " << m_position << '\n';
	std::cerr << "Rotation X : " << m_rotation.x * POIDS_RAD_DEG << '\n';
	std::cerr << "Rotation Y : " << m_rotation.y * POIDS_RAD_DEG << '\n';
	std::cerr << "Rotation Z : " << m_rotation.z * POIDS_RAD_DEG << '\n';

	std::cerr << "données reconstruites\n";
	std::cerr << "Position : " << m_camera_vers_monde[3][0] << ',' << m_camera_vers_monde[3][1] << ',' << m_camera_vers_monde[3][2] << '\n';

	std::cerr << "Rotation X : " << (rot.x * POIDS_RAD_DEG) << '\n';
	std::cerr << "Rotation Y : " << (rot.y * POIDS_RAD_DEG) << '\n';
	std::cerr << "Rotation Z : " << (rot.z * POIDS_RAD_DEG) << '\n';
#endif
}

float Camera3D::eloigne() const
{
	return m_eloigne;
}
#else
Rayon Camera3D::genere_rayon(const EchantillonCamera &echantillon) const
{
	const auto &start = dls::math::deprojette(
							dls::math::vec3f(echantillon.x, hauteur() - echantillon.y, 0.0f),
							MV(),
							P(),
							dls::math::vec4f(0, 0, largeur(), hauteur()));

	const auto &end = dls::math::deprojette(
						  dls::math::vec3f(echantillon.x, hauteur() - echantillon.y, 1.0f),
						  MV(),
						  P(),
						  dls::math::vec4f(0, 0, largeur(), hauteur()));

	const auto origine = m_position;
	const auto direction = dls::math::normalise(end - start);

	Rayon r;
	r.origine = dls::math::point3d(origine.x, origine.y, origine.z);
	r.direction = dls::math::vec3d(direction.x, direction.y, direction.z);

	for (int i = 0; i < 3; ++i) {
		r.inverse_direction[i] = 1.0 / r.direction[i];
	}

	r.distance_min = 0.0;
	r.distance_max = INFINITE;

	return r;
}

/* ************************************************************************** */

Transformation orthogaphique(double znear, double zfar)
{
	return echelle(1.0, 1.0, 1.0 / (zfar - znear)) * translation(0.0, 0.0, -znear);
}

Transformation perspective(double fov, double n, double f)
{
	dls::math::mat4d matrice({1.0, 0.0, 0.0, 0.0,
								  0.0, 1.0, 0.0, 0.0,
								  0.0, 0.0, f / (f - n), -f * n / (f - n),
								  0.0, 0.0, 1.0, 0.0});

	const auto tan_angle_inv = 1.0 / (std::tan(fov / 2.0));
	return echelle(tan_angle_inv, tan_angle_inv, 1.0) * Transformation(matrice);
}

/* ************************************************************************** */

void Camera::ouverture_obturateur(double valeur)
{
	m_obturateur_ouvert = valeur;
}

double Camera::ouverture_obturateur()
{
	return m_obturateur_ouvert;
}

void Camera::fermeture_obturateur(double valeur)
{
	m_obturateur_ferme = valeur;
}

double Camera::fermeture_obturateur()
{
	return m_obturateur_ferme;
}

void Camera::position(const dls::math::vec3d &valeur)
{
	m_position = valeur;
}

dls::math::vec3d Camera::position()
{
	return m_position;
}

void Camera::rotation(const dls::math::vec3d &valeur)
{
	m_rotation = valeur;
}

dls::math::vec3d Camera::rotation()
{
	return m_rotation;
}

void Camera::ajourne()
{
	m_camera_vers_monde = Transformation();

	m_camera_vers_monde *= translation(m_position);
	m_camera_vers_monde *= rotation_x(m_rotation[0]);
	m_camera_vers_monde *= rotation_y(m_rotation[1]);
	m_camera_vers_monde *= rotation_z(m_rotation[2]);
}

void Camera::pellicule(Pellicule *pellicule)
{
	m_pellicule = pellicule;
}

const Transformation &Camera::camera_vers_monde() const
{
	return m_camera_vers_monde;
}

/* ************************************************************************** */

ProjectiveCamera::ProjectiveCamera()
	: Camera()
{}

void ProjectiveCamera::fenetre_ecran(double valeur[])
{
	for (int i = 0; i < 4; ++i) {
		m_fenetre_ecran[i] = valeur[i];
	}
}

void ProjectiveCamera::rayon_lentille(double valeur)
{
	m_rayon_lentille = valeur;
}

double ProjectiveCamera::rayon_lentille()
{
	return m_rayon_lentille;
}

void ProjectiveCamera::champs_de_vue(double valeur)
{
	m_champs_de_vue = valeur;
}

double ProjectiveCamera::champs_de_vue()
{
	return m_champs_de_vue;
}

void ProjectiveCamera::distance_focale(double valeur)
{
	m_distance_focale = valeur;
}

double ProjectiveCamera::distance_focale()
{
	return m_distance_focale;
}

void ProjectiveCamera::ajourne()
{
	Camera::ajourne();

	m_ecran_vers_rateau = echelle(m_pellicule->largeur(), m_pellicule->hauteur(), 1.0);
	m_ecran_vers_rateau *= echelle(1.0 / (m_fenetre_ecran[1] - m_fenetre_ecran[0]), 1.0 / (m_fenetre_ecran[2] - m_fenetre_ecran[3]), 1.0);
	m_ecran_vers_rateau *= translation(-m_fenetre_ecran[0], -m_fenetre_ecran[3], 0.0);

	m_rateau_vers_ecran = inverse(m_ecran_vers_rateau);

	m_camera_vers_ecran = m_projection;
	m_rateau_vers_camera = inverse(m_camera_vers_ecran) * m_rateau_vers_ecran;
}

/* ************************************************************************** */

CameraOrthographique::CameraOrthographique()
	: ProjectiveCamera()
{}

Rayon CameraOrthographique::genere_rayon(const EchantillonCamera &echantillon) const
{
	auto point_rateau = dls::math::point3d(echantillon.x, echantillon.y, 0.0);
	dls::math::point3d point_camera;

	m_rateau_vers_camera(point_rateau, &point_camera);

	Rayon rayon;
	rayon.origine = point_camera;
	rayon.direction = dls::math::vec3d(0.0, 0.0, 1.0);
	rayon.distance_min = 0.0;
	rayon.distance_max = INFINITE;
	rayon.temps = dls::math::interp_lineaire(
					  m_obturateur_ouvert, m_obturateur_ferme, echantillon.temps);

	m_camera_vers_monde(rayon.origine, &rayon.origine);
	m_camera_vers_monde(rayon.direction, &rayon.direction);

	return rayon;
}

void CameraOrthographique::ajourne()
{
	m_projection = orthogaphique(0.0, 1.0);

	ProjectiveCamera::ajourne();

	m_camera_dx = m_rateau_vers_camera(dls::math::vec3d(1.0, 0.0, 0.0));
	m_camera_dy = m_rateau_vers_camera(dls::math::vec3d(0.0, 1.0, 0.0));
}

/* ************************************************************************** */

CameraPerspective::CameraPerspective()
	: ProjectiveCamera()
{}

Rayon CameraPerspective::genere_rayon(const EchantillonCamera &echantillon) const
{
	auto point_rateau = dls::math::point3d(echantillon.x, echantillon.y, 0.0);
	dls::math::point3d point_camera;

	m_rateau_vers_camera(point_rateau, &point_camera);

	Rayon rayon;
	rayon.origine = dls::math::point3d(0.0, 0.0, 0.0);

	rayon.direction = dls::math::vec3d(
						  point_camera.x,
						  point_camera.y,
						  point_camera.z);

	rayon.distance_min = 0.0;
	rayon.distance_max = INFINITE;
	rayon.temps = dls::math::interp_lineaire(
					  m_obturateur_ouvert, m_obturateur_ferme, echantillon.temps);

	/* À FAIRE : profondeur de champs. */

	m_camera_vers_monde(rayon.origine, &rayon.origine);
	m_camera_vers_monde(rayon.direction, &rayon.direction);

	for (int i = 0; i < 3; ++i) {
		rayon.inverse_direction[i] = 1.0 / rayon.direction[i];
	}

	return rayon;
}

void CameraPerspective::ajourne()
{
	m_projection = perspective(m_champs_de_vue * M_PI / 180, 1e-2, 1000.0);

	ProjectiveCamera::ajourne();

	m_camera_dx = m_rateau_vers_camera(dls::math::vec3d(1.0, 0.0, 0.0));
	m_camera_dy = m_rateau_vers_camera(dls::math::vec3d(0.0, 1.0, 0.0));
}
#endif

}  /* namespace vision */
