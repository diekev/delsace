/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <any>

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/phys/couleur.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

namespace danjo {

enum class TypePropriete : int {
    ENTIER,
    DECIMAL,
    VECTEUR_DECIMAL,
    VECTEUR_ENTIER,
    COULEUR,
    FICHIER_ENTREE,
    FICHIER_SORTIE,
    CHAINE_CARACTERE,
    BOOL,
    ENUM,
    COURBE_COULEUR,
    COURBE_VALEUR,
    RAMPE_COULEUR,
    TEXTE,
    LISTE,
    LISTE_MANIP,
};

enum etat_propriete : char {
    VIERGE = 0,
    EST_ANIMEE = (1 << 0),
    EST_ANIMABLE = (1 << 1),
    EST_ACTIVEE = (1 << 2),
    EST_ACTIVABLE = (1 << 3),
    EST_VERROUILLEE = (1 << 4),
};

DEFINIS_OPERATEURS_DRAPEAU(etat_propriete)

template <typename T>
struct plage_valeur {
    T min{};
    T max{};
};

/* ------------------------------------------------------------------------- */
/** \name BasePropriété
 *  Type de base pour toutes les propriétés.
 * \{ */

class BasePropriete {
  public:
    virtual ~BasePropriete() = default;

    virtual TypePropriete type() const = 0;

    /* Définit si la propriété est ajoutée par l'utilisateur. */
    virtual bool est_extra() const = 0;

    /* Définit si la propriété est visible. */
    virtual void definit_visibilité(bool ouinon) = 0;
    virtual bool est_visible() const = 0;

    virtual std::string donnne_infobulle() const = 0;
    virtual std::string donnne_suffixe() const = 0;

    virtual int donne_dimensions_vecteur() const = 0;

    /* Évaluation des valeurs. */
    virtual bool evalue_bool(int temps) const = 0;
    virtual int evalue_entier(int temps) const = 0;
    virtual float evalue_decimal(int temps) const = 0;
    virtual void evalue_vecteur_décimal(int temps, float *données) const = 0;
    virtual void evalue_vecteur_entier(int temps, int *données) const = 0;
    virtual dls::phys::couleur32 evalue_couleur(int temps) const = 0;
    virtual std::string evalue_chaine(int temps) const = 0;
    virtual std::string evalue_énum(int temps) const = 0;

    /* Définition des valeurs. */
    virtual void définis_valeur_entier(int valeur) = 0;
    virtual void définis_valeur_décimal(float valeur) = 0;
    virtual void définis_valeur_bool(bool valeur) = 0;
    virtual void définis_valeur_vec3(dls::math::vec3f valeur) = 0;
    virtual void définis_valeur_vec3(dls::math::vec3i valeur) = 0;
    virtual void définis_valeur_couleur(dls::phys::couleur32 valeur) = 0;
    virtual void définis_valeur_chaine(std::string const &valeur) = 0;
    virtual void définis_valeur_énum(std::string const &valeur) = 0;

    /* Plage des valeurs. */
    virtual plage_valeur<float> plage_valeur_decimal() const = 0;
    virtual plage_valeur<int> plage_valeur_entier() const = 0;
    virtual plage_valeur<float> plage_valeur_vecteur_décimal() const = 0;
    virtual plage_valeur<int> plage_valeur_vecteur_entier() const = 0;
    virtual plage_valeur<float> plage_valeur_couleur() const = 0;

    /* Animation des valeurs. */
    virtual void ajoute_cle(const int v, int temps) = 0;
    virtual void ajoute_cle(const float v, int temps) = 0;
    virtual void ajoute_cle(const dls::math::vec3f &v, int temps) = 0;
    virtual void ajoute_cle(const dls::math::vec3i &v, int temps) = 0;
    virtual void ajoute_cle(const dls::phys::couleur32 &v, int temps) = 0;

    virtual void supprime_animation() = 0;

    virtual bool est_animee() const = 0;
    virtual bool est_animable() const = 0;

    virtual bool possede_cle(int temps) const = 0;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Propriété
 *  Implémentation par défaut d'une propriété.
 * \{ */

struct Propriete : public BasePropriete {
    std::any valeur{};
    TypePropriete type_{};

    union {
        float f;
        int i;
    } valeur_min{};

    union {
        float f;
        int i;
    } valeur_max{};

    etat_propriete etat = etat_propriete::VIERGE;

    bool est_extra_ = false;

    bool visible = true;

    bool pad[2];

    dls::tableau<std::pair<int, std::any>> courbe{};

    dls::chaine infobulle{};
    dls::chaine suffixe{};

    static Propriete *cree(TypePropriete type);

    static Propriete *cree(TypePropriete type, std::any valeur_);

    TypePropriete type() const override
    {
        return type_;
    }

    bool est_extra() const override
    {
        return est_extra_;
    }

    void definit_visibilité(bool ouinon) override
    {
        visible = ouinon;
    }

    bool est_visible() const override
    {
        return visible;
    }

    bool est_animee() const override;

    bool est_animable() const override
    {
        return (etat & etat_propriete::EST_ANIMABLE) == etat_propriete::VIERGE;
    }

    std::string donnne_infobulle() const override;
    std::string donnne_suffixe() const override;

    int donne_dimensions_vecteur() const override
    {
        return 3;
    }

    /* Évaluation des valeurs. */
    bool evalue_bool(int temps) const override;
    int evalue_entier(int temps) const override;
    float evalue_decimal(int temps) const override;
    void evalue_vecteur_décimal(int temps, float *données) const override;
    void evalue_vecteur_entier(int temps, int *données) const override;
    dls::phys::couleur32 evalue_couleur(int temps) const override;
    std::string evalue_chaine(int temps) const override;
    std::string evalue_énum(int temps) const override;

    /* Définition des valeurs. */
    void définis_valeur_entier(int valeur) override;
    void définis_valeur_décimal(float valeur) override;
    void définis_valeur_bool(bool valeur) override;
    void définis_valeur_vec3(dls::math::vec3f valeur) override;
    void définis_valeur_vec3(dls::math::vec3i valeur) override;
    void définis_valeur_couleur(dls::phys::couleur32 valeur) override;
    void définis_valeur_chaine(std::string const &valeur) override;
    void définis_valeur_énum(std::string const &valeur) override;

    /* Plage des valeurs. */
    plage_valeur<float> plage_valeur_decimal() const override;
    plage_valeur<int> plage_valeur_entier() const override;
    plage_valeur<float> plage_valeur_vecteur_décimal() const override;
    plage_valeur<int> plage_valeur_vecteur_entier() const override;
    plage_valeur<float> plage_valeur_couleur() const override;

    /* Animation des valeurs. */
    void ajoute_cle(const int v, int temps) override;
    void ajoute_cle(const float v, int temps) override;
    void ajoute_cle(const dls::math::vec3f &v, int temps) override;
    void ajoute_cle(const dls::math::vec3i &v, int temps) override;
    void ajoute_cle(const dls::phys::couleur32 &v, int temps) override;

    bool possede_cle(int temps) const override;

    void supprime_animation() override;

  private:
    void ajoute_cle_impl(const std::any &v, int temps);

    void tri_courbe();

    bool trouve_valeurs_temps(int temps, std::any &v1, std::any &v2, int &t1, int &t2) const;

    template <typename T>
    T évalue_valeur_impl(int temps) const;
};

/** \} */

}  // namespace danjo
