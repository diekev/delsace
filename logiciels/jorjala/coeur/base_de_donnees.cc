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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "base_de_donnees.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/chaine.hh"

#include "base_de_donnees.hh"
#include "composite.h"
#include "noeud_image.h"
#include "nuanceur.hh"
#include "objet.h"
#include "rendu.hh"

/* ************************************************************************** */

/* À FAIRE : noms uniques. Les graphes sont pour le moment chargés de maintenir
 * les listes des noms pour éviter tout conflit. Lors de l'ouverture de fichier
 * notamment, nous devons tous détruire pour s'assurer que les listes des noms
 * soient bel et bien détruites pour ne pas colléser avec les noms des objets
 * lus. Peut-être que la gestion des noms peut être releguée à la BaseDeDonnées.
 */

BaseDeDonnees::BaseDeDonnees()
{
    m_racine.nom = "";
    m_racine_objets.nom = "objets";
    m_racine_composites.nom = "composites";
    m_racine_nuanceurs.nom = "nuanceurs";
    m_racine_rendus.nom = "rendus";

    m_racine_objets.graphe.type = type_graphe::RACINE_OBJET;
    m_racine_composites.graphe.type = type_graphe::RACINE_COMPOSITE;
    m_racine_nuanceurs.graphe.type = type_graphe::RACINE_NUANCEUR;
    m_racine_rendus.graphe.type = type_graphe::RACINE_RENDU;

    m_racine.enfants.ajoute(&m_racine_objets);
    m_racine.enfants.ajoute(&m_racine_composites);
    m_racine.enfants.ajoute(&m_racine_nuanceurs);
    m_racine.enfants.ajoute(&m_racine_rendus);
}

BaseDeDonnees::~BaseDeDonnees()
{
    reinitialise();
}

void BaseDeDonnees::reinitialise()
{
    for (auto objet : m_objets) {
        memoire::deloge("objet", objet);
    }

    m_objets.efface();
    m_racine_objets.graphe.noeud_actif = nullptr;
    m_racine_objets.graphe.supprime_tout();

    for (auto compo : m_composites) {
        memoire::deloge("compo", compo);
    }

    m_composites.efface();
    m_racine_composites.graphe.noeud_actif = nullptr;
    m_racine_composites.graphe.supprime_tout();

    for (auto nuance : m_nuanceurs) {
        memoire::deloge("nuanceur", nuance);
    }

    m_nuanceurs.efface();
    m_racine_nuanceurs.graphe.noeud_actif = nullptr;
    m_racine_nuanceurs.graphe.supprime_tout();

    for (auto rend : m_rendus) {
        memoire::deloge("rendu", rend);
    }

    m_rendus.efface();
    m_racine_rendus.graphe.noeud_actif = nullptr;
    m_racine_rendus.graphe.supprime_tout();
}

Noeud *BaseDeDonnees::racine()
{
    return &m_racine;
}

Noeud const *BaseDeDonnees::racine() const
{
    return &m_racine;
}

/* ************************************************************************** */

Objet *BaseDeDonnees::cree_objet(dls::chaine const &nom, type_objet type)
{
    auto objet = memoire::loge<Objet>("objet");
    objet->type = type;

    switch (objet->type) {
        case type_objet::NUL:
        {
            break;
        }
        case type_objet::CORPS:
        {
            objet->donnees = memoire::loge<DonneesCorps>("DonneesCorps");
            break;
        }
        case type_objet::CAMERA:
        {
            objet->donnees = memoire::loge<DonneesCamera>("DonneesCamera", 1920, 1080);
            break;
        }
        case type_objet::LUMIERE:
        {
            objet->donnees = memoire::loge<DonneesLumiere>("DonneesLumiere");
            break;
        }
    }

    auto noeud = m_racine_objets.graphe.cree_noeud(nom, type_noeud::OBJET);

    objet->noeud = noeud;

    noeud->donnees = objet;
    noeud->peut_avoir_graphe = true;
    noeud->graphe.type = type_graphe::OBJET;

    m_objets.ajoute(objet);

    return objet;
}

Objet *BaseDeDonnees::objet(dls::chaine const &nom) const
{
    for (auto objet : m_objets) {
        if (objet->noeud->nom == nom) {
            return objet;
        }
    }

    return nullptr;
}

void BaseDeDonnees::enleve_objet(Objet *objet)
{
    auto iter = std::find(m_objets.debut(), m_objets.fin(), objet);
    m_objets.erase(iter);

    m_racine_objets.graphe.supprime(objet->noeud);

    memoire::deloge("objet", objet);
}

const dls::tableau<Objet *> &BaseDeDonnees::objets() const
{
    return m_objets;
}

Graphe *BaseDeDonnees::graphe_objets()
{
    return &m_racine_objets.graphe;
}

const Graphe *BaseDeDonnees::graphe_objets() const
{
    return &m_racine_objets.graphe;
}

/* ************************************************************************** */

Composite *BaseDeDonnees::cree_composite(dls::chaine const &nom)
{
    auto compo = memoire::loge<Composite>("compo");

    auto noeud = m_racine_composites.graphe.cree_noeud(nom, type_noeud::COMPOSITE);

    compo->noeud = noeud;
    noeud->donnees = compo;
    noeud->peut_avoir_graphe = true;
    noeud->graphe.type = type_graphe::COMPOSITE;

    m_composites.ajoute(compo);

    return compo;
}

Composite *BaseDeDonnees::composite(dls::chaine const &nom) const
{
    for (auto compo : m_composites) {
        if (compo->noeud->nom == nom) {
            return compo;
        }
    }

    return nullptr;
}

void BaseDeDonnees::enleve_composite(Composite *compo)
{
    auto iter = std::find(m_composites.debut(), m_composites.fin(), compo);
    m_composites.erase(iter);

    m_racine_composites.graphe.supprime(compo->noeud);

    memoire::deloge("compo", compo);
}

const dls::tableau<Composite *> &BaseDeDonnees::composites() const
{
    return m_composites;
}

Graphe *BaseDeDonnees::graphe_composites()
{
    return &m_racine_composites.graphe;
}

const Graphe *BaseDeDonnees::graphe_composites() const
{
    return &m_racine_composites.graphe;
}

/* ************************************************************************** */

Nuanceur *BaseDeDonnees::cree_nuanceur(const dls::chaine &nom)
{
    auto noeud = m_racine_nuanceurs.graphe.cree_noeud(nom, type_noeud::NUANCEUR);
    auto nuance = memoire::loge<Nuanceur>("nuanceur", *noeud);

    m_nuanceurs.ajoute(nuance);

    return nuance;
}

Nuanceur *BaseDeDonnees::nuanceur(const dls::chaine &nom) const
{
    for (auto nuance : m_nuanceurs) {
        if (nuance->noeud.nom == nom) {
            return nuance;
        }
    }

    return nullptr;
}

void BaseDeDonnees::enleve_nuanceur(Nuanceur *nuanceur)
{
    auto iter = std::find(m_nuanceurs.debut(), m_nuanceurs.fin(), nuanceur);
    m_nuanceurs.erase(iter);

    m_racine_nuanceurs.graphe.supprime(&nuanceur->noeud);

    memoire::deloge("nuanceur", nuanceur);
}

const dls::tableau<Nuanceur *> &BaseDeDonnees::nuanceurs() const
{
    return m_nuanceurs;
}

Graphe *BaseDeDonnees::graphe_nuanceurs()
{
    return &m_racine_nuanceurs.graphe;
}

const Graphe *BaseDeDonnees::graphe_nuanceurs() const
{
    return &m_racine_nuanceurs.graphe;
}

/* ************************************************************************** */

Rendu *BaseDeDonnees::cree_rendu(const dls::chaine &nom)
{
    auto noeud = m_racine_rendus.graphe.cree_noeud(nom, type_noeud::RENDU);
    auto rend = memoire::loge<Rendu>("rendu", *noeud);

    m_rendus.ajoute(rend);

    return rend;
}

Rendu *BaseDeDonnees::rendu(const dls::chaine &nom) const
{
    for (auto rend : m_rendus) {
        if (rend->noeud.nom == nom) {
            return rend;
        }
    }

    return nullptr;
}

void BaseDeDonnees::enleve_rendu(Rendu *rendu)
{
    auto iter = std::find(m_rendus.debut(), m_rendus.fin(), rendu);
    m_rendus.erase(iter);

    m_racine_rendus.graphe.supprime(&rendu->noeud);

    memoire::deloge("rendu", rendu);
}

const dls::tableau<Rendu *> &BaseDeDonnees::rendus() const
{
    return m_rendus;
}

Graphe *BaseDeDonnees::graphe_rendus()
{
    return &m_racine_rendus.graphe;
}

const Graphe *BaseDeDonnees::graphe_rendus() const
{
    return &m_racine_rendus.graphe;
}

static auto trouve_enfant(Noeud const *noeud, dls::chaine const &nom)
{
    for (auto enfant : noeud->enfants) {
        if (enfant->nom == nom) {
            return enfant;
        }
    }

    return static_cast<Noeud *>(nullptr);
}

Noeud *cherche_noeud_pour_chemin(BaseDeDonnees &base, const dls::chaine &chemin)
{
    auto morceaux = dls::morcelle(chemin, '/');

    auto noeud_racine = base.racine();

    if (morceaux.est_vide()) {
        return nullptr;
    }

    for (auto morceau : morceaux) {
        auto enfant = trouve_enfant(noeud_racine, morceau);

        if (enfant == nullptr) {
            return nullptr;
        }

        noeud_racine = enfant;
    }

    return noeud_racine;
}

Noeud const *cherche_noeud_pour_chemin(BaseDeDonnees const &base, dls::chaine const &chemin)
{
    auto morceaux = dls::morcelle(chemin, '/');

    auto noeud_racine = base.racine();

    if (morceaux.est_vide()) {
        return nullptr;
    }

    for (auto morceau : morceaux) {
        auto enfant = trouve_enfant(noeud_racine, morceau);

        if (enfant == nullptr) {
            return nullptr;
        }

        noeud_racine = enfant;
    }

    return noeud_racine;
}
