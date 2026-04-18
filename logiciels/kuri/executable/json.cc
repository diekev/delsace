/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2026 Kévin Dietrich. */

#include "json.hh"

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"

#include "parsage/lexeuse.hh"

#include "utilitaires/log.hh"

namespace tori {

const char *chaine_type(type_objet type)
{
    switch (type) {
        case type_objet::NUL:
        {
            return "NUL";
        }
        case type_objet::DICTIONNAIRE:
        {
            return "DICTIONNAIRE";
        }
        case type_objet::TABLEAU:
        {
            return "TABLEAU";
        }
        case type_objet::CHAINE:
        {
            return "CHAINE";
        }
        case type_objet::NOMBRE_ENTIER:
        {
            return "NOMBRE_ENTIER";
        }
        case type_objet::NOMBRE_REEL:
        {
            return "NOMBRE_REEL";
        }
    }

    return "INVALIDE";
}

/* ************************************************************************** */

static void detruit_objet(Objet *objet)
{
    switch (objet->type) {
        case type_objet::NUL:
        {
            delete objet;
            break;
        }
        case type_objet::DICTIONNAIRE:
        {
            delete static_cast<ObjetDictionnaire *>(objet);
            break;
        }
        case type_objet::TABLEAU:
        {
            delete static_cast<ObjetTableau *>(objet);
            break;
        }
        case type_objet::CHAINE:
        {
            delete static_cast<ObjetChaine *>(objet);
            break;
        }
        case type_objet::NOMBRE_ENTIER:
        {
            delete static_cast<ObjetNombreEntier *>(objet);
            break;
        }
        case type_objet::NOMBRE_REEL:
        {
            delete static_cast<ObjetNombreReel *>(objet);
            break;
        }
    }
}

std::shared_ptr<Objet> construit_objet(type_objet type)
{
    auto objet = static_cast<Objet *>(nullptr);

    switch (type) {
        case type_objet::NUL:
            objet = new Objet{};
            break;
        case type_objet::DICTIONNAIRE:
            objet = new ObjetDictionnaire{};
            break;
        case type_objet::TABLEAU:
            objet = new ObjetTableau{};
            break;
        case type_objet::CHAINE:
            objet = new ObjetChaine{};
            break;
        case type_objet::NOMBRE_ENTIER:
            objet = new ObjetNombreEntier{};
            break;
        case type_objet::NOMBRE_REEL:
            objet = new ObjetNombreReel{};
            break;
    }

    objet->type = type;

    return std::shared_ptr<Objet>(objet, detruit_objet);
}

std::shared_ptr<Objet> construit_objet(long v)
{
    auto objet = std::make_shared<ObjetNombreEntier>();
    objet->valeur = v;
    objet->type = type_objet::NOMBRE_ENTIER;
    return objet;
}

std::shared_ptr<Objet> construit_objet(double v)
{
    auto objet = std::make_shared<ObjetNombreReel>();
    objet->valeur = v;
    objet->type = type_objet::NOMBRE_REEL;
    return objet;
}

std::shared_ptr<Objet> construit_objet(kuri::chaine const &v)
{
    auto objet = std::make_shared<ObjetChaine>();
    objet->valeur = v;
    objet->type = type_objet::CHAINE;
    return objet;
}

std::shared_ptr<Objet> construit_objet(char const *v)
{
    auto objet = std::make_shared<ObjetChaine>();
    objet->valeur = v;
    objet->type = type_objet::CHAINE;
    return objet;
}

/* ************************************************************************** */

static Objet *cherche_propriete(ObjetDictionnaire *dico, kuri::chaine const &nom, type_objet type)
{
    auto objet = dico->objet(nom);

    if (objet == nullptr) {
#if 0
    std::cerr << "La propriété « " << nom << " » n'existe pas !\n";
#endif
        return nullptr;
    }

    if (objet->type != type) {
#if 0
    std::cerr << "La propriété « " << nom << " » n'est pas de type « "
              << chaine_type(type) << " » (mais de type « "
              << chaine_type(objet->type) << " ») !\n";
#endif
        return nullptr;
    }

    return objet;
}

ObjetChaine *cherche_chaine(ObjetDictionnaire *dico, const kuri::chaine &nom)
{
    auto objet = cherche_propriete(dico, nom, type_objet::CHAINE);

    if (objet == nullptr) {
        return nullptr;
    }

    return extrait_chaine(objet);
}

ObjetDictionnaire *cherche_dico(ObjetDictionnaire *dico, const kuri::chaine &nom)
{
    auto objet = cherche_propriete(dico, nom, type_objet::DICTIONNAIRE);

    if (objet == nullptr) {
        return nullptr;
    }

    return extrait_dictionnaire(objet);
}

ObjetNombreEntier *cherche_nombre_entier(ObjetDictionnaire *dico, const kuri::chaine &nom)
{
    auto objet = cherche_propriete(dico, nom, type_objet::NOMBRE_ENTIER);

    if (objet == nullptr) {
        return nullptr;
    }

    return extrait_nombre_entier(objet);
}

ObjetNombreReel *cherche_nombre_reel(ObjetDictionnaire *dico, const kuri::chaine &nom)
{
    auto objet = cherche_propriete(dico, nom, type_objet::NOMBRE_REEL);

    if (objet == nullptr) {
        return nullptr;
    }

    return extrait_nombre_reel(objet);
}

ObjetTableau *cherche_tableau(ObjetDictionnaire *dico, const kuri::chaine &nom)
{
    auto objet = cherche_propriete(dico, nom, type_objet::TABLEAU);

    if (objet == nullptr) {
        return nullptr;
    }

    return extrait_tableau(objet);
}

} /* namespace tori */

namespace json {

const char *chaine_identifiant(id_morceau id)
{
    switch (id) {
        case id_morceau::PARENTHESE_OUVRANTE:
            return "id_morceau::PARENTHESE_OUVRANTE";
        case id_morceau::PARENTHESE_FERMANTE:
            return "id_morceau::PARENTHESE_FERMANTE";
        case id_morceau::VIRGULE:
            return "id_morceau::VIRGULE";
        case id_morceau::DOUBLE_POINTS:
            return "id_morceau::DOUBLE_POINTS";
        case id_morceau::CROCHET_OUVRANT:
            return "id_morceau::CROCHET_OUVRANT";
        case id_morceau::CROCHET_FERMANT:
            return "id_morceau::CROCHET_FERMANT";
        case id_morceau::ACCOLADE_OUVRANTE:
            return "id_morceau::ACCOLADE_OUVRANTE";
        case id_morceau::ACCOLADE_FERMANTE:
            return "id_morceau::ACCOLADE_FERMANTE";
        case id_morceau::CHAINE_CARACTERE:
            return "id_morceau::CHAINE_CARACTERE";
        case id_morceau::NOMBRE_ENTIER:
            return "id_morceau::NOMBRE_ENTIER";
        case id_morceau::NOMBRE_REEL:
            return "id_morceau::NOMBRE_REEL";
        case id_morceau::NOMBRE_BINAIRE:
            return "id_morceau::NOMBRE_BINAIRE";
        case id_morceau::NOMBRE_OCTAL:
            return "id_morceau::NOMBRE_OCTAL";
        case id_morceau::NOMBRE_HEXADECIMAL:
            return "id_morceau::NOMBRE_HEXADECIMAL";
        case id_morceau::INCONNU:
            return "id_morceau::INCONNU";
    };

    return "ERREUR";
}

/* ************************************************************************** */

static void imprime_tab(std::ostream &os, int tab)
{
    for (int i = 0; i < tab; ++i) {
        os << ' ' << ' ';
    }
}

void imprime_arbre(tori::Objet *racine, int tab, std::ostream &os)
{
    switch (racine->type) {
        case tori::type_objet::NUL:
        {
            os << "nul,\n";
            break;
        }
        case tori::type_objet::DICTIONNAIRE:
        {
            imprime_tab(os, tab);
            os << "{\n";

            // auto obj_dico = static_cast<tori::ObjetDictionnaire *>(racine);

            // for (auto &paire : obj_dico->valeur) {
            //     imprime_tab(os, tab + 1);
            //     os << '"' << paire.first << '"' << " : ";
            //     imprime_arbre(paire.second.get(), tab + 1, os);
            // }

            imprime_tab(os, tab);
            os << "},\n";

            break;
        }
        case tori::type_objet::TABLEAU:
        {
            os << "[\n";

            auto obj_tabl = static_cast<tori::ObjetTableau *>(racine);

            for (auto &obj : obj_tabl->valeur) {
                imprime_arbre(obj.get(), tab + 1, os);
            }

            imprime_tab(os, tab);
            os << "],\n";
            break;
        }
        case tori::type_objet::CHAINE:
        {
            auto obj = static_cast<tori::ObjetChaine *>(racine);
            os << '"' << obj->valeur << '"' << ",\n";
            break;
        }
        case tori::type_objet::NOMBRE_ENTIER:
        {
            auto obj = static_cast<tori::ObjetNombreEntier *>(racine);
            os << obj->valeur << ",\n";
            break;
        }
        case tori::type_objet::NOMBRE_REEL:
        {
            auto obj = static_cast<tori::ObjetNombreReel *>(racine);
            os << obj->valeur << ",\n";
            break;
        }
    }
}

/* ************************************************************************** */

analyseuse_grammaire::analyseuse_grammaire(Fichier *fichier) : BaseSyntaxeuse(fichier)
{
}

analyseuse_grammaire::~analyseuse_grammaire()
{
}

void analyseuse_grammaire::analyse_une_chose()
{
    if (m_lexèmes.taille() == 0) {
        return;
    }

    consomme(GenreLexème::ACCOLADE_OUVRANTE);
    analyse_objet();
    consomme(GenreLexème::ACCOLADE_FERMANTE);
}

assembleuse_objet::ptr_objet analyseuse_grammaire::objet() const
{
    return m_assembleuse.racine;
}

void analyseuse_grammaire::analyse_objet()
{
    CONSOMME_IDENTIFIANT_VOID(nom_objet, "Attendu une chaine de caractères");
    consomme(GenreLexème::DOUBLE_POINTS);

    analyse_valeur(lexème_nom_objet->chaine);

    if (apparie(GenreLexème::VIRGULE)) {
        consomme();
        analyse_objet();
    }
}

void analyseuse_grammaire::analyse_valeur(kuri::chaine_statique nom_objet)
{
    auto lexème = lexème_courant();
    switch (lexème->genre) {
        case GenreLexème::ACCOLADE_OUVRANTE:
        {
            consomme();

            auto obj = m_assembleuse.cree_objet(nom_objet, tori::type_objet::DICTIONNAIRE);
            m_assembleuse.empile_objet(obj);

            analyse_objet();

            m_assembleuse.depile_objet();

            consomme(GenreLexème::ACCOLADE_FERMANTE);
            break;
        }
        case GenreLexème::CROCHET_OUVRANT:
        {
            consomme();

            auto obj = m_assembleuse.cree_objet(nom_objet, tori::type_objet::TABLEAU);
            m_assembleuse.empile_objet(obj);

            while (true) {
                if (apparie(GenreLexème::CROCHET_FERMANT)) {
                    break;
                }

                analyse_valeur("");

                if (apparie(GenreLexème::VIRGULE)) {
                    consomme();
                }
            }

            m_assembleuse.depile_objet();

            consomme();

            break;
        }
        case GenreLexème::NOMBRE_ENTIER:
        {
            consomme();

            auto obj = m_assembleuse.cree_objet(nom_objet, tori::type_objet::NOMBRE_ENTIER);
            auto obj_chaine = static_cast<tori::ObjetNombreEntier *>(obj.get());
            obj_chaine->valeur = long(lexème->valeur_entiere);

            break;
        }
        case GenreLexème::NOMBRE_RÉEL:
        {
            consomme();

            auto obj = m_assembleuse.cree_objet(nom_objet, tori::type_objet::NOMBRE_REEL);
            auto obj_chaine = static_cast<tori::ObjetNombreReel *>(obj.get());
            obj_chaine->valeur = lexème->valeur_reelle;

            break;
        }
        case GenreLexème::CHAINE_CARACTERE:
        {
            consomme();

            auto obj = m_assembleuse.cree_objet(nom_objet, tori::type_objet::CHAINE);
            auto obj_chaine = static_cast<tori::ObjetChaine *>(obj.get());

            obj_chaine->valeur = lexème->chaine;

            break;
        }
        default:
        {
            consomme();
            rapporte_erreur("Élément inattendu");
        }
    }
}

void analyseuse_grammaire::gère_erreur_rapportée(kuri::chaine_statique message_erreur,
                                                 Lexème const * /*lexème*/)
{
    dbg() << message_erreur;
}

assembleuse_objet::assembleuse_objet()
    : racine(tori::construit_objet(tori::type_objet::DICTIONNAIRE))
{
    empile_objet(racine);
}

assembleuse_objet::ptr_objet assembleuse_objet::cree_objet(kuri::chaine_statique nom,
                                                           tori::type_objet type)
{
    auto objet = tori::construit_objet(type);

    if (!objets.est_vide()) {
        auto objet_haut = objets.haut().get();

        if (objet_haut->type == tori::type_objet::DICTIONNAIRE) {
            auto dico = static_cast<tori::ObjetDictionnaire *>(objet_haut);
            dico->insere(nom, objet);
        }
        else {
            auto tabl = static_cast<tori::ObjetTableau *>(objet_haut);
            tabl->ajoute(objet);
        }
    }

    return objet;
}

void assembleuse_objet::empile_objet(assembleuse_objet::ptr_objet objet)
{
    assert(objet->type == tori::type_objet::DICTIONNAIRE ||
           objet->type == tori::type_objet::TABLEAU);
    objets.empile(objet);
}

void assembleuse_objet::depile_objet()
{
    objets.dépile();
}

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

std::shared_ptr<tori::Objet> compile_script(const char *chemin)
{
    auto texte_source = charge_contenu_fichier(chemin);

    Fichier fichier;
    fichier.tampon_ = TamponSource(texte_source);
    fichier.chemin_ = "";

    ArgumentsCompilatrice arguments;
    arguments.importe_kuri = false;
    auto compilatrice = Compilatrice("", arguments);
    auto espace = EspaceDeTravail(compilatrice, {}, "");

    auto contexte_lexage = ContexteLexage{
        compilatrice.gérante_chaine, compilatrice.table_identifiants, imprime_erreur};

    Lexeuse lexeuse(contexte_lexage, &fichier);
    lexeuse.performe_lexage();

    if (lexeuse.possède_erreur()) {
        return nullptr;
    }

    analyseuse_grammaire syntaxeuse(&fichier);
    syntaxeuse.analyse();

    return syntaxeuse.objet();
}

} /* namespace json */
