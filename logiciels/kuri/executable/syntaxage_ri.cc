/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "biblinternes/langage/erreur.hh"

#include <iostream>

#include "arbre_syntaxique/assembleuse.hh"

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"

#include "parsage/lexeuse.hh"

#include "representation_intermediaire/analyse.hh"
#include "representation_intermediaire/impression.hh"
#include "representation_intermediaire/syntaxage.hh"

#include "structures/chemin_systeme.hh"

#include "utilitaires/log.hh"

static void imprime_erreur(SiteSource site, kuri::chaine message)
{
    auto fichier = site.fichier;
    auto index_ligne = site.index_ligne;
    auto index_colonne = site.index_colonne;

    auto ligne_courante = fichier->tampon()[index_ligne];

    Enchaineuse enchaineuse;
    enchaineuse << "Erreur : " << fichier->chemin() << ":" << index_ligne + 1 << ":\n";
    enchaineuse << ligne_courante;

    /* La position ligne est en octet, il faut donc compter le nombre d'octets
     * de chaque point de code pour bien formater l'erreur. */
    for (auto i = 0l; i < index_colonne;) {
        if (ligne_courante[i] == '\t') {
            enchaineuse << '\t';
        }
        else {
            enchaineuse << ' ';
        }

        i += lng::decalage_pour_caractere(ligne_courante, i);
    }

    enchaineuse << "^~~~\n";
    enchaineuse << message;

    dbg() << enchaineuse.chaine();
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        dbg() << "Utilisation " << argv[0] << " "
              << "FICHIER_RI";
        return 1;
    }

    auto chemin_fichier_ri = kuri::chemin_systeme::chemin_temporaire(argv[1]);
    if (!kuri::chemin_systeme::existe(chemin_fichier_ri)) {
        std::cerr << "Fichier '" << argv[1] << "' inconnu.";
        return 1;
    }

    auto texte = charge_contenu_fichier(
        {chemin_fichier_ri.pointeur(), chemin_fichier_ri.taille()});

    auto tampon = lng::tampon_source(texte.c_str());

    Enchaineuse enchaineuse;

    kuri::chaine texte_source;
    kuri::chaine texte_résultat;

    for (auto i = 0; i < tampon.nombre_lignes(); i++) {
        auto ligne = tampon[i];

        if (ligne.taille() > 4) {
            auto sous_chaine = dls::vue_chaine(ligne.begin(), 4);
            if (sous_chaine == "----") {
                texte_source = supprime_espaces_blanches_autour(enchaineuse.chaine());
                enchaineuse.réinitialise();
                continue;
            }
        }

        enchaineuse << ligne;
    }

    texte_résultat = supprime_espaces_blanches_autour(enchaineuse.chaine());

    Fichier fichier;
    fichier.tampon_ = lng::tampon_source(dls::chaine(texte_source.begin(), texte_source.end()));
    fichier.chemin_ = "";

    ArgumentsCompilatrice arguments;
    arguments.importe_kuri = false;
    auto compilatrice = Compilatrice("", arguments);

    auto contexte_lexage = ContexteLexage{
        compilatrice.gérante_chaine, compilatrice.table_identifiants, imprime_erreur};

    Lexeuse lexeuse(contexte_lexage, &fichier);
    lexeuse.performe_lexage();

    if (lexeuse.possède_erreur()) {
        return 1;
    }

    PrésyntaxeuseRI pré_syntaxeuse(&fichier);
    pré_syntaxeuse.analyse();

    if (pré_syntaxeuse.possède_erreur()) {
        return 1;
    }

    SyntaxeuseRI syntaxeuse(
        &fichier, compilatrice.typeuse, *compilatrice.registre_ri, pré_syntaxeuse);
    syntaxeuse.analyse();

    //    POUR (syntaxeuse.donne_fonctions()) {
    //        dbg() << imprime_fonction(it);
    //    }

    auto contexte_analyse = ContexteAnalyseRI();

    POUR (syntaxeuse.donne_fonctions()) {
        contexte_analyse.analyse_ri(
            *compilatrice.espace_de_travail_defaut, syntaxeuse.donne_constructrice(), it);

        auto résultat = supprime_espaces_blanches_autour(imprime_fonction(it));

        if (résultat != texte_résultat) {
            dbg() << "Erreur : différence dans la sortie\n";
            dbg() << "Obtenu :\n";
            dbg() << résultat;
            dbg() << "Voulu :\n";
            dbg() << texte_résultat;
            return 1;
        }
    }

    return 0;
}
