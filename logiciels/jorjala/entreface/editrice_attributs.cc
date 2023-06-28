/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "editrice_attributs.hh"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "coeur/jorjala.hh"

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxilliaires locales.
 * \{ */

std::optional<JJL::NoeudCorps> donne_noeud_corps_actif(JJL::Jorjala &jorjala)
{
    auto graphe = jorjala.graphe();
    if (graphe == nullptr) {
        return {};
    }

    auto noeud = graphe.noeud_actif();
    if (noeud == nullptr) {
        return {};
    }

    if (!noeud.est_noeud_corps()) {
        return {};
    }

    return transtype<JJL::NoeudCorps>(noeud);
}

static int nombre_de_colonnes_pour_type_attribut(const JJL::TypeAttribut type)
{
    switch (type) {
        case JJL::TypeAttribut::BOOL:
        case JJL::TypeAttribut::ENTIER:
        case JJL::TypeAttribut::RÉEL:
            return 1;
        case JJL::TypeAttribut::VEC2:
            return 2;
        case JJL::TypeAttribut::VEC3:
            return 3;
        case JJL::TypeAttribut::VEC4:
        case JJL::TypeAttribut::COULEUR:
            return 4;
    }

    return 0;
}

static void accumule_nom_attribut(QStringList &liste,
                                  JJL::TypeAttribut type,
                                  std::string const &nom)
{
    switch (type) {
        case JJL::TypeAttribut::BOOL:
        case JJL::TypeAttribut::ENTIER:
        case JJL::TypeAttribut::RÉEL:
            liste.append(nom.c_str());
            break;
        case JJL::TypeAttribut::VEC2:
            liste.append((nom + ".x").c_str());
            liste.append((nom + ".y").c_str());
            break;
        case JJL::TypeAttribut::VEC3:
            liste.append((nom + ".x").c_str());
            liste.append((nom + ".y").c_str());
            liste.append((nom + ".z").c_str());
            break;
        case JJL::TypeAttribut::VEC4:
            liste.append((nom + ".x").c_str());
            liste.append((nom + ".y").c_str());
            liste.append((nom + ".z").c_str());
            liste.append((nom + ".w").c_str());
            break;
        case JJL::TypeAttribut::COULEUR:
            liste.append((nom + ".r").c_str());
            liste.append((nom + ".v").c_str());
            liste.append((nom + ".b").c_str());
            liste.append((nom + ".a").c_str());
            break;
    }
}

static void remplis_table_avec_attribut(QTableWidget *table,
                                        JJL::Attribut attribut,
                                        const int nombre_de_valeurs,
                                        const int index_colonne)
{
    switch (attribut.type()) {
        case JJL::TypeAttribut::BOOL:
        {
            for (int i = 0; i < nombre_de_valeurs; i++) {
                auto v = attribut.bool_pour_index(i);
                table->setCellWidget(i, index_colonne, new QLabel(v ? "vrai" : "faux"));
            }
            break;
        }
        case JJL::TypeAttribut::ENTIER:
        {
            for (int i = 0; i < nombre_de_valeurs; i++) {
                auto v = attribut.entier_pour_index(i);
                auto chn_v = QString::number(v);
                table->setCellWidget(i, index_colonne, new QLabel(chn_v));
            }
            break;
        }
        case JJL::TypeAttribut::RÉEL:
        {
            for (int i = 0; i < nombre_de_valeurs; i++) {
                auto v = attribut.réel_pour_index(i);
                auto chn_v = QString::number(v);
                table->setCellWidget(i, index_colonne, new QLabel(chn_v));
            }
            break;
        }
        case JJL::TypeAttribut::VEC2:
        {
            for (int i = 0; i < nombre_de_valeurs; i++) {
                auto v = attribut.vec2_pour_index(i);
                auto chn_vx = QString::number(v.x());
                auto chn_vy = QString::number(v.y());
                table->setCellWidget(i, index_colonne, new QLabel(chn_vx));
                table->setCellWidget(i, index_colonne + 1, new QLabel(chn_vy));
            }
            break;
        }
        case JJL::TypeAttribut::VEC3:
        {
            for (int i = 0; i < nombre_de_valeurs; i++) {
                auto v = attribut.vec3_pour_index(i);
                auto chn_vx = QString::number(v.x());
                auto chn_vy = QString::number(v.y());
                auto chn_vz = QString::number(v.z());
                table->setCellWidget(i, index_colonne, new QLabel(chn_vx));
                table->setCellWidget(i, index_colonne + 1, new QLabel(chn_vy));
                table->setCellWidget(i, index_colonne + 2, new QLabel(chn_vz));
            }
            break;
        }
        case JJL::TypeAttribut::VEC4:
        {
            for (int i = 0; i < nombre_de_valeurs; i++) {
                auto v = attribut.vec4_pour_index(i);
                auto chn_vx = QString::number(v.x());
                auto chn_vy = QString::number(v.y());
                auto chn_vz = QString::number(v.z());
                auto chn_vw = QString::number(v.w());
                table->setCellWidget(i, index_colonne, new QLabel(chn_vx));
                table->setCellWidget(i, index_colonne + 1, new QLabel(chn_vy));
                table->setCellWidget(i, index_colonne + 2, new QLabel(chn_vz));
                table->setCellWidget(i, index_colonne + 3, new QLabel(chn_vw));
            }
            break;
        }
        case JJL::TypeAttribut::COULEUR:
        {
            for (int i = 0; i < nombre_de_valeurs; i++) {
                auto v = attribut.couleur_pour_index(i);
                auto chn_vx = QString::number(v.r());
                auto chn_vy = QString::number(v.v());
                auto chn_vz = QString::number(v.b());
                auto chn_vw = QString::number(v.a());
                table->setCellWidget(i, index_colonne, new QLabel(chn_vx));
                table->setCellWidget(i, index_colonne + 1, new QLabel(chn_vy));
                table->setCellWidget(i, index_colonne + 2, new QLabel(chn_vz));
                table->setCellWidget(i, index_colonne + 3, new QLabel(chn_vw));
            }
            break;
        }
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ÉditriceAttributs
 * \{ */

#define DOMAINE_POINT 0
#define DOMAINE_PRIMITIVE 1

EditriceAttributs::EditriceAttributs(JJL::Jorjala &jorjala, QWidget *parent)
    : BaseEditrice("attributs", jorjala, parent), m_table(new QTableWidget(this)),
      m_label_pour_noeud_manquant(new QLabel("Aucun noeud actif", this)),
      m_sélecteur_domaine(new QComboBox(this))
{
    auto disposition_vert = new QVBoxLayout();
    auto disposition_barre = new QHBoxLayout();

    m_sélecteur_domaine->addItem("Point", DOMAINE_POINT);
    m_sélecteur_domaine->addItem("Primitive", DOMAINE_PRIMITIVE);

    connect(m_sélecteur_domaine,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(ajourne_pour_changement_domaine(int)));

    disposition_barre->addWidget(m_sélecteur_domaine);

    disposition_vert->addLayout(disposition_barre);
    disposition_vert->addWidget(m_label_pour_noeud_manquant);
    disposition_vert->addWidget(m_table);

    m_main_layout->addLayout(disposition_vert);
}

void EditriceAttributs::ajourne_état(JJL::TypeEvenement évènement)
{
    auto creation = (évènement == (JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::AJOUTÉ));
    creation |= (évènement == (JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::ENLEVÉ));
    creation |= (évènement == (JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ));
    creation |= (évènement == (JJL::TypeEvenement::RAFRAICHISSEMENT));

    if (!creation) {
        return;
    }

    auto noeud = donne_noeud_corps_actif(m_jorjala);
    if (!noeud) {
        m_noeud = {};
        définit_visibilité_table(false);
        return;
    }

    if (m_noeud.has_value() &&
        noeud->poignee() == reinterpret_cast<JJL_NoeudCorps *>(m_noeud.value().poignee())) {
        return;
    }

    auto noeud_corps = noeud.value();
    auto corps = noeud_corps.donne_corps();

    if (!corps) {
        m_noeud = {};
        définit_visibilité_table(false);
        return;
    }

    m_noeud = noeud;

    auto attributs = (m_domaine == DOMAINE_POINT) ? corps.liste_des_attributs_points() :
                                                    corps.liste_des_attributs_primitives();

    auto nombre_de_valeurs = (m_domaine == DOMAINE_POINT) ? corps.nombre_de_points() :
                                                            corps.nombre_de_primitives();

    QStringList titres_colonnes;
    if (m_domaine == DOMAINE_POINT) {
        accumule_nom_attribut(titres_colonnes, JJL::TypeAttribut::VEC3, "P");
    }
    for (auto attribut : attributs) {
        accumule_nom_attribut(titres_colonnes, attribut.type(), attribut.nom().vers_std_string());
    }

    m_table->clear();
    m_table->setColumnCount(titres_colonnes.size());
    m_table->setHorizontalHeaderLabels(titres_colonnes);
    m_table->setRowCount(nombre_de_valeurs);

    int index_colonne = 0;
    if (m_domaine == DOMAINE_POINT) {
        for (int i = 0; i < corps.nombre_de_points(); i++) {
            auto point = corps.point_pour_index(i);

            auto chn_x = QString::number(point.x());
            auto chn_y = QString::number(point.x());
            auto chn_z = QString::number(point.x());

            m_table->setCellWidget(i, 0, new QLabel(chn_x));
            m_table->setCellWidget(i, 1, new QLabel(chn_y));
            m_table->setCellWidget(i, 2, new QLabel(chn_z));
        }

        index_colonne = 3;
    }

    for (auto attribut : attributs) {
        remplis_table_avec_attribut(m_table, attribut, nombre_de_valeurs, index_colonne);
        index_colonne += nombre_de_colonnes_pour_type_attribut(attribut.type());
    }

    définit_visibilité_table(true);
}

void EditriceAttributs::définit_visibilité_table(bool est_visible)
{
    m_table->setVisible(est_visible);
    m_label_pour_noeud_manquant->setVisible(!est_visible);
}

void EditriceAttributs::ajourne_pour_changement_domaine(int domaine)
{
    m_domaine = domaine;
    ajourne_état(JJL::TypeEvenement::RAFRAICHISSEMENT);
    /* Invalide le cache. */
    m_noeud = {};
}

/** \} */
