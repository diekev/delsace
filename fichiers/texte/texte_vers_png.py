# -*- coding:utf8 -*-

import Image
import ImageDraw
import ImageFont

def calcule_taille_fonte(texte, fonte):
    image_test = Image.new('RGB', (1, 1))
    dessin_test = ImageDraw.Draw(image_test)
    return dessin_test.textsize(texte, fonte)

nom_fonte = "LiberationMono-Bold.ttf"
taille_fonte = 32
texte = u"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ ,?;.:/!-*0123456789ÉÊÈÀÂÏÎÔÙÜéêèàâïîôùü"

couleur_texte = "white"

DECALAGE = 6

fonte = ImageFont.truetype(nom_fonte, taille_fonte)
largeur, hauteur = calcule_taille_fonte(texte, fonte)

image = Image.new('RGBA', (largeur, hauteur + DECALAGE))
d = ImageDraw.Draw(image)
d.text((0, 0), texte, fill=couleur_texte, font=fonte)

image.save("texture_texte.png")

print("static constexpr auto HAUTEUR_TEXTURE_POLICE = {};".format(hauteur + DECALAGE))
print("static constexpr auto LARGEUR_TEXTURE_POLICE = {};".format(largeur))
print("static constexpr auto LARGEUR_PAR_LETTRE = {};".format(largeur / len(texte)))

"""
index = 0

chaine = "std::unordered_map<char, int> table_uv_texte({"

for caractere in texte:
    if index % 7 == 0:
        chaine += "\n\t"

    chaine += u"{"
    chaine += u"'{}', {}".format(caractere, index)
    chaine += u"}, "
    index += 1

chaine += "\n});\n"

print(chaine)
"""
