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

#include "controle_propriete_couleur.h"

#include <QHBoxLayout>

#include "compilation/morceaux.h"

#include "controles/controle_couleur.h"

#include "donnees_controle.h"

#include <sstream>
static std::vector<std::string> decoupe(const std::string &chaine, const char delimiteur)
{
	std::vector<std::string> resultat;
	std::stringstream ss(chaine);
	std::string temp;

	while (std::getline(ss, temp, delimiteur)) {
		resultat.push_back(temp);
	}

	return resultat;
}

namespace danjo {

ControleProprieteCouleur::ControleProprieteCouleur(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_controle_couleur(new ControleCouleur(this))
{
	m_agencement->addWidget(m_controle_couleur);

	connect(m_controle_couleur, &ControleCouleur::couleur_changee,
			this, &ControleProprieteCouleur::ajourne_couleur);
}

void ControleProprieteCouleur::finalise(const DonneesControle &donnees)
{
	auto min = 0.0f;
	if (donnees.valeur_min != "") {
		min = static_cast<float>(std::atof(donnees.valeur_min.c_str()));
	}

	auto max = 1.0f;
	if (donnees.valeur_max != "") {
		max = static_cast<float>(std::atof(donnees.valeur_max.c_str()));
	}

	m_controle_couleur->ajourne_plage(min, max);

	if (donnees.initialisation) {
		auto valeurs = decoupe(donnees.valeur_defaut, ',');
		auto index = 0;

		dls::phys::couleur32 valeur_defaut(1.0f);

		for (auto v : valeurs) {
			valeur_defaut[index++] = static_cast<float>(std::atof(v.c_str()));
		}

		m_propriete->valeur = valeur_defaut;
	}

	m_controle_couleur->couleur(std::experimental::any_cast<dls::phys::couleur32>(m_propriete->valeur));

	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteCouleur::ajourne_couleur()
{
	m_propriete->valeur = m_controle_couleur->couleur();
	Q_EMIT(controle_change());
}

}  /* namespace danjo */
