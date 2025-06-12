/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "arbre_syntaxique/assembleuse.hh"

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"
#include "compilation/syntaxeuse.hh"

#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>

#include "biblinternes/tests/test_aleatoire.hh"

namespace test_decoupage {

static int test_entree_aleatoire(const u_char *donnees, size_t taille)
{
    try {
        auto donnees_char = reinterpret_cast<const char *>(donnees);

        kuri::chaine texte;
        texte.réserve(static_cast<int64_t>(taille) + 1l);

        for (auto i = 0ul; i < taille; ++i) {
            texte.ajoute(donnees_char[i]);
        }

        auto compilatrice = Compilatrice("", {});
        auto espace = compilatrice.espace_defaut_compilation();
        auto fichier = Fichier();
        fichier.charge_tampon(TamponSource(texte));

        Lexeuse lexeuse(compilatrice.contexte_lexage(nullptr), &fichier);
        lexeuse.performe_lexage();

        auto tacheronne = Tacheronne(compilatrice);
        auto unite = UniteCompilation(espace);
        auto analyseuse = Syntaxeuse(tacheronne, &unite);

        std::ostream os(nullptr);
        analyseuse.analyse();
    }
    catch (...) {
    }

    return 0;
}

}  // namespace test_decoupage

namespace test_analyse {

static GenreLexème sequence_declaration_fonction[] = {GenreLexème::FONC,
                                                      GenreLexème::CHAINE_CARACTERE,
                                                      GenreLexème::PARENTHESE_OUVRANTE,
                                                      GenreLexème::PARENTHESE_FERMANTE,
                                                      GenreLexème::DOUBLE_POINTS,
                                                      GenreLexème::RIEN,
                                                      GenreLexème::ACCOLADE_OUVRANTE};

namespace arbre_expression {

using visiteur_arbre = std::function<void(GenreLexème)>;

static GenreLexème id_operateurs_unaire[] = {
    /* on utilise PLUS et MOINS, et non PLUS_UNAIRE et MOINS_UNAIRE, pour
     * pouvoir tester la détection des opérateurs unaires. */
    GenreLexème::PLUS,
    GenreLexème::MOINS,
    GenreLexème::FOIS_UNAIRE,
    GenreLexème::EXCLAMATION,
    GenreLexème::TILDE,
    GenreLexème::CROCHET_OUVRANT,
};

static GenreLexème id_operateurs_binaire[] = {
    GenreLexème::PLUS,           GenreLexème::MOINS,           GenreLexème::FOIS,
    GenreLexème::DIVISE,         GenreLexème::ESPERLUETTE,     GenreLexème::POURCENT,
    GenreLexème::INFERIEUR,      GenreLexème::INFERIEUR_EGAL,  GenreLexème::SUPERIEUR,
    GenreLexème::SUPERIEUR_EGAL, GenreLexème::DECALAGE_DROITE, GenreLexème::DECALAGE_GAUCHE,
    GenreLexème::DIFFÉRENCE,     GenreLexème::ESP_ESP,         GenreLexème::EGALITE,
    GenreLexème::BARRE_BARRE,    GenreLexème::BARRE,           GenreLexème::CHAPEAU,
    GenreLexème::EGAL,           GenreLexème::POINT,
};

static GenreLexème id_variables[] = {
    GenreLexème::CHAINE_CARACTERE,
    GenreLexème::NOMBRE_ENTIER,
    GenreLexème::NOMBRE_REEL,
    GenreLexème::CARACTÈRE,
};

struct expression {
    virtual ~expression();
    virtual void visite(visiteur_arbre visiteur) = 0;
};

expression::~expression()
{
}

struct variable : public expression {
    virtual void visite(visiteur_arbre visiteur) override;
};

void variable::visite(visiteur_arbre visiteur)
{
    std::random_device device{};
    std::uniform_int_distribution<int> dist{0, sizeof(*id_variables) - 1};

    visiteur(id_variables[dist(device)]);
}

struct operation_unaire : public expression {
    expression *droite{};

    virtual void visite(visiteur_arbre visiteur) override;
};

void operation_unaire::visite(visiteur_arbre visiteur)
{
    std::random_device device{};
    std::uniform_int_distribution<int> dist{0, sizeof(*id_operateurs_unaire) - 1};

    visiteur(id_operateurs_unaire[dist(device)]);
    droite->visite(visiteur);
}

struct operation_binaire : public expression {
    expression *gauche{};
    expression *droite{};

    virtual void visite(visiteur_arbre visiteur) override;
};

void operation_binaire::visite(visiteur_arbre visiteur)
{
    std::random_device device{};
    std::uniform_int_distribution<int> dist{0, sizeof(*id_operateurs_binaire) - 1};

    gauche->visite(visiteur);
    visiteur(id_operateurs_binaire[dist(device)]);
    droite->visite(visiteur);
}

struct parenthese : public expression {
    expression *centre{};

    virtual void visite(visiteur_arbre visiteur) override;
};

void parenthese::visite(visiteur_arbre visiteur)
{
    visiteur(GenreLexème::PARENTHESE_OUVRANTE);
    centre->visite(visiteur);
    visiteur(GenreLexème::PARENTHESE_FERMANTE);
}

struct appel_fonction : public expression {
    kuri::tableau<expression *> params{};

    virtual void visite(visiteur_arbre visiteur) override;
};

void appel_fonction::visite(visiteur_arbre visiteur)
{
    visiteur(GenreLexème::CHAINE_CARACTERE);
    visiteur(GenreLexème::PARENTHESE_OUVRANTE);

    for (auto enfant : params) {
        enfant->visite(visiteur);
    }

    visiteur(GenreLexème::PARENTHESE_FERMANTE);
}

struct acces_tableau : public expression {
    expression *param{};

    virtual void visite(visiteur_arbre visiteur) override;
};

void acces_tableau::visite(visiteur_arbre visiteur)
{
    visiteur(GenreLexème::CHAINE_CARACTERE);
    visiteur(GenreLexème::CROCHET_OUVRANT);
    param->visite(visiteur);
    visiteur(GenreLexème::CROCHET_FERMANT);
}

struct arbre {
    expression *racine{nullptr};
    kuri::tableau<expression *> noeuds{};
    std::random_device device{};
    std::uniform_real_distribution<double> rng{0.0, 1.0};

    ~arbre()
    {
        for (auto n : noeuds) {
            delete n;
        }
    }

    void visite(visiteur_arbre visiteur)
    {
        this->racine->visite(visiteur);
    }

    void construit_expression()
    {
        this->racine = construit_expression_ex(1.0, 0);
    }

    expression *construit_expression_ex(double prob, int profondeur)
    {
        auto p = this->rng(this->device) * prob;

        if (profondeur >= 32) {
            auto noeud = new variable{};
            this->noeuds.ajoute(noeud);
            return noeud;
        }

        if (p > 0.5) {
            auto noeud = new parenthese{};
            noeud->centre = construit_expression_ex(prob / 1.2, profondeur + 1);
            this->noeuds.ajoute(noeud);
            return noeud;
        }

        auto pi = static_cast<size_t>(this->rng(this->device) * 4);

        switch (pi) {
            case 0:
            {
                auto noeud = new variable{};
                this->noeuds.ajoute(noeud);
                return noeud;
            }
            case 1:
            {
                auto noeud = new operation_unaire{};
                noeud->droite = construit_expression_ex(prob / 1.2, profondeur + 1);
                this->noeuds.ajoute(noeud);
                return noeud;
            }
            case 2:
            {
                auto noeud = new appel_fonction{};

                auto n = static_cast<size_t>(this->rng(this->device) * 10);

                for (auto i = 0ul; i < n; ++i) {
                    /* construction d'une nouvelle expression, donc réinitialise prob */
                    auto enfant = construit_expression_ex(1.0, profondeur + 1);
                    noeud->params.ajoute(enfant);
                }

                this->noeuds.ajoute(noeud);
                return noeud;
            }
            case 3:
            {
                auto noeud = new acces_tableau{};
                noeud->param = construit_expression_ex(prob / 1.2, profondeur + 1);
                this->noeuds.ajoute(noeud);
                return noeud;
            }
            default:
            case 4:
            {
                auto noeud = new operation_binaire{};
                noeud->droite = construit_expression_ex(prob / 1.2, profondeur + 1);
                noeud->gauche = construit_expression_ex(prob / 1.2, profondeur + 1);
                this->noeuds.ajoute(noeud);
                return noeud;
            }
        }
    }
};

}  // namespace arbre_expression

static void rempli_tampon(u_char *donnees, size_t taille_tampon)
{
#if 0
	auto const max_lexemes = taille_tampon / sizeof(Lexème);

	kuri::tableau<Lexème> lexemes;
	lexemes.réserve(max_lexemes);

	auto dm = Lexème{};
	dm.chaine = "texte_test";
	dm.ligne_pos = 0ul;

	for (auto id : sequence_declaration_fonction) {
		dm.genre = id;
		lexemes.ajoute(dm);
	}

	for (auto n = lexemes.taille(); n < max_lexemes - 1; ++n) {
		auto arbre = arbre_expression::arbre{};
		arbre.construit_expression();

		auto visiteur = [&](id_lexeme id)
		{
			dm.genre = static_cast<id_lexeme>(id);
			lexemes.ajoute(dm);
		};

		arbre.visite(visiteur);

		dm.genre = id_lexeme::POINT_VIRGULE;
		lexemes.ajoute(dm);

		n += arbre.noeuds.taille();
	}

	dm.genre = id_lexeme::ACCOLADE_FERMANTE;
	lexemes.ajoute(dm);

	auto const taille_octet = sizeof(Lexème) * lexemes.taille();

	memcpy(donnees, lexemes.données(), std::min(taille_tampon, taille_octet));
#else
    auto const max_lexemes = taille_tampon / sizeof(GenreLexème);

    kuri::tableau<GenreLexème> lexemes;
    lexemes.réserve(static_cast<int64_t>(max_lexemes));

    for (auto id : sequence_declaration_fonction) {
        lexemes.ajoute(id);
    }

    for (auto n = lexemes.taille(); n < static_cast<int64_t>(max_lexemes) - 1; ++n) {
        auto arbre = arbre_expression::arbre{};
        arbre.construit_expression();

        auto visiteur = [&](GenreLexème id) { lexemes.ajoute(id); };

        arbre.visite(visiteur);

        lexemes.ajoute(GenreLexème::POINT_VIRGULE);

        n += arbre.noeuds.taille();
    }

    lexemes.ajoute(GenreLexème::ACCOLADE_FERMANTE);

    auto const taille_octet = sizeof(Lexème) * static_cast<size_t>(lexemes.taille());

    memcpy(donnees, lexemes.données(), std::min(taille_tampon, taille_octet));
#endif
}

static void rempli_tampon_aleatoire(u_char *donnees, size_t taille_tampon)
{
#if 0
	auto const max_lexemes = taille_tampon / sizeof(Lexème);

	kuri::tableau<Lexème> lexemes;
	lexemes.réserve(max_lexemes);

	std::random_device device{};
	std::uniform_int_distribution<u_char> rng{
		static_cast<int>(id_lexeme::EXCLAMATION),
		static_cast<int>(id_lexeme::INCONNU)
	};

	auto dm = Lexème{};
	dm.chaine = "texte_test";
	dm.ligne_pos = 0ul;

	for (auto id : sequence_declaration_fonction) {
		dm.genre = id;
		lexemes.ajoute(dm);
	}

	for (auto n = lexemes.taille(); n < max_lexemes - 1; ++n) {
		dm.genre = static_cast<id_lexeme>(rng(device));
		lexemes.ajoute(dm);
	}

	dm.genre = id_lexeme::ACCOLADE_FERMANTE;
	lexemes.ajoute(dm);

	auto const taille_octet = sizeof(Lexème) * lexemes.taille();

	memcpy(donnees, lexemes.données(), std::min(taille_tampon, taille_octet));
#else
    auto const max_lexemes = taille_tampon / sizeof(GenreLexème);

    std::random_device device{};
    std::uniform_int_distribution<uint16_t> rng{static_cast<int>(GenreLexème::EXCLAMATION),
                                                static_cast<int>(GenreLexème::INCONNU)};

    kuri::tableau<GenreLexème> lexemes;
    lexemes.réserve(static_cast<int64_t>(max_lexemes));

    for (auto id : sequence_declaration_fonction) {
        lexemes.ajoute(id);
    }

    for (auto n = lexemes.taille(); n < static_cast<int64_t>(max_lexemes) - 1; ++n) {
        lexemes.ajoute(static_cast<GenreLexème>(rng(device)));
    }

    lexemes.ajoute(GenreLexème::ACCOLADE_FERMANTE);

    auto const taille_octet = sizeof(Lexème) * static_cast<size_t>(lexemes.taille());

    memcpy(donnees, lexemes.données(), std::min(taille_tampon, taille_octet));
#endif
}

static int test_entree_aleatoire(const u_char *donnees, size_t taille)
{
    auto donnees_lexemes = reinterpret_cast<const GenreLexème *>(donnees);
    auto nombre_lexemes = taille / sizeof(GenreLexème);

    kuri::tableau<Lexème, int> lexemes;
    lexemes.réserve(static_cast<int>(nombre_lexemes));

    auto dm = Lexème{};
    dm.chaine = "texte_test";
    dm.fichier = 0;

    for (size_t i = 0; i < nombre_lexemes; ++i) {
        dm.genre = donnees_lexemes[i];
        lexemes.ajoute(dm);
    }

    try {
        auto compilatrice = Compilatrice("", {});
        auto tacheronne = Tacheronne(compilatrice);
        auto espace = compilatrice.espace_defaut_compilation();

        auto module = compilatrice.sys_module->trouve_ou_crée_module(ID::chaine_vide, "");
        auto résultat = compilatrice.sys_module->trouve_ou_crée_fichier(module, "", "");
        auto fichier = static_cast<Fichier *>(std::get<FichierNeuf>(résultat));
        fichier->charge_tampon(TamponSource("texte_test"));
        fichier->lexèmes = lexemes;
        fichier->fut_lexé = true;

        auto unite = UniteCompilation(espace);
        auto analyseuse = Syntaxeuse(tacheronne, &unite);

        std::ostream os(nullptr);
        analyseuse.analyse();
    }
    catch (...) {
    }

    return 0;
}

}  // namespace test_analyse

int main()
{
    dls::test_aleatoire::Testeuse testeuse;
    testeuse.ajoute_tests(
        "analyse", test_analyse::rempli_tampon, test_analyse::test_entree_aleatoire);
    testeuse.ajoute_tests(
        "analyse", test_analyse::rempli_tampon_aleatoire, test_analyse::test_entree_aleatoire);
    testeuse.ajoute_tests("decoupage", nullptr, test_decoupage::test_entree_aleatoire);

    return testeuse.performe_tests(std::cerr);
}
