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

#include "assembleuse_disposition.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>

#include "biblinternes/outils/chaine.hh"

#include "controles/action.h"
#include "controles/bouton.h"
#include "controles/menu_filtrable.hh"

#include "controles_proprietes/controle_propriete_bool.h"
#include "controles_proprietes/controle_propriete_chaine.h"
#include "controles_proprietes/controle_propriete_couleur.h"
#include "controles_proprietes/controle_propriete_courbe_couleur.h"
#include "controles_proprietes/controle_propriete_courbe_valeur.h"
#include "controles_proprietes/controle_propriete_decimal.h"
#include "controles_proprietes/controle_propriete_entier.h"
#include "controles_proprietes/controle_propriete_enum.h"
#include "controles_proprietes/controle_propriete_etiquette.h"
#include "controles_proprietes/controle_propriete_fichier.h"
#include "controles_proprietes/controle_propriete_liste.h"
#include "controles_proprietes/controle_propriete_liste_manip.hh"
#include "controles_proprietes/controle_propriete_rampe_couleur.h"
#include "controles_proprietes/controle_propriete_vecteur.h"

#include "conteneur_controles.h"
#include "manipulable.h"
#include "repondant_bouton.h"

namespace danjo {

static ControlePropriete *cree_controle_pour_propriete(BasePropriete *propriete, int temps)
{
    switch (propriete->type()) {
        case TypePropriete::ENTIER:
            return new ControleProprieteEntier(propriete, temps);
        case TypePropriete::DECIMAL:
            return new ControleProprieteDecimal(propriete, temps);
        case TypePropriete::ENUM:
            return new ControleProprieteEnum(propriete, temps);
        case TypePropriete::LISTE:
            return new ControleProprieteListe(propriete, temps);
        case TypePropriete::BOOL:
            return new ControleProprieteBool(propriete, temps);
        case TypePropriete::CHAINE_CARACTERE:
            return new ControleProprieteChaineCaractere(propriete, temps);
        case TypePropriete::FICHIER_ENTREE:
            return new ControleProprieteFichier(propriete, temps, true);
        case TypePropriete::FICHIER_SORTIE:
            return new ControleProprieteFichier(propriete, temps, false);
        case TypePropriete::COULEUR:
            return new ControleProprieteCouleur(propriete, temps);
        case TypePropriete::VECTEUR_DECIMAL:
            return new ControleProprieteVecteurDecimal(propriete, temps);
        case TypePropriete::VECTEUR_ENTIER:
            return new ControleProprieteVecteurEntier(propriete, temps);
        case TypePropriete::COURBE_COULEUR:
            return new ControleProprieteCourbeCouleur(propriete, temps);
        case TypePropriete::COURBE_VALEUR:
            return new ControleProprieteCourbeValeur(propriete, temps);
        case TypePropriete::RAMPE_COULEUR:
            return new ControleProprieteRampeCouleur(propriete, temps);
        case TypePropriete::TEXTE:
            return new ControleProprieteEditeurTexte(propriete, temps);
        case TypePropriete::LISTE_MANIP:
            return new ControleProprieteListeManip(propriete, temps);
        default:
            return nullptr;
    }
}

static void connecte_propriété_au_conteneur(ControlePropriete *prop, ConteneurControles *conteneur)
{
    if (conteneur == nullptr) {
        return;
    }

    QObject::connect(prop,
                     &ControlePropriete::debute_changement_controle,
                     conteneur,
                     &ConteneurControles::debute_changement_controle);

    QObject::connect(prop,
                     &ControlePropriete::controle_change,
                     conteneur,
                     &ConteneurControles::ajourne_manipulable);

    QObject::connect(prop,
                     &ControlePropriete::termine_changement_controle,
                     conteneur,
                     &ConteneurControles::termine_changement_controle);
}

static ControlePropriete *crée_controle_propriété(DonneesControle const &donnees,
                                                  BasePropriete *prop,
                                                  int temps,
                                                  ConteneurControles *conteneur)
{
    auto résultat = cree_controle_pour_propriete(prop, temps);
    résultat->finalise(donnees);

    if (prop->type() == TypePropriete::LISTE) {
        static_cast<ControleProprieteListe *>(résultat)->conteneur(conteneur);
    }

    connecte_propriété_au_conteneur(résultat, conteneur);
    return résultat;
}

/* ------------------------------------------------------------------------- */
/** \name Maçonnes
 * \{ */

MaçonneDisposition::MaçonneDisposition(ContexteMaçonnage *ctx, MaçonneDisposition *parent)
    : m_ctx(ctx), m_parent(parent)
{
}

ControlePropriete *MaçonneDisposition::crée_controle_propriété(
    const DonneesControle &données_controle, BasePropriete *prop)
{
    m_ctx->manipulable->ajoute_propriete(données_controle.nom, prop);
    auto résultat = danjo::crée_controle_propriété(données_controle, prop, 0, m_ctx->conteneur);
    if (m_ctx->gestionnaire) {
        m_ctx->gestionnaire->ajoute_controle(données_controle.nom, résultat);
    }
    return résultat;
}

ControlePropriete *MaçonneDisposition::crée_étiquette(std::string_view nom)
{
    auto controle = new ControleProprieteEtiquette(
        QString::fromStdString(std::string(nom.data(), nom.size())), m_ctx->conteneur);
    return controle;
}

ControlePropriete *MaçonneDisposition::crée_étiquette_activable(std::string_view nom,
                                                                BasePropriete *prop)
{
    auto nom_prop = std::string(nom.data(), nom.size());
    auto controle = new ControleProprieteEtiquetteActivable(
        QString::fromStdString(nom_prop), prop, 0, m_ctx->conteneur);
    connecte_propriété_au_conteneur(controle, m_ctx->conteneur);
    /* Ne doit pas être ajoutée au gestionnaire car le controle ne doit jamais être désactivé. */
    return controle;
}

ControlePropriete *MaçonneDisposition::crée_étiquette_propriété(std::string_view nom,
                                                                BasePropriete *prop)
{
    auto nom_prop = std::string(nom.data(), nom.size());
    auto résultat = new ControleProprieteEtiquettePropriete(prop, 0, m_ctx->conteneur);
    if (m_ctx->gestionnaire) {
        m_ctx->gestionnaire->ajoute_controle(nom_prop, résultat);
    }
    return résultat;
}

MaçonneDispositionLigne *MaçonneDisposition::crée_maçonne_ligne()
{
    return new MaçonneDispositionLigne(m_ctx, this);
}

MaçonneDispositionColonne *MaçonneDisposition::crée_maçonne_colonne()
{
    return new MaçonneDispositionColonne(m_ctx, this);
}

MaçonneDispositionGrille *MaçonneDisposition::crée_maçonne_grille()
{
    return new MaçonneDispositionGrille(m_ctx, this);
}

MaçonneDispositionLigne::MaçonneDispositionLigne(ContexteMaçonnage *ctx,
                                                 MaçonneDisposition *parent)
    : MaçonneDisposition(ctx, parent)
{
    m_layout = new QHBoxLayout();
}

MaçonneDispositionLigne *MaçonneDispositionLigne::débute_ligne()
{
    auto résultat = crée_maçonne_ligne();
    m_layout->addLayout(résultat->donne_layout());
    return résultat;
}

MaçonneDispositionColonne *MaçonneDispositionLigne::débute_colonne()
{
    auto résultat = crée_maçonne_colonne();
    m_layout->addLayout(résultat->donne_layout());
    return résultat;
}

MaçonneDispositionGrille *MaçonneDispositionLigne::débute_grille()
{
    auto résultat = crée_maçonne_grille();
    m_layout->addLayout(résultat->donne_layout());
    return résultat;
}

void MaçonneDispositionLigne::ajoute_controle(DonneesControle const &données_controle,
                                              BasePropriete *prop)
{
    auto controle = crée_controle_propriété(données_controle, prop);
    m_layout->addWidget(controle);
}

void MaçonneDispositionLigne::ajoute_etiquette(std::string_view nom)
{
    auto étiquette = crée_étiquette(nom);
    m_layout->addWidget(étiquette);
}

void MaçonneDispositionLigne::ajoute_étiquette_activable(std::string_view nom, BasePropriete *prop)
{
    auto étiquette = crée_étiquette_activable(nom, prop);
    m_layout->addWidget(étiquette);
}

void MaçonneDispositionLigne::ajoute_étiquette_propriété(std::string_view nom, BasePropriete *prop)
{
    auto étiquette = crée_étiquette_propriété(nom, prop);
    m_layout->addWidget(étiquette);
}

void MaçonneDispositionLigne::ajoute_espaceur(int taille)
{
    auto spaceur = new QSpacerItem(taille, taille);
    m_layout->addItem(spaceur);
}

QLayout *MaçonneDispositionLigne::donne_layout()
{
    return m_layout;
}

MaçonneDispositionColonne::MaçonneDispositionColonne(ContexteMaçonnage *ctx,
                                                     MaçonneDisposition *parent)
    : MaçonneDisposition(ctx, parent)
{
    m_layout = new QVBoxLayout();
}

MaçonneDispositionLigne *MaçonneDispositionColonne::débute_ligne()
{
    auto résultat = crée_maçonne_ligne();
    m_layout->addLayout(résultat->donne_layout());
    return résultat;
}

MaçonneDispositionColonne *MaçonneDispositionColonne::débute_colonne()
{
    auto résultat = crée_maçonne_colonne();
    m_layout->addLayout(résultat->donne_layout());
    return résultat;
}

MaçonneDispositionGrille *MaçonneDispositionColonne::débute_grille()
{
    auto résultat = crée_maçonne_grille();
    m_layout->addLayout(résultat->donne_layout());
    return résultat;
}

void MaçonneDispositionColonne::ajoute_controle(DonneesControle const &données_controle,
                                                BasePropriete *prop)
{
    auto controle = crée_controle_propriété(données_controle, prop);
    m_layout->addWidget(controle);
}

void MaçonneDispositionColonne::ajoute_etiquette(std::string_view nom)
{
    auto étiquette = crée_étiquette(nom);
    m_layout->addWidget(étiquette);
}

void MaçonneDispositionColonne::ajoute_étiquette_activable(std::string_view nom,
                                                           BasePropriete *prop)
{
    auto étiquette = crée_étiquette_activable(nom, prop);
    m_layout->addWidget(étiquette);
}

void MaçonneDispositionColonne::ajoute_étiquette_propriété(std::string_view nom,
                                                           BasePropriete *prop)
{
    auto étiquette = crée_étiquette_propriété(nom, prop);
    m_layout->addWidget(étiquette);
}

void MaçonneDispositionColonne::ajoute_espaceur(int taille)
{
    auto spaceur = new QSpacerItem(taille, taille);
    m_layout->addItem(spaceur);
}

QLayout *MaçonneDispositionColonne::donne_layout()
{
    return m_layout;
}

MaçonneDispositionGrille::MaçonneDispositionGrille(ContexteMaçonnage *ctx,
                                                   MaçonneDisposition *parent)
    : MaçonneDisposition(ctx, parent)
{
    m_layout = new QGridLayout();
}

MaçonneDispositionLigne *MaçonneDispositionGrille::débute_ligne(int ligne,
                                                                int colonne,
                                                                int empan_ligne,
                                                                int empan_colonne)
{
    auto résultat = crée_maçonne_ligne();
    m_layout->addLayout(résultat->donne_layout(), ligne, colonne, empan_ligne, empan_colonne);
    return résultat;
}

MaçonneDispositionColonne *MaçonneDispositionGrille::débute_colonne(int ligne,
                                                                    int colonne,
                                                                    int empan_ligne,
                                                                    int empan_colonne)
{
    auto résultat = crée_maçonne_colonne();
    m_layout->addLayout(résultat->donne_layout(), ligne, colonne, empan_ligne, empan_colonne);
    return résultat;
}

MaçonneDispositionGrille *MaçonneDispositionGrille::débute_grille(int ligne,
                                                                  int colonne,
                                                                  int empan_ligne,
                                                                  int empan_colonne)
{
    auto résultat = crée_maçonne_grille();
    m_layout->addLayout(résultat->donne_layout(), ligne, colonne, empan_ligne, empan_colonne);
    return résultat;
}

void MaçonneDispositionGrille::ajoute_controle(DonneesControle const &données_controle,
                                               BasePropriete *prop,
                                               int ligne,
                                               int colonne,
                                               int empan_ligne,
                                               int empan_colonne)
{
    auto controle = crée_controle_propriété(données_controle, prop);
    m_layout->addWidget(controle, ligne, colonne, empan_ligne, empan_colonne);
}

void MaçonneDispositionGrille::ajoute_etiquette(
    std::string_view nom, int ligne, int colonne, int empan_ligne, int empan_colonne)
{
    auto étiquette = crée_étiquette(nom);
    m_layout->addWidget(étiquette, ligne, colonne, empan_ligne, empan_colonne);
}

void MaçonneDispositionGrille::ajoute_étiquette_activable(std::string_view nom,
                                                          BasePropriete *prop,
                                                          int ligne,
                                                          int colonne,
                                                          int empan_ligne,
                                                          int empan_colonne)
{
    auto étiquette = crée_étiquette_activable(nom, prop);
    m_layout->addWidget(étiquette, ligne, colonne, empan_ligne, empan_colonne);
}

void MaçonneDispositionGrille::ajoute_étiquette_propriété(std::string_view nom,
                                                          BasePropriete *prop,
                                                          int ligne,
                                                          int colonne,
                                                          int empan_ligne,
                                                          int empan_colonne)
{
    auto étiquette = crée_étiquette_propriété(nom, prop);
    m_layout->addWidget(étiquette, ligne, colonne, empan_ligne, empan_colonne);
}

void MaçonneDispositionGrille::ajoute_espaceur(
    int taille, int ligne, int colonne, int empan_ligne, int empan_colonne)
{
    auto spaceur = new QSpacerItem(taille, taille);
    m_layout->addItem(spaceur, ligne, colonne, empan_ligne, empan_colonne);
}

QLayout *MaçonneDispositionGrille::donne_layout()
{
    return m_layout;
}

/** \} */

static TypePropriete type_propriete_pour_lexeme(id_morceau lexeme)
{
    switch (lexeme) {
        case id_morceau::ENTIER:
            return TypePropriete::ENTIER;
        case id_morceau::DECIMAL:
            return TypePropriete::DECIMAL;
        case id_morceau::ENUM:
            return TypePropriete::ENUM;
        case id_morceau::LISTE:
            return TypePropriete::LISTE;
        case id_morceau::CASE:
            return TypePropriete::BOOL;
        case id_morceau::CHAINE:
            return TypePropriete::CHAINE_CARACTERE;
        case id_morceau::FICHIER_ENTREE:
            return TypePropriete::FICHIER_ENTREE;
        case id_morceau::FICHIER_SORTIE:
            return TypePropriete::FICHIER_SORTIE;
        case id_morceau::COULEUR:
            return TypePropriete::COULEUR;
        case id_morceau::VECTEUR:
            // À FAIRE : vecteur entier
            return TypePropriete::VECTEUR_DECIMAL;
        case id_morceau::COURBE_COULEUR:
            return TypePropriete::COURBE_COULEUR;
        case id_morceau::COURBE_VALEUR:
            return TypePropriete::COURBE_VALEUR;
        case id_morceau::RAMPE_COULEUR:
            return TypePropriete::RAMPE_COULEUR;
        case id_morceau::TEXTE:
            return TypePropriete::TEXTE;
        case id_morceau::LISTE_MANIP:
            return TypePropriete::LISTE_MANIP;
        default:
            break;
    }

    assert(false);
    return TypePropriete::ENTIER;
}

/* Il s'emblerait que std::atof a du mal à convertir les string en float. */
template <typename T>
static T parse_valeur_ou_defaut(const dls::chaine &valeur, T defaut)
{
    if (valeur == "") {
        return defaut;
    }

    std::istringstream ss(valeur.c_str());
    T result;

    ss >> result;

    return result;
}

static Propriete *crée_propriété(DonneesControle const &donnees)
{
    auto résultat = memoire::loge<Propriete>("danjo::Propriete");
    résultat->type_ = donnees.type;
    résultat->etat = donnees.etat;

    switch (donnees.type) {
        case TypePropriete::ENTIER:
        {
            auto min = parse_valeur_ou_defaut(donnees.valeur_min, std::numeric_limits<int>::min());
            auto max = parse_valeur_ou_defaut(donnees.valeur_min, std::numeric_limits<int>::max());
            auto valeur_defaut = parse_valeur_ou_defaut(donnees.valeur_defaut, 0);
            résultat->valeur_min.i = min;
            résultat->valeur_max.i = max;
            résultat->valeur = valeur_defaut;
            break;
        }
        case TypePropriete::DECIMAL:
        {
            auto min = parse_valeur_ou_defaut(donnees.valeur_min,
                                              -std::numeric_limits<float>::max());
            auto max = parse_valeur_ou_defaut(donnees.valeur_min,
                                              std::numeric_limits<float>::max());
            auto valeur_defaut = parse_valeur_ou_defaut(donnees.valeur_defaut, 0.0f);
            résultat->valeur_min.f = min;
            résultat->valeur_max.f = max;
            résultat->valeur = valeur_defaut;
            break;
        }
        case TypePropriete::ENUM:
        {
            break;
        }
        case TypePropriete::BOOL:
        {
            résultat->valeur = (donnees.valeur_defaut == "vrai");
            break;
        }
        case TypePropriete::LISTE:
        case TypePropriete::CHAINE_CARACTERE:
        case TypePropriete::TEXTE:
        case TypePropriete::FICHIER_ENTREE:
        case TypePropriete::FICHIER_SORTIE:
        {
            résultat->valeur = donnees.valeur_defaut;
            break;
        }
        case TypePropriete::COULEUR:
        {
            auto min = parse_valeur_ou_defaut(donnees.valeur_min, 0.0f);
            auto max = parse_valeur_ou_defaut(donnees.valeur_min, 1.0f);
            auto valeurs = dls::morcelle(donnees.valeur_defaut, ',');
            auto index = 0;

            dls::phys::couleur32 valeur_defaut(1.0f);
            for (auto const &v : valeurs) {
                valeur_defaut[index++] = parse_valeur_ou_defaut(v, 0.0f);
            }

            résultat->valeur_min.f = min;
            résultat->valeur_max.f = max;
            résultat->valeur = valeur_defaut;
            break;
        }
        case TypePropriete::VECTEUR_DECIMAL:
        {
            auto min = parse_valeur_ou_defaut(donnees.valeur_min,
                                              -std::numeric_limits<float>::max());
            auto max = parse_valeur_ou_defaut(donnees.valeur_min,
                                              std::numeric_limits<float>::max());
            auto valeurs = dls::morcelle(donnees.valeur_defaut, ',');
            auto index = 0ul;

            dls::math::vec3f valeur_defaut(1.0f);
            for (auto const &v : valeurs) {
                valeur_defaut[index++] = parse_valeur_ou_defaut(v, 0.0f);
            }

            résultat->valeur_min.f = min;
            résultat->valeur_max.f = max;
            résultat->valeur = valeur_defaut;
            break;
        }
        case TypePropriete::VECTEUR_ENTIER:
        {
            // À FAIRE
            break;
        }
        case TypePropriete::COURBE_COULEUR:
        {
            break;
        }
        case TypePropriete::COURBE_VALEUR:
        {
            break;
        }
        case TypePropriete::RAMPE_COULEUR:
        {
            break;
        }
        case TypePropriete::LISTE_MANIP:
        {
            break;
        }
    }

    return résultat;
}

AssembleurDisposition::AssembleurDisposition(const DonneesInterface &donnees,
                                             int temps,
                                             bool initialisation_seule)
    : m_donnees(donnees), m_temps(temps), m_initialisation_seule(initialisation_seule)
{
}

void AssembleurDisposition::ajoute_disposition(id_morceau identifiant)
{
    if (m_initialisation_seule) {
        return;
    }

    QBoxLayout *disposition = nullptr;

    switch (identifiant) {
        default:
        case id_morceau::LIGNE:
            disposition = new QHBoxLayout;
            break;
        case id_morceau::COLONNE:
            disposition = new QVBoxLayout;
            break;
    }

    if (!m_pile_dispositions.est_vide()) {
        m_pile_dispositions.haut()->addLayout(disposition);
    }

    m_pile_dispositions.empile(disposition);
}

void AssembleurDisposition::ajoute_étiquette(dls::chaine texte)
{
    auto controle = new ControleProprieteEtiquette(texte.c_str());
    m_pile_dispositions.haut()->addWidget(controle);
}

void AssembleurDisposition::ajoute_controle_pour_propriété(DonneesControle const &donnees,
                                                           BasePropriete *prop)
{
    auto controle = crée_controle_propriété(donnees, prop, m_temps, m_donnees.conteneur);
    m_pile_dispositions.haut()->addWidget(controle);
    controles.insere({donnees.nom, controle});
}

void AssembleurDisposition::ajoute_controle(id_morceau identifiant)
{
    m_donnees_controle = DonneesControle();
    m_identifiant_donnees_controle = identifiant;
    if (identifiant != id_morceau::ETIQUETTE) {
        m_donnees_controle.type = type_propriete_pour_lexeme(identifiant);
    }
}

void AssembleurDisposition::cree_controles_proprietes_extra()
{
    if (m_initialisation_seule) {
        return;
    }

    auto debut = m_donnees.manipulable->debut();
    auto fin = m_donnees.manipulable->fin();

    for (auto iter = debut; iter != fin; ++iter) {
        auto const prop = iter->second;

        if (!prop->est_extra()) {
            continue;
        }

        ajoute_disposition(id_morceau::LIGNE);

        ajoute_étiquette(iter->first);

        m_donnees_controle.nom = iter->first;
        ajoute_controle_pour_propriété(m_donnees_controle, prop);

        sors_disposition();
    }
}

void AssembleurDisposition::ajoute_item_liste(const dls::chaine &nom, const dls::chaine &valeur)
{
    if (m_initialisation_seule) {
        return;
    }

    m_donnees_controle.valeur_enum.ajoute({nom, valeur});
}

void AssembleurDisposition::ajoute_bouton()
{
    if (m_initialisation_seule) {
        return;
    }

    Bouton *bouton = new Bouton;
    bouton->installe_repondant(m_donnees.repondant_bouton);

    m_dernier_bouton = bouton;
    m_pile_dispositions.haut()->addWidget(bouton);
}

void AssembleurDisposition::finalise_controle()
{
    if (m_identifiant_donnees_controle == id_morceau::ETIQUETTE) {
        ajoute_étiquette(m_donnees_controle.valeur_defaut);
        return;
    }

    auto prop = m_donnees.manipulable->propriete(m_donnees_controle.nom);

    /* Crée une propriété depuis la définition si aucune n'existe. */
    if (prop == nullptr) {
        prop = crée_propriété(m_donnees_controle);
        m_donnees.manipulable->ajoute_propriete(m_donnees_controle.nom, prop);
    }

    if (m_initialisation_seule) {
        /* Si on ne fait qu'initialiser les propriétés du manipulable, nous pouvons
         * nous arrêter là. */
        return;
    }

    ajoute_controle_pour_propriété(m_donnees_controle, prop);
}

void AssembleurDisposition::sors_disposition()
{
    if (m_initialisation_seule) {
        return;
    }

    m_pile_dispositions.depile();
}

void AssembleurDisposition::propriete_controle(id_morceau identifiant, const dls::chaine &valeur)
{
    switch (identifiant) {
        case id_morceau::INFOBULLE:
            m_donnees_controle.infobulle = valeur;
            break;
        case id_morceau::MIN:
            m_donnees_controle.valeur_min = valeur;
            break;
        case id_morceau::MAX:
            m_donnees_controle.valeur_max = valeur;
            break;
        case id_morceau::VALEUR:
            m_donnees_controle.valeur_defaut = valeur;
            break;
        case id_morceau::ATTACHE:
            m_donnees_controle.nom = valeur;
            break;
        case id_morceau::PRECISION:
            m_donnees_controle.precision = valeur;
            break;
        case id_morceau::PAS:
            m_donnees_controle.pas = valeur;
            break;
        case id_morceau::FILTRES:
            m_donnees_controle.filtres = valeur;
            break;
        case id_morceau::SUFFIXE:
            m_donnees_controle.suffixe = valeur;
            break;
        case id_morceau::ANIMABLE:
            m_donnees_controle.etat |= EST_ANIMABLE;
            break;
        case id_morceau::ACTIVABLE:
            m_donnees_controle.etat |= EST_ACTIVABLE;
            break;
        default:
            break;
    }
}

void AssembleurDisposition::propriete_bouton(id_morceau identifiant, const dls::chaine &valeur)
{
    if (m_initialisation_seule) {
        return;
    }

    switch (identifiant) {
        case id_morceau::INFOBULLE:
            m_dernier_bouton->etablie_infobulle(valeur);
            break;
        case id_morceau::VALEUR:
            m_dernier_bouton->etablie_valeur(valeur);
            break;
        case id_morceau::ATTACHE:
            m_dernier_bouton->etablie_attache(valeur);
            break;
        case id_morceau::METADONNEE:
            m_dernier_bouton->etablie_metadonnee(valeur);
            break;
        case id_morceau::ICONE:
            m_dernier_bouton->etablie_icone(valeur);
            break;
        default:
            break;
    }
}

void AssembleurDisposition::propriete_action(id_morceau identifiant, const dls::chaine &valeur)
{
    if (m_initialisation_seule) {
        return;
    }

    switch (identifiant) {
        case id_morceau::INFOBULLE:
            m_derniere_action->etablie_infobulle(valeur.c_str());
            break;
        case id_morceau::VALEUR:
            m_derniere_action->etablie_valeur(valeur.c_str());
            break;
        case id_morceau::ATTACHE:
            m_derniere_action->etablie_attache(valeur);
            break;
        case id_morceau::METADONNEE:
            m_derniere_action->etablie_metadonnee(valeur);
            break;
        case id_morceau::ICONE:
            m_derniere_action->etablie_icone(valeur);
            break;
        default:
            break;
    }
}

QBoxLayout *AssembleurDisposition::disposition()
{
    if (m_pile_dispositions.est_vide()) {
        return nullptr;
    }

    return m_pile_dispositions.haut();
}

QMenu *AssembleurDisposition::menu()
{
    return m_menu_racine;
}

void AssembleurDisposition::ajoute_menu(const dls::chaine &nom)
{
    if (m_initialisation_seule) {
        return;
    }

    auto menu = new MenuFiltrable(nom.c_str(), m_donnees.parent_menu);

    if (!m_pile_menus.est_vide()) {
        m_pile_menus.haut()->addMenu(menu);
    }
    else {
        m_menu_racine = menu;
    }

    m_donnees_menus.ajoute({nom, menu});

    m_pile_menus.empile(menu);
}

void AssembleurDisposition::sort_menu()
{
    if (m_initialisation_seule) {
        return;
    }

    m_pile_menus.depile();
}

void AssembleurDisposition::ajoute_action()
{
    if (m_initialisation_seule) {
        return;
    }

    QWidget *parent_action;
    if (m_barre_outils) {
        parent_action = m_barre_outils;
    }
    else {
        parent_action = m_pile_menus.haut();
    }

    auto action = new Action(parent_action);
    action->installe_repondant(m_donnees.repondant_bouton);

    /* À FAIRE : trouve une meilleure manière de procéder. */
    if (m_barre_outils) {
        m_barre_outils->addAction(action);
    }
    else {
        m_pile_menus.haut()->addAction(action);
    }

    m_derniere_action = action;
}

void AssembleurDisposition::ajoute_separateur()
{
    if (m_initialisation_seule) {
        return;
    }

    m_pile_menus.haut()->addSeparator();
}

void AssembleurDisposition::ajoute_dossier()
{
    if (m_initialisation_seule) {
        return;
    }

    QTabWidget *dossier = new QTabWidget;

    m_pile_dispositions.haut()->addWidget(dossier);

    m_dernier_dossier = dossier;
}

void AssembleurDisposition::finalise_dossier()
{
    if (m_initialisation_seule) {
        return;
    }

    m_dernier_dossier->setCurrentIndex(m_donnees.manipulable->onglet_courant);

    if (m_donnees.conteneur != nullptr) {
        QObject::connect(m_dernier_dossier,
                         &QTabWidget::currentChanged,
                         m_donnees.conteneur,
                         &ConteneurControles::onglet_dossier_change);
    }

    m_dernier_dossier = nullptr;
}

void AssembleurDisposition::ajoute_onglet(const dls::chaine &nom)
{
    if (m_initialisation_seule) {
        return;
    }

    /* À FAIRE : expose contrôle de la direction. */
    auto disp_onglet = new QVBoxLayout;

    auto onglet = new QWidget;
    onglet->setLayout(disp_onglet);

    m_pile_dispositions.empile(disp_onglet);

    m_dernier_dossier->addTab(onglet, nom.c_str());
}

void AssembleurDisposition::finalise_onglet()
{
    if (m_initialisation_seule) {
        return;
    }

    m_pile_dispositions.depile();
}

void AssembleurDisposition::nom_disposition(const dls::chaine &chaine)
{
    m_nom = chaine;
}

dls::chaine AssembleurDisposition::nom_disposition() const
{
    return m_nom;
}

const dls::tableau<std::pair<dls::chaine, QMenu *>> &AssembleurDisposition::donnees_menus() const
{
    return m_donnees_menus;
}

void AssembleurDisposition::ajoute_barre_outils()
{
    if (m_initialisation_seule) {
        return;
    }

    if (m_barre_outils) {
        delete m_barre_outils;
    }

    m_barre_outils = new QToolBar(m_donnees.parent_barre_outils);
}

QToolBar *AssembleurDisposition::barre_outils() const
{
    return m_barre_outils;
}

} /* namespace danjo */
