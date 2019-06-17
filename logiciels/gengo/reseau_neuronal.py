# -*- coding: utf-8 -*-

# ##### BEGIN GPL LICENSE BLOCK #####
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software  Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The Original Code is Copyright (C) 2016 Kévin Dietrich.
# All rights reserved.
#
# ##### END GPL LICENSE BLOCK #####

from langues import langues, fichiers_echantillons, vecteurs_langues
from utils import convertie_mot, LONGUEUR_MOT_MAX

import array
import codecs
import numpy as np

np.random.seed(1)

NOMBRE_NEURONES_COUCHE1 = LONGUEUR_MOT_MAX + 2
NOMBRE_SORTIES = 3


def active(x):
    return (np.tanh(x) + 1.0) / 2.0


def active_derivee(x):
    tanhx = np.tanh(x)
    tanhx_sqr = tanhx * tanhx

    return (1.0 - tanhx_sqr) / 2.0


def genere_matrice(rangees, colonnes):
    return 2.0 * np.random.random((rangees, colonnes)) - 1.0


class ReseauNeuronal():
    def __init__(self):
        self.nombre_entrees = LONGUEUR_MOT_MAX
        self.nombre_caches = NOMBRE_NEURONES_COUCHE1 - 4
        self.nombre_sorties = NOMBRE_SORTIES

        self.entrees_couches = genere_matrice(LONGUEUR_MOT_MAX + 1,
                                              NOMBRE_NEURONES_COUCHE1 + 1)

        self.couches_sorties = genere_matrice(NOMBRE_NEURONES_COUCHE1 + 1,
                                              NOMBRE_SORTIES)

        self.entrees = []
        self.sorties = []
        self.neurones = []

        for i in range(self.nombre_entrees + 1):
            self.entrees.append(1.0)

        for i in range(self.nombre_caches + 1):
            self.neurones.append(1.0)

        for i in range(self.nombre_sorties + 1):
            self.sorties.append(1.0)

    def avance(self, entrees):
        l0 = np.array(entrees)
        l1 = active(np.dot(l0, self.entrees_couches))
        l2 = active(np.dot(l1, self.couches_sorties))

        self.entrees = entrees
        self.neurones = l1
        self.sorties = l2


class EntraineurReseau():
    def __init__(self, reseau):
        self.reseau = reseau
        self.erreur = 0

    def entraine(self):
        for i in range(50):
            print "Époque", i
            self.charge_langues()

    def charge_langues(self):
        for langue in langues:
            fichier = fichiers_echantillons[langue]
            sortie_attendue = vecteurs_langues[langue]

            print "Ouverture du fichier:", fichier
            print "Sortie attendue:", sortie_attendue

            with open(fichier) as f:
                self.entraine_impl(f, sortie_attendue)

    def entraine_impl(self, liste_mots, sortie_attendue):
        index = 0

        for mot in liste_mots:
            mot = mot.rstrip('\n')

            imprime_erreur = index == 0  # ((index % 500) == 0)
            index += 1

#            if imprime_erreur:
#                print "Mot courrant:", mot

            entree = convertie_mot(mot)
            self.reseau.avance(entree)
            self.retroprojette_erreur(sortie_attendue, imprime_erreur)

    def retroprojette_erreur(self, sortie_attendue, imprime_erreur=False):
        erreur_sortie = sortie_attendue - self.reseau.sorties

        if imprime_erreur is True:
            print "Erreur moyenne:", str(np.mean(np.abs(erreur_sortie)))

        delta_erreur_sortie = erreur_sortie * active_derivee(self.reseau.sorties)
        erreur_caches = delta_erreur_sortie.dot(self.reseau.couches_sorties.T)

        delta_erreur_caches = erreur_caches * active_derivee(self.reseau.neurones)

        self.reseau.couches_sorties += self.reseau.sorties.T.dot(delta_erreur_sortie) * 0.0001
        self.reseau.entrees_couches += self.reseau.neurones.T.dot(delta_erreur_caches) * 0.0001
