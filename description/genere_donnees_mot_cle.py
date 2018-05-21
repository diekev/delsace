# -*- coding:utf8 -*-

mot_cle = [
    "disposition",
	"menu",
	"barre_outils",
	"colonne",
	"ligne",
	"dossier",
	"onglet",
	"étiquette",
	"entier",
	"décimal",
	"liste",
	"case",
	"chaine",
	"fichier_entrée",
	"fichier_sortie",
	"couleur",
	"vecteur",
	"bouton",
	"action",
	"séparateur",
	"infobulle",
	"min",
	"max",
	"valeur",
	"attache",
	"précision",
	"pas",
	"items",
	"nom",
	"métadonnée",
	"icône",
    "vrai",
	"faux",
	"logique",
	"feuille",
	"entrée",
	"interface",
	"relation",
	"quand",
	"sortie",
	"résultat",
]

caracteres_double = [
	["...","TROIS_POINT"],
	["==","EGALITE"],
	["!=","DIFFERENCE"],
	["<=","INFERIEUR_EGAL"],
	[">=","SUPERIEUR_EGAL"],
	["->","FLECHE"],
	["&&","ESP_ESP"],
	["++","PLUS_PLUS"],
	["--","MOINS_MOINS"],
	["+=","PLUS_EGAL"],
	["-=","MOINS_EGAL"],
	["*=","FOIS_EGAL"],
	["/=","DIVISE_EGAL"],
	["^=","OUX_EGAL"],
	["|=","OU_EGAL"],
	["&=","ET_EGAL"],
	["||","BARE_BARRE"],
	["<<","DECALAGE_GAUCHE"],
	[">>","DECALAGE_DROITE"]
]

caracteres_speciaux = [
	["<","INFERIEUR"],
	[">","SUPERIEUR"],
	["=","EGAL"],
	["!","EXCLAMATION"],
	["+","PLUS"],
	["-","MOINS"],
	["*","FOIS"],
	["/","DIVISE"],
	["%","POURCENT"],
	["&","ESPERLUETTE"],
	["|","BARRE"],
	["[","CROCHET_OUVRANT"],
	["]","CROCHET_FERMANT"],
	["^","CHAPEAU"],
	["(","PARENTHESE_OUVRANTE"],
	[")","PARENTHESE_FERMANTE"],
	["{","ACCOLADE_OUVRANTE"],
	["}","ACCOLADE_FERMANTE"],
	["\"","GUILLEMET"],
	["'","APOSTROPHE"],
	[";","POINT_VIRGULE"],
	[":","DOUBLE_POINT"],
	[".","POINT"],
	["#","DIESE"],
	["~","TILDE"],
	[",","VIRGULE"]
]

identifiant_extra = [
	"CHAINE_CARACTERE",
	"CHAINE_LITTERALE",
	"CARACTERE",
	"NOMBRE",
	"NUL",
]

from operator import itemgetter

mot_cle = sorted(mot_cle)
caracteres_double = sorted(caracteres_double, key=itemgetter(0))
caracteres_speciaux = sorted(caracteres_speciaux, key=itemgetter(0))

def imprime_identifiants():
	print 'enum {'
	for mc in mot_cle:
		mcn = mc.replace('é', 'e').replace('ê', 'e').replace('î', 'i')
		mcn = mcn.upper()

		print('\tIDENTIFIANT_{},'.format(mcn))

	for cs in caracteres_speciaux:
		print('\tIDENTIFIANT_{},'.format(cs[1]))

	for cs in caracteres_double:
		print('\tIDENTIFIANT_{},'.format(cs[1]))

	for id_extra in identifiant_extra:
		print('\tIDENTIFIANT_{},'.format(id_extra))

	print '};'

	for mc in mot_cle:
		mcn = mc.replace('é', 'e').replace('ê', 'e').replace('î', 'i').replace('ô', 'o')
		mcn = mcn.upper()

		print('\t{ '+'IDENTIFIANT_{}, "{}"'.format(mcn, mc)+' },')

	for cs in caracteres_double:
		print("\t{ "+'IDENTIFIANT_{}, "{}"'.format(cs[1], cs[0])+' },')

	for cs in caracteres_speciaux:
		print("\t{ "+"IDENTIFIANT_{}, '{}'".format(cs[1], cs[0])+' },')


def imprime_impression_identifiant():
	print 'const char *imprime_identifiant(int identifiant)'
	print '{'
	print '\tswitch (identifiant) {'

	for mc in mot_cle:
		mcn = mc.replace('é', 'e').replace('ê', 'e').replace('î', 'i').replace('ô', 'o')
		mcn = 'IDENTIFIANT_' + mcn.upper()

		print '\t\tcase {}:'.format(mcn)
		print '\t\t\treturn "{}";'.format(mcn)

	for cs in caracteres_double:
		print '\t\tcase IDENTIFIANT_{}:'.format(cs[1])
		print '\t\t\treturn "IDENTIFIANT_{}";'.format(cs[1])

	for cs in caracteres_speciaux:
		print '\t\tcase IDENTIFIANT_{}:'.format(cs[1])
		print '\t\t\treturn "IDENTIFIANT_{}";'.format(cs[1])

	for id_extra in identifiant_extra:
		print '\t\tcase IDENTIFIANT_{}:'.format(id_extra)
		print '\t\t\treturn "IDENTIFIANT_{}";'.format(id_extra)

	print '\t}'
	print '\treturn "NULL";'
	print '}'

imprime_identifiants()
