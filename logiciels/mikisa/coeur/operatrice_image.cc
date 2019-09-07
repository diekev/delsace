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

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/structures/pile.hh"

#include "corps/corps.h"

#include "chef_execution.hh"
#include "noeud.hh"
#include "noeud_image.h"
#include "operatrice_graphe_detail.hh"
#include "usine_operatrice.h"

/* ************************************************************************** */

EntreeOperatrice::EntreeOperatrice(PriseEntree *prise)
	: m_ptr(prise)
{}

bool EntreeOperatrice::connectee() const
{
	return !m_ptr->liens.est_vide();
}

long EntreeOperatrice::nombre_connexions() const
{
	return m_ptr->liens.taille();
}

Image const *EntreeOperatrice::requiers_image(
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval,
		int index)
{
	if (m_ptr->liens.est_vide() || index < 0 || index >= m_ptr->liens.taille()) {
		return nullptr;
	}

	auto lien = m_ptr->liens[index];

	m_liste_noms_calques.efface();

	if (lien == nullptr) {
		return nullptr;
	}

	auto noeud = lien->parent;

	execute_noeud(*noeud, contexte, donnees_aval);

	auto operatrice = extrait_opimage(noeud->donnees);
	auto image = operatrice->image();

	for (auto const &calque : image->calques()) {
		m_liste_noms_calques.pousse(calque->nom);
	}

	return image;
}

Image *EntreeOperatrice::requiers_copie_image(
		Image &image,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval,
		int index)
{
	auto image_op = this->requiers_image(contexte, donnees_aval, index);

	if (image_op == nullptr) {
		return nullptr;
	}

	image = *image_op;

	for (auto const &calque : image.calques()) {
		m_liste_noms_calques.pousse(calque->nom);
	}

	return &image;
}

const Corps *EntreeOperatrice::requiers_corps(
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval,
		int index)
{
	if (m_ptr->liens.est_vide() || index < 0 || index >= m_ptr->liens.taille()) {
		return nullptr;
	}

	auto lien = m_ptr->liens[index];

	if (lien == nullptr) {
		return nullptr;
	}

	auto noeud = lien->parent;

	execute_noeud(*noeud, contexte, donnees_aval);

	auto operatrice = extrait_opimage(noeud->donnees);

	return operatrice->corps();
}

Corps *EntreeOperatrice::requiers_copie_corps(
		Corps *corps,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval,
		int index)
{
	auto corps_lien = this->requiers_corps(contexte, donnees_aval, index);

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
	auto operatrice = extrait_opimage(noeud->donnees);
	auto corps = operatrice->corps();

	if (corps == nullptr) {
		return;
	}

	for (auto const &attributs : corps->attributs()) {
		chaines.pousse(attributs.nom());
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
	auto operatrice = extrait_opimage(noeud->donnees);
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
	auto operatrice = extrait_opimage(noeud->donnees);
	auto corps = operatrice->corps();

	if (corps == nullptr) {
		return;
	}

	for (auto const &groupe : corps->groupes_points()) {
		chaines.pousse(groupe.nom);
	}
}

PriseEntree *EntreeOperatrice::pointeur()
{
	return m_ptr;
}

void EntreeOperatrice::signale_cache(ChefExecution *chef) const
{
	if (m_ptr->liens.est_vide()) {
		return;
	}

	auto lien = m_ptr->liens[0];

	if (lien == nullptr) {
		return;
	}

	/* nous poussons les noeuds dans une liste pour pouvoir donner une
	 * progression correcte */

	auto liste = dls::tableau<Noeud *>();
	auto pile = dls::pile<Noeud *>();
	pile.empile(lien->parent);

	while (!pile.est_vide()) {
		auto noeud = pile.depile();

		liste.pousse(noeud);

		for (auto entree : noeud->entrees) {
			for (auto sortie : entree->liens) {
				pile.empile(sortie->parent);
			}
		}
	}

	auto progres = 0.0f;
	auto delta = 100.0f / static_cast<float>(liste.taille());

	for (auto noeud : liste) {
		noeud->besoin_execution = true;
		auto op = extrait_opimage(noeud->donnees);
		op->libere_memoire();

		chef->indique_progression(progres + delta);
		progres += delta;
	}
}

/* ************************************************************************** */

SortieOperatrice::SortieOperatrice(PriseSortie *prise)
	: m_ptr(prise)
{}

PriseSortie *SortieOperatrice::pointeur()
{
	return m_ptr;
}

/* ************************************************************************** */

OperatriceImage::OperatriceImage(Graphe &graphe_parent, Noeud &n)
	: m_graphe_parent(graphe_parent)
	, noeud(n)
{
	noeud.donnees = this;
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

bool OperatriceImage::execute_toujours() const
{
	return m_execute_toujours;
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
	static const char *noms[] = {
		"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
		"O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
	};

	return noms[n];
}

type_prise OperatriceImage::type_entree(int n) const
{
	switch (n) {
		default:
		case 0: return type_prise::IMAGE;
	}
}

bool OperatriceImage::connexions_multiples(int n) const
{
	INUTILISE(n);
	return false;
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
	static const char *noms[] = {
		"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
		"O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
	};

	return noms[n];
}

type_prise OperatriceImage::type_sortie(int n) const
{
	switch (n) {
		default:
		case 0: return type_prise::IMAGE;
	}
}

const char *OperatriceImage::chemin_entreface() const
{
	return "";
}

void OperatriceImage::transfere_image(Image &image)
{
	image = m_image;
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

Image *OperatriceImage::image()
{
	return &m_image;
}

Image const *OperatriceImage::image() const
{
	return &m_image;
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

void OperatriceImage::renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud_reseau)
{
	INUTILISE(contexte);
	INUTILISE(compilatrice);
	INUTILISE(noeud_reseau);
}

bool OperatriceImage::depend_sur_temps() const
{
	return false;
}

void OperatriceImage::amont_change(PriseEntree *entree)
{
	INUTILISE(entree);
	return;
}

void OperatriceImage::parametres_changes()
{
	return;
}

void OperatriceImage::libere_memoire()
{
	m_image.reinitialise();
	cache_est_invalide = true;
}

/* ************************************************************************** */

calque_image const *cherche_calque(
		OperatriceImage &op,
		Image const *image,
		dls::chaine const &nom_calque)
{
	if (image == nullptr) {
		op.ajoute_avertissement("Aucune image trouvée en entrée !");
		return nullptr;
	}

	if (nom_calque.est_vide()) {
		op.ajoute_avertissement("Le nom du calque est vide");
		return nullptr;
	}

	auto tampon = image->calque_pour_lecture(nom_calque);

	if (tampon == nullptr) {
		auto flux = dls::flux_chaine();
		flux << "Calque '" << nom_calque << "' introuvable !\n";
		op.ajoute_avertissement(flux.chn());
		return nullptr;
	}

	return tampon;
}
