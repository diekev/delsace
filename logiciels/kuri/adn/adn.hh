/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "parsage/base_syntaxeuse.hh"
#include "parsage/lexemes.hh"

#include "structures/chaine.hh"
#include "structures/tableau.hh"
#include "structures/tableau_page.hh"

#include "outils_independants_des_lexemes.hh"

template <typename Tag>
class FluxSortie {
    std::ostream &m_os;

    template <typename T>
    friend FluxSortie &operator<<(FluxSortie &flux, const T &valeur);

  public:
    explicit FluxSortie(std::ostream &os) : m_os(os)
    {
    }

    operator std::ostream &()
    {
        return m_os;
    }
};

struct FluxSortieKuri : public FluxSortie<FluxSortieKuri> {
    using FluxSortie::FluxSortie;
};

template <typename T>
FluxSortieKuri &operator<<(FluxSortieKuri &flux, const T &valeur)
{
    static_cast<std::ostream &>(flux) << valeur;
    return flux;
}

struct FluxSortieCPP : public FluxSortie<FluxSortieCPP> {
    using FluxSortie::FluxSortie;
};

template <typename T>
FluxSortieCPP &operator<<(FluxSortieCPP &flux, const T &valeur)
{
    static_cast<std::ostream &>(flux) << valeur;
    return flux;
}

struct IdentifiantADN {
  private:
    kuri::chaine_statique m_nom = "";

  public:
    IdentifiantADN() = default;

    IdentifiantADN(kuri::chaine_statique n) : m_nom(n)
    {
    }

    kuri::chaine_statique nom() const
    {
        return m_nom;
    }

    bool est_nul() const
    {
        return m_nom == "";
    }
};

FluxSortieCPP &operator<<(FluxSortieCPP &flux, IdentifiantADN const &ident);

FluxSortieKuri &operator<<(FluxSortieKuri &flux, IdentifiantADN const &ident);

class Protéine;
struct TypeNominal;
struct TypePointeur;
struct TypeTableau;

struct Type {
    bool est_const = false;

    virtual ~Type() = default;

    virtual bool est_tableau() const
    {
        return false;
    }

    virtual const TypeTableau *comme_tableau() const
    {
        return nullptr;
    }

    virtual bool est_pointeur() const
    {
        return false;
    }

    virtual const TypePointeur *comme_pointeur() const
    {
        return nullptr;
    }

    virtual bool est_nominal() const
    {
        return false;
    }

    virtual const TypeNominal *comme_nominal() const
    {
        return nullptr;
    }

    virtual kuri::chaine_statique accesseur() const
    {
        return "";
    }

    virtual kuri::chaine_statique valeur_defaut() const
    {
        return "{}";
    }

    template <typename... TypesChaines>
    bool est_nominal(TypesChaines... chaines) const;

    const IdentifiantADN &accede_nom() const;
};

struct TypePointeur final : public Type {
    Type *type_pointe = nullptr;

    bool est_pointeur() const override
    {
        return true;
    }

    const TypePointeur *comme_pointeur() const override
    {
        return this;
    }

    kuri::chaine_statique accesseur() const override
    {
        return "->";
    }

    virtual kuri::chaine_statique valeur_defaut() const override
    {
        return "nullptr";
    }
};

struct TypeTableau final : public Type {
    Type *type_pointe = nullptr;
    bool est_compresse = false;
    bool est_synchrone = false;

    bool est_tableau() const override
    {
        return true;
    }

    const TypeTableau *comme_tableau() const override
    {
        return this;
    }

    kuri::chaine_statique accesseur() const override
    {
        return (est_synchrone ? "->" : ".");
    }
};

struct TypeNominal final : public Type {
    kuri::chaine identifiant{};
    IdentifiantADN nom_cpp{};
    IdentifiantADN nom_kuri{};

    Protéine *est_protéine = nullptr;

    bool est_nominal() const override
    {
        return true;
    }

    const TypeNominal *comme_nominal() const override
    {
        return this;
    }

    kuri::chaine_statique accesseur() const override
    {
        // À FAIRE: vérifie que nous avons une structure
        return ".";
    }

    virtual kuri::chaine_statique valeur_defaut() const override
    {
        if (nom_cpp.nom() == "bool") {
            return "false";
        }

        // chaine et chaine_statique
        if (nom_kuri.nom() == "chaine") {
            return R"("")";
        }

        if (nom_kuri.nom() == "rien") {
            return "";
        }

        return "{}";
    }
};

// Déclare ceci après TypeNominal
template <typename... TypesChaines>
bool Type::est_nominal(TypesChaines... chaines) const
{
    if (!est_nominal()) {
        return false;
    }

    const auto type_nominal = comme_nominal();
    return ((type_nominal->nom_cpp.nom() == chaines) || ...);
}

bool est_type_noeud(Type const *type);

struct Typeuse {
  private:
    kuri::tableau_page<TypePointeur> types_pointeurs{};
    kuri::tableau_page<TypeTableau> types_tableaux{};
    kuri::tableau_page<TypeNominal> types_nominaux{};

    Type *m_type_rien = nullptr;

  public:
    Typeuse()
    {
        auto type_chaine_statique = crée_type_nominal("kuri::chaine_statique", "chaine");
        type_chaine_statique->identifiant = "chaine_statique";
        auto type_chaine = crée_type_nominal("kuri::chaine", "chaine");
        type_chaine->identifiant = "chaine";
        crée_type_nominal("unsigned char", "n8");
        crée_type_nominal("uint8_t", "n8");
        crée_type_nominal("unsigned short", "n16");
        crée_type_nominal("uint16_t", "n16");
        crée_type_nominal("unsigned int", "n32");
        crée_type_nominal("uint32_t", "n32");
        crée_type_nominal("unsigned long", "n64");
        crée_type_nominal("uint64_t", "n64");
        crée_type_nominal("char", "z8");
        crée_type_nominal("int8_t", "z8");
        crée_type_nominal("short", "z16");
        crée_type_nominal("int16_t", "z16");
        crée_type_nominal("int", "z32");
        crée_type_nominal("int32_t", "z32");
        crée_type_nominal("long", "z64");
        crée_type_nominal("int64_t", "z64");
        crée_type_nominal("octet_t", "octet");
        crée_type_nominal("r16", "r16");
        crée_type_nominal("float", "r32");
        crée_type_nominal("double", "r64");
        crée_type_nominal("bool", "bool");
        m_type_rien = crée_type_nominal("void", "rien");
    }

    EMPECHE_COPIE(Typeuse);

    Type *type_rien()
    {
        return m_type_rien;
    }

    TypePointeur *crée_type_pointeur(Type *type_pointe)
    {
        POUR_TABLEAU_PAGE (types_pointeurs) {
            if (it.type_pointe == type_pointe) {
                return &it;
            }
        }

        auto résultat = types_pointeurs.ajoute_élément();
        résultat->type_pointe = type_pointe;
        return résultat;
    }

    TypeTableau *crée_type_tableau(Type *type_pointe, bool compresse, bool synchrone)
    {
        POUR_TABLEAU_PAGE (types_tableaux) {
            if (it.type_pointe == type_pointe && compresse == it.est_compresse &&
                synchrone == it.est_synchrone) {
                return &it;
            }
        }

        auto résultat = types_tableaux.ajoute_élément();
        résultat->type_pointe = type_pointe;
        résultat->est_compresse = compresse;
        résultat->est_synchrone = synchrone;
        return résultat;
    }

    TypeNominal *crée_type_nominal(kuri::chaine_statique nom_cpp)
    {
        return crée_type_nominal(nom_cpp, nom_cpp);
    }

    TypeNominal *crée_type_nominal(kuri::chaine_statique nom_cpp, kuri::chaine_statique nom_kuri)
    {
        POUR_TABLEAU_PAGE (types_nominaux) {
            if (it.identifiant == nom_cpp) {
                return &it;
            }
        }

        auto résultat = types_nominaux.ajoute_élément();
        résultat->nom_cpp = nom_cpp;
        résultat->nom_kuri = nom_kuri;
        résultat->identifiant = nom_cpp;
        return résultat;
    }
};

FluxSortieCPP &operator<<(FluxSortieCPP &os, Type const &type);

FluxSortieKuri &operator<<(FluxSortieKuri &os, Type const &type);

struct Membre {
    IdentifiantADN nom{};
    Type *type = nullptr;

    bool est_code = false;
    bool est_enfant = false;
    bool est_a_copier = false;
    /* Si vrai, ajoute "mutable" à la déclaration du membre dans le code C++. */
    bool est_mutable = false;
    /* Si vrai, ajoute un paramètre pour le membre à la fonction de création. */
    bool est_requis_pour_construction = false;

    bool valeur_defaut_est_acces = false;
    kuri::chaine_statique valeur_defaut = "";
};

class ProtéineEnum;
class ProtéineStruct;
class ProtéineFonction;

class Protéine {
  protected:
    IdentifiantADN m_nom{};

  public:
    explicit Protéine(IdentifiantADN nom);

    virtual ~Protéine() = default;
    virtual void génère_code_cpp(FluxSortieCPP &os, bool pour_entête) = 0;
    virtual void génère_code_kuri(FluxSortieKuri &os) = 0;

    virtual bool est_fonction() const
    {
        return false;
    }

    const IdentifiantADN &nom() const
    {
        return m_nom;
    }

    virtual ProtéineEnum *comme_enum()
    {
        return nullptr;
    }

    virtual ProtéineStruct *comme_struct()
    {
        return nullptr;
    }

    virtual ProtéineStruct const *comme_struct() const
    {
        return nullptr;
    }

    virtual ProtéineFonction const *comme_fonction() const
    {
        return nullptr;
    }
};

class ProtéineStruct final : public Protéine {
    kuri::tableau<Membre> m_membres{};

    ProtéineStruct *m_mere = nullptr;

    IdentifiantADN m_nom_code{};
    IdentifiantADN m_nom_genre{};
    IdentifiantADN m_nom_comme{};
    IdentifiantADN m_genre_valeur{};

    kuri::tableau<ProtéineStruct *> m_protéines_derivees{};

    bool m_possède_enfant = false;
    bool m_possède_membre_a_copier = false;
    bool m_possède_tableaux = false;

    ProtéineStruct *m_paire = nullptr;

    ProtéineEnum *m_enum_discriminante = nullptr;

  public:
    explicit ProtéineStruct(IdentifiantADN nom);

    ProtéineStruct(ProtéineStruct const &) = default;
    ProtéineStruct &operator=(ProtéineStruct const &) = default;

    void génère_code_cpp(FluxSortieCPP &os, bool pour_entête) override;

    void génère_code_cpp_apres_déclaration(FluxSortieCPP &os);

    void génère_code_kuri(FluxSortieKuri &os) override;

    void ajoute_membre(Membre const membre);

    void descend_de(ProtéineStruct *protéine)
    {
        m_mere = protéine;
        m_possède_tableaux |= m_mere->m_possède_tableaux;
        m_possède_enfant |= m_mere->m_possède_enfant;
        m_possède_membre_a_copier |= m_mere->m_possède_membre_a_copier;
        m_mere->m_protéines_derivees.ajoute(this);
    }

    ProtéineStruct *comme_struct() override
    {
        return this;
    }

    ProtéineStruct const *comme_struct() const override
    {
        return this;
    }

    ProtéineStruct *mere() const
    {
        return m_mere;
    }

    void mute_paire(ProtéineStruct *paire)
    {
        m_paire = paire;
        m_paire->m_paire = this;
        m_paire->m_nom_genre = m_nom_genre;
        m_paire->m_nom_comme = m_nom_comme;
    }

    ProtéineStruct *paire() const
    {
        return m_paire;
    }

    void mute_nom_comme(kuri::chaine_statique chaine)
    {
        m_nom_comme = chaine;

        if (m_paire) {
            m_paire->m_nom_comme = m_nom_comme;
        }
    }

    void mute_genre_valeur(kuri::chaine_statique chaine)
    {
        m_genre_valeur = chaine;
    }

    void mute_nom_code(kuri::chaine_statique chaine)
    {
        m_nom_code = chaine;
    }

    void mute_nom_genre(kuri::chaine_statique chaine)
    {
        m_nom_genre = chaine;

        if (m_paire) {
            m_paire->m_nom_genre = m_nom_genre;
        }
    }

    const IdentifiantADN &accede_nom_comme() const
    {
        return m_nom_comme;
    }

    const IdentifiantADN &accede_nom_code() const
    {
        return m_nom_code;
    }

    const IdentifiantADN &accede_nom_genre() const
    {
        return m_nom_genre;
    }

    bool est_classe_de_base() const
    {
        return !m_protéines_derivees.est_vide();
    }

    bool est_racine_hiérarchie() const
    {
        return est_classe_de_base() && m_mere == nullptr;
    }

    bool est_racine_soushierachie() const
    {
        return est_classe_de_base() && m_mere != nullptr;
    }

    bool possède_enfants() const
    {
        return m_possède_enfant;
    }

    bool possède_membre_a_copier() const
    {
        return m_possède_membre_a_copier;
    }

    const kuri::tableau<ProtéineStruct *> &derivees() const
    {
        return m_protéines_derivees;
    }

    const kuri::tableau<Membre> &membres() const
    {
        return m_membres;
    }

    bool possède_tableau() const
    {
        return m_possède_tableaux;
    }

    void mute_enum_discriminante(ProtéineEnum *enum_discriminante)
    {
        m_enum_discriminante = enum_discriminante;
    }

    ProtéineEnum *enum_discriminante()
    {
        if (m_enum_discriminante) {
            return m_enum_discriminante;
        }

        if (m_mere) {
            return m_mere->enum_discriminante();
        }

        return nullptr;
    }

    void pour_chaque_membre_recursif(std::function<void(Membre const &)> rappel);

    void pour_chaque_copie_extra_recursif(std::function<void(Membre const &)> rappel);

    void pour_chaque_enfant_recursif(std::function<void(const Membre &)> rappel);

    void pour_chaque_derivee_recursif(std::function<void(const ProtéineStruct &)> rappel);

    kuri::tableau<Membre> donne_membres_pour_construction();
};

class ProtéineEnum final : public Protéine {
    kuri::tableau<Membre> m_membres{};

    Type *m_type = nullptr;

    kuri::chaine_statique m_type_discrimine = "";

    bool m_est_horslignee = false;

  public:
    explicit ProtéineEnum(IdentifiantADN nom);

    EMPECHE_COPIE(ProtéineEnum);

    void génère_code_cpp(FluxSortieCPP &os, bool pour_entête) override;

    void génère_code_kuri(FluxSortieKuri &os) override;

    void ajoute_membre(Membre const membre);

    ProtéineEnum *comme_enum() override
    {
        return this;
    }

    Type *&type()
    {
        return m_type;
    }

    void marque_horslignee()
    {
        m_est_horslignee = true;
    }

    bool est_horslignee() const
    {
        return m_est_horslignee;
    }

    void type_discrimine(kuri::chaine_statique type_discrimine)
    {
        m_type_discrimine = type_discrimine;
    }

    kuri::chaine_statique type_discrimine() const
    {
        return m_type_discrimine;
    }

    kuri::tableau_statique<Membre> donne_membres() const
    {
        return m_membres;
    }
};

struct Parametre {
    IdentifiantADN nom{};
    Type *type = nullptr;
};

class ProtéineFonction final : public Protéine {
    kuri::tableau<Parametre> m_parametres{};
    Type *m_type_sortie = nullptr;
    bool m_est_ipa_compilatrice = false;
    bool m_est_intrinsèque = false;
    bool m_exclus_métaprogramme = false;
    kuri::chaine_statique m_genre_intrinsèque = "";
    /* Pour les intrinsèques, le symbole GCC correspondant. */
    kuri::chaine_statique m_symbole_gcc = "";

  public:
    explicit ProtéineFonction(IdentifiantADN nom);

    EMPECHE_COPIE(ProtéineFonction);

    void génère_code_cpp(FluxSortieCPP &os, bool pour_entête) override;

    void génère_code_kuri(FluxSortieKuri &os) override;

    Type *&type_sortie()
    {
        return m_type_sortie;
    }

    void ajoute_parametre(Parametre const parametre);

    bool est_fonction() const override
    {
        return true;
    }

    ProtéineFonction const *comme_fonction() const override
    {
        return this;
    }

    const kuri::tableau<Parametre> &donne_paramètres() const
    {
        return m_parametres;
    }

    Type *donne_type_sortie() const
    {
        return m_type_sortie;
    }

    kuri::chaine_statique donne_symbole_gcc() const
    {
        return m_symbole_gcc;
    }
    void définis_symbole_gcc(kuri::chaine_statique symbole)
    {
        m_symbole_gcc = symbole;
    }

    bool est_marquée_intrinsèque() const
    {
        return m_est_intrinsèque;
    }
    void marque_intrinsèque(kuri::chaine_statique genre_intrinsèque)
    {
        m_genre_intrinsèque = genre_intrinsèque;
        m_est_intrinsèque = true;
    }

    bool est_exclus_métaprogramme() const
    {
        return m_exclus_métaprogramme;
    }
    void marque_exclus_métaprogramme()
    {
        m_exclus_métaprogramme = true;
    }

    kuri::chaine_statique donne_genre_intrinsèque() const
    {
        return m_genre_intrinsèque;
    }

    bool est_marquée_ipa_compilarice() const
    {
        return m_est_ipa_compilatrice;
    }
    void marque_ipa_compilarice()
    {
        m_est_ipa_compilatrice = true;
    }
};

struct SyntaxeuseADN : public BaseSyntaxeuse {
    kuri::tableau<Protéine *> protéines{};
    kuri::tableau<Protéine *> protéines_paires{};

    Typeuse m_typeuse{};

    explicit SyntaxeuseADN(Fichier *fichier);

    ~SyntaxeuseADN() override;

    template <typename T>
    T *crée_protéine(IdentifiantADN nom)
    {
        auto protéine = new T(nom);
        protéines.ajoute(protéine);
        return protéine;
    }

  private:
    void analyse_une_chose() override;

    void parse_fonction();

    void parse_enum();

    void parse_struct();

    Type *parse_type();

    ProtéineEnum *donne_énum_pour_nom(kuri::chaine_statique nom) const;

    void gère_erreur_rapportée(kuri::chaine_statique message_erreur) override;
};

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxillaires.
 * \{ */

/** Pour le code créant des IdentifiantCodes, génère les déclarations et initialisation de ces
 * IdentifiantCodes.
 *
 * Le paramètre `identifiant_fonction` est utilisé pour créer une fonction d'initialisation appelée
 * « initialise_identifiants_{identifiant_fonction} ». Cette fonction devra être appelée
 * manuellement.
 *
 * Le code généré inclus également toutes les déclarations et inclusions de fichiers nécessaire,
 * cette fonction ne doit pas être appelé pour générer du code dans un espace de nom.
 */
void genere_déclaration_identifiants_code(const kuri::tableau<Protéine *> &protéines,
                                          FluxSortieCPP &os,
                                          bool pour_entête,
                                          kuri::chaine_statique identifiant_fonction);

void génère_déclaration_fonctions_discrimination(FluxSortieCPP &os,
                                                 IdentifiantADN const &nom_noeud,
                                                 IdentifiantADN const &nom_comme);

void génère_définition_fonctions_discrimination(FluxSortieCPP &os,
                                                kuri::chaine_statique nom_classe,
                                                ProtéineStruct const &derivee,
                                                bool pour_noeud_code);

void génère_code_cpp(FluxSortieCPP &os,
                     const kuri::tableau<Protéine *> &protéines,
                     bool pour_entête);

void génère_code_kuri(FluxSortieKuri &os, kuri::tableau<Protéine *> const &protéines);

/** \} */
