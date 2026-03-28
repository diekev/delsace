/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2026 Kévin Dietrich. */

#pragma once

#include <memory>

#include "parsage/base_syntaxeuse.hh"

#include "structures/chaine.hh"
#include "structures/pile.hh"
#include "structures/table_hachage.hh"

namespace tori {

enum type_objet : unsigned {
    NUL,
    DICTIONNAIRE,
    TABLEAU,
    CHAINE,
    NOMBRE_ENTIER,
    NOMBRE_REEL,
};

const char *chaine_type(type_objet type);

/* ************************************************************************** */

struct Objet {
    type_objet type = type_objet::NUL;
    int pad{};
};

template <typename T>
std::shared_ptr<Objet> construit_objet(T const &valeur);

std::shared_ptr<Objet> construit_objet(long v);

std::shared_ptr<Objet> construit_objet(double v);

std::shared_ptr<Objet> construit_objet(kuri::chaine const &v);
std::shared_ptr<Objet> construit_objet(char const *v);

struct ObjetDictionnaire final : public Objet {
    kuri::table_hachage<kuri::chaine, std::shared_ptr<Objet>> valeur{""};

    ObjetDictionnaire() : Objet{type_objet::DICTIONNAIRE}
    {
    }

    template <typename T>
    void insere(kuri::chaine_statique cle, T const &v)
    {
        valeur.insère(cle, construit_objet(v));
    }

    void insere(kuri::chaine_statique cle, std::shared_ptr<Objet> const &v)
    {
        valeur.insère(cle, v);
    }

    bool possede(kuri::chaine_statique cle) const
    {
        return valeur.possède(cle);
    }

    bool possede(kuri::tableau<kuri::chaine> const &cles) const
    {
        for (auto const &cle : cles) {
            if (!possede(cle)) {
                return false;
            }
        }

        return true;
    }

    Objet *objet(kuri::chaine const &cle) const
    {
        bool trouvé = false;
        auto v = valeur.trouve(cle, trouvé);
        if (!trouvé) {
            return nullptr;
        }
        return v.get();
    }
};

struct ObjetTableau final : public Objet {
    kuri::tableau<std::shared_ptr<Objet>> valeur{};

    ObjetTableau() : Objet{type_objet::TABLEAU}
    {
    }

    template <typename... Args>
    static std::shared_ptr<Objet> construit(Args &&...args)
    {
        auto tableau = std::make_shared<ObjetTableau>();
        (tableau->ajoute(args), ...);
        return tableau;
    }

    template <typename T>
    void ajoute(T const &v)
    {
        valeur.ajoute(construit_objet(v));
    }

    void ajoute(std::shared_ptr<Objet> const &v)
    {
        valeur.ajoute(v);
    }
};

struct ObjetChaine final : public Objet {
    kuri::chaine valeur{};

    ObjetChaine() : Objet{type_objet::CHAINE}
    {
    }
};

struct ObjetNombreEntier final : public Objet {
    long valeur{};

    ObjetNombreEntier() : Objet{type_objet::NOMBRE_ENTIER}
    {
    }
};

struct ObjetNombreReel final : public Objet {
    double valeur{};

    ObjetNombreReel() : Objet{type_objet::NOMBRE_REEL}
    {
    }
};

std::shared_ptr<Objet> construit_objet(type_objet type);

/* ************************************************************************** */

inline auto extrait_dictionnaire(Objet *objet)
{
    assert(objet->type == type_objet::DICTIONNAIRE);
    return static_cast<ObjetDictionnaire *>(objet);
}

inline auto extrait_dictionnaire(Objet const *objet)
{
    assert(objet->type == type_objet::DICTIONNAIRE);
    return static_cast<ObjetDictionnaire const *>(objet);
}

inline auto extrait_tableau(Objet *objet)
{
    assert(objet->type == type_objet::TABLEAU);
    return static_cast<ObjetTableau *>(objet);
}

inline auto extrait_tableau(Objet const *objet)
{
    assert(objet->type == type_objet::TABLEAU);
    return static_cast<ObjetTableau const *>(objet);
}

inline auto extrait_chaine(Objet *objet)
{
    assert(objet->type == type_objet::CHAINE);
    return static_cast<ObjetChaine *>(objet);
}

inline auto extrait_chaine(Objet const *objet)
{
    assert(objet->type == type_objet::CHAINE);
    return static_cast<ObjetChaine const *>(objet);
}

inline auto extrait_nombre_entier(Objet *objet)
{
    assert(objet->type == type_objet::NOMBRE_ENTIER);
    return static_cast<ObjetNombreEntier *>(objet);
}

inline auto extrait_nombre_entier(Objet const *objet)
{
    assert(objet->type == type_objet::NOMBRE_ENTIER);
    return static_cast<ObjetNombreEntier const *>(objet);
}

inline auto extrait_nombre_reel(Objet *objet)
{
    assert(objet->type == type_objet::NOMBRE_REEL);
    return static_cast<ObjetNombreReel *>(objet);
}

inline auto extrait_nombre_reel(Objet const *objet)
{
    assert(objet->type == type_objet::NOMBRE_REEL);
    return static_cast<ObjetNombreReel const *>(objet);
}

/* ************************************************************************** */

ObjetChaine *cherche_chaine(ObjetDictionnaire *dico, kuri::chaine const &nom);

ObjetDictionnaire *cherche_dico(ObjetDictionnaire *dico, kuri::chaine const &nom);

ObjetNombreEntier *cherche_nombre_entier(ObjetDictionnaire *dico, kuri::chaine const &nom);

ObjetNombreReel *cherche_nombre_reel(ObjetDictionnaire *dico, kuri::chaine const &nom);

ObjetTableau *cherche_tableau(ObjetDictionnaire *dico, kuri::chaine const &nom);

} /* namespace tori */

namespace json {

enum class id_morceau : unsigned int {
    PARENTHESE_OUVRANTE,
    PARENTHESE_FERMANTE,
    VIRGULE,
    DOUBLE_POINTS,
    CROCHET_OUVRANT,
    CROCHET_FERMANT,
    ACCOLADE_OUVRANTE,
    ACCOLADE_FERMANTE,
    CHAINE_CARACTERE,
    NOMBRE_ENTIER,
    NOMBRE_REEL,
    NOMBRE_BINAIRE,
    NOMBRE_OCTAL,
    NOMBRE_HEXADECIMAL,
    INCONNU,
};

struct DonneesMorceau {
    using type = id_morceau;
    static constexpr type INCONNU = id_morceau::INCONNU;

    kuri::chaine_statique chaine;
    uint64_t ligne_pos;
    id_morceau genre;
};

const char *chaine_identifiant(id_morceau id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, id_morceau &i);

struct assembleuse_objet {
    using ptr_objet = std::shared_ptr<tori::Objet>;

    ptr_objet racine{};

    kuri::pile<ptr_objet> objets{};

    assembleuse_objet();

    ptr_objet cree_objet(kuri::chaine_statique nom, tori::type_objet type);

    void empile_objet(ptr_objet objet);

    void depile_objet();
};

class analyseuse_grammaire : public BaseSyntaxeuse {
    assembleuse_objet m_assembleuse{};

  public:
    analyseuse_grammaire(Fichier *fichier);

    EMPECHE_COPIE(analyseuse_grammaire);

    ~analyseuse_grammaire() override;

    assembleuse_objet::ptr_objet objet() const;

  protected:
    void analyse_une_chose() override;

    void gère_erreur_rapportée(kuri::chaine_statique message_erreur,
                               Lexème const *lexème) override;

  private:
    void analyse_objet();
    void analyse_valeur(kuri::chaine_statique nom_objet);
};

std::shared_ptr<tori::Objet> compile_script(const char *chemin);

} /* namespace json */
