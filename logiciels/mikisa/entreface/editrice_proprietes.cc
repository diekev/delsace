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

#include "editrice_proprietes.h"

#include "danjo/danjo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "biblinternes/outils/fichier.hh"

#include "evaluation/evaluation.hh"

#include "coeur/composite.h"
#include "coeur/contexte_evaluation.hh"
#include "coeur/evenement.h"
#include "coeur/operatrice_graphe_detail.hh"
#include "coeur/objet.h"
#include "coeur/mikisa.h"
#include "coeur/noeud_image.h"
#include "coeur/nuanceur.hh"
#include "coeur/operatrice_image.h"

EditriceProprietes::EditriceProprietes(Mikisa &mikisa, QWidget *parent)
	: BaseEditrice(mikisa, parent)
    , m_widget(new QWidget())
	, m_conteneur_avertissements(new QWidget())
	, m_conteneur_disposition(new QWidget())
    , m_scroll(new QScrollArea())
	, m_disposition_widget(new QVBoxLayout(m_widget))
{
	m_widget->setSizePolicy(m_frame->sizePolicy());

	m_scroll->setWidget(m_widget);
	m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scroll->setWidgetResizable(true);

	/* Cache le cadre du scroll area. */
	m_scroll->setFrameStyle(0);

	m_main_layout->addWidget(m_scroll);

	m_disposition_widget->addWidget(m_conteneur_avertissements);
	m_disposition_widget->addWidget(m_conteneur_disposition);
}

void EditriceProprietes::ajourne_etat(int evenement)
{
	auto creation = (evenement == (type_evenement::noeud | type_evenement::selectionne));
	creation |= (evenement == (type_evenement::noeud | type_evenement::ajoute));
	creation |= (evenement == (type_evenement::noeud | type_evenement::enleve));
	creation |= (evenement == (type_evenement::temps | type_evenement::modifie));
	creation |= (evenement == (type_evenement::propriete | type_evenement::ajoute));
	creation |= (evenement == (type_evenement::objet | type_evenement::manipule));
	creation |= (evenement == (type_evenement::rafraichissement));

	/* n'ajourne pas durant les animation */
	if (evenement == (type_evenement::temps | type_evenement::modifie)) {
		if (m_mikisa.animation) {
			return;
		}
	}

	/* ajourne l'entreface d'avertissement */
	auto creation_avert = (evenement == (type_evenement::image | type_evenement::traite));

	if (!(creation | creation_avert)) {
		return;
	}

	reinitialise_entreface(creation_avert);

	auto graphe = m_mikisa.graphe;
	auto noeud = graphe->noeud_actif;

	if (noeud == nullptr) {
		return;
	}

	auto manipulable = static_cast<danjo::Manipulable *>(nullptr);
	auto chemin_entreface = "";

	switch (noeud->type) {
		case type_noeud::COMPOSITE:
		{
			/* RÀF */
			break;
		}
		case type_noeud::INVALIDE:
		{
			/* RÀF */
			break;
		}
		case type_noeud::NUANCEUR:
		{
			/* RÀF */
			break;
		}
		case type_noeud::OBJET:
		{
			auto objet = extrait_objet(noeud->donnees);
			chemin_entreface = objet->chemin_entreface();
			manipulable = objet->noeud;
			break;
		}
		case type_noeud::OPERATRICE:
		{
			auto operatrice = extrait_opimage(noeud->donnees);
			chemin_entreface = operatrice->chemin_entreface();
			manipulable = operatrice;

			operatrice->ajourne_proprietes();

			/* avertissements */
			if (operatrice->avertissements().taille() > 0) {
				auto disposition_avertissements = new QGridLayout();
				auto ligne = 0;
				auto const &pixmap = QPixmap("icones/icone_avertissement.png");

				for (auto const &avertissement : operatrice->avertissements()) {
					auto icone = new QLabel();
					icone->setPixmap(pixmap);

					auto texte = new QLabel(avertissement.c_str());

					disposition_avertissements->addWidget(icone, ligne, 0, Qt::AlignRight);
					disposition_avertissements->addWidget(texte, ligne, 1);

					++ligne;
				}

				m_conteneur_avertissements->setLayout(disposition_avertissements);
				m_conteneur_avertissements->show();
			}
			else {
				m_conteneur_avertissements->hide();
			}

			break;
		}
		case type_noeud::RENDU:
		{
			/* RÀF */
			break;
		}
	}

	/* l'évènement a peut-être été lancé depuis cet éditeur, supprimer
	 * l'entreface de controles crashera le logiciel car nous sommes dans la
	 * méthode du bouton ou controle à l'origine de l'évènement, donc nous ne
	 * rafraichissement que les avertissements. */
	if (creation_avert) {
		return;
	}

	auto const &texte = dls::contenu_fichier(chemin_entreface);

	danjo::DonneesInterface donnees{};
	donnees.manipulable = manipulable;
	donnees.conteneur = this;

	auto disposition = m_mikisa.gestionnaire_entreface->compile_entreface(donnees, texte.c_str(), m_mikisa.temps_courant);
	m_conteneur_disposition->setLayout(disposition);
}

void EditriceProprietes::reinitialise_entreface(bool creation_avert)
{
	/* Qt ne permet d'extrait la disposition d'un widget que si celle-ci est
	 * assignée à un autre widget. Donc pour détruire la disposition précédente
	 * nous la reparentons à un widget temporaire qui la détruira dans son
	 * destructeur. */

	if (m_conteneur_avertissements->layout()) {
		QWidget temp;
		temp.setLayout(m_conteneur_avertissements->layout());
	}

	if (!creation_avert && m_conteneur_disposition->layout()) {
		QWidget temp;
		temp.setLayout(m_conteneur_disposition->layout());
	}
}

void EditriceProprietes::ajourne_manipulable()
{
	std::cerr << "Controle changé !\n";
	auto graphe = m_mikisa.graphe;
	auto noeud = graphe->noeud_actif;

	if (noeud == nullptr) {
		return;
	}

	switch (noeud->type) {
		case type_noeud::INVALIDE:
		{
			break;
		}
		case type_noeud::OBJET:
		{
			auto objet = extrait_objet(noeud->donnees);
			objet->ajourne_parametres();
			break;
		}
		case type_noeud::COMPOSITE:
		{
			break;
		}
		case type_noeud::NUANCEUR:
		{
			auto nuanceur = extrait_nuanceur(noeud->donnees);
			nuanceur->temps_modifie += 1;
			break;
		}
		case type_noeud::RENDU:
		{
			break;
		}
		case type_noeud::OPERATRICE:
		{
			/* Marque le noeud courant et ceux en son aval surannées. */
			marque_surannee(noeud, [](Noeud *n, PriseEntree *prise)
			{
				auto op = extrait_opimage(n->donnees);
				op->amont_change(prise);
			});

			if (noeud->parent->type == type_noeud::NUANCEUR) {
				auto nuanceur = extrait_nuanceur(noeud->parent->donnees);
				nuanceur->temps_modifie += 1;
			}

			/* Notifie les graphes des noeuds parents comme étant surrannés */
			marque_parent_surannee(noeud->parent, [](Noeud *n, PriseEntree *prise)
			{
				if (n->type != type_noeud::OPERATRICE) {
					return;
				}

				auto op = extrait_opimage(n->donnees);
				op->amont_change(prise);
			});

			auto op = extrait_opimage(noeud->donnees);
			op->parametres_changes();
			break;
		}
	}

	requiers_evaluation(m_mikisa, PARAMETRE_CHANGE, "réponse modification propriété manipulable");
}

void EditriceProprietes::precontrole_change()
{
	std::cerr << "---- Précontrole changé !\n";
	m_mikisa.empile_etat();
}

void EditriceProprietes::obtiens_liste(
		dls::chaine const &attache,
		dls::tableau<dls::chaine> &chaines)
{
	auto graphe = m_mikisa.graphe;
	auto noeud = graphe->noeud_actif;

	if (noeud == nullptr || noeud->type != type_noeud::OPERATRICE) {
		return;
	}

	auto operatrice = extrait_opimage(noeud->donnees);
	auto contexte = cree_contexte_evaluation(m_mikisa);

	operatrice->obtiens_liste(contexte, attache, chaines);
}

void EditriceProprietes::onglet_dossier_change(int index)
{
	auto graphe = m_mikisa.graphe;
	auto noeud = graphe->noeud_actif;

	if (noeud == nullptr) {
		return;
	}

	switch (noeud->type) {
		case type_noeud::COMPOSITE:
		case type_noeud::INVALIDE:
		case type_noeud::NUANCEUR:
		case type_noeud::OBJET:
		case type_noeud::RENDU:
		{
			noeud->onglet_courant = index;
			break;
		}
		case type_noeud::OPERATRICE:
		{
			auto op = extrait_opimage(noeud->donnees);
			op->onglet_courant = index;

			break;
		}
	}
}
