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
# The Original Code is Copyright (C) 2015 Kévin Dietrich.
# All rights reserved.
#
# ##### END GPL LICENSE BLOCK #####

from utils import make_unicode


class ScreenPlayOutline():
    def __init__(self, title, pages, desc, start, end):
        self.title = title
        self.pages = pages
        self.desc = desc
        self.plot_point_start = start
        self.plot_point_end = end


PLOT_POINT_1 = 0
PLOT_POINT_2 = 1
PLOT_POINT_3 = 2
PLOT_POINT_4 = 3
PLOT_POINT_5 = 4
PLOT_POINT_6 = 5
PLOT_POINT_7 = 6
PLOT_POINT_8 = 7
PLOT_POINT_9 = 8
PLOT_POINT_10 = 9
PLOT_POINT_11 = 10
PLOT_POINT_12 = 11
CENTRAL_QUESTION = 12

PLOT_POINT_13 = 13
PLOT_POINT_14 = 14
PLOT_POINT_15 = 15
PLOT_POINT_16 = 16
PLOT_POINT_17 = 17
PLOT_POINT_18 = 18
PLOT_POINT_19 = 19
PLOT_POINT_20 = 20
PLOT_POINT_21 = 21
PLOT_POINT_22 = 22
PLOT_POINT_23 = 23
PLOT_POINT_24 = 24
PLOT_POINT_25 = 25
PLOT_POINT_26 = 26

PLOT_POINT_27 = 27
PLOT_POINT_28 = 28
PLOT_POINT_29 = 29
PLOT_POINT_30 = 30
PLOT_POINT_31 = 31
PLOT_POINT_32 = 32
PLOT_POINT_33 = 33
PLOT_POINT_34 = 34
PLOT_POINT_35 = 35
PLOT_POINT_36 = 36
PLOT_POINT_37 = 37
PLOT_POINT_38 = 38
PLOT_POINT_39 = 39
PLOT_POINT_40 = 40

PLOT_POINT_41 = 41
PLOT_POINT_42 = 42
PLOT_POINT_43 = 43
PLOT_POINT_44 = 44

outline_points = [
    ScreenPlayOutline(
        make_unicode("Je n'ai aucun respect !"),
        make_unicode("(pages 1-6)"),
        make_unicode("La plupart des scénarios sont à propos d'une chose... le respect!\n"
        "Votre protagoniste ne l'a pas, sait qu'il ne l'a pas, et le veut.\n"
        "Dans la première section, faites comprendre au lecteur que votre\n"
        "personnage principal est un orphelin, un étranger regardant un monde\n"
        "qui ne veut pas de lui."),
        PLOT_POINT_1,
        PLOT_POINT_3
    ),
    ScreenPlayOutline(
        make_unicode("Tu sais ce que c'est ton problème ?"),
        make_unicode("(pages 7-12)"),
        make_unicode("Précisez le problème ou le défaut du protagoniste, et au protagoniste,\n"
        "et au lecteur. Dans le doute, vous pouvez utiliser la ligne de dialogue\n"
        "(surutilisée) « Tu sais ce que c'est ton problème? », prononcé par\n"
        "l'allié ou le meilleur ami du protagoniste."),
        PLOT_POINT_4,
        PLOT_POINT_5
    ),
    ScreenPlayOutline(
        make_unicode("Appels & Signaux Occupés"),
        make_unicode("(pages 12-17)"),
        make_unicode("Donnez au protagoniste le fameux « appel à l'aventure » qui est suivi\n"
        "par l'également célèbre « refus de l'appel »."),
        PLOT_POINT_6,
        PLOT_POINT_8
    ),
    ScreenPlayOutline(
        make_unicode("De l'autre côté du miroir"),
        make_unicode("(pages 17-28)"),
        make_unicode("Forcez le protagoniste à sortir du monde normal et demandez-lui de\n"
        "répondre à « l'appel à l'aventure ». Parfois, le protagoniste le fait\n"
        "par choix, parfois par circonstance."),
        PLOT_POINT_9,
        PLOT_POINT_12
    ),
    ScreenPlayOutline(
        make_unicode("Donner un coup de pied dans la fourmilière"),
        make_unicode("(pages 28-35)"),
        make_unicode("Montrer que les méchants ne sont pas seulement mauvais, mais vraiment,\n"
        "Vraiment, VRAIMENT mauvais. Donnez au protagoniste (et au lecteur)\n"
        "l'idée que répondre à la question centrale va être plus difficile que\n"
        "la pensée initialement. Si vous n'avez pas un antagoniste standard\n"
        "(comme dans une histoire d'amour) montrer que les obstacles au\n"
        "protagoniste sont d'une force écrasante."),
        PLOT_POINT_13,
        PLOT_POINT_18
    ),
    ScreenPlayOutline(
        make_unicode("Quel est le droit chemin ?"),
        make_unicode("(pages 35-45)"),
        make_unicode("Donnez au protagoniste une série de succès et d'échecs tandis qu'il\n"
        "« erre » et commence à maîtriser les compétences nécessaires pour\n"
        "répondre en définitive à la question centrale."),
        PLOT_POINT_19,
        PLOT_POINT_22
    ),
    ScreenPlayOutline(
        make_unicode("Quand la vie vous donne du citron..."),
        make_unicode("(pages 45-55)"),
        make_unicode("Juste quand votre protagoniste pensait qu'il faisait des progrès,\n"
        "vous tirez le tapis de dessous ! Forcez votre protagoniste à arrêter\n"
        "d'errer et à commencer à lutter."),
        PLOT_POINT_23,
        PLOT_POINT_26
    ),
    ScreenPlayOutline(
        make_unicode("... faites de la limonade."),
        make_unicode("(pages 55-65)"),
        make_unicode("Demandez à votre protagoniste d'entrer en confrontation directe dans\n"
        "une grande voie."),
        PLOT_POINT_27,
        PLOT_POINT_32
    ),
    ScreenPlayOutline(
        make_unicode("À l'intérieur de la baleine"),
        make_unicode("(pages 65-75)"),
        make_unicode("Dans le récit mythologique classique, c'est le moment du « ventre de\n"
        "la bête » ou de « l'intérieur de la grotte la plus sombre » pour le\n"
        "personnage principal. Souvent, la scène se déroule dans un espace\n"
        "confiné, représentant que les forces en jeu contre le protagoniste se\n"
        "referment, se serrent, et le protagoniste doit creuser\n"
        "« profondément » et affronter sa peur la plus sombre."),
        PLOT_POINT_33,
        PLOT_POINT_36
    ),
    ScreenPlayOutline(
        make_unicode("Mort & Rennaissance"),
        make_unicode("(pages 75-85)"),
        make_unicode("Un autre moment classique. Demandez à votre personnage principal de\n"
        "mourir et de renaître. À bien des égards, c'est le moment ultime dans\n"
        "l'arc de votre protagoniste. Le moment où il jette la peau de sa\n"
        "vieille vie et émerge nouvellement formé, auto-actualisé, et prêt à\n"
        "prouver lui-même au monde."),
        PLOT_POINT_37,
        PLOT_POINT_40
    ),
    ScreenPlayOutline(
        make_unicode("Que peut-il arriver de pire ?"),
        make_unicode("(pages 85-95)"),
        make_unicode("Le titre dit tout ! La vie entière de votre protagoniste (toute votre\n"
        "histoire) a été construite à la fois pour éviter ce moment et pour\n"
        "l'affronter. Éviter, parce que c'est sa pire crainte. Affronter parce\n"
        "que c'est ce qu'il doit faire pour devenir la personne qu'il doit être.\n"
        "\n"
        "La mort et la renaissance peuvent masser dans une variété de façons,\n"
        "et vous pouvez parfois le remettre au personnage le plus étroitement\n"
        "associé avec les aspirations les plus élevées de votre protagoniste.\n"
        "Dans les comédies romantiques, écrivez cette section afin qu'elle\n"
        "trace la mort de la relation espérée suivie par la réalisation de ce\n"
        "qui est nécessaire pour lui donner un nouvel espoir."),
        PLOT_POINT_41,
        PLOT_POINT_42
    ),
    ScreenPlayOutline(
        make_unicode("Le gentil versus le méchant sur les enjeux"),
        make_unicode("(pages 95-105)"),
        make_unicode("Le point culminant de chaque histoire bien racontée est le\n"
        "protagoniste dans la bataille campée contre l'antagoniste sur les\n"
        "enjeux de l'histoire. J'ai vu des films où la bataille finale est\n"
        "remis à un personnage subordonné ou mineur, et vous pouvez sentir\n"
        "votre cerveau se rebellé tout en regardant le film. Dans votre histoire,\n"
        "assurez-vous que c'est votre personnage principal qui doit se salir\n"
        "les mains, pas quelqu'un d'autre. Votre personnage principal pourrait\n"
        "obtenir une aide désespérément nécessaire, mais les choix et l'action\n"
        "appartiennent à votre personnage principal.\n"
        "\n"
        "Dans les comédies romantiques, cela peut être un moment apparemment\n"
        "petit à la toute fin de l'histoire, parce qu'après tout, quand le\n"
        "garçon obtient la fille, c'est fini."),
        PLOT_POINT_43,
        PLOT_POINT_44
    ),
]
