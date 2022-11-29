/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "test_decoupage.h"

#include <cstring>

#include "compilation/compilatrice.hh"

#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"

#undef DEBOGUE_MORCEAUX

template <typename I1, typename I2>
bool verifie_lexemes(I1 debut1, I1 fin1, I2 debut2, I2 fin2)
{
    auto const dist1 = std::distance(debut1, fin1);
    auto const dist2 = std::distance(debut2, fin2);

    if (dist1 != dist2) {
#ifdef DEBOGUE_MORCEAUX
        std::cerr << "Les distances ne correspondent pas : " << dist1 << " vs " << dist2 << '\n';
#endif
        return false;
    }

    while (debut1 != fin1 && debut2 != fin2) {
        if ((*debut1).genre != (*debut2).genre) {
#ifdef DEBOGUE_MORCEAUX
            std::cerr << "Les identifiants ne correspondent pas : "
                      << chaine_identifiant((*debut1).genre) << " vs "
                      << chaine_identifiant((*debut2).genre) << '\n';
#endif
            return false;
        }

        if ((*debut1).chaine != (*debut2).chaine) {
#ifdef DEBOGUE_MORCEAUX
            std::cerr << "Les chaines ne correspondent pas : " << (*debut1).chaine << " vs "
                      << (*debut2).chaine << '\n';
#endif
            return false;
        }

        ++debut1;
        ++debut2;
    }

    return true;
}

bool test_decoupage_texte1()
{
    const char *texte =
        R"(# Ceci est un commentaire
« ceci est une chaine française avec espaces »
«ceci est une chaine française sans espaces»
str='a';
str0='\0';
discr nombre {
	0...1_000: imprime(1000);
	11_000...2_0000: imprime(20000);
	sinon:imprime(inconnu);
}
Lexeuse lexeuse(str, str + len);
)";

    const Lexeme donnees_lexemes[] = {
        {" ceci est une chaine française avec espaces ", {0ull}, GenreLexeme::CHAINE_LITTERALE},
        {"\n", {0ull}, GenreLexeme::POINT_VIRGULE},
        {"ceci est une chaine française sans espaces", {0ull}, GenreLexeme::CHAINE_LITTERALE},
        {"\n", {0ull}, GenreLexeme::POINT_VIRGULE},
        {"str", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {"=", {0ull}, GenreLexeme::EGAL},
        {"a", {0ull}, GenreLexeme::CARACTERE},
        {";", {0ull}, GenreLexeme::POINT_VIRGULE},
        {"str0", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {"=", {0ull}, GenreLexeme::EGAL},
        {"\\0", {0ull}, GenreLexeme::CARACTERE},
        {";", {0ull}, GenreLexeme::POINT_VIRGULE},
        {"discr", {0ull}, GenreLexeme::DISCR},
        {"nombre", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {"{", {0ull}, GenreLexeme::ACCOLADE_OUVRANTE},
        {"0", {0ull}, GenreLexeme::NOMBRE_ENTIER},
        {"...", {0ull}, GenreLexeme::TROIS_POINTS},
        {"1_000", {0ull}, GenreLexeme::NOMBRE_ENTIER},
        {":", {0ull}, GenreLexeme::DOUBLE_POINTS},
        {"imprime", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {"(", {0ull}, GenreLexeme::PARENTHESE_OUVRANTE},
        {"1000", {0ull}, GenreLexeme::NOMBRE_ENTIER},
        {")", {0ull}, GenreLexeme::PARENTHESE_FERMANTE},
        {";", {0ull}, GenreLexeme::POINT_VIRGULE},
        {"11_000", {0ull}, GenreLexeme::NOMBRE_ENTIER},
        {"...", {0ull}, GenreLexeme::TROIS_POINTS},
        {"2_0000", {0ull}, GenreLexeme::NOMBRE_ENTIER},
        {":", {0ull}, GenreLexeme::DOUBLE_POINTS},
        {"imprime", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {"(", {0ull}, GenreLexeme::PARENTHESE_OUVRANTE},
        {"20000", {0ull}, GenreLexeme::NOMBRE_ENTIER},
        {")", {0ull}, GenreLexeme::PARENTHESE_FERMANTE},
        {";", {0ull}, GenreLexeme::POINT_VIRGULE},
        {"sinon", {0ull}, GenreLexeme::SINON},
        {":", {0ull}, GenreLexeme::DOUBLE_POINTS},
        {"imprime", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {"(", {0ull}, GenreLexeme::PARENTHESE_OUVRANTE},
        {"inconnu", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {")", {0ull}, GenreLexeme::PARENTHESE_FERMANTE},
        {";", {0ull}, GenreLexeme::POINT_VIRGULE},
        {"}", {0ull}, GenreLexeme::ACCOLADE_FERMANTE},
        {"lexeuse_texte", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {"lexeuse", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {"(", {0ull}, GenreLexeme::PARENTHESE_OUVRANTE},
        {"str", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {",", {0ull}, GenreLexeme::VIRGULE},
        {"str", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {"+", {0ull}, GenreLexeme::PLUS},
        {"len", {0ull}, GenreLexeme::CHAINE_CARACTERE},
        {")", {0ull}, GenreLexeme::PARENTHESE_FERMANTE},
        {";", {0ull}, GenreLexeme::POINT_VIRGULE}};

    auto donnees_fichier = Fichier();
    donnees_fichier.charge_tampon(lng::tampon_source(texte));

    auto compilatrice = Compilatrice("", {});

    Lexeuse lexeuse(compilatrice.contexte_lexage(nullptr), &donnees_fichier);
    lexeuse.performe_lexage();

    return verifie_lexemes(donnees_fichier.lexemes.begin(),
                           donnees_fichier.lexemes.end(),
                           std::begin(donnees_lexemes),
                           std::end(donnees_lexemes));
}

void test_decoupage(dls::test_unitaire::Controleuse &controleuse)
{
    CU_VERIFIE_CONDITION(controleuse, test_decoupage_texte1());
}
