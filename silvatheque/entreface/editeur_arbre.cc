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

#include "editeur_arbre.h"

#include <danjo/danjo.h>

#include <QVBoxLayout>
#include <QScrollArea>

#include "coeur/arbre.h"
#include "coeur/creation_arbre.h"
#include "coeur/evenement.h"
#include "coeur/silvatheque.h"

VueArbre::VueArbre(Silvatheque *silvatheque)
	: m_silvatheque(silvatheque)
{
	ajoute_propriete("Shape", danjo::TypePropriete::ENTIER, 1);
	ajoute_propriete("BaseSize", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("Scale", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("ScaleV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("ZScale", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("ZScaleV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("Levels", danjo::TypePropriete::ENTIER, 1);
	ajoute_propriete("Ratio", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("RatioPower", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("Lobes", danjo::TypePropriete::ENTIER, 1);
	ajoute_propriete("LobeDepth", danjo::TypePropriete::DECIMAL, 1);
	ajoute_propriete("Flare", danjo::TypePropriete::DECIMAL, 1.0f);

	ajoute_propriete("_0Scale", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0ScaleV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0Length", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0LengthV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0Taper", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0BaseSplits", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0SegSplits", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0SplitAngle", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0SplitAngleV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0CurveRes", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0Curve", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0CurveBack", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_0CurveV", danjo::TypePropriete::DECIMAL, 1.0f);

	ajoute_propriete("_1DownAngle", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1DownAngleV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1Rotate", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1RotateV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1Branches", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1Length", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1LengthV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1Taper", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1SegSplits", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1SplitAngle", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1SplitAngleV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1CurveRes", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1Curve", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1CurveBack", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_1CurveV", danjo::TypePropriete::DECIMAL, 1.0f);

	ajoute_propriete("_2DownAngle", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2DownAngleV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2Rotate", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2RotateV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2Branches", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2Length", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2LengthV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2Taper", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2SegSplits", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2SplitAngle", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2SplitAngleV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2CurveRes", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2Curve", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2CurveBack", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_2CurveV", danjo::TypePropriete::DECIMAL, 1.0f);

	ajoute_propriete("_3DownAngle", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3DownAngleV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3Rotate", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3RotateV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3Branches", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3Length", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3LengthV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3Taper", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3SegSplits", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3SplitAngle", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3SplitAngleV", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3CurveRes", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3Curve", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3CurveBack", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("_3CurveV", danjo::TypePropriete::DECIMAL, 1.0f);

	ajoute_propriete("Leaves", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("LeafShape", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("LeafScale", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("LeafScaleX", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("AttractionUp", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("PruneRatio", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("PruneWidth", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("PruneWidthPeak", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("PrunePowerLow", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("PrunePowerHigh", danjo::TypePropriete::DECIMAL, 1.0f);
}

void VueArbre::ajourne_donnees()
{
	Parametres *parametres = m_silvatheque->arbre->parametres();
	parametres->Shape = evalue_entier("Shape");
	parametres->BaseSize = evalue_decimal("BaseSize");
	parametres->Scale = evalue_decimal("Scale");
	parametres->ScaleV = evalue_decimal("ScaleV");
	parametres->ZScale = evalue_decimal("ZScale");
	parametres->ZScaleV = evalue_decimal("ZScaleV");
	parametres->Levels = evalue_entier("Levels");
	parametres->Ratio = evalue_decimal("Ratio");
	parametres->RatioPower = evalue_decimal("RatioPower");
	parametres->Lobes = evalue_entier("Lobes");
	parametres->LobeDepth = evalue_decimal("LobeDepth");
	parametres->Flare = evalue_decimal("Flare");

	parametres->_0Scale = evalue_decimal("_0Scale");
	parametres->_0ScaleV = evalue_decimal("_0ScaleV");
	parametres->_0Length = evalue_decimal("_0Length");
	parametres->_0LengthV = evalue_decimal("_0LengthV");
	parametres->_0BaseSplits = evalue_decimal("_0BaseSplits");
	parametres->_0SegSplits = evalue_decimal("_0SegSplits");
	parametres->_0SplitAngle = evalue_decimal("_0SplitAngle");
	parametres->_0SplitAngleV = evalue_decimal("_0SplitAngleV");
	parametres->_0CurveRes = evalue_decimal("_0CurveRes");
	parametres->_0Curve = evalue_decimal("_0Curve");
	parametres->_0CurveBack = evalue_decimal("_0CurveBack");
	parametres->_0CurveV = evalue_decimal("_0CurveV");

	parametres->_1DownAngle = evalue_decimal("_1DownAngle");
	parametres->_1DownAngleV = evalue_decimal("_1DownAngleV");
	parametres->_1Rotate = evalue_decimal("_1Rotate");
	parametres->_1RotateV = evalue_decimal("_1RotateV");
	parametres->_1Branches = evalue_decimal("_1Branches");
	parametres->_1Length = evalue_decimal("_1Length");
	parametres->_1LengthV = evalue_decimal("_1LengthV");
	parametres->_1Taper = evalue_decimal("_1Taper");
	parametres->_1SegSplits = evalue_decimal("_1SegSplits");
	parametres->_1SplitAngle = evalue_decimal("_1SplitAngle");
	parametres->_1SplitAngleV = evalue_decimal("_1SplitAngleV");
	parametres->_1CurveRes = evalue_decimal("_1CurveRes");
	parametres->_1Curve = evalue_decimal("_1Curve");
	parametres->_1CurveBack = evalue_decimal("_1CurveBack");
	parametres->_1CurveV = evalue_decimal("_1CurveV");

	parametres->_2DownAngle = evalue_decimal("_2DownAngle");
	parametres->_2DownAngleV = evalue_decimal("_2DownAngleV");
	parametres->_2Rotate = evalue_decimal("_2Rotate");
	parametres->_2RotateV = evalue_decimal("_2RotateV");
	parametres->_2Branches = evalue_decimal("_2Branches");
	parametres->_2Length = evalue_decimal("_2Length");
	parametres->_2LengthV = evalue_decimal("_2LengthV");
	parametres->_2Taper = evalue_decimal("_2Taper");
	parametres->_2SegSplits = evalue_decimal("_2SegSplits");
	parametres->_2SplitAngle = evalue_decimal("_2SplitAngle");
	parametres->_2SplitAngleV = evalue_decimal("_2SplitAngleV");
	parametres->_2CurveRes = evalue_decimal("_2CurveRes");
	parametres->_2Curve = evalue_decimal("_2Curve");
	parametres->_2CurveBack = evalue_decimal("_2CurveBack");
	parametres->_2CurveV = evalue_decimal("_2CurveV");

	parametres->_3DownAngle = evalue_decimal("_3DownAngle");
	parametres->_3DownAngleV = evalue_decimal("_3DownAngleV");
	parametres->_3Rotate = evalue_decimal("_3Rotate");
	parametres->_3RotateV = evalue_decimal("_3RotateV");
	parametres->_3Branches = evalue_decimal("_3Branches");
	parametres->_3Length = evalue_decimal("_3Length");
	parametres->_3LengthV = evalue_decimal("_3LengthV");
	parametres->_3Taper = evalue_decimal("_3Taper");
	parametres->_3SegSplits = evalue_decimal("_3SegSplits");
	parametres->_3SplitAngle = evalue_decimal("_3SplitAngle");
	parametres->_3SplitAngleV = evalue_decimal("_3SplitAngleV");
	parametres->_3CurveRes = evalue_decimal("_3CurveRes");
	parametres->_3Curve = evalue_decimal("_3Curve");
	parametres->_3CurveBack = evalue_decimal("_3CurveBack");
	parametres->_3CurveV = evalue_decimal("_3CurveV");

	parametres->Leaves = evalue_decimal("Leaves");
	parametres->LeafShape = evalue_decimal("LeafShape");
	parametres->LeafScale = evalue_decimal("LeafScale");
	parametres->LeafScaleX = evalue_decimal("LeafScaleX");
	parametres->AttractionUp = evalue_decimal("AttractionUp");
	parametres->PruneRatio = evalue_decimal("PruneRatio");
	parametres->PruneWidth = evalue_decimal("PruneWidth");
	parametres->PruneWidthPeak = evalue_decimal("PruneWidthPeak");
	parametres->PrunePowerLow = evalue_decimal("PrunePowerLow");
	parametres->PrunePowerHigh = evalue_decimal("PrunePowerHigh");
}

bool VueArbre::ajourne_proprietes()
{
	Parametres *parametres = m_silvatheque->arbre->parametres();
	valeur_entier("Shape", parametres->Shape);
	valeur_decimal("BaseSize", parametres->BaseSize);
	valeur_decimal("Scale", parametres->Scale);
	valeur_decimal("ScaleV", parametres->ScaleV);
	valeur_decimal("ZScale", parametres->ZScale);
	valeur_decimal("ZScaleV", parametres->ZScaleV);
	valeur_entier("Levels", parametres->Levels);
	valeur_decimal("Ratio", parametres->Ratio);
	valeur_decimal("RatioPower", parametres->RatioPower);
	valeur_entier("Lobes", parametres->Lobes);
	valeur_decimal("LobeDepth", parametres->LobeDepth);
	valeur_decimal("Flare", parametres->Flare);

	valeur_decimal("_0Scale", parametres->_0Scale);
	valeur_decimal("_0ScaleV", parametres->_0ScaleV);
	valeur_decimal("_0Length", parametres->_0Length);
	valeur_decimal("_0LengthV", parametres->_0LengthV);
	valeur_decimal("_0BaseSplits", parametres->_0BaseSplits);
	valeur_decimal("_0SegSplits", parametres->_0SegSplits);
	valeur_decimal("_0SplitAngle", parametres->_0SplitAngle);
	valeur_decimal("_0SplitAngleV", parametres->_0SplitAngleV);
	valeur_decimal("_0CurveRes", parametres->_0CurveRes);
	valeur_decimal("_0Curve", parametres->_0Curve);
	valeur_decimal("_0CurveBack", parametres->_0CurveBack);
	valeur_decimal("_0CurveV", parametres->_0CurveV);

	valeur_decimal("_1DownAngle", parametres->_1DownAngle);
	valeur_decimal("_1DownAngleV", parametres->_1DownAngleV);
	valeur_decimal("_1Rotate", parametres->_1Rotate);
	valeur_decimal("_1RotateV", parametres->_1RotateV);
	valeur_decimal("_1Branches", parametres->_1Branches);
	valeur_decimal("_1Length", parametres->_1Length);
	valeur_decimal("_1LengthV", parametres->_1LengthV);
	valeur_decimal("_1Taper", parametres->_1Taper);
	valeur_decimal("_1SegSplits", parametres->_1SegSplits);
	valeur_decimal("_1SplitAngle", parametres->_1SplitAngle);
	valeur_decimal("_1SplitAngleV", parametres->_1SplitAngleV);
	valeur_decimal("_1CurveRes", parametres->_1CurveRes);
	valeur_decimal("_1Curve", parametres->_1Curve);
	valeur_decimal("_1CurveBack", parametres->_1CurveBack);
	valeur_decimal("_1CurveV", parametres->_1CurveV);

	valeur_decimal("_2DownAngle", parametres->_2DownAngle);
	valeur_decimal("_2DownAngleV", parametres->_2DownAngleV);
	valeur_decimal("_2Rotate", parametres->_2Rotate);
	valeur_decimal("_2RotateV", parametres->_2RotateV);
	valeur_decimal("_2Branches", parametres->_2Branches);
	valeur_decimal("_2Length", parametres->_2Length);
	valeur_decimal("_2LengthV", parametres->_2LengthV);
	valeur_decimal("_2Taper", parametres->_2Taper);
	valeur_decimal("_2SegSplits", parametres->_2SegSplits);
	valeur_decimal("_2SplitAngle", parametres->_2SplitAngle);
	valeur_decimal("_2SplitAngleV", parametres->_2SplitAngleV);
	valeur_decimal("_2CurveRes", parametres->_2CurveRes);
	valeur_decimal("_2Curve", parametres->_2Curve);
	valeur_decimal("_2CurveBack", parametres->_2CurveBack);
	valeur_decimal("_2CurveV", parametres->_2CurveV);

	valeur_decimal("_3DownAngle", parametres->_3DownAngle);
	valeur_decimal("_3DownAngleV", parametres->_3DownAngleV);
	valeur_decimal("_3Rotate", parametres->_3Rotate);
	valeur_decimal("_3RotateV", parametres->_3RotateV);
	valeur_decimal("_3Branches", parametres->_3Branches);
	valeur_decimal("_3Length", parametres->_3Length);
	valeur_decimal("_3LengthV", parametres->_3LengthV);
	valeur_decimal("_3Taper", parametres->_3Taper);
	valeur_decimal("_3SegSplits", parametres->_3SegSplits);
	valeur_decimal("_3SplitAngle", parametres->_3SplitAngle);
	valeur_decimal("_3SplitAngleV", parametres->_3SplitAngleV);
	valeur_decimal("_3CurveRes", parametres->_3CurveRes);
	valeur_decimal("_3Curve", parametres->_3Curve);
	valeur_decimal("_3CurveBack", parametres->_3CurveBack);
	valeur_decimal("_3CurveV", parametres->_3CurveV);

	valeur_decimal("Leaves", parametres->Leaves);
	valeur_decimal("LeafShape", parametres->LeafShape);
	valeur_decimal("LeafScale", parametres->LeafScale);
	valeur_decimal("LeafScaleX", parametres->LeafScaleX);
	valeur_decimal("AttractionUp", parametres->AttractionUp);
	valeur_decimal("PruneRatio", parametres->PruneRatio);
	valeur_decimal("PruneWidth", parametres->PruneWidth);
	valeur_decimal("PruneWidthPeak", parametres->PruneWidthPeak);
	valeur_decimal("PrunePowerLow", parametres->PrunePowerLow);
	valeur_decimal("PrunePowerHigh", parametres->PrunePowerHigh);

	return true;
}

EditeurArbre::EditeurArbre(Silvatheque *silvatheque, QWidget *parent)
	: BaseEditrice(*silvatheque, parent)
	, m_vue(new VueArbre(silvatheque))
	, m_widget(new QWidget())
	, m_conteneur_disposition(new QWidget())
	, m_scroll(new QScrollArea())
	, m_glayout(new QVBoxLayout(m_widget))
{
	m_widget->setSizePolicy(m_cadre->sizePolicy());

	m_scroll->setWidget(m_widget);
	m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scroll->setWidgetResizable(true);

	/* Hide scroll area's frame. */
	m_scroll->setFrameStyle(0);

	m_agencement_principal->addWidget(m_scroll);

	m_glayout->addWidget(m_conteneur_disposition);
}

EditeurArbre::~EditeurArbre()
{
	delete m_vue;
}

void EditeurArbre::ajourne_etat(int evenement)
{
	if (evenement != type_evenement::rafraichissement) {
		return;
	}

	m_vue->ajourne_proprietes();

	danjo::DonneesInterface donnees;
	donnees.conteneur = this;
	donnees.manipulable = m_vue;
	donnees.repondant_bouton = nullptr;

	const auto contenu_fichier = danjo::contenu_fichier("scripts/arbre.jo");
	auto disposition = danjo::compile_entreface(donnees, contenu_fichier.c_str());

	if (m_conteneur_disposition->layout()) {
		QWidget tmp;
		tmp.setLayout(m_conteneur_disposition->layout());
	}

	m_conteneur_disposition->setLayout(disposition);
}

void EditeurArbre::ajourne_manipulable()
{
	m_vue->ajourne_donnees();
	cree_arbre(m_silvatheque->arbre);
	m_silvatheque->notifie_auditeurs(type_evenement::arbre | type_evenement::modifie);
}
