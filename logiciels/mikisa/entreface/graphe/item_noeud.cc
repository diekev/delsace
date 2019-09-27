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

#include "item_noeud.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFont>
#include <QPen>
#pragma GCC diagnostic pop

#include "coeur/noeud.hh"
#include "coeur/noeud_image.h"
#include "coeur/operatrice_image.h"

static auto const COULEUR_DECIMAL       = QColor::fromRgb(128, 128, 128);
static auto const COULEUR_ENTIER        = QColor::fromRgb(255, 255, 255);
static auto const COULEUR_VEC2          = QColor::fromHsl(static_cast<int>(143.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_VEC3          = QColor::fromHsl(static_cast<int>(154.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_VEC4          = QColor::fromHsl(static_cast<int>(165.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_MAT3          = QColor::fromHsl(static_cast<int>(189.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_MAT4          = QColor::fromHsl(static_cast<int>(200.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_COULEUR       = QColor::fromHsl(static_cast<int>(176.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_CORPS         = QColor::fromHsl(static_cast<int>( 90.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_IMAGE         = QColor::fromHsl(static_cast<int>(156.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_OBJET         = QColor::fromHsl(static_cast<int>( 90.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_TABLEAU       = QColor::fromHsl(static_cast<int>(211.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_POLYMORPHIQUE = QColor::fromHsl(static_cast<int>(249.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_CHAINE        = QColor::fromHsl(static_cast<int>(132.0 / 255.0 * 359.0), 190, 79);
static auto const COULEUR_INVALIDE      = QColor::fromHsl(0, 0, 0);

static QBrush brosse_pour_type(type_prise type)
{
	switch (type) {
		case type_prise::DECIMAL:
		{
			return QBrush(COULEUR_DECIMAL);
		}
		case type_prise::ENTIER:
		{
			return QBrush(COULEUR_ENTIER);
		}
		case type_prise::VEC2:
		{
			return QBrush(COULEUR_VEC2);
		}
		case type_prise::VEC3:
		{
			return QBrush(COULEUR_VEC3);
		}
		case type_prise::VEC4:
		{
			return QBrush(COULEUR_VEC4);
		}
		case type_prise::MAT3:
		{
			return QBrush(COULEUR_MAT3);
		}
		case type_prise::MAT4:
		{
			return QBrush(COULEUR_MAT4);
		}
		case type_prise::COULEUR:
		{
			return QBrush(COULEUR_COULEUR);
		}
		case type_prise::CORPS:
		{
			return QBrush(COULEUR_CORPS);
		}
		case type_prise::IMAGE:
		{
			return QBrush(COULEUR_IMAGE);
		}
		case type_prise::OBJET:
		{
			return QBrush(COULEUR_OBJET);
		}
		case type_prise::TABLEAU:
		{
			return QBrush(COULEUR_TABLEAU);
		}
		case type_prise::POLYMORPHIQUE:
		{
			return QBrush(COULEUR_POLYMORPHIQUE);
		}
		case type_prise::CHAINE:
		{
			return QBrush(COULEUR_CHAINE);
		}
		case type_prise::INVALIDE:
		{
			return QBrush(COULEUR_INVALIDE);
		}
	}

	return QBrush(COULEUR_INVALIDE);
}

ItemNoeud::ItemNoeud(
		Noeud *noeud,
		bool selectionne,
		bool est_noeud_detail,
		QGraphicsItem *parent)
	: QGraphicsRectItem(parent)
{
	auto operatrice = static_cast<OperatriceImage *>(nullptr);
	auto brosse_couleur = QBrush();

	switch (noeud->type) {
		case type_noeud::COMPOSITE:
		{
			brosse_couleur = brosse_pour_type(type_prise::IMAGE);
			break;
		}
		case type_noeud::INVALIDE:
		{
			break;
		}
		case type_noeud::NUANCEUR:
		{
			brosse_couleur = brosse_pour_type(type_prise::IMAGE);
			break;
		}
		case type_noeud::OBJET:
		{
			brosse_couleur = brosse_pour_type(type_prise::OBJET);
			break;
		}
		case type_noeud::OPERATRICE:
		{
			operatrice = extrait_opimage(noeud->donnees);

			switch (operatrice->type()) {
				default:
				case OPERATRICE_SORTIE_IMAGE:
				case OPERATRICE_IMAGE:
				case OPERATRICE_PIXEL:
					brosse_couleur = brosse_pour_type(type_prise::IMAGE);
					break;
				case OPERATRICE_GRAPHE_DETAIL:
				case OPERATRICE_SIMULATION:
				case OPERATRICE_CORPS:
				case OPERATRICE_SORTIE_CORPS:
					brosse_couleur = brosse_pour_type(type_prise::CORPS);
					break;
				case OPERATRICE_OBJET:
					brosse_couleur = brosse_pour_type(type_prise::OBJET);
					break;
			}

			break;
		}
		case type_noeud::RENDU:
		{
			brosse_couleur = brosse_pour_type(type_prise::IMAGE);
			break;
		}
	}

	if (est_noeud_detail) {
		dessine_noeud_detail(noeud, operatrice, selectionne);
	}
	else {
		dessine_noeud_generique(noeud, operatrice, brosse_couleur, selectionne);
	}
}

void ItemNoeud::dessine_noeud_detail(
		Noeud *noeud,
		OperatriceImage *operatrice,
		bool selectionne)
{
	auto const pos_x = static_cast<double>(noeud->pos_x());
	auto const pos_y = static_cast<double>(noeud->pos_y());

	/* crée le texte en premier pour calculer sa taille */
	auto const decalage_texte = 8;
	auto texte = new QGraphicsTextItem(noeud->nom.c_str(), this);
	auto police = QFont();
	police.setPointSize(16);
	texte->setFont(police);

	auto const largeur_texte = texte->boundingRect().width() + decalage_texte * 2;
	auto const hauteur_texte = texte->boundingRect().height();

	/* crée le texte pour chacune des entrées/sorties */

	auto textes_entrees = dls::tableau<QGraphicsTextItem *>();
	auto textes_sorties = dls::tableau<QGraphicsTextItem *>();

	auto const hauteur_prise = 32.0;
	auto const largeur_prise = 32.0;

	auto largeur_entrees = 0.0;
	auto hauteur_entrees = 0.0;

	for (auto prise : noeud->entrees) {
		auto texte_prise = new QGraphicsTextItem(prise->nom.c_str(), this);
		texte_prise->setFont(police);
		textes_entrees.pousse(texte_prise);

		largeur_entrees = std::max(largeur_entrees, texte_prise->boundingRect().width());
		hauteur_entrees += texte_prise->boundingRect().height();
	}

	if (noeud->entrees.taille() != 0) {
		largeur_entrees += largeur_prise;
	}

	auto largeur_sorties = 0.0;
	auto hauteur_sorties = 0.0;

	for (auto prise : noeud->sorties) {
		auto texte_prise = new QGraphicsTextItem(prise->nom.c_str(), this);
		texte_prise->setFont(police);
		textes_sorties.pousse(texte_prise);

		largeur_sorties = std::max(largeur_sorties, texte_prise->boundingRect().width());
		hauteur_sorties += texte_prise->boundingRect().height();
	}

	if (noeud->sorties.taille() != 0) {
		largeur_sorties += largeur_prise;
	}

	auto hauteur_noeud = hauteur_texte + decalage_texte + std::max(hauteur_entrees, hauteur_sorties);
	auto largeur_noeud = std::max(largeur_texte, largeur_entrees + largeur_sorties + 2 * decalage_texte);

	/* positionne les textes */
	texte->setDefaultTextColor(Qt::white);
	texte->setPos(pos_x + decalage_texte + (largeur_noeud - largeur_texte) / 2, pos_y);

	auto ligne = new QGraphicsLineItem(this);
	ligne->setPen(QPen(Qt::white));
	ligne->setLine(pos_x, pos_y + hauteur_texte, pos_x + largeur_noeud, pos_y + hauteur_texte);

	auto pos_y_entree = hauteur_texte + decalage_texte;

	for (auto i = 0; i < textes_entrees.taille(); ++i) {
		auto prise = noeud->entree(i);

		auto item_prise = new QGraphicsRectItem(this);
		item_prise->setRect(pos_x, pos_y + pos_y_entree, largeur_prise, hauteur_prise);
		item_prise->setBrush(brosse_pour_type(prise->type));
		item_prise->setPen(QPen(Qt::white, 0.5));

		textes_entrees[i]->setDefaultTextColor(Qt::white);
		textes_entrees[i]->setPos(pos_x + largeur_prise + decalage_texte, pos_y + pos_y_entree);

		prise->rectangle.x = static_cast<float>(pos_x);
		prise->rectangle.y = static_cast<float>(pos_y + pos_y_entree);
		prise->rectangle.hauteur = hauteur_prise;
		prise->rectangle.largeur = largeur_prise;

		pos_y_entree += hauteur_texte;
	}

	auto pos_y_sortie = hauteur_texte + decalage_texte;

	for (auto i = 0; i < textes_sorties.taille(); ++i) {
		auto prise = noeud->sortie(i);

		auto item_prise = new QGraphicsRectItem(this);
		item_prise->setRect(pos_x + largeur_noeud - largeur_prise, pos_y + pos_y_sortie, largeur_prise, hauteur_prise);
		item_prise->setBrush(brosse_pour_type(prise->type));
		item_prise->setPen(QPen(Qt::white, 0.5));

		textes_sorties[i]->setDefaultTextColor(Qt::white);
		textes_sorties[i]->setPos(pos_x + largeur_noeud - decalage_texte - (textes_sorties[i]->boundingRect().width()) - largeur_prise, pos_y + pos_y_sortie);

		prise->rectangle.x = static_cast<float>(pos_x + largeur_noeud - largeur_prise);
		prise->rectangle.y = static_cast<float>(pos_y + pos_y_sortie);
		prise->rectangle.hauteur = hauteur_prise;
		prise->rectangle.largeur = largeur_prise;

		pos_y_sortie += hauteur_texte;
	}

	if (selectionne) {
		/* pinceaux pour le contour du noeud */
		auto stylo = QPen(Qt::yellow);
		stylo.setWidthF(1.0);
		setPen(stylo);
	}
	else {
		/* pinceaux pour le contour du noeud */
		auto stylo = QPen(Qt::white);
		stylo.setWidthF(0.5);
		setPen(stylo);
	}

	finalise_dessin(
				noeud,
				operatrice,
				selectionne,
				pos_x,
				pos_y,
				largeur_noeud,
				hauteur_noeud);
}

void ItemNoeud::dessine_noeud_generique(
		Noeud *noeud,
		OperatriceImage *operatrice,
		QBrush const &brosse_couleur,
		bool selectionne)
{
	auto const pos_x = static_cast<double>(noeud->pos_x());
	auto const pos_y = static_cast<double>(noeud->pos_y());

	/* crée le texte en premier pour calculer sa taille */
	auto const decalage_texte = 8;
	auto texte = new QGraphicsTextItem(noeud->nom.c_str(), this);
	auto police = QFont();
	police.setPointSize(16);
	texte->setFont(police);

	auto const largeur_texte = texte->boundingRect().width() + decalage_texte * 2;
	auto const hauteur_texte = texte->boundingRect().height();

	auto hauteur_noeud = 0.0;
	auto largeur_noeud = 0.0;

	auto const hauteur_icone = 64.0;
	auto const largeur_icone = 64.0;

	auto const hauteur_prise = 32.0;
	auto const largeur_prise = 32.0;

	auto const nombre_entrees = noeud->entrees.taille();
	auto const nombre_sorties = noeud->sorties.taille();

	auto decalage_icone_y = pos_y;
	auto decalage_texte_y = pos_y;
	auto decalage_sorties_y = pos_y;

	if (nombre_entrees > 0) {
		hauteur_noeud += hauteur_prise + hauteur_prise * 0.5;
		decalage_icone_y += hauteur_noeud;
		decalage_texte_y += hauteur_noeud;
		decalage_sorties_y += hauteur_noeud;
	}

	if (nombre_sorties > 0) {
		decalage_sorties_y += hauteur_icone + hauteur_prise * 0.5;
		hauteur_noeud += hauteur_prise + hauteur_prise * 0.5;
	}

	largeur_noeud += largeur_icone;
	largeur_noeud += largeur_texte;
	hauteur_noeud += hauteur_icone;

	/* entrées du noeud */
	if (nombre_entrees > 0) {
		auto const etendue_entree = (largeur_noeud / static_cast<double>(nombre_entrees));
		auto const pos_debut_entrees = etendue_entree * 0.5 - largeur_prise * 0.5;
		auto pos_entree = pos_x + pos_debut_entrees;

		for (PriseEntree *prise : noeud->entrees) {
			auto entree = new QGraphicsRectItem(this);
			auto largeur_lien = largeur_prise;

			if (prise->multiple_connexions) {
				pos_entree -= largeur_prise * 0.5;
				largeur_lien *= 2.0;
			}

			entree->setRect(pos_entree, pos_y, largeur_lien, hauteur_prise);
			entree->setBrush(brosse_pour_type(prise->type));
			entree->setPen(QPen(Qt::white, 0.5));

			prise->rectangle.x = static_cast<float>(pos_entree);
			prise->rectangle.y = static_cast<float>(pos_y);
			prise->rectangle.hauteur = hauteur_prise;
			prise->rectangle.largeur = static_cast<float>(largeur_lien);

			pos_entree += etendue_entree;
		}

		auto ligne = new QGraphicsLineItem(this);
		ligne->setPen(QPen(Qt::white));
		ligne->setLine(pos_x, decalage_icone_y, pos_x + largeur_noeud, decalage_icone_y);
	}

	/* icone */
	auto icone = new QGraphicsRectItem(this);
	icone->setRect(pos_x + 1, decalage_icone_y + 1, largeur_icone - 2, hauteur_icone - 2);
	icone->setBrush(brosse_couleur);
	icone->setPen(QPen(QColor(0, 0, 0, 0), 0.0));

	/* nom du noeud */
	texte->setDefaultTextColor(Qt::white);
	texte->setPos(pos_x + largeur_icone + decalage_texte, decalage_texte_y + (hauteur_icone - hauteur_texte) / 2);

	/* sorties du noeud */
	if (nombre_sorties > 0) {
		auto ligne = new QGraphicsLineItem(this);
		ligne->setPen(QPen(Qt::white));
		ligne->setLine(pos_x, decalage_icone_y + hauteur_icone, pos_x + largeur_noeud, decalage_icone_y + hauteur_icone);

		auto const etendue_sortie = (largeur_noeud / static_cast<double>(nombre_sorties));
		auto const pos_debut_sorties = etendue_sortie * 0.5 - largeur_prise * 0.5;
		auto pos_sortie = pos_x + pos_debut_sorties;

		for (PriseSortie *prise : noeud->sorties) {
			auto sortie = new QGraphicsRectItem(this);

			sortie->setRect(pos_sortie, decalage_sorties_y, largeur_prise, hauteur_prise);
			sortie->setBrush(brosse_pour_type(prise->type));
			sortie->setPen(QPen(Qt::white, 0.5));

			prise->rectangle.x = static_cast<float>(pos_sortie);
			prise->rectangle.y = static_cast<float>(decalage_sorties_y);
			prise->rectangle.hauteur = hauteur_prise;
			prise->rectangle.largeur = largeur_prise;

			pos_sortie += etendue_sortie;
		}
	}

	finalise_dessin(
				noeud,
				operatrice,
				selectionne,
				pos_x,
				pos_y,
				largeur_noeud,
				hauteur_noeud);
}

void ItemNoeud::finalise_dessin(
		Noeud *noeud,
		OperatriceImage *operatrice,
		bool selectionne,
		double pos_x,
		double pos_y,
		double largeur_noeud,
		double hauteur_noeud)
{
	if (selectionne) {
		//			auto cadre = new QGraphicsRectItem(this);
		//			cadre->setRect(pos_x - 5, pos_y - 5, largeur_noeud + 5, hauteur_noeud + 5);
		//			cadre->setBrush(QBrush(QColor(128, 128, 128)));
		//			cadre->setPen(QPen(Qt::white, 0.0f));

		/* pinceaux pour le contour du noeud */
		auto stylo = QPen(Qt::yellow);
		stylo.setWidthF(1.0);
		setPen(stylo);
	}
	else {
		/* pinceaux pour le contour du noeud */
		auto stylo = QPen(Qt::white);
		stylo.setWidthF(0.5);
		setPen(stylo);
	}

	/* pinceaux pour le coeur du noeud */
	QBrush brosse;

	if (!operatrice || operatrice->avertissements().est_vide()) {
		brosse = QBrush(QColor(45, 45, 45));
	}
	else {
		brosse = QBrush(QColor(255, 180, 10));
	}

	setBrush(brosse);

	setRect(pos_x, pos_y, largeur_noeud, hauteur_noeud);

	noeud->largeur(static_cast<int>(largeur_noeud));
	noeud->hauteur(static_cast<int>(hauteur_noeud));
}
