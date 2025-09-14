/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include <iostream>

#include "arbre_syntaxique/assembleuse.hh"

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"

#include "parsage/lexeuse.hh"

#include "representation_intermediaire/analyse.hh"
#include "representation_intermediaire/impression.hh"
#include "representation_intermediaire/optimisations.hh"
#include "representation_intermediaire/syntaxage.hh"

#include "structures/chemin_systeme.hh"

#include "utilitaires/log.hh"

static void imprime_erreur(SiteSource site, kuri::chaine message)
{
    auto fichier = site.fichier;
    auto indice_ligne = site.indice_ligne;
    auto indice_colonne = site.indice_colonne;

    auto ligne_courante = fichier->tampon()[indice_ligne];

    Enchaineuse enchaineuse;
    enchaineuse << "Erreur : " << fichier->chemin() << ":" << indice_ligne + 1 << ":\n";
    enchaineuse << ligne_courante;

    /* La position ligne est en octet, il faut donc compter le nombre d'octets
     * de chaque point de code pour bien formater l'erreur. */
    for (auto i = 0l; i < indice_colonne;) {
        if (ligne_courante[i] == '\t') {
            enchaineuse << '\t';
        }
        else {
            enchaineuse << ' ';
        }

        i += ligne_courante.décalage_pour_caractère(i);
    }

    enchaineuse << "^~~~\n";
    enchaineuse << message;

    dbg() << enchaineuse.chaine();
}

static bool commence_par(kuri::chaine_statique chn1, kuri::chaine_statique chn2)
{
    if (chn1.taille() < chn2.taille()) {
        return false;
    }

    auto sous_chaine = chn1.sous_chaine(0, chn2.taille());
    return sous_chaine == chn2;
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

    auto const est_test_enlignage = commence_par(chemin_fichier_ri.nom_fichier(),
                                                 "test-ri-enlignage");

    auto texte = charge_contenu_fichier(chemin_fichier_ri);

    auto tampon = TamponSource(texte);

    Enchaineuse enchaineuse;

    kuri::chaine texte_source;
    kuri::chaine texte_résultat;

    for (auto i = 0; i < tampon.nombre_lignes(); i++) {
        auto ligne = tampon[i];

        if (ligne.taille() > 4) {
            auto sous_chaine = kuri::chaine_statique(ligne.begin(), 4);
            if (sous_chaine == "----") {
                texte_source = supprime_espaces_blanches_autour(enchaineuse.chaine());
                enchaineuse.réinitialise();
                continue;
            }
        }

        enchaineuse << ligne;
    }

    texte_résultat = supprime_espaces_blanches_autour(enchaineuse.chaine());

    if (texte_source.taille() == 0) {
        texte_source = texte_résultat;
    }

    Fichier fichier;
    fichier.tampon_ = TamponSource(texte_source);
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

    auto contexte_analyse = ContexteAnalyseRI();

    POUR (syntaxeuse.donne_fonctions()) {
        contexte_analyse.analyse_ri(
            *compilatrice.espace_de_travail_défaut, syntaxeuse.donne_constructrice(), it);

        if (est_test_enlignage) {
            optimise_code(
                *compilatrice.espace_de_travail_défaut, syntaxeuse.donne_constructrice(), it);

            contexte_analyse.analyse_ri(
                *compilatrice.espace_de_travail_défaut, syntaxeuse.donne_constructrice(), it);
        }

        auto résultat = supprime_espaces_blanches_autour(imprime_fonction(it));

        if (résultat != texte_résultat) {
            dbg() << "Erreur : différence dans la sortie\n";
            dbg() << "Obtenu :\n";
            dbg() << résultat;
            dbg() << "Voulu :\n";
            dbg() << texte_résultat;
            return 1;
        }

        if (est_test_enlignage) {
            break;
        }
    }

    return 0;
}
