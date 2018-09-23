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

#include <QFont>
#include <QPen>

#include "bibliotheques/graphe/noeud.h"

#include "coeur/noeud_image.h"
#include "coeur/operatrice_image.h"

static const auto COULEUR_OBJET  = QColor::fromHsl( 90 / 255.0 * 359.0, 190, 79);
static const auto COULEUR_IMAGE  = QColor::fromHsl(156 / 255.0 * 359.0, 190, 79);
static const auto COULEUR_PIXEL  = QColor::fromHsl(176 / 255.0 * 359.0, 190, 79);
static const auto COULEUR_CAMERA = QColor::fromHsl(211 / 255.0 * 359.0, 190, 79);
static const auto COULEUR_SCENE  = QColor::fromHsl(249 / 255.0 * 359.0, 190, 79);

static QBrush brosse_pour_type(int type)
{
	switch (type) {
		default:
		case OPERATRICE_IMAGE:
			return QBrush(COULEUR_IMAGE);
		case OPERATRICE_GRAPHE_PIXEL:
		case OPERATRICE_PIXEL:
			return QBrush(COULEUR_PIXEL);
		case OPERATRICE_SCENE:
			return QBrush(COULEUR_SCENE);
		case OPERATRICE_CORPS:
		case OPERATRICE_OBJET:
			return QBrush(COULEUR_OBJET);
		case OPERATRICE_CAMERA:
			return QBrush(COULEUR_CAMERA);
	}
}

ItemNoeud::ItemNoeud(Noeud *noeud, bool selectionne, QGraphicsItem *parent)
	: QGraphicsRectItem(parent)
{
	auto operatrice = static_cast<OperatriceImage *>(noeud->donnees());

	const auto pos_x = noeud->pos_x();
	const auto pos_y = noeud->pos_y();

	/* crée le texte en premier pour calculer sa taille */
	const auto decalage_texte = 8;
	auto texte = new QGraphicsTextItem(noeud->nom().c_str(), this);
	auto police = QFont();
	police.setPointSize(16);
	texte->setFont(police);

	const auto largeur_texte = texte->boundingRect().width() + decalage_texte * 2;
	const auto hauteur_texte = texte->boundingRect().height();

	auto hauteur_noeud = 0;
	auto largeur_noeud = 0;

	const auto hauteur_icone = 64;
	const auto largeur_icone = 64;

	const auto hauteur_prise = 32;
	const auto largeur_prise = 32;

	const auto nombre_entrees = noeud->entrees().size();
	const auto nombre_sorties = noeud->sorties().size();

	auto decalage_icone_y = pos_y;
	auto decalage_texte_y = pos_y;
	auto decalage_sorties_y = pos_y;

	if (nombre_entrees > 0) {
		hauteur_noeud += hauteur_prise + hauteur_prise * 0.5f;
		decalage_icone_y += hauteur_noeud;
		decalage_texte_y += hauteur_noeud;
		decalage_sorties_y += hauteur_noeud;
	}

	if (nombre_sorties > 0) {
		decalage_sorties_y += hauteur_icone + hauteur_prise * 0.5f;
		hauteur_noeud += hauteur_prise + hauteur_prise * 0.5f;
	}

	largeur_noeud += largeur_icone;
	largeur_noeud += largeur_texte;
	hauteur_noeud += hauteur_icone;

	/* entrées du noeud */
	if (nombre_entrees > 0) {
		const auto etendue_entree = (largeur_noeud / static_cast<float>(nombre_entrees));
		const auto pos_debut_entrees = etendue_entree * 0.5f - largeur_prise * 0.5f;
		auto pos_entree = pos_x + pos_debut_entrees;

		for (PriseEntree *prise : noeud->entrees()) {
			auto entree = new QGraphicsRectItem(this);

			entree->setRect(pos_entree, pos_y, largeur_prise, hauteur_prise);
			entree->setBrush(brosse_pour_type(prise->type));
			entree->setPen(QPen(Qt::white, 0.5f));

			prise->rectangle.x = pos_entree;
			prise->rectangle.y = pos_y;
			prise->rectangle.hauteur = hauteur_prise;
			prise->rectangle.largeur = largeur_prise;

			pos_entree += etendue_entree;
		}

		auto ligne = new QGraphicsLineItem(this);
		ligne->setPen(QPen(Qt::white));
		ligne->setLine(pos_x, decalage_icone_y, pos_x + largeur_noeud, decalage_icone_y);
	}

	/* icone */
	auto icone = new QGraphicsRectItem(this);
	icone->setRect(pos_x + 1, decalage_icone_y + 1, largeur_icone - 2, hauteur_icone - 2);
	icone->setBrush(brosse_pour_type(operatrice->type()));
	icone->setPen(QPen(QColor(0, 0, 0, 0), 0.0));

	/* nom du noeud */
	texte->setDefaultTextColor(Qt::white);
	texte->setPos(pos_x + largeur_icone + decalage_texte, decalage_texte_y + (hauteur_icone - hauteur_texte) / 2);

	/* sorties du noeud */
	if (nombre_sorties > 0) {
		auto ligne = new QGraphicsLineItem(this);
		ligne->setPen(QPen(Qt::white));
		ligne->setLine(pos_x, decalage_icone_y + hauteur_icone, pos_x + largeur_noeud, decalage_icone_y + hauteur_icone);

		const auto etendue_sortie = (largeur_noeud / static_cast<float>(nombre_sorties));
		const auto pos_debut_sorties = etendue_sortie * 0.5f - largeur_prise * 0.5f;
		auto pos_sortie = pos_x + pos_debut_sorties;

		for (PriseSortie *prise : noeud->sorties()) {
			auto sortie = new QGraphicsRectItem(this);

			sortie->setRect(pos_sortie, decalage_sorties_y, largeur_prise, hauteur_prise);
			sortie->setBrush(brosse_pour_type(prise->type));
			sortie->setPen(QPen(Qt::white, 0.5f));

			prise->rectangle.x = pos_sortie;
			prise->rectangle.y = decalage_sorties_y;
			prise->rectangle.hauteur = hauteur_prise;
			prise->rectangle.largeur = largeur_prise;

			pos_sortie += etendue_sortie;
		}
	}

	if (selectionne) {
		//			auto cadre = new QGraphicsRectItem(this);
		//			cadre->setRect(pos_x - 5, pos_y - 5, largeur_noeud + 5, hauteur_noeud + 5);
		//			cadre->setBrush(QBrush(QColor(128, 128, 128)));
		//			cadre->setPen(QPen(Qt::white, 0.0f));

		/* pinceaux pour le contour du noeud */
		auto stylo = QPen(Qt::yellow);
		stylo.setWidthF(1.0f);
		setPen(stylo);
	}
	else {
		/* pinceaux pour le contour du noeud */
		auto stylo = QPen(Qt::white);
		stylo.setWidthF(0.5f);
		setPen(stylo);
	}

	/* pinceaux pour le coeur du noeud */
	QBrush brosse;

	if (operatrice->avertissements().empty()) {
		brosse = QBrush(QColor(45, 45, 45));
	}
	else {
		brosse = QBrush(QColor(255, 180, 10));
	}

	setBrush(brosse);

	setRect(pos_x, pos_y, largeur_noeud, hauteur_noeud);

	noeud->largeur(largeur_noeud);
	noeud->hauteur(hauteur_noeud);
}
