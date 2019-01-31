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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrice_image.h"

#include "bibliotheques/graphe/noeud.h"

#include "corps/corps.h"

#include "noeud_image.h"

/* ************************************************************************** */

numero7::image::Pixel<float> Calque::valeur(size_t x, size_t y) const
{
	x = std::max(0ul, std::min(x, static_cast<size_t>(tampon.nombre_colonnes()) - 1));
	y = std::max(0ul, std::min(y, static_cast<size_t>(tampon.nombre_lignes()) - 1));
	return tampon[static_cast<int>(y)][static_cast<int>(x)];
}

void Calque::valeur(size_t x, size_t y, const numero7::image::Pixel<float> &pixel)
{
	x = std::max(0ul, std::min(x, static_cast<size_t>(tampon.nombre_colonnes()) - 1));
	y = std::max(0ul, std::min(y, static_cast<size_t>(tampon.nombre_lignes()) - 1));
	tampon[static_cast<int>(y)][static_cast<int>(x)] = pixel;
}

numero7::image::Pixel<float> Calque::echantillone(float x, float y) const
{
	auto const res_x = tampon.nombre_colonnes();
	auto const res_y = tampon.nombre_lignes();

	auto const entier_x = static_cast<int>(x);
	auto const entier_y = static_cast<int>(y);

	auto const fract_x = x - static_cast<float>(entier_x);
	auto const fract_y = y - static_cast<float>(entier_y);

	auto const x1 = std::max(0, std::min(entier_x, res_x - 1));
	auto const y1 = std::max(0, std::min(entier_y, res_y - 1));
	auto const x2 = std::max(0, std::min(entier_x + 1, res_x - 1));
	auto const y2 = std::max(0, std::min(entier_y + 1, res_y - 1));

	auto valeur = numero7::image::Pixel<float>(0.0f);
	valeur += fract_x * fract_y * tampon[y1][x1];
	valeur += (1.0f - fract_x) * fract_y * tampon[y1][x2];
	valeur += fract_x * (1.0f - fract_y) * tampon[y2][x1];
	valeur += (1.0f - fract_x) * (1.0f - fract_y) * tampon[y2][x2];

	return valeur;
}

/* ************************************************************************** */

Image::~Image()
{
	reinitialise();
}

Calque *Image::ajoute_calque(const std::string &nom, const Rectangle &rectangle)
{
	auto tampon = new Calque();
	tampon->nom = nom;
	tampon->tampon = type_image(numero7::math::Hauteur(static_cast<int>(rectangle.hauteur)),
								numero7::math::Largeur(static_cast<int>(rectangle.largeur)));

	auto pixel = numero7::image::Pixel<float>(0.0f);
	pixel.a = 1.0f;

	tampon->tampon.remplie(pixel);

	m_calques.push_back(tampon);

	return tampon;
}

Calque *Image::calque(const std::string &nom) const
{
	for (Calque *tampon : m_calques) {
		if (tampon->nom == nom) {
			return tampon;
		}
	}

	return nullptr;
}

Image::plage_calques Image::calques()
{
	return plage_calques(m_calques.begin(), m_calques.end());
}

Image::plage_calques_const Image::calques() const
{
	return plage_calques_const(m_calques.cbegin(), m_calques.cend());
}

void Image::reinitialise(bool garde_memoires)
{
	if (!garde_memoires) {
		for (Calque *tampon : m_calques) {
			delete tampon;
		}
	}

	m_calques.clear();
}

void Image::nom_calque_actif(const std::string &nom)
{
	m_nom_calque = nom;
}

const std::string &Image::nom_calque_actif() const
{
	return m_nom_calque;
}

/* ************************************************************************** */

EntreeOperatrice::EntreeOperatrice(PriseEntree *prise)
	: m_ptr(prise)
{}

bool EntreeOperatrice::connectee() const
{
	return m_ptr->lien != nullptr;
}

void EntreeOperatrice::requiers_image(
		Image &image,
		const Rectangle &rectangle,
		const int temps)
{
	auto lien = m_ptr->lien;
	m_liste_noms_calques.clear();

	if (lien != nullptr) {
		auto noeud = lien->parent;

		execute_noeud(noeud, rectangle, temps);

		auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());
		operatrice->transfere_image(image);

		for (auto const &calque : image.calques()) {
			m_liste_noms_calques.push_back(calque->nom);
		}
	}
}

vision::Camera3D *EntreeOperatrice::requiers_camera(const Rectangle &rectangle, const int temps)
{
	auto lien = m_ptr->lien;

	if (lien == nullptr) {
		return nullptr;
	}

	auto noeud = lien->parent;

	execute_noeud(noeud, rectangle, temps);

	auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());
	return operatrice->camera();
}

Objet *EntreeOperatrice::requiers_objet(const Rectangle &rectangle, const int temps)
{
	auto lien = m_ptr->lien;

	if (lien == nullptr) {
		return nullptr;
	}

	auto noeud = lien->parent;

	execute_noeud(noeud, rectangle, temps);

	auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());
	return operatrice->objet();
}

TextureImage *EntreeOperatrice::requiers_texture(const Rectangle &rectangle, const int temps)
{
	auto lien = m_ptr->lien;

	if (lien == nullptr) {
		return nullptr;
	}

	auto noeud = lien->parent;

	execute_noeud(noeud, rectangle, temps);

	auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());
	return operatrice->texture();
}

const Corps *EntreeOperatrice::requiers_corps(const Rectangle &rectangle, const int temps)
{
	auto lien = m_ptr->lien;

	if (lien == nullptr) {
		return nullptr;
	}

	auto noeud = lien->parent;

	execute_noeud(noeud, rectangle, temps);

	auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());

	return operatrice->corps();
}

Corps *EntreeOperatrice::requiers_copie_corps(Corps *corps, const Rectangle &rectangle, const int temps)
{
	auto corps_lien = this->requiers_corps(rectangle, temps);

	if (corps_lien == nullptr) {
		return nullptr;
	}

	if (corps != nullptr) {
		corps_lien->copie_vers(corps);
		return corps;
	}

	return corps_lien->copie();
}

void EntreeOperatrice::obtiens_liste_calque(std::vector<std::string> &chaines) const
{
	chaines = m_liste_noms_calques;
}

/* ************************************************************************** */

OperatriceImage::OperatriceImage(Graphe &graphe_parent, Noeud *node)
	: m_graphe_parent(graphe_parent)
{
	node->donnees(this);
	m_input_data.resize(m_num_inputs);
	m_sorties.resize(m_num_outputs);
}

int OperatriceImage::type() const
{
	return OPERATRICE_IMAGE;
}

void OperatriceImage::inputs(size_t number)
{
	m_num_inputs = number;
	m_input_data.resize(number);
}

size_t OperatriceImage::inputs() const
{
	return m_num_inputs;
}

const char *OperatriceImage::nom_entree(int n)
{
	switch (n) {
		default:
		case 0: return "A";
		case 1: return "B";
	}
}

int OperatriceImage::type_entree(int n) const
{
	switch (n) {
		default:
		case 0: return OPERATRICE_IMAGE;
		case 1: return OPERATRICE_IMAGE;
	}
}

EntreeOperatrice *OperatriceImage::input(size_t index)
{
	if (index >= m_num_inputs) {
		return nullptr;
	}

	return &m_input_data[index];
}

const EntreeOperatrice *OperatriceImage::input(size_t index) const
{
	if (index >= m_num_inputs) {
		return nullptr;
	}

	return &m_input_data[index];
}

void OperatriceImage::set_input_data(size_t index, PriseEntree *socket)
{
	if (index >= m_input_data.size()) {
		std::cerr << class_name() << " : Overflow while setting data (inputs: "
				  << m_input_data.size() << ", index: " << index << ")\n";
		return;
	}

	m_input_data[index] = EntreeOperatrice(socket);
}

SortieOperatrice *OperatriceImage::output(size_t index)
{
	if (index >= m_num_outputs) {
		return nullptr;
	}

	return &m_sorties[index];
}

void OperatriceImage::set_output_data(size_t index, PriseSortie *prise)
{
	if (index >= m_sorties.size()) {
		std::cerr << class_name() << " : Overflow while setting data (outputs: "
				  << m_sorties.size() << ", index: " << index << ")\n";
		return;
	}

	m_sorties[index] = SortieOperatrice(prise);
}

void OperatriceImage::outputs(size_t number)
{
	m_num_outputs = number;
	m_sorties.resize(number);
}

size_t OperatriceImage::outputs() const
{
	return m_num_outputs;
}

const char *OperatriceImage::nom_sortie(int n)
{
	switch (n) {
		default:
		case 0: return "R";
	}
}

int OperatriceImage::type_sortie(int n) const
{
	switch (n) {
		default:
		case 0: return OPERATRICE_IMAGE;
	}
}

const char *OperatriceImage::chemin_entreface() const
{
	return "";
}

void OperatriceImage::transfere_image(Image &image)
{
	image = m_image;
	m_image.reinitialise(true);
}

void OperatriceImage::ajoute_avertissement(const std::string &avertissement)
{
	m_avertissements.push_back(avertissement);
}

void OperatriceImage::reinitialise_avertisements()
{
	m_avertissements.clear();
}

const std::vector<std::string> &OperatriceImage::avertissements() const
{
	return m_avertissements;
}

vision::Camera3D *OperatriceImage::camera()
{
	return nullptr;
}

Scene *OperatriceImage::scene()
{
	return nullptr;
}

TextureImage *OperatriceImage::texture()
{
	return nullptr;
}

Objet *OperatriceImage::objet()
{
	return nullptr;
}

Corps *OperatriceImage::corps()
{
	return nullptr;
}

bool OperatriceImage::possede_manipulatrice_3d(int /*type*/) const
{
	return false;
}

Manipulatrice3D *OperatriceImage::manipulatrice_3d(int /*type*/)
{
	return nullptr;
}

void OperatriceImage::ajourne_selon_manipulatrice_3d(int /*type*/, const int /*temps*/)
{
	/* rien à faire par défaut */
}

void OperatriceImage::obtiens_liste(const std::string &/*attache*/, std::vector<std::string> &chaines) const
{
	if (inputs() == 0) {
		chaines.clear();
		return;
	}

	/* Par défaut, on utilise la liste de la première entrée. */
	input(0)->obtiens_liste_calque(chaines);
}

/* ************************************************************************** */

void supprime_operatrice_image(std::any pointeur)
{
	delete std::any_cast<OperatriceImage *>(pointeur);
}
