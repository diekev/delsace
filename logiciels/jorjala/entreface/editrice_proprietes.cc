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

#include "danjo/compilation/assembleuse_disposition.h"
#include "danjo/controles_proprietes/donnees_controle.h"
#include "danjo/danjo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#pragma GCC diagnostic pop

#include "biblinternes/patrons_conception/repondant_commande.h"

#include "coeur/evenement.h"
#include "coeur/jorjala.hh"

/* ------------------------------------------------------------------------- */

std::optional<danjo::TypePropriete> type_propriété_danjo(JJL::TypeParametre type_param)
{
    switch (type_param) {
        case JJL::TypeParametre::CHAINE:
            return danjo::TypePropriete::CHAINE_CARACTERE;
        case JJL::TypeParametre::CHEMIN_FICHIER:
            return danjo::TypePropriete::FICHIER_ENTREE;
        case JJL::TypeParametre::NOMBRE_ENTIER:
            return danjo::TypePropriete::ENTIER;
        case JJL::TypeParametre::NOMBRE_RÉEL:
            return danjo::TypePropriete::DECIMAL;
        case JJL::TypeParametre::VALEUR_BOOLÉENNE:
            return danjo::TypePropriete::BOOL;
        case JJL::TypeParametre::VEC3:
            return danjo::TypePropriete::VECTEUR;
        default:  // À FAIRE
        case JJL::TypeParametre::VEC2:
        case JJL::TypeParametre::CORPS:
            return {};
    }
}

class EnveloppeParametre : public danjo::BasePropriete {
    mutable JJL::Noeud m_noeud;
    mutable JJL::TableParametres_Parametre m_param;

  public:
    EnveloppeParametre(JJL::Noeud noeud, JJL::TableParametres_Parametre param)
        : m_noeud(noeud), m_param(param){};

    danjo::TypePropriete type() const override
    {
        return type_propriété_danjo(m_param.type()).value();
    }

    /* Définit si la propriété est ajoutée par l'utilisateur. */
    bool est_extra() const override
    {
        // À FAIRE
        return false;
    }

    /* Définit si la propriété est visible. */
    void definit_visibilité(bool /*ouinon*/) override
    {
        // À FAIRE
    }
    bool est_visible() const override
    {
        return m_param.est_actif();
    }

    std::string donnne_infobulle() const override
    {
        // À FAIRE
        return "";
    }

    /* Évaluation des valeurs. */
    bool evalue_bool(int temps) const override
    {
        // À FAIRE: animation
        return m_param.lis_valeur_bool();
    }
    int evalue_entier(int temps) const override
    {
        // À FAIRE: animation
        return m_param.lis_valeur_entier();
    }
    float evalue_decimal(int temps) const override
    {
        // À FAIRE: animation
        return m_param.lis_valeur_réel();
    }
    dls::math::vec3f evalue_vecteur(int temps) const override
    {
        // À FAIRE: animation
        auto résultat = m_param.lis_valeur_vec3();
        return dls::math::vec3f{résultat.x(), résultat.y(), résultat.z()};
    }
    dls::phys::couleur32 evalue_couleur(int temps) const override
    {
        // À FAIRE
        return dls::phys::couleur32();
    }
    std::string evalue_chaine(int /*temps*/) const override
    {
        return m_param.lis_valeur_chaine().vers_std_string();
    }

    /* Définition des valeurs. */
    void définit_valeur_entier(int valeur) override
    {
        m_noeud.définit_param_entier(m_param, valeur);
    }
    void définit_valeur_décimal(float valeur) override
    {
        m_noeud.définit_param_réel(m_param, valeur);
    }
    void définit_valeur_bool(bool valeur) override
    {
        m_noeud.définit_param_bool(m_param, valeur);
    }
    void définit_valeur_vec3(dls::math::vec3f valeur) override
    {
        JJL::Vec3 résultat({});
        résultat.x(valeur.x);
        résultat.y(valeur.y);
        résultat.z(valeur.z);
        m_noeud.définit_param_vec3(m_param, résultat);
    }
    void définit_valeur_couleur(dls::phys::couleur32 valeur) override
    {
        // À FAIRE
    }
    void définit_valeur_chaine(std::string const &valeur) override
    {
        m_noeud.définit_param_chaine(m_param, valeur.c_str());
    }

    /* Plage des valeurs. */
    danjo::plage_valeur<float> plage_valeur_decimal() const override
    {
        // À FAIRE
        return {-1.0f, 1.0f};
    }
    danjo::plage_valeur<int> plage_valeur_entier() const override
    {
        // À FAIRE
        return {-10000, 10000};
    }
    danjo::plage_valeur<float> plage_valeur_vecteur() const override
    {
        // À FAIRE
        return {-1.0f, 1.0f};
    }
    danjo::plage_valeur<float> plage_valeur_couleur() const override
    {
        // À FAIRE
        return {-1.0f, 1.0f};
    }

    /* Animation des valeurs. */
    void ajoute_cle(const int v, int temps) override
    {
        // À FAIRE
    }
    void ajoute_cle(const float v, int temps) override
    {
        // À FAIRE
    }
    void ajoute_cle(const dls::math::vec3f &v, int temps) override
    {
        // À FAIRE
    }
    void ajoute_cle(const dls::phys::couleur32 &v, int temps) override
    {
        // À FAIRE
    }

    void supprime_animation() override
    {
        // À FAIRE
    }

    bool est_animee() const override
    {
        // À FAIRE
        return false;
    }
    bool est_animable() const override
    {
        // À FAIRE
        return false;
    }

    bool possede_cle(int temps) const override
    {
        // À FAIRE
        return false;
    }
};

static QBoxLayout *crée_disposition_paramètres(danjo::Manipulable *manipulable,
                                               danjo::RepondantBouton *repondant_bouton,
                                               danjo::ConteneurControles *conteneur,
                                               JJL::Noeud &noeud)
{
    auto table = noeud.table_paramètres();
    if (table == nullptr) {
        return nullptr;
    }

    danjo::AssembleurDisposition assembleuse(manipulable, repondant_bouton, conteneur);

    /* Ajout d'une disposition par défaut. */
    assembleuse.ajoute_disposition(danjo::id_morceau::COLONNE);

    for (auto param : table.paramètres()) {
        auto type = type_propriété_danjo(param.type());

        if (!type.has_value()) {
            continue;
        }

        assembleuse.ajoute_disposition(danjo::id_morceau::LIGNE);

        dls::chaine nom(param.nom().vers_std_string());
        assembleuse.ajoute_étiquette(nom);

        auto prop = memoire::loge<EnveloppeParametre>("EnveloppeParametre", noeud, param);
        manipulable->ajoute_propriete(nom, prop);

        danjo::DonneesControle donnees_controle;
        donnees_controle.nom = nom;
        assembleuse.ajoute_controle_pour_propriété(donnees_controle, prop);

        assembleuse.sors_disposition();

        if (noeud.paramètre_est_erroné(param.nom())) {
            auto erreur_param = noeud.erreur_pour_paramètre(param.nom());

            // À FAIRE : icone, stylisation du paramètre
            assembleuse.ajoute_disposition(danjo::id_morceau::LIGNE);
            assembleuse.ajoute_étiquette(erreur_param.vers_std_string());
            assembleuse.sors_disposition();
        }
    }

    return assembleuse.disposition();
}

/* ------------------------------------------------------------------------- */

EditriceProprietes::EditriceProprietes(JJL::Jorjala &jorjala, QWidget *parent)
    : BaseEditrice("propriétés", jorjala, parent), m_widget(new QWidget()),
      m_conteneur_avertissements(new QWidget()), m_conteneur_disposition(new QWidget()),
      m_scroll(new QScrollArea()), m_disposition_widget(new QVBoxLayout(m_widget))
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
        if (m_jorjala.animation_en_cours()) {
            return;
        }
    }

    /* ajourne l'entreface d'avertissement */
    auto creation_avert = (evenement == (type_evenement::image | type_evenement::traite));

    if (!(creation | creation_avert)) {
        return;
    }

    reinitialise_entreface(creation_avert);

    auto graphe = m_jorjala.graphe();
    if (graphe == nullptr) {
        return;
    }

    auto noeud = graphe.noeud_actif();
    if (noeud == nullptr) {
        return;
    }

    /* Rafraichis les avertissements. */
    ajoute_avertissements(noeud);

    /* l'évènement a peut-être été lancé depuis cet éditeur, supprimer
     * l'entreface de controles crashera le logiciel car nous sommes dans la
     * méthode du bouton ou controle à l'origine de l'évènement, donc nous ne
     * rafraichissement que les avertissements. */
    if (creation_avert) {
        return;
    }

    danjo::Manipulable manipulable;
    auto repondant = repondant_commande(m_jorjala);

    auto disposition = crée_disposition_paramètres(&manipulable, repondant, this, noeud);
    if (disposition == nullptr) {
        return;
    }

    // À FAIRE
    // gestionnaire->ajourne_entreface(manipulable);
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

void EditriceProprietes::ajoute_avertissements(JJL::Noeud &noeud)
{
    m_conteneur_avertissements->hide();

    auto disposition_avertissements = new QGridLayout();
    auto ligne = 0;
    auto const &pixmap = QPixmap("icones/icone_avertissement.png");

    for (auto erreur : noeud.erreurs()) {
        if (erreur.type() == JJL::TypeErreurNoeud::PARAMÉTRIQUE) {
            continue;
        }

        auto icone = new QLabel();
        icone->setPixmap(pixmap);

        auto texte = new QLabel(erreur.message().vers_std_string().c_str());

        disposition_avertissements->addWidget(icone, ligne, 0, Qt::AlignRight);
        disposition_avertissements->addWidget(texte, ligne, 1);

        ++ligne;
    }

    m_conteneur_avertissements->setLayout(disposition_avertissements);
    m_conteneur_avertissements->show();
}

void EditriceProprietes::ajourne_manipulable()
{
    auto graphe = m_jorjala.graphe();
    if (graphe == nullptr) {
        return;
    }

    auto requête = JJL::RequeteEvaluation({});
    requête.raison(JJL::RaisonEvaluation::PARAMETRE_CHANGÉ);
    requête.graphe(graphe);
    // À FAIRE : message ?
    // requête.message("réponse modification propriété manipulable");

    m_jorjala.requiers_évaluation(requête);
}

void EditriceProprietes::precontrole_change()
{
#if 0
	std::cerr << "---- Précontrole changé !\n";
	m_jorjala.empile_etat();
#endif
}

void EditriceProprietes::obtiens_liste(dls::chaine const &attache,
                                       dls::tableau<dls::chaine> &chaines)
{
#if 0
	auto graphe = m_jorjala.graphe;
	auto noeud = graphe->noeud_actif;

	if (noeud == nullptr || noeud->type != type_noeud::OPERATRICE) {
		return;
	}

	auto operatrice = extrait_opimage(noeud->donnees);
	auto contexte = cree_contexte_evaluation(m_jorjala);

	operatrice->obtiens_liste(contexte, attache, chaines);
#endif
}

void EditriceProprietes::onglet_dossier_change(int index)
{
#if 0
	auto graphe = m_jorjala.graphe;
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
#endif
}
