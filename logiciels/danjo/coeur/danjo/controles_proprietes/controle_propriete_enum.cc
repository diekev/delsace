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

#include "controle_propriete_enum.h"

#include <QComboBox>
#include <QHBoxLayout>

#include "donnees_controle.h"

namespace danjo {

ControleProprieteEnum::ControleProprieteEnum(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout)
	, m_liste_deroulante(new QComboBox(this))
	, m_pointeur(nullptr)
	, m_index_valeur_defaut(0)
	, m_index_courant(0)
{
	m_agencement->addWidget(m_liste_deroulante);
	this->setLayout(m_agencement);

	/* On ne peut pas utilisé le nouveau style de connection de Qt5 :
	 *     connect(m_liste_deroulante, &QComboBox::currentIndexChanged,
	 *             this, &ControleEnum::ajourne_valeur_pointee);
	 * car currentIndexChanged a deux signatures possibles et le compileur
	 * ne sait pas laquelle choisir. */
	connect(m_liste_deroulante, SIGNAL(currentIndexChanged(int)),
			this, SLOT(ajourne_valeur_pointee(int)));
}

void ControleProprieteEnum::ajourne_valeur_pointee(int /*valeur*/)
{
	Q_EMIT(precontrole_change());
	*m_pointeur = m_liste_deroulante->currentData().toString().toStdString();
	Q_EMIT(controle_change());
}

void ControleProprieteEnum::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<dls::chaine *>(donnees.pointeur);

	auto valeur_defaut = donnees.valeur_defaut;

	if (donnees.initialisation) {
		*m_pointeur = valeur_defaut;
	}
	else {
		valeur_defaut = *m_pointeur;
	}

	const auto vieil_etat = m_liste_deroulante->blockSignals(true);

	auto index_courant = 0;

	for (const auto &pair : donnees.valeur_enum) {
		m_liste_deroulante->addItem(pair.first.c_str(),
									QVariant(pair.second.c_str()));

		if (pair.second == valeur_defaut) {
			m_index_valeur_defaut = index_courant;
		}

		index_courant++;
	}

	m_liste_deroulante->setCurrentIndex(m_index_valeur_defaut);

	m_liste_deroulante->blockSignals(vieil_etat);

	setToolTip(donnees.infobulle.c_str());
}

}  /* namespace danjo */
