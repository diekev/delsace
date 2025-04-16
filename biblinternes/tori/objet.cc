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

#include "objet.hh"

#include <iostream>

namespace tori {

const char *chaine_type(type_objet type) {
  switch (type) {
  case type_objet::NUL: {
    return "NUL";
  }
  case type_objet::DICTIONNAIRE: {
    return "DICTIONNAIRE";
  }
  case type_objet::TABLEAU: {
    return "TABLEAU";
  }
  case type_objet::CHAINE: {
    return "CHAINE";
  }
  case type_objet::NOMBRE_ENTIER: {
    return "NOMBRE_ENTIER";
  }
  case type_objet::NOMBRE_REEL: {
    return "NOMBRE_REEL";
  }
  }

  return "INVALIDE";
}

/* ************************************************************************** */

static void detruit_objet(Objet *objet) {
  switch (objet->type) {
  case type_objet::NUL: {
    delete objet;
    break;
  }
  case type_objet::DICTIONNAIRE: {
    delete static_cast<ObjetDictionnaire *>(objet);
    break;
  }
  case type_objet::TABLEAU: {
    delete static_cast<ObjetTableau *>(objet);
    break;
  }
  case type_objet::CHAINE: {
    delete static_cast<ObjetChaine *>(objet);
    break;
  }
  case type_objet::NOMBRE_ENTIER: {
    delete static_cast<ObjetNombreEntier *>(objet);
    break;
  }
  case type_objet::NOMBRE_REEL: {
    delete static_cast<ObjetNombreReel *>(objet);
    break;
  }
  }
}

std::shared_ptr<Objet> construit_objet(type_objet type) {
  auto objet = static_cast<Objet *>(nullptr);

  switch (type) {
  case type_objet::NUL:
    objet = new Objet{};
    break;
  case type_objet::DICTIONNAIRE:
    objet = new ObjetDictionnaire{};
    break;
  case type_objet::TABLEAU:
    objet = new ObjetTableau{};
    break;
  case type_objet::CHAINE:
    objet = new ObjetChaine{};
    break;
  case type_objet::NOMBRE_ENTIER:
    objet = new ObjetNombreEntier{};
    break;
  case type_objet::NOMBRE_REEL:
    objet = new ObjetNombreReel{};
    break;
  }

  objet->type = type;

  return std::shared_ptr<Objet>(objet, detruit_objet);
}

std::shared_ptr<Objet> construit_objet(long v) {
  auto objet = std::make_shared<ObjetNombreEntier>();
  objet->valeur = v;
  objet->type = type_objet::NOMBRE_ENTIER;
  return objet;
}

std::shared_ptr<Objet> construit_objet(double v) {
  auto objet = std::make_shared<ObjetNombreReel>();
  objet->valeur = v;
  objet->type = type_objet::NOMBRE_REEL;
  return objet;
}

std::shared_ptr<Objet> construit_objet(dls::chaine const &v) {
  auto objet = std::make_shared<ObjetChaine>();
  objet->valeur = v;
  objet->type = type_objet::CHAINE;
  return objet;
}

std::shared_ptr<Objet> construit_objet(char const *v) {
  auto objet = std::make_shared<ObjetChaine>();
  objet->valeur = v;
  objet->type = type_objet::CHAINE;
  return objet;
}

/* ************************************************************************** */

static Objet *cherche_propriete(ObjetDictionnaire *dico, dls::chaine const &nom,
                                type_objet type) {
  auto objet = dico->objet(nom);

  if (objet == nullptr) {
    std::cerr << "La propriété « " << nom << " » n'existe pas !\n";
    return nullptr;
  }

  if (objet->type != type) {
    std::cerr << "La propriété « " << nom << " » n'est pas de type « "
              << chaine_type(type) << " » (mais de type « "
              << chaine_type(objet->type) << " ») !\n";
    return nullptr;
  }

  return objet;
}

ObjetChaine *cherche_chaine(ObjetDictionnaire *dico, const dls::chaine &nom) {
  auto objet = cherche_propriete(dico, nom, type_objet::CHAINE);

  if (objet == nullptr) {
    return nullptr;
  }

  return extrait_chaine(objet);
}

ObjetDictionnaire *cherche_dico(ObjetDictionnaire *dico,
                                const dls::chaine &nom) {
  auto objet = cherche_propriete(dico, nom, type_objet::DICTIONNAIRE);

  if (objet == nullptr) {
    return nullptr;
  }

  return extrait_dictionnaire(objet);
}

ObjetNombreEntier *cherche_nombre_entier(ObjetDictionnaire *dico,
                                         const dls::chaine &nom) {
  auto objet = cherche_propriete(dico, nom, type_objet::NOMBRE_ENTIER);

  if (objet == nullptr) {
    return nullptr;
  }

  return extrait_nombre_entier(objet);
}

ObjetNombreReel *cherche_nombre_reel(ObjetDictionnaire *dico,
                                     const dls::chaine &nom) {
  auto objet = cherche_propriete(dico, nom, type_objet::NOMBRE_REEL);

  if (objet == nullptr) {
    return nullptr;
  }

  return extrait_nombre_reel(objet);
}

ObjetTableau *cherche_tableau(ObjetDictionnaire *dico, const dls::chaine &nom) {
  auto objet = cherche_propriete(dico, nom, type_objet::TABLEAU);

  if (objet == nullptr) {
    return nullptr;
  }

  return extrait_tableau(objet);
}

} /* namespace tori */
