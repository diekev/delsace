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

#include "biblinternes/graphe/noeud.h"
#include "biblinternes/outils/definitions.h"

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "corps/corps.h"

#include "noeud_image.h"
#include "usine_operatrice.h"

/* ************************************************************************** */

dls::image::Pixel<float> Calque::valeur(size_t x, size_t y) const
{
	x = std::max(0ul, std::min(x, static_cast<size_t>(tampon.nombre_colonnes()) - 1));
	y = std::max(0ul, std::min(y, static_cast<size_t>(tampon.nombre_lignes()) - 1));
	return tampon[static_cast<int>(y)][static_cast<int>(x)];
}

void Calque::valeur(size_t x, size_t y, dls::image::Pixel<float> const &pixel)
{
	x = std::max(0ul, std::min(x, static_cast<size_t>(tampon.nombre_colonnes()) - 1));
	y = std::max(0ul, std::min(y, static_cast<size_t>(tampon.nombre_lignes()) - 1));
	tampon[static_cast<int>(y)][static_cast<int>(x)] = pixel;
}

dls::image::Pixel<float> Calque::echantillone(float x, float y) const
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

	auto valeur = dls::image::Pixel<float>(0.0f);
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

Calque *Image::ajoute_calque(dls::chaine const &nom, Rectangle const &rectangle)
{
	auto tampon = memoire::loge<Calque>("Calque");
	tampon->nom = nom;
	tampon->tampon = type_image(dls::math::Hauteur(static_cast<int>(rectangle.hauteur)),
								dls::math::Largeur(static_cast<int>(rectangle.largeur)));

	auto pixel = dls::image::Pixel<float>(0.0f);
	pixel.a = 1.0f;

	tampon->tampon.remplie(pixel);

	m_calques.pousse(tampon);

	return tampon;
}

Calque *Image::calque(dls::chaine const &nom) const
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
	return plage_calques(m_calques.debut(), m_calques.fin());
}

Image::plage_calques_const Image::calques() const
{
	return plage_calques_const(m_calques.debut(), m_calques.fin());
}

void Image::reinitialise(bool garde_memoires)
{
	if (!garde_memoires) {
		for (Calque *tampon : m_calques) {
			memoire::deloge("Calque", tampon);
		}
	}

	m_calques.efface();
}

void Image::nom_calque_actif(dls::chaine const &nom)
{
	m_nom_calque = nom;
}

dls::chaine const &Image::nom_calque_actif() const
{
	return m_nom_calque;
}

/* ************************************************************************** */

EntreeOperatrice::EntreeOperatrice(PriseEntree *prise)
	: m_ptr(prise)
{}

bool EntreeOperatrice::connectee() const
{
	return !m_ptr->liens.est_vide();
}

void EntreeOperatrice::requiers_image(Image &image, ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	if (m_ptr->liens.est_vide()) {
		return;
	}

	auto lien = m_ptr->liens[0];

	m_liste_noms_calques.efface();

	if (lien != nullptr) {
		auto noeud = lien->parent;

		execute_noeud(noeud, contexte, donnees_aval);

		auto operatrice = extrait_opimage(noeud->donnees());
		operatrice->transfere_image(image);

		for (auto const &calque : image.calques()) {
			m_liste_noms_calques.pousse(calque->nom);
		}
	}
}

vision::Camera3D *EntreeOperatrice::requiers_camera(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	if (m_ptr->liens.est_vide()) {
		return nullptr;
	}

	auto lien = m_ptr->liens[0];

	if (lien == nullptr) {
		return nullptr;
	}

	auto noeud = lien->parent;

	execute_noeud(noeud, contexte, donnees_aval);

	auto operatrice = extrait_opimage(noeud->donnees());
	return operatrice->camera();
}

TextureImage *EntreeOperatrice::requiers_texture(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	if (m_ptr->liens.est_vide()) {
		return nullptr;
	}

	auto lien = m_ptr->liens[0];

	if (lien == nullptr) {
		return nullptr;
	}

	auto noeud = lien->parent;

	execute_noeud(noeud, contexte, donnees_aval);

	auto operatrice = extrait_opimage(noeud->donnees());
	return operatrice->texture();
}

const Corps *EntreeOperatrice::requiers_corps(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	if (m_ptr->liens.est_vide()) {
		return nullptr;
	}

	auto lien = m_ptr->liens[0];

	if (lien == nullptr) {
		return nullptr;
	}

	auto noeud = lien->parent;

	execute_noeud(noeud, contexte, donnees_aval);

	auto operatrice = extrait_opimage(noeud->donnees());

	return operatrice->corps();
}

Corps *EntreeOperatrice::requiers_copie_corps(Corps *corps, ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	auto corps_lien = this->requiers_corps(contexte, donnees_aval);

	if (corps_lien == nullptr) {
		return nullptr;
	}

	if (corps != nullptr) {
		corps_lien->copie_vers(corps);
		return corps;
	}

	return corps_lien->copie();
}

void EntreeOperatrice::obtiens_liste_calque(dls::tableau<dls::chaine> &chaines) const
{
	chaines = m_liste_noms_calques;
}

void EntreeOperatrice::obtiens_liste_attributs(dls::tableau<dls::chaine> &chaines) const
{
	if (m_ptr->liens.est_vide()) {
		return;
	}

	auto lien = m_ptr->liens[0];

	if (lien == nullptr) {
		return;
	}

	auto noeud = lien->parent;
	auto operatrice = extrait_opimage(noeud->donnees());
	auto corps = operatrice->corps();

	if (corps == nullptr) {
		return;
	}

	for (auto attributs : corps->attributs()) {
		chaines.pousse(attributs->nom());
	}
}

void EntreeOperatrice::obtiens_liste_groupes_prims(dls::tableau<dls::chaine> &chaines) const
{
	if (m_ptr->liens.est_vide()) {
		return;
	}

	auto lien = m_ptr->liens[0];

	if (lien == nullptr) {
		return;
	}

	auto noeud = lien->parent;
	auto operatrice = extrait_opimage(noeud->donnees());
	auto corps = operatrice->corps();

	if (corps == nullptr) {
		return;
	}

	for (auto const &groupe : corps->groupes_prims()) {
		chaines.pousse(groupe.nom);
	}
}

void EntreeOperatrice::obtiens_liste_groupes_points(dls::tableau<dls::chaine> &chaines) const
{
	if (m_ptr->liens.est_vide()) {
		return;
	}

	auto lien = m_ptr->liens[0];

	if (lien == nullptr) {
		return;
	}

	auto noeud = lien->parent;
	auto operatrice = extrait_opimage(noeud->donnees());
	auto corps = operatrice->corps();

	if (corps == nullptr) {
		return;
	}

	for (auto const &groupe : corps->groupes_points()) {
		chaines.pousse(groupe.nom);
	}
}

/* ************************************************************************** */

OperatriceImage::OperatriceImage(Graphe &graphe_parent, Noeud *node)
	: m_graphe_parent(graphe_parent)
{
	node->donnees(this);
	m_input_data.redimensionne(m_num_inputs);
	m_sorties.redimensionne(m_num_outputs);
}

void OperatriceImage::usine(UsineOperatrice *usine_op)
{
	m_usine = usine_op;
}

UsineOperatrice *OperatriceImage::usine() const
{
	return m_usine;
}

int OperatriceImage::type() const
{
	return OPERATRICE_IMAGE;
}

void OperatriceImage::entrees(long number)
{
	m_num_inputs = number;
	m_input_data.redimensionne(number);
}

long OperatriceImage::entrees() const
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

EntreeOperatrice *OperatriceImage::entree(long index)
{
	if (index >= m_num_inputs) {
		return nullptr;
	}

	return &m_input_data[index];
}

const EntreeOperatrice *OperatriceImage::entree(long index) const
{
	if (index >= m_num_inputs) {
		return nullptr;
	}

	return &m_input_data[index];
}

void OperatriceImage::donnees_entree(long index, PriseEntree *socket)
{
	if (index >= m_input_data.taille()) {
		std::cerr << nom_classe() << " : Overflow while setting data (inputs: "
				  << m_input_data.taille() << ", index: " << index << ")\n";
		return;
	}

	m_input_data[index] = EntreeOperatrice(socket);
}

SortieOperatrice *OperatriceImage::sortie(long index)
{
	if (index >= m_num_outputs) {
		return nullptr;
	}

	return &m_sorties[index];
}

void OperatriceImage::donnees_sortie(long index, PriseSortie *prise)
{
	if (index >= m_sorties.taille()) {
		std::cerr << nom_classe() << " : Overflow while setting data (outputs: "
				  << m_sorties.taille() << ", index: " << index << ")\n";
		return;
	}

	m_sorties[index] = SortieOperatrice(prise);
}

void OperatriceImage::sorties(long number)
{
	m_num_outputs = number;
	m_sorties.redimensionne(number);
}

long OperatriceImage::sorties() const
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

void OperatriceImage::ajoute_avertissement(dls::chaine const &avertissement)
{
	m_avertissements.pousse(avertissement);
}

void OperatriceImage::reinitialise_avertisements()
{
	m_avertissements.efface();
}

dls::tableau<dls::chaine> const &OperatriceImage::avertissements() const
{
	return m_avertissements;
}

vision::Camera3D *OperatriceImage::camera()
{
	return nullptr;
}

TextureImage *OperatriceImage::texture()
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
{}

void OperatriceImage::obtiens_liste(
		ContexteEvaluation const &contexte,
		dls::chaine const &attache,
		dls::tableau<dls::chaine> &chaines)
{
	INUTILISE(contexte);
	INUTILISE(attache);

	if (entrees() == 0) {
		chaines.efface();
		return;
	}

	/* Par défaut, on utilise la liste de la première entrée. */
	entree(0)->obtiens_liste_calque(chaines);
}

void OperatriceImage::renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud)
{
	INUTILISE(contexte);
	INUTILISE(compilatrice);
	INUTILISE(noeud);
}

bool OperatriceImage::possede_animation()
{
	for (auto iter = this->debut(); iter != this->fin(); ++iter) {
		auto &prop = iter->second;

		if (prop.est_anime()) {
			return true;
		}
	}

	return false;
}

bool OperatriceImage::depend_sur_temps() const
{
	return false;
}

void OperatriceImage::amont_change()
{
	return;
}

/* ************************************************************************** */

static void supprime_operatrice_image(std::any pointeur)
{
	auto ptr = extrait_opimage(pointeur);

	/* Lorsque nous logeons un pointeur nous utilisons la taille de la classe
	 * dérivée pour estimer la quantité de mémoire allouée. Donc pour déloger
	 * proprement l'opératrice, en prenant en compte la taille de la classe
	 * dériviée, il faut transtyper le pointeur vers le bon type dérivée. Pour
	 * ce faire, nous devons utiliser l'usine pour avoir accès aux descriptions
	 * des opératrices. */
	auto usine = ptr->usine();
	usine->deloge(ptr);
}

Noeud *cree_noeud_image()
{
	return memoire::loge<Noeud>("Noeud", supprime_operatrice_image);
}

void supprime_noeud_image(Noeud *noeud)
{
	memoire::deloge("Noeud", noeud);
}
