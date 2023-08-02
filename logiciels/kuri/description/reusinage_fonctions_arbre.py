# -*- coding:utf-8 -*-

# genre de noeud         'GenreNoeud::DECLARATION_ENTETE_FONCTION'
# noeud du type de noeud 'NoeudDeclarationEnteteFonction'
# nom du comme()         'entete_fonction'
# nom du fichier_source  'noeud_entete_fonction'

# interface
# x valide_semantique
# x calcul_etendue
# x simplifie
# x imprime
# genere_ri
# noeud_code
# x copie
# x aplatis

class Interface():
    def __init__(self):
        self.valide_semantique = []
        self.calcul_etendue = []
        self.simplifie = []
        self.imprime = []
        self.genere_ri = []
        self.noeud_code = []
        self.copie = []
        self.aplatis = []


class NoeudSyntaxique():
    def __init__(self, genre_noeud, type_struct, nom_court, nom_fichier):
        self.genre_noeud = genre_noeud
        self.type_struct = type_struct
        self.nom_court = nom_court
        self.nom_fichier = nom_fichier
        self.interface = None

    def accede_interface(self):
        if self.interface is None:
            self.interface = Interface()
        return self.interface

n = NoeudSyntaxique("DECLARATION_ENTETE_FONCTION", "NoeudDeclarationEnteteFonction", "entete_fonction", "entete_fonction")

def analyse_fonction_avec_switch(plage_lignes, noeud, accede_interface):
    interface = accede_interface(noeud.accede_interface())

    suivante = None

    while True:
        if suivante is None:
            l = next(plage_lignes)
        else:
            l = suivante

        suivante = None

        ls = l.strip()

        if l.startswith("		case GenreNoeud::DECLARATION_ENTETE_FONCTION"):
            while True:
                l = next(plage_lignes)

                if l.startswith("		case GenreNoeud::"):
                    suivante = l
                    break

                #print(l.rstrip())
                interface.append(l.rstrip())

                if l == "		}\n":
                    break


        # print(ls)
        if l == "}\n":
            break

# valide_semantique
# /home/kevin/src/repos/delsace/logiciels/kuri/compilation/validation_semantique.cc
# ResultatValidation ContexteValidationCode::valide_semantique_noeud

# /home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc
# void Simplificatrice::simplifie(NoeudExpression *noeud)

# calcul_etendue
# "/home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc"
# "Etendue calcule_etendue_noeud(const NoeudExpression *racine, Fichier *fichier)"

# /home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc
# void imprime_arbre(NoeudExpression *racine, std::ostream &os, int tab, bool substitution)

# copie
# /home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc
# NoeudExpression *copie_noeud(

# aplatis_arbre
# /home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc
# static void aplatis_arbre(

chemin = "/home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc"
nom_fonction_switch = "NoeudExpression *copie_noeud("

class Fonction():
    def __init__(self, chemin, ligne_definition, nom, type_retour, arguments, accede_interface):
        self.chemin = chemin
        self.ligne_definition = ligne_definition
        self.nom = nom
        self.type_retour = type_retour
        self.arguments = arguments
        self.accede_interface = accede_interface

fonctions = [
    Fonction("/home/kevin/src/repos/delsace/logiciels/kuri/compilation/validation_semantique.cc",
     "ResultatValidation ContexteValidationCode::valide_semantique_noeud",
     "valide_semantique",
     "ResultatValidation",
     "()",
     lambda x: x.valide_semantique),
    Fonction("/home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc",
     "void Simplificatrice::simplifie(NoeudExpression *noeud)",
     "simplifie",
     "void",
     "(Simplificatrice &simplificatrice, NoeudExpression *noeud)",
     lambda x: x.simplifie),
    Fonction("/home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc",
     "Etendue calcule_etendue_noeud(const NoeudExpression *racine, Fichier *fichier)",
     "calcul_etendue",
     "Etendue",
     "(NoeudExpression const *racine, Fichier *fichier)",
     lambda x: x.calcul_etendue),
    Fonction("/home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc",
     "void imprime_arbre(NoeudExpression *racine, std::ostream &os, int tab, bool substitution)",
     "imprime",
     "void",
     "(NoeudExpression *racine, std::ostream &os, int tab, bool substitution)",
     lambda x: x.imprime),
    Fonction("/home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc",
     "NoeudExpression *copie_noeud(",
     "copie",
     "NoeudExpression *",
     "(AssembleuseArbre *assem, NoeudExpression const *racine, NoeudBloc *bloc_parent)",
     lambda x: x.copie),
    Fonction("/home/kevin/src/repos/delsace/logiciels/kuri/compilation/arbre_syntaxique.cc",
     "static void aplatis_arbre(",
     "aplatis",
     "void",
     "(NoeudExpression *racine, kuri::tableau<NoeudExpression *, int> &arbre_aplatis, DrapeauxNoeud drapeau)",
     lambda x: x.aplatis),
    Fonction("/home/kevin/src/repos/delsace/logiciels/kuri/compilation/noeud_code.cc",
     "NoeudCode *ConvertisseuseNoeudCode::converti_noeud_syntaxique",
     "noeud_code",
     "NoeudCode *",
     "(EspaceDeTravail *espace, NoeudExpression *noeud_expression)",
     lambda x: x.noeud_code),
    Fonction("/home/kevin/src/repos/delsace/logiciels/kuri/representation_intermediaire/constructrice_ri.cc",
     "void ConstructriceRI::genere_ri_pour_noeud(NoeudExpression *noeud)",
     "genere_ri",
     "void",
     "(NoeudExpression *noeud)",
     lambda x: x.genere_ri)
]

for f in fonctions:
    with open(f.chemin, "r") as fichier:
        lignes = fichier.readlines()

        plage_lignes = iter(lignes)
        for p in plage_lignes:
            if p.startswith(f.ligne_definition):
                analyse_fonction_avec_switch(plage_lignes, n, f.accede_interface)

def imprime_lignes_interface(message, interface):
    print(message)
    for l in interface:
        print(l)

# imprime_lignes_interface("Interface pour validation semantique", n.interface.valide_semantique)
# imprime_lignes_interface("Interface pour entendue", n.interface.calcul_etendue)
# imprime_lignes_interface("Interface pour simplification", n.interface.simplifie)
# imprime_lignes_interface("Interface pour impression", n.interface.imprime)
# imprime_lignes_interface("Interface pour génération RI", n.interface.genere_ri)
# imprime_lignes_interface("Interface pour noeud code", n.interface.noeud_code)
# imprime_lignes_interface("Interface pour copie", n.interface.copie)
# imprime_lignes_interface("Interface pour aplatissement", n.interface.aplatis)

# "static TableVirtuelleNoeudSyntaxique table_virtuelle_noeud;"

def genere_fonction(message, noeud, source, type_retour, arguments):
    if len(source) == 0:
        return ""

    texte = type_retour + " " + message + '_' + noeud.nom_court + arguments + "\n"

    for s in source:
        texte += s[2:] + '\n'

    return texte

def cree_fonction_enregistrement_table(noeud):
    source = "void enregistre_table_pour_" + noeud.nom_court
    source += "(TableVirtuelleNoeudSyntaxique *table)\n"
    source += "{\n"

    for f in fonctions:
        interface = f.accede_interface(noeud.interface)
        if len(interface) != 0:
            source += "\ttable->" + f.nom + " = " + f.nom + "_" + noeud.nom_court + ";\n"

    source += "}\n"

    return source

# print(cree_fonction_enregistrement_table(n))

for f in fonctions:
    print(genere_fonction(f.nom, n, f.accede_interface(n.interface), f.type_retour, f.arguments))

print(cree_fonction_enregistrement_table(n))
