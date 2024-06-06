/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include "ipa_danjo.hh"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QHBoxLayout>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "danjo/compilation/assembleuse_disposition.h"
#include "danjo/controles_proprietes/controle_propriete.h"
#include "danjo/controles_proprietes/donnees_controle.h"

/* ------------------------------------------------------------------------- */
/** \name ConstructriceParamètreÉnum
 * \{ */

class ConstructriceParamètreÉnum final : public DNJ_Constructrice_Parametre_Enum {
    danjo::DonneesControle &m_données_controle;

    static void ajoute_item_impl(DNJ_Constructrice_Parametre_Enum *ctrice,
                                 QT_Chaine nom,
                                 QT_Chaine valeur)
    {
        auto constructrice = static_cast<ConstructriceParamètreÉnum *>(ctrice);
        constructrice->m_données_controle.valeur_enum.ajoute(
            {nom.vers_std_string(), valeur.vers_std_string()});
    }

  public:
    ConstructriceParamètreÉnum(danjo::DonneesControle &données_controle)
        : m_données_controle(données_controle)
    {
        ajoute_item = ajoute_item_impl;
    }
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name EnveloppeParamètre
 * \{ */

static danjo::TypePropriete convertis_type_parametre(DNJ_Type_Parametre type)
{
    switch (type) {
        ENEMERE_TYPE_PARAMETRE_DANJO(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return danjo::TypePropriete::ENTIER;
}

class EnveloppeParamètre final : public danjo::BasePropriete {
    DNJ_Rappels_Enveloppe_Parametre *m_rappels = nullptr;

  public:
    EnveloppeParamètre(DNJ_Rappels_Enveloppe_Parametre *rappels) : m_rappels(rappels)
    {
    }

    EMPECHE_COPIE(EnveloppeParamètre);

    danjo::TypePropriete type() const override
    {
        if (m_rappels && m_rappels->donne_type_parametre) {
            auto résultat = m_rappels->donne_type_parametre(m_rappels);
            return convertis_type_parametre(résultat);
        }

        return danjo::TypePropriete::ENTIER;
    }

    /* Définit si la propriété est ajoutée par l'utilisateur. */
    bool est_extra() const override
    {
        if (m_rappels && m_rappels->est_extra) {
            return m_rappels->est_extra(m_rappels);
        }
        return false;
    }

    /* Définit si la propriété est visible. */
    void definit_visibilité(bool ouinon) override
    {
        if (m_rappels && m_rappels->definis_visibilite) {
            m_rappels->definis_visibilite(m_rappels, ouinon);
        }
    }
    bool est_visible() const override
    {
        if (m_rappels && m_rappels->est_visible) {
            return m_rappels->est_visible(m_rappels);
        }
        return true;
    }

    std::string donnne_infobulle() const override
    {
        if (m_rappels && m_rappels->donne_infobulle) {
            QT_Chaine résultat;
            m_rappels->donne_infobulle(m_rappels, &résultat);
            return résultat.vers_std_string();
        }
        return "";
    }

    std::string donnne_suffixe() const override
    {
        if (m_rappels && m_rappels->donnne_suffixe) {
            QT_Chaine résultat;
            m_rappels->donnne_suffixe(m_rappels, &résultat);
            return résultat.vers_std_string();
        }
        return "";
    }

    int donne_dimensions_vecteur() const override
    {
        if (m_rappels && m_rappels->donne_dimensions_vecteur) {
            return m_rappels->donne_dimensions_vecteur(m_rappels);
        }
        return 0;
    }

    void crée_valeurs_énums(danjo::DonneesControle &données_controle)
    {
        if (m_rappels && m_rappels->cree_items_enum) {
            auto constructrice = ConstructriceParamètreÉnum(données_controle);
            m_rappels->cree_items_enum(m_rappels, &constructrice);
        }
    }

    /* Évaluation des valeurs. */
    bool evalue_bool(int temps) const override
    {
        if (m_rappels && m_rappels->evalue_bool) {
            return m_rappels->evalue_bool(m_rappels, temps);
        }
        return false;
    }
    int evalue_entier(int temps) const override
    {
        if (m_rappels && m_rappels->evalue_entier) {
            return m_rappels->evalue_entier(m_rappels, temps);
        }
        return 0;
    }
    float evalue_decimal(int temps) const override
    {
        if (m_rappels && m_rappels->evalue_decimal) {
            return m_rappels->evalue_decimal(m_rappels, temps);
        }
        return 0.0f;
    }
    void evalue_vecteur_décimal(int temps, float *données) const override
    {
        if (m_rappels && m_rappels->evalue_vecteur_decimal) {
            m_rappels->evalue_vecteur_decimal(m_rappels, temps, données);
        }
    }
    void evalue_vecteur_entier(int temps, int *données) const override
    {
        if (m_rappels && m_rappels->evalue_vecteur_entier) {
            m_rappels->evalue_vecteur_entier(m_rappels, temps, données);
        }
    }
    dls::phys::couleur32 evalue_couleur(int temps) const override
    {
        dls::phys::couleur32 résultat(0.f, 0.0f, 0.0f, 1.0f);
        if (m_rappels && m_rappels->evalue_couleur) {
            m_rappels->evalue_couleur(m_rappels, temps, &résultat.r);
        }
        return résultat;
    }
    std::string evalue_chaine(int temps) const override
    {
        if (m_rappels && m_rappels->evalue_chaine) {
            QT_Chaine résultat;
            m_rappels->evalue_chaine(m_rappels, temps, &résultat);
            return résultat.vers_std_string();
        }
        return "";
    }
    std::string evalue_énum(int temps) const override
    {
        if (m_rappels && m_rappels->evalue_enum) {
            QT_Chaine résultat;
            m_rappels->evalue_enum(m_rappels, temps, &résultat);
            return résultat.vers_std_string();
        }
        return "";
    }

    /* Définition des valeurs. */
    void définis_valeur_entier(int valeur) override
    {
        if (m_rappels && m_rappels->definis_entier) {
            m_rappels->definis_entier(m_rappels, valeur);
        }
    }
    void définis_valeur_décimal(float valeur) override
    {
        if (m_rappels && m_rappels->definis_decimal) {
            m_rappels->definis_decimal(m_rappels, valeur);
        }
    }
    void définis_valeur_bool(bool valeur) override
    {
        if (m_rappels && m_rappels->definis_bool) {
            m_rappels->definis_bool(m_rappels, valeur);
        }
    }
    void définis_valeur_vec3(dls::math::vec3f valeur) override
    {
        if (m_rappels && m_rappels->definis_vecteur_decimal) {
            m_rappels->definis_vecteur_decimal(m_rappels, valeur);
        }
    }
    void définis_valeur_vec3(dls::math::vec3i valeur) override
    {
        if (m_rappels && m_rappels->definis_vecteur_entier) {
            m_rappels->definis_vecteur_entier(m_rappels, valeur);
        }
    }
    void définis_valeur_couleur(dls::phys::couleur32 valeur) override
    {
        if (m_rappels && m_rappels->definis_couleur) {
            m_rappels->definis_couleur(m_rappels, &valeur.r);
        }
    }
    void définis_valeur_chaine(std::string const &valeur) override
    {
        if (m_rappels && m_rappels->definis_chaine) {
            QT_Chaine chaine;
            chaine.caractères = const_cast<char *>(valeur.data());
            chaine.taille = int64_t(valeur.size());
            m_rappels->definis_chaine(m_rappels, &chaine);
        }
    }
    void définis_valeur_énum(std::string const &valeur) override
    {
        if (m_rappels && m_rappels->definis_enum) {
            QT_Chaine chaine;
            chaine.caractères = const_cast<char *>(valeur.data());
            chaine.taille = int64_t(valeur.size());
            m_rappels->definis_enum(m_rappels, &chaine);
        }
    }

    /* Plage des valeurs. */
    danjo::plage_valeur<float> plage_valeur_decimal() const override
    {
        auto min = -std::numeric_limits<float>::max();
        auto max = std::numeric_limits<float>::max();
        if (m_rappels && m_rappels->donne_plage_decimal) {
            m_rappels->donne_plage_decimal(m_rappels, &min, &max);
        }
        return {min, max};
    }
    danjo::plage_valeur<int> plage_valeur_entier() const override
    {
        auto min = -std::numeric_limits<int>::max();
        auto max = std::numeric_limits<int>::max();
        if (m_rappels && m_rappels->donne_plage_entier) {
            m_rappels->donne_plage_entier(m_rappels, &min, &max);
        }
        return {min, max};
    }
    danjo::plage_valeur<float> plage_valeur_vecteur_décimal() const override
    {
        auto min = -std::numeric_limits<float>::max();
        auto max = std::numeric_limits<float>::max();
        if (m_rappels && m_rappels->donne_plage_vecteur_decimal) {
            m_rappels->donne_plage_vecteur_decimal(m_rappels, &min, &max);
        }
        return {min, max};
    }
    danjo::plage_valeur<int> plage_valeur_vecteur_entier() const override
    {
        auto min = -std::numeric_limits<int>::max();
        auto max = std::numeric_limits<int>::max();
        if (m_rappels && m_rappels->donne_plage_vecteur_entier) {
            m_rappels->donne_plage_vecteur_entier(m_rappels, &min, &max);
        }
        return {min, max};
    }
    danjo::plage_valeur<float> plage_valeur_couleur() const override
    {
        auto min = -std::numeric_limits<float>::max();
        auto max = std::numeric_limits<float>::max();
        if (m_rappels && m_rappels->donne_plage_couleur) {
            m_rappels->donne_plage_couleur(m_rappels, &min, &max);
        }
        return {min, max};
    }

    /* Animation des valeurs. */
    void ajoute_cle(const int v, int temps) override
    {
        if (m_rappels && m_rappels->ajoute_image_cle_entier) {
            m_rappels->ajoute_image_cle_entier(m_rappels, v, temps);
        }
    }
    void ajoute_cle(const float v, int temps) override
    {
        if (m_rappels && m_rappels->ajoute_image_cle_decimal) {
            m_rappels->ajoute_image_cle_decimal(m_rappels, v, temps);
        }
    }
    void ajoute_cle(const dls::math::vec3f &v, int temps) override
    {
        if (m_rappels && m_rappels->ajoute_image_cle_vecteur_decimal) {
            m_rappels->ajoute_image_cle_vecteur_decimal(
                m_rappels, const_cast<float *>(&v.x), temps);
        }
    }
    void ajoute_cle(const dls::math::vec3i &v, int temps) override
    {
        if (m_rappels && m_rappels->ajoute_image_cle_vecteur_entier) {
            m_rappels->ajoute_image_cle_vecteur_entier(m_rappels, const_cast<int *>(&v.x), temps);
        }
    }
    void ajoute_cle(const dls::phys::couleur32 &v, int temps) override
    {
        if (m_rappels && m_rappels->ajoute_image_cle_couleur) {
            m_rappels->ajoute_image_cle_couleur(m_rappels, const_cast<float *>(&v.r), temps);
        }
    }

    void supprime_animation() override
    {
        if (m_rappels && m_rappels->supprime_animation) {
            m_rappels->supprime_animation(m_rappels);
        }
    }

    bool est_animee() const override
    {
        if (m_rappels && m_rappels->est_anime) {
            return m_rappels->est_anime(m_rappels);
        }
        return false;
    }
    bool est_animable() const override
    {
        if (m_rappels && m_rappels->est_animable) {
            return m_rappels->est_animable(m_rappels);
        }
        return false;
    }

    bool possede_cle(int temps) const override
    {
        if (m_rappels && m_rappels->possede_image_cle) {
            return m_rappels->possede_image_cle(m_rappels, temps);
        }
        return false;
    }
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ConstructriceListe
 * \{ */

class ConstructriceListe : public DNJ_Constructrice_Liste {
    dls::tableau<dls::chaine> &m_éléments;

    static void ajoute_élément_impl(struct DNJ_Constructrice_Liste *liste, QT_Chaine élément)
    {
        auto constructrice = static_cast<ConstructriceListe *>(liste);
        constructrice->m_éléments.ajoute(élément.vers_std_string());
    }

  public:
    ConstructriceListe(dls::tableau<dls::chaine> &chaines) : m_éléments(chaines)
    {
        ajoute_element = ajoute_élément_impl;
    }
};

/** \} */

#define ASSIGNE_RAPPEL(nom) nom = nom##_impl

class Maconne_Disposition_Ligne;
class Maconne_Disposition_Colonne;
class Maconne_Disposition_Grille;

class Maconne_Disposition_Ligne : public DNJ_Maconne_Disposition_Ligne {
    danjo::MaçonneDispositionLigne *m_maçonne = nullptr;
    dls::tableau<std::shared_ptr<Maconne_Disposition_Ligne>> m_lignes{};
    dls::tableau<std::shared_ptr<Maconne_Disposition_Colonne>> m_colonnes{};
    dls::tableau<std::shared_ptr<Maconne_Disposition_Grille>> m_grilles{};

    static DNJ_Maconne_Disposition_Ligne *débute_ligne_impl(
        DNJ_Maconne_Disposition_Ligne *instance);

    static DNJ_Maconne_Disposition_Colonne *débute_colonne_impl(
        DNJ_Maconne_Disposition_Ligne *instance);

    static DNJ_Maconne_Disposition_Grille *débute_grille_impl(
        DNJ_Maconne_Disposition_Ligne *instance);

    static void ajoute_controle_impl(DNJ_Maconne_Disposition_Ligne *instance,
                                     QT_Chaine nom,
                                     struct DNJ_Rappels_Enveloppe_Parametre *rappels_params);

    static void ajoute_etiquette_impl(DNJ_Maconne_Disposition_Ligne *instance, QT_Chaine nom);

    static void ajoute_etiquette_activable_impl(DNJ_Maconne_Disposition_Ligne *instance,
                                                QT_Chaine nom,
                                                DNJ_Rappels_Enveloppe_Parametre *rappels_params);

    static void ajoute_etiquette_propriete_impl(DNJ_Maconne_Disposition_Ligne *instance,
                                                QT_Chaine nom,
                                                DNJ_Rappels_Enveloppe_Parametre *rappels_params);

    static void ajoute_espaceur_impl(DNJ_Maconne_Disposition_Ligne *instance, int taille);

  public:
    Maconne_Disposition_Ligne(danjo::MaçonneDispositionLigne *maçonne);

    danjo::MaçonneDispositionLigne *donne_maçonne()
    {
        return m_maçonne;
    }
};

class Maconne_Disposition_Colonne : public DNJ_Maconne_Disposition_Colonne {
    danjo::MaçonneDispositionColonne *m_maçonne = nullptr;
    dls::tableau<std::shared_ptr<Maconne_Disposition_Ligne>> m_lignes{};
    dls::tableau<std::shared_ptr<Maconne_Disposition_Colonne>> m_colonnes{};
    dls::tableau<std::shared_ptr<Maconne_Disposition_Grille>> m_grilles{};

    static DNJ_Maconne_Disposition_Ligne *débute_ligne_impl(
        DNJ_Maconne_Disposition_Colonne *instance);

    static DNJ_Maconne_Disposition_Colonne *débute_colonne_impl(
        DNJ_Maconne_Disposition_Colonne *instance);

    static DNJ_Maconne_Disposition_Grille *débute_grille_impl(
        DNJ_Maconne_Disposition_Colonne *instance);

    static void ajoute_controle_impl(DNJ_Maconne_Disposition_Colonne *instance,
                                     QT_Chaine nom,
                                     struct DNJ_Rappels_Enveloppe_Parametre *rappels_params);

    static void ajoute_etiquette_impl(DNJ_Maconne_Disposition_Colonne *instance, QT_Chaine nom);

    static void ajoute_etiquette_activable_impl(DNJ_Maconne_Disposition_Colonne *instance,
                                                QT_Chaine nom,
                                                DNJ_Rappels_Enveloppe_Parametre *rappels_params);

    static void ajoute_etiquette_propriete_impl(DNJ_Maconne_Disposition_Colonne *instance,
                                                QT_Chaine nom,
                                                DNJ_Rappels_Enveloppe_Parametre *rappels_params);

    static void ajoute_espaceur_impl(DNJ_Maconne_Disposition_Colonne *instance, int taille);

  public:
    Maconne_Disposition_Colonne(danjo::MaçonneDispositionColonne *maçonne);

    danjo::MaçonneDispositionColonne *donne_maçonne()
    {
        return m_maçonne;
    }
};

class Maconne_Disposition_Grille : public DNJ_Maconne_Disposition_Grille {
    danjo::MaçonneDispositionGrille *m_maçonne = nullptr;
    dls::tableau<std::shared_ptr<Maconne_Disposition_Ligne>> m_lignes{};
    dls::tableau<std::shared_ptr<Maconne_Disposition_Colonne>> m_colonnes{};
    dls::tableau<std::shared_ptr<Maconne_Disposition_Grille>> m_grilles{};

    static DNJ_Maconne_Disposition_Ligne *débute_ligne_impl(
        DNJ_Maconne_Disposition_Grille *instance,
        int ligne,
        int colonne,
        int empan_ligne,
        int empan_colonne);

    static DNJ_Maconne_Disposition_Colonne *débute_colonne_impl(
        DNJ_Maconne_Disposition_Grille *instance,
        int ligne,
        int colonne,
        int empan_ligne,
        int empan_colonne);

    static DNJ_Maconne_Disposition_Grille *débute_grille_impl(
        DNJ_Maconne_Disposition_Grille *instance,
        int ligne,
        int colonne,
        int empan_ligne,
        int empan_colonne);

    static void ajoute_controle_impl(DNJ_Maconne_Disposition_Grille *instance,
                                     QT_Chaine nom,
                                     struct DNJ_Rappels_Enveloppe_Parametre *rappels_params,
                                     int ligne,
                                     int colonne,
                                     int empan_ligne,
                                     int empan_colonne);

    static void ajoute_etiquette_impl(DNJ_Maconne_Disposition_Grille *instance,
                                      QT_Chaine nom,
                                      int ligne,
                                      int colonne,
                                      int empan_ligne,
                                      int empan_colonne);

    static void ajoute_etiquette_activable_impl(DNJ_Maconne_Disposition_Grille *instance,
                                                QT_Chaine nom,
                                                DNJ_Rappels_Enveloppe_Parametre *rappels_params,
                                                int ligne,
                                                int colonne,
                                                int empan_ligne,
                                                int empan_colonne);

    static void ajoute_etiquette_propriete_impl(DNJ_Maconne_Disposition_Grille *instance,
                                                QT_Chaine nom,
                                                DNJ_Rappels_Enveloppe_Parametre *rappels_params,
                                                int ligne,
                                                int colonne,
                                                int empan_ligne,
                                                int empan_colonne);

    static void ajoute_espaceur_impl(DNJ_Maconne_Disposition_Grille *,
                                     int taille,
                                     int ligne,
                                     int colonne,
                                     int empan_ligne,
                                     int empan_colonne);

  public:
    Maconne_Disposition_Grille(danjo::MaçonneDispositionGrille *maçonne);

    danjo::MaçonneDispositionGrille *donne_maçonne()
    {
        return m_maçonne;
    }
};

DNJ_Maconne_Disposition_Ligne *Maconne_Disposition_Ligne::débute_ligne_impl(
    DNJ_Maconne_Disposition_Ligne *instance)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Ligne *>(instance);
    auto maçonne = inst->m_maçonne->débute_ligne();
    auto résultat = std::make_shared<Maconne_Disposition_Ligne>(maçonne);
    inst->m_lignes.ajoute(résultat);
    return résultat.get();
}

DNJ_Maconne_Disposition_Colonne *Maconne_Disposition_Ligne::débute_colonne_impl(
    DNJ_Maconne_Disposition_Ligne *instance)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Ligne *>(instance);
    auto maçonne = inst->m_maçonne->débute_colonne();
    auto résultat = std::make_shared<Maconne_Disposition_Colonne>(maçonne);
    inst->m_colonnes.ajoute(résultat);
    return résultat.get();
}

DNJ_Maconne_Disposition_Grille *Maconne_Disposition_Ligne::débute_grille_impl(
    DNJ_Maconne_Disposition_Ligne *instance)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Ligne *>(instance);
    auto maçonne = inst->m_maçonne->débute_grille();
    auto résultat = std::make_shared<Maconne_Disposition_Grille>(maçonne);
    inst->m_grilles.ajoute(résultat);
    return résultat.get();
}

void Maconne_Disposition_Ligne::ajoute_controle_impl(
    DNJ_Maconne_Disposition_Ligne *instance,
    QT_Chaine nom,
    DNJ_Rappels_Enveloppe_Parametre *rappels_params)
{
    danjo::DonneesControle données_controle;
    données_controle.nom = nom.vers_std_string();

    auto prop = memoire::loge<EnveloppeParamètre>("EnveloppeParamètre", rappels_params);
    if (prop->type() == danjo::TypePropriete::ENUM) {
        prop->crée_valeurs_énums(données_controle);
    }

    auto inst = reinterpret_cast<Maconne_Disposition_Ligne *>(instance);
    inst->m_maçonne->ajoute_controle(données_controle, prop);
}

void Maconne_Disposition_Ligne::ajoute_etiquette_impl(DNJ_Maconne_Disposition_Ligne *instance,
                                                      QT_Chaine nom)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Ligne *>(instance);
    inst->m_maçonne->ajoute_etiquette(nom.vers_std_string());
}

void Maconne_Disposition_Ligne::ajoute_etiquette_activable_impl(
    DNJ_Maconne_Disposition_Ligne *instance,
    QT_Chaine nom,
    DNJ_Rappels_Enveloppe_Parametre *rappels_params)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Ligne *>(instance);
    auto prop = memoire::loge<EnveloppeParamètre>("EnveloppeParamètre", rappels_params);
    inst->m_maçonne->ajoute_étiquette_activable(nom.vers_std_string(), prop);
}

void Maconne_Disposition_Ligne::ajoute_etiquette_propriete_impl(
    DNJ_Maconne_Disposition_Ligne *instance,
    QT_Chaine nom,
    DNJ_Rappels_Enveloppe_Parametre *rappels_params)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Ligne *>(instance);
    auto prop = memoire::loge<EnveloppeParamètre>("EnveloppeParamètre", rappels_params);
    inst->m_maçonne->ajoute_étiquette_propriété(nom.vers_std_string(), prop);
}

void Maconne_Disposition_Ligne::ajoute_espaceur_impl(DNJ_Maconne_Disposition_Ligne *instance,
                                                     int taille)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Ligne *>(instance);
    inst->m_maçonne->ajoute_espaceur(taille);
}

Maconne_Disposition_Ligne::Maconne_Disposition_Ligne(danjo::MaçonneDispositionLigne *maçonne)
    : m_maçonne(maçonne)
{
    ASSIGNE_RAPPEL(débute_ligne);
    ASSIGNE_RAPPEL(débute_colonne);
    ASSIGNE_RAPPEL(débute_grille);
    ASSIGNE_RAPPEL(ajoute_controle);
    ASSIGNE_RAPPEL(ajoute_etiquette);
    ASSIGNE_RAPPEL(ajoute_etiquette_activable);
    ASSIGNE_RAPPEL(ajoute_etiquette_propriete);
    ASSIGNE_RAPPEL(ajoute_espaceur);
}

DNJ_Maconne_Disposition_Ligne *Maconne_Disposition_Colonne::débute_ligne_impl(
    DNJ_Maconne_Disposition_Colonne *instance)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Colonne *>(instance);
    auto maçonne = inst->m_maçonne->débute_ligne();
    auto résultat = std::make_shared<Maconne_Disposition_Ligne>(maçonne);
    inst->m_lignes.ajoute(résultat);
    return résultat.get();
}

DNJ_Maconne_Disposition_Colonne *Maconne_Disposition_Colonne::débute_colonne_impl(
    DNJ_Maconne_Disposition_Colonne *instance)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Colonne *>(instance);
    auto maçonne = inst->m_maçonne->débute_colonne();
    auto résultat = std::make_shared<Maconne_Disposition_Colonne>(maçonne);
    inst->m_colonnes.ajoute(résultat);
    return résultat.get();
}

DNJ_Maconne_Disposition_Grille *Maconne_Disposition_Colonne::débute_grille_impl(
    DNJ_Maconne_Disposition_Colonne *instance)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Colonne *>(instance);
    auto maçonne = inst->m_maçonne->débute_grille();
    auto résultat = std::make_shared<Maconne_Disposition_Grille>(maçonne);
    inst->m_grilles.ajoute(résultat);
    return résultat.get();
}

void Maconne_Disposition_Colonne::ajoute_controle_impl(
    DNJ_Maconne_Disposition_Colonne *instance,
    QT_Chaine nom,
    DNJ_Rappels_Enveloppe_Parametre *rappels_params)
{
    danjo::DonneesControle données_controle;
    données_controle.nom = nom.vers_std_string();

    auto prop = memoire::loge<EnveloppeParamètre>("EnveloppeParamètre", rappels_params);
    if (prop->type() == danjo::TypePropriete::ENUM) {
        prop->crée_valeurs_énums(données_controle);
    }

    auto inst = reinterpret_cast<Maconne_Disposition_Colonne *>(instance);
    inst->m_maçonne->ajoute_controle(données_controle, prop);
}

void Maconne_Disposition_Colonne::ajoute_etiquette_impl(DNJ_Maconne_Disposition_Colonne *instance,
                                                        QT_Chaine nom)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Colonne *>(instance);
    inst->m_maçonne->ajoute_etiquette(nom.vers_std_string());
}

void Maconne_Disposition_Colonne::ajoute_etiquette_activable_impl(
    DNJ_Maconne_Disposition_Colonne *instance,
    QT_Chaine nom,
    DNJ_Rappels_Enveloppe_Parametre *rappels_params)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Colonne *>(instance);
    auto prop = memoire::loge<EnveloppeParamètre>("EnveloppeParamètre", rappels_params);
    inst->m_maçonne->ajoute_étiquette_activable(nom.vers_std_string(), prop);
}

void Maconne_Disposition_Colonne::ajoute_etiquette_propriete_impl(
    DNJ_Maconne_Disposition_Colonne *instance,
    QT_Chaine nom,
    DNJ_Rappels_Enveloppe_Parametre *rappels_params)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Colonne *>(instance);
    auto prop = memoire::loge<EnveloppeParamètre>("EnveloppeParamètre", rappels_params);
    inst->m_maçonne->ajoute_étiquette_propriété(nom.vers_std_string(), prop);
}

void Maconne_Disposition_Colonne::ajoute_espaceur_impl(DNJ_Maconne_Disposition_Colonne *instance,
                                                       int taille)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Colonne *>(instance);
    inst->m_maçonne->ajoute_espaceur(taille);
}

Maconne_Disposition_Colonne::Maconne_Disposition_Colonne(danjo::MaçonneDispositionColonne *maçonne)
    : m_maçonne(maçonne)
{
    ASSIGNE_RAPPEL(débute_ligne);
    ASSIGNE_RAPPEL(débute_colonne);
    ASSIGNE_RAPPEL(débute_grille);
    ASSIGNE_RAPPEL(ajoute_controle);
    ASSIGNE_RAPPEL(ajoute_etiquette);
    ASSIGNE_RAPPEL(ajoute_etiquette_activable);
    ASSIGNE_RAPPEL(ajoute_etiquette_propriete);
    ASSIGNE_RAPPEL(ajoute_espaceur);
}

DNJ_Maconne_Disposition_Ligne *Maconne_Disposition_Grille::débute_ligne_impl(
    DNJ_Maconne_Disposition_Grille *instance,
    int ligne,
    int colonne,
    int empan_ligne,
    int empan_colonne)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Grille *>(instance);
    auto maçonne = inst->m_maçonne->débute_ligne(ligne, colonne, empan_ligne, empan_colonne);
    auto résultat = std::make_shared<Maconne_Disposition_Ligne>(maçonne);
    inst->m_lignes.ajoute(résultat);
    return résultat.get();
}

DNJ_Maconne_Disposition_Colonne *Maconne_Disposition_Grille::débute_colonne_impl(
    DNJ_Maconne_Disposition_Grille *instance,
    int ligne,
    int colonne,
    int empan_ligne,
    int empan_colonne)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Grille *>(instance);
    auto maçonne = inst->m_maçonne->débute_colonne(ligne, colonne, empan_ligne, empan_colonne);
    auto résultat = std::make_shared<Maconne_Disposition_Colonne>(maçonne);
    inst->m_colonnes.ajoute(résultat);
    return résultat.get();
}

DNJ_Maconne_Disposition_Grille *Maconne_Disposition_Grille::débute_grille_impl(
    DNJ_Maconne_Disposition_Grille *instance,
    int ligne,
    int colonne,
    int empan_ligne,
    int empan_colonne)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Grille *>(instance);
    auto maçonne = inst->m_maçonne->débute_grille(ligne, colonne, empan_ligne, empan_colonne);
    auto résultat = std::make_shared<Maconne_Disposition_Grille>(maçonne);
    inst->m_grilles.ajoute(résultat);
    return résultat.get();
}

void Maconne_Disposition_Grille::ajoute_controle_impl(
    DNJ_Maconne_Disposition_Grille *instance,
    QT_Chaine nom,
    DNJ_Rappels_Enveloppe_Parametre *rappels_params,
    int ligne,
    int colonne,
    int empan_ligne,
    int empan_colonne)
{
    danjo::DonneesControle données_controle;
    données_controle.nom = nom.vers_std_string();

    auto prop = memoire::loge<EnveloppeParamètre>("EnveloppeParamètre", rappels_params);
    if (prop->type() == danjo::TypePropriete::ENUM) {
        prop->crée_valeurs_énums(données_controle);
    }

    auto inst = reinterpret_cast<Maconne_Disposition_Grille *>(instance);
    inst->m_maçonne->ajoute_controle(
        données_controle, prop, ligne, colonne, empan_ligne, empan_colonne);
}

void Maconne_Disposition_Grille::ajoute_etiquette_impl(DNJ_Maconne_Disposition_Grille *instance,
                                                       QT_Chaine nom,
                                                       int ligne,
                                                       int colonne,
                                                       int empan_ligne,
                                                       int empan_colonne)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Grille *>(instance);
    inst->m_maçonne->ajoute_etiquette(
        nom.vers_std_string(), ligne, colonne, empan_ligne, empan_colonne);
}

void Maconne_Disposition_Grille::ajoute_etiquette_activable_impl(
    DNJ_Maconne_Disposition_Grille *instance,
    QT_Chaine nom,
    DNJ_Rappels_Enveloppe_Parametre *rappels_params,
    int ligne,
    int colonne,
    int empan_ligne,
    int empan_colonne)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Grille *>(instance);
    auto prop = memoire::loge<EnveloppeParamètre>("EnveloppeParamètre", rappels_params);
    inst->m_maçonne->ajoute_étiquette_activable(
        nom.vers_std_string(), prop, ligne, colonne, empan_ligne, empan_colonne);
}

void Maconne_Disposition_Grille::ajoute_etiquette_propriete_impl(
    DNJ_Maconne_Disposition_Grille *instance,
    QT_Chaine nom,
    DNJ_Rappels_Enveloppe_Parametre *rappels_params,
    int ligne,
    int colonne,
    int empan_ligne,
    int empan_colonne)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Grille *>(instance);
    auto prop = memoire::loge<EnveloppeParamètre>("EnveloppeParamètre", rappels_params);
    inst->m_maçonne->ajoute_étiquette_propriété(
        nom.vers_std_string(), prop, ligne, colonne, empan_ligne, empan_colonne);
}

void Maconne_Disposition_Grille::ajoute_espaceur_impl(DNJ_Maconne_Disposition_Grille *instance,
                                                      int taille,
                                                      int ligne,
                                                      int colonne,
                                                      int empan_ligne,
                                                      int empan_colonne)
{
    auto inst = reinterpret_cast<Maconne_Disposition_Grille *>(instance);
    inst->m_maçonne->ajoute_espaceur(taille, ligne, colonne, empan_ligne, empan_colonne);
}

Maconne_Disposition_Grille::Maconne_Disposition_Grille(danjo::MaçonneDispositionGrille *maçonne)
    : m_maçonne(maçonne)
{
    ASSIGNE_RAPPEL(débute_ligne);
    ASSIGNE_RAPPEL(débute_colonne);
    ASSIGNE_RAPPEL(débute_grille);
    ASSIGNE_RAPPEL(ajoute_controle);
    ASSIGNE_RAPPEL(ajoute_etiquette);
    ASSIGNE_RAPPEL(ajoute_espaceur);
    ASSIGNE_RAPPEL(ajoute_etiquette_activable);
    ASSIGNE_RAPPEL(ajoute_etiquette_propriete);
}

/* ------------------------------------------------------------------------- */
/** \name ConstructriceInterfaceParametres
 * \{ */

class ConstructriceInterfaceParametres : public DNJ_ConstructriceInterfaceParametres {
    danjo::AssembleurDisposition &m_assembleuse;
    danjo::Manipulable &m_manipulable;
    danjo::ContexteMaçonnage ctx_maçonnage{};

    static void commence_disposition_impl(DNJ_ConstructriceInterfaceParametres *constructrice,
                                          DNJ_Type_Disposition type)
    {
        auto ctrice = static_cast<ConstructriceInterfaceParametres *>(constructrice);

        if (type == DNJ_TYPE_DISPOSITION_COLONNE) {
            ctrice->m_assembleuse.ajoute_disposition(danjo::id_morceau::COLONNE);
        }
        else {
            ctrice->m_assembleuse.ajoute_disposition(danjo::id_morceau::LIGNE);
        }
    }

    static void termine_disposition_impl(DNJ_ConstructriceInterfaceParametres *constructrice)
    {
        auto ctrice = static_cast<ConstructriceInterfaceParametres *>(constructrice);
        ctrice->m_assembleuse.sors_disposition();
    }

    static void ajoute_etiquette_impl(DNJ_ConstructriceInterfaceParametres *constructrice,
                                      QT_Chaine texte)
    {
        auto ctrice = static_cast<ConstructriceInterfaceParametres *>(constructrice);
        ctrice->m_assembleuse.ajoute_étiquette(texte.vers_std_string());
    }

    static void ajoute_propriete_impl(DNJ_ConstructriceInterfaceParametres *constructrice,
                                      QT_Chaine nom_param,
                                      DNJ_Rappels_Enveloppe_Parametre *rappels_params)
    {
        auto ctrice = static_cast<ConstructriceInterfaceParametres *>(constructrice);
        auto &manipulable = ctrice->m_manipulable;
        auto &assembleuse = ctrice->m_assembleuse;

        danjo::DonneesControle données_controle;
        données_controle.nom = nom_param.vers_std_string();

        auto prop = memoire::loge<EnveloppeParamètre>("EnveloppeParamètre", rappels_params);
        manipulable.ajoute_propriete(données_controle.nom, prop);

        if (prop->type() == danjo::TypePropriete::ENUM) {
            prop->crée_valeurs_énums(données_controle);
        }

        assembleuse.ajoute_controle_pour_propriété(données_controle, prop);
    }

    static DNJ_Maconne_Disposition_Ligne *débute_ligne_impl(
        DNJ_ConstructriceInterfaceParametres *constructrice)
    {
        auto ctrice = static_cast<ConstructriceInterfaceParametres *>(constructrice);
        auto résultat = new danjo::MaçonneDispositionLigne(&ctrice->ctx_maçonnage, nullptr);
        ctrice->m_assembleuse.disposition()->addLayout(résultat->donne_layout());
        return new Maconne_Disposition_Ligne(résultat);
    }

    static void termine_ligne_impl(DNJ_ConstructriceInterfaceParametres * /*constructrice*/,
                                   DNJ_Maconne_Disposition_Ligne *ligne)
    {
        auto mligne = reinterpret_cast<Maconne_Disposition_Ligne *>(ligne);
        delete mligne->donne_maçonne();
        delete mligne;
    }

    static DNJ_Maconne_Disposition_Colonne *débute_colonne_impl(
        DNJ_ConstructriceInterfaceParametres *constructrice)
    {
        auto ctrice = static_cast<ConstructriceInterfaceParametres *>(constructrice);
        auto résultat = new danjo::MaçonneDispositionColonne(&ctrice->ctx_maçonnage, nullptr);
        ctrice->m_assembleuse.disposition()->addLayout(résultat->donne_layout());
        return new Maconne_Disposition_Colonne(résultat);
    }

    static void termine_colonne_impl(DNJ_ConstructriceInterfaceParametres * /*constructrice*/,
                                     DNJ_Maconne_Disposition_Colonne *colonne)
    {
        auto mcolonne = reinterpret_cast<Maconne_Disposition_Colonne *>(colonne);
        delete mcolonne->donne_maçonne();
        delete mcolonne;
    }

    static DNJ_Maconne_Disposition_Grille *débute_grille_impl(
        DNJ_ConstructriceInterfaceParametres *constructrice)
    {
        auto ctrice = static_cast<ConstructriceInterfaceParametres *>(constructrice);
        auto résultat = new danjo::MaçonneDispositionGrille(&ctrice->ctx_maçonnage, nullptr);
        ctrice->m_assembleuse.disposition()->addLayout(résultat->donne_layout());
        return new Maconne_Disposition_Grille(résultat);
    }

    static void termine_grille_impl(DNJ_ConstructriceInterfaceParametres * /*constructrice*/,
                                    DNJ_Maconne_Disposition_Grille *grille)
    {
        auto mgrille = reinterpret_cast<Maconne_Disposition_Grille *>(grille);
        delete mgrille->donne_maçonne();
        delete mgrille;
    }

  public:
    ConstructriceInterfaceParametres(danjo::AssembleurDisposition &assembleuse,
                                     danjo::Manipulable &manipulable,
                                     danjo::ConteneurControles *conteneur,
                                     danjo::GestionnaireInterface *gestionnaire)
        : m_assembleuse(assembleuse), m_manipulable(manipulable)
    {
        commence_disposition = commence_disposition_impl;
        termine_disposition = termine_disposition_impl;
        ajoute_etiquette = ajoute_etiquette_impl;
        ajoute_propriete = ajoute_propriete_impl;
        debute_ligne = débute_ligne_impl;
        debute_colonne = débute_colonne_impl;
        debute_grille = débute_grille_impl;
        ASSIGNE_RAPPEL(termine_ligne);
        ASSIGNE_RAPPEL(termine_colonne);
        ASSIGNE_RAPPEL(termine_grille);

        ctx_maçonnage.manipulable = &manipulable;
        ctx_maçonnage.conteneur = conteneur;
        ctx_maçonnage.gestionnaire = gestionnaire;
    }
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ConteneurControles
 * \{ */

ConteneurControles::ConteneurControles(DNJ_Rappels_Widget *rappels, QWidget *parent)
    : danjo::ConteneurControles(parent), m_rappels(rappels)
{
}

ConteneurControles::~ConteneurControles()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

void ConteneurControles::ajourne_manipulable()
{
    if (m_rappels && m_rappels->sur_changement_parametre) {
        m_rappels->sur_changement_parametre(m_rappels);
    }
}

void ConteneurControles::debute_changement_controle()
{
    if (m_rappels && m_rappels->sur_pre_changement_parametre) {
        m_rappels->sur_pre_changement_parametre(m_rappels);
    }
}

void ConteneurControles::termine_changement_controle()
{
    if (m_rappels && m_rappels->sur_post_changement_parametre) {
        m_rappels->sur_post_changement_parametre(m_rappels);
    }
}

void ConteneurControles::onglet_dossier_change(int index)
{
    if (m_rappels && m_rappels->sur_changement_onglet_dossier) {
        m_rappels->sur_changement_onglet_dossier(m_rappels, index);
    }
}

void ConteneurControles::obtiens_liste(const dls::chaine &attache,
                                       dls::tableau<dls::chaine> &chaines)
{
    if (!m_rappels || !m_rappels->sur_requete_liste) {
        return;
    }

    QT_Chaine chn_attache;
    chn_attache.caractères = const_cast<char *>(attache.c_str());
    chn_attache.taille = attache.taille();

    auto constructrice = ConstructriceListe(chaines);

    m_rappels->sur_requete_liste(m_rappels, chn_attache, &constructrice);
}

RepondantCommande *ConteneurControles::donne_repondant_commande()
{
    if (m_rappels && m_rappels->donne_pilote_clique) {
        auto pilote = m_rappels->donne_pilote_clique(m_rappels);
        return reinterpret_cast<PiloteClique *>(pilote);
    }

    return nullptr;
}

danjo::GestionnaireInterface *ConteneurControles::donne_gestionnaire()
{
    if (m_rappels && m_rappels->donne_gestionnaire) {
        auto gestionnaire = m_rappels->donne_gestionnaire(m_rappels);
        return reinterpret_cast<danjo::GestionnaireInterface *>(gestionnaire);
    }

    return nullptr;
}

void ConteneurControles::crée_interface()
{
    if (!m_rappels || !m_rappels->sur_creation_interface) {
        return;
    }

    efface_controles();

    danjo::Manipulable manipulable;

    danjo::DonneesInterface données_interface{};
    données_interface.manipulable = &manipulable;
    données_interface.repondant_bouton = donne_repondant_commande();
    données_interface.conteneur = this;

    danjo::AssembleurDisposition assembleuse(données_interface);

    auto gestionnaire = donne_gestionnaire();

    /* Ajout d'une disposition par défaut. */
    assembleuse.ajoute_disposition(danjo::id_morceau::COLONNE);

    auto constructrice = ConstructriceInterfaceParametres(
        assembleuse, manipulable, this, gestionnaire);

    m_rappels->sur_creation_interface(m_rappels, &constructrice);

    auto disp = assembleuse.disposition();
    if (disp == nullptr) {
        return;
    }

    disp->addStretch();

    if (auto layout_existant = layout()) {
        QWidget temp;
        temp.setLayout(layout_existant);
    }
    setLayout(disp);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name PiloteClique
 * \{ */

static QT_Chaine vers_ipa(dls::chaine const &chaine)
{
    QT_Chaine résultat;
    résultat.caractères = const_cast<char *>(chaine.c_str());
    résultat.taille = chaine.taille();
    return résultat;
}

PiloteClique::PiloteClique(DNJ_Rappels_Pilote_Clique *rappels) : m_rappels(rappels)
{
}

PiloteClique::~PiloteClique()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

bool PiloteClique::evalue_predicat(dls::chaine const &identifiant, dls::chaine const &metadonnee)
{
    if (m_rappels && m_rappels->sur_évaluation_prédicat) {
        return m_rappels->sur_évaluation_prédicat(
            m_rappels, vers_ipa(identifiant), vers_ipa(metadonnee));
    }
    return false;
}

void PiloteClique::repond_clique(dls::chaine const &identifiant, dls::chaine const &metadonnee)
{
    if (m_rappels && m_rappels->sur_clique) {
        m_rappels->sur_clique(m_rappels, vers_ipa(identifiant), vers_ipa(metadonnee));
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name FournisseuseIcône
 * \{ */

static DNJ_Etat_Icone vers_ipa(danjo::ÉtatIcône état)
{
    if (état == danjo::ÉtatIcône::INACTIF) {
        return DNJ_ETAT_ICONE_INACTIF;
    }

    return DNJ_ETAT_ICONE_ACTIF;
}

static DNJ_Icone_Pour_Bouton vers_ipa(danjo::IcônePourBouton icône)
{
    switch (icône) {
        ENUMERE_DNJ_ICONE_POUR_BOUTON(ENUMERE_TRANSLATION_ENUM_QT_VERS_IPA)
    }

    return DNJ_ICONE_POUR_BOUTON_AJOUTE;
}

FournisseuseIcône::FournisseuseIcône(DNJ_Rappels_Fournisseuse_Icone *rappels) : m_rappels(rappels)
{
}

FournisseuseIcône::~FournisseuseIcône()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

std::optional<QIcon> FournisseuseIcône::icone_pour_bouton(const danjo::IcônePourBouton bouton,
                                                          danjo::ÉtatIcône état)
{
    if (!m_rappels || !m_rappels->donne_icone_pour_bouton) {
        return {};
    }

    auto chn_résultat = QT_Chaine{};
    auto résultat = m_rappels->donne_icone_pour_bouton(
        m_rappels, vers_ipa(bouton), vers_ipa(état), &chn_résultat);
    if (!résultat) {
        return {};
    }

    auto qchn_résultat = chn_résultat.vers_std_string();
    return QIcon(qchn_résultat.c_str());
}

std::optional<QIcon> FournisseuseIcône::icone_pour_identifiant(std::string const &identifiant,
                                                               danjo::ÉtatIcône état)
{
    if (!m_rappels || !m_rappels->donne_icone_pour_identifiant) {
        return {};
    }

    auto ident = QT_Chaine{};
    ident.caractères = const_cast<char *>(identifiant.c_str());
    ident.taille = int64_t(identifiant.size());

    auto chn_résultat = QT_Chaine{};
    auto résultat = m_rappels->donne_icone_pour_identifiant(
        m_rappels, ident, vers_ipa(état), &chn_résultat);
    if (!résultat) {
        return {};
    }

    auto qchn_résultat = chn_résultat.vers_std_string();
    return QIcon(qchn_résultat.c_str());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DialoguesChemins
 * \{ */

#define CHAINE_VERS_IPA(nom)                                                                      \
    auto std_##nom = nom.toStdString();                                                           \
    auto nom##_ipa = QT_Chaine                                                                    \
    {                                                                                             \
        const_cast<char *>(std_##nom.c_str()), int64_t(std_##nom.size())                          \
    }

DialoguesChemins::DialoguesChemins(DNJ_Rappels_DialoguesChemins *rappels) : m_rappels(rappels)
{
}

DialoguesChemins::~DialoguesChemins()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

QString DialoguesChemins::donne_chemin_pour_ouverture(QString const &chemin_existant,
                                                      QString const &caption,
                                                      QString const &dossier,
                                                      QString const &filtres)
{
    if (!m_rappels || !m_rappels->donne_chemin_pour_ouverture) {
        return "";
    }

    CHAINE_VERS_IPA(chemin_existant);
    CHAINE_VERS_IPA(caption);
    CHAINE_VERS_IPA(dossier);
    CHAINE_VERS_IPA(filtres);

    DNJ_Parametre_Dialogue_Chemin paramètres;
    paramètres.chemin_existant = chemin_existant_ipa;
    paramètres.caption = caption_ipa;
    paramètres.dossier = dossier_ipa;
    paramètres.filtres = filtres_ipa;

    auto résultat = m_rappels->donne_chemin_pour_ouverture(m_rappels, &paramètres);
    return résultat.vers_std_string().c_str();
}

QString DialoguesChemins::donne_chemin_pour_écriture(QString const &chemin_existant,
                                                     QString const &caption,
                                                     QString const &dossier,
                                                     QString const &filtres)
{
    if (!m_rappels || !m_rappels->donne_chemin_pour_écriture) {
        return "";
    }

    CHAINE_VERS_IPA(chemin_existant);
    CHAINE_VERS_IPA(caption);
    CHAINE_VERS_IPA(dossier);
    CHAINE_VERS_IPA(filtres);

    DNJ_Parametre_Dialogue_Chemin paramètres;
    paramètres.chemin_existant = chemin_existant_ipa;
    paramètres.caption = caption_ipa;
    paramètres.dossier = dossier_ipa;
    paramètres.filtres = filtres_ipa;

    auto résultat = m_rappels->donne_chemin_pour_écriture(m_rappels, &paramètres);
    return résultat.vers_std_string().c_str();
}

/** \} */
