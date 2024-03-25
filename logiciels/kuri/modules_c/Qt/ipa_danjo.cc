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
    EnveloppeParamètre(DNJ_Rappels_Enveloppe_Parametre *rappels) : m_rappels(rappels){};

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

/* ------------------------------------------------------------------------- */
/** \name ConstructriceInterfaceParametres
 * \{ */

class ConstructriceInterfaceParametres : public DNJ_ConstructriceInterfaceParametres {
    danjo::AssembleurDisposition &m_assembleuse;
    danjo::Manipulable &m_manipulable;

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

  public:
    ConstructriceInterfaceParametres(danjo::AssembleurDisposition &assembleuse,
                                     danjo::Manipulable &manipulable)
        : m_assembleuse(assembleuse), m_manipulable(manipulable)
    {
        commence_disposition = commence_disposition_impl;
        termine_disposition = termine_disposition_impl;
        ajoute_etiquette = ajoute_etiquette_impl;
        ajoute_propriete = ajoute_propriete_impl;
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
    chn_attache.taille = int64_t(attache.taille());

    auto constructrice = ConstructriceListe(chaines);

    m_rappels->sur_requete_liste(m_rappels, chn_attache, &constructrice);
}

RepondantCommande *ConteneurControles::donne_repondant_commande()
{
    if (m_rappels && m_rappels->donne_pilote_clique) {
        auto pilote = m_rappels->donne_pilote_clique;
        return reinterpret_cast<RepondantCommande *>(pilote);
    }

    return nullptr;
}

QLayout *ConteneurControles::crée_interface()
{
    if (!m_rappels || !m_rappels->sur_creation_interface) {
        return nullptr;
    }

    danjo::Manipulable manipulable;

    danjo::DonneesInterface données_interface{};
    données_interface.manipulable = &manipulable;
    données_interface.repondant_bouton = donne_repondant_commande();
    données_interface.conteneur = this;

    danjo::AssembleurDisposition assembleuse(données_interface);

    /* Ajout d'une disposition par défaut. */
    assembleuse.ajoute_disposition(danjo::id_morceau::COLONNE);

    auto constructrice = ConstructriceInterfaceParametres(assembleuse, manipulable);

    m_rappels->sur_creation_interface(m_rappels, &constructrice);

    auto disp = assembleuse.disposition();
    if (disp == nullptr) {
        return nullptr;
    }

    disp->addStretch();
    return disp;
}

/** \} */
