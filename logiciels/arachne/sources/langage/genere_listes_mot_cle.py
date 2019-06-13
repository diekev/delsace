# -*- coding:utf8 -*-

mot_cle = [
	"CRÉE",
	"BASE_DE_DONNÉES",
	"UTILISE",
	"SUPPRIME",
	"SI",
	"RETOURNE",
	"ET",
	"OU",
	"CHARGE",
	"ÉCRIT",
	"FICHIER",
	"VRAI",
	"FAUX",
	"NUL",
	"TROUVE",
]

caracteres_double = [
	["==","EGALITE"],
	["!=","DIFFERENCE"],
	["<=","INFERIEUR_EGAL"],
	[">=","SUPERIEUR_EGAL"],
]

caracteres_speciaux = [
	["<","INFERIEUR"],
	[">","SUPERIEUR"],
	["[","CROCHET_OUVRANT"],
	["]","CROCHET_FERMANT"],
	["(","PARENTHESE_OUVRANTE"],
	[")","PARENTHESE_FERMANTE"],
	["{","ACCOLADE_OUVRANTE"],
	["}","ACCOLADE_FERMANTE"],
	["\"","GUILLEMET"],
	[";","POINT_VIRGULE"],
	[":","DOUBLE_POINT"],
	[".","POINT"],
	[",","VIRGULE"],
	["'","APOSTROPHE"],
]

identifiant_extra = [
	"CHAINE_CARACTERE",
	"CHAINE_LITTERALE",
	"CARACTERE",
	"NOMBRE",
	"NOMBRE_DECIMAL",
	"BOOL",
	"NUL",
]

from operator import itemgetter

mot_cle = sorted(mot_cle)
caracteres_double = sorted(caracteres_double, key=itemgetter(0))
caracteres_speciaux = sorted(caracteres_speciaux, key=itemgetter(0))

def imprime_identifiants():
	print 'enum {'
	for mc in mot_cle:
		mcn = mc.replace('É', 'E')
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
		mcn = mc.replace('É', 'E')
		mcn = mcn.upper()

		print('\t{ '+'IDENTIFIANT_{}, "{}"'.format(mcn, mc)+' },')

	for cs in caracteres_double:
		print("\t{ "+'IDENTIFIANT_{}, "{}"'.format(cs[1], cs[0])+' },')

	for cs in caracteres_speciaux:
		print("\t{ "+"IDENTIFIANT_{}, '{}'".format(cs[1], cs[0])+' },')


def imprime_chaines_identifiants():
	print 'const char *chaine_identifiant(int identifiant)'
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
imprime_chaines_identifiants()
