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

#include "controle_propriete_decimal.h"

#include <QHBoxLayout>

#include <sstream>

#include "controles/controle_nombre_decimal.h"

#include "donnees_controle.h"

namespace danjo {

/* Il s'emblerait que std::atof a du mal à convertir les string en float. */
template <typename T>
static T convertie(const std::string &valeur)
{
	std::istringstream ss(valeur);
	T result;

	ss >> result;

	return result;
}

ControleProprieteDecimal::ControleProprieteDecimal(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_controle(new ControleNombreDecimal(this))
	, m_pointeur(nullptr)
{
	connect(m_controle, &ControleNombreDecimal::valeur_changee, this, &ControleProprieteDecimal::ajourne_valeur_pointee);
}

void ControleProprieteDecimal::ajourne_valeur_pointee(float valeur)
{
	*m_pointeur = valeur;
	Q_EMIT(controle_change());
}

void ControleProprieteDecimal::finalise(const DonneesControle &donnees)
{
	auto min = -std::numeric_limits<float>::max();

	if (donnees.valeur_min != "") {
		min = convertie<float>(donnees.valeur_min.c_str());
	}

	auto max = std::numeric_limits<float>::max();

	if (donnees.valeur_max != "") {
		max = convertie<float>(donnees.valeur_max.c_str());
	}

	m_controle->ajourne_plage(min, max);

	m_pointeur = static_cast<float *>(donnees.pointeur);

	const auto valeur_defaut = convertie<float>(donnees.valeur_defaut);

	if (donnees.initialisation) {
		*m_pointeur = valeur_defaut;
	}

	m_controle->valeur(*m_pointeur);

	setToolTip(donnees.infobulle.c_str());
}

}  /* namespace danjo */
