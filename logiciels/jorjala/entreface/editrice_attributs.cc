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
#include <QTableView>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "coeur/jorjala.hh"

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxilliaires locales.
 * \{ */

std::optional<JJL::NoeudCorps> donne_noeud_corps_actif(JJL::Jorjala &jorjala)
{
    auto graphe = jorjala.donne_graphe();
    if (graphe == nullptr) {
        return {};
    }

    auto noeud = graphe.donne_noeud_actif();
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

static void accumule_nom_attribut(dls::tableau<QString> &liste,
                                  JJL::TypeAttribut type,
                                  std::string const &nom)
{
    switch (type) {
        case JJL::TypeAttribut::BOOL:
        case JJL::TypeAttribut::ENTIER:
        case JJL::TypeAttribut::RÉEL:
            liste.ajoute(nom.c_str());
            break;
        case JJL::TypeAttribut::VEC2:
            liste.ajoute((nom + ".x").c_str());
            liste.ajoute((nom + ".y").c_str());
            break;
        case JJL::TypeAttribut::VEC3:
            liste.ajoute((nom + ".x").c_str());
            liste.ajoute((nom + ".y").c_str());
            liste.ajoute((nom + ".z").c_str());
            break;
        case JJL::TypeAttribut::VEC4:
            liste.ajoute((nom + ".x").c_str());
            liste.ajoute((nom + ".y").c_str());
            liste.ajoute((nom + ".z").c_str());
            liste.ajoute((nom + ".w").c_str());
            break;
        case JJL::TypeAttribut::COULEUR:
            liste.ajoute((nom + ".r").c_str());
            liste.ajoute((nom + ".v").c_str());
            liste.ajoute((nom + ".b").c_str());
            liste.ajoute((nom + ".a").c_str());
            break;
    }
}

static QString qstring_pour_attribut(JJL::Attribut attribut, const int ligne, const int dimension)
{
    switch (attribut.donne_type()) {
        case JJL::TypeAttribut::BOOL:
        {
            auto v = attribut.bool_pour_index(ligne);
            return v ? "vrai" : "faux";
        }
        case JJL::TypeAttribut::ENTIER:
        {
            auto v = attribut.entier_pour_index(ligne);
            return QString::number(v);
        }
        case JJL::TypeAttribut::RÉEL:
        {
            auto v = attribut.réel_pour_index(ligne);
            return QString::number(static_cast<double>(v));
        }
        case JJL::TypeAttribut::VEC2:
        {
            auto v = attribut.vec2_pour_index(ligne);
            if (dimension == 0) {
                return QString::number(static_cast<double>(v.donne_x()));
            }
            return QString::number(static_cast<double>(v.donne_y()));
        }
        case JJL::TypeAttribut::VEC3:
        {
            auto v = attribut.vec3_pour_index(ligne);
            if (dimension == 0) {
                return QString::number(static_cast<double>(v.donne_x()));
            }
            if (dimension == 1) {
                return QString::number(static_cast<double>(v.donne_y()));
            }
            return QString::number(static_cast<double>(v.donne_z()));
        }
        case JJL::TypeAttribut::VEC4:
        {
            auto v = attribut.vec4_pour_index(ligne);
            if (dimension == 0) {
                return QString::number(static_cast<double>(v.donne_x()));
            }
            if (dimension == 1) {
                return QString::number(static_cast<double>(v.donne_y()));
            }
            if (dimension == 2) {
                return QString::number(static_cast<double>(v.donne_z()));
            }
            return QString::number(static_cast<double>(v.donne_w()));
        }
        case JJL::TypeAttribut::COULEUR:
        {
            auto v = attribut.couleur_pour_index(ligne);
            if (dimension == 0) {
                return QString::number(static_cast<double>(v.donne_r()));
            }
            if (dimension == 1) {
                return QString::number(static_cast<double>(v.donne_v()));
            }
            if (dimension == 2) {
                return QString::number(static_cast<double>(v.donne_b()));
            }
            return QString::number(static_cast<double>(v.donne_a()));
        }
    }

    return QString("N/A");
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QAbstractTableModel pour afficher les attributs.
 * \{ */

#define DOMAINE_POINT 0
#define DOMAINE_PRIMITIVE 1

class ModèleTableAttribut final : public QAbstractTableModel {
    mutable JJL::Corps m_corps;
    int m_domaine;

    dls::tableau<JJL::Attribut> m_attributs{};
    dls::tableau<QString> m_noms_colonnes{};
    dls::tableau<int> m_index_colonne_vers_index_attribut{};
    dls::tableau<int> m_index_colonne_vers_dimension_valeur{};

  public:
    ModèleTableAttribut(JJL::Corps corps, int domaine);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  private:
    JJL::tableau<JJL_Attribut *, JJL::Attribut> donne_liste_attribut()
    {
        if (m_domaine == DOMAINE_POINT) {
            return m_corps.liste_des_attributs_points();
        }

        return m_corps.liste_des_attributs_primitives();
    }
};

ModèleTableAttribut::ModèleTableAttribut(JJL::Corps corps, int domaine)
    : QAbstractTableModel(), m_corps(corps), m_domaine(domaine)
{
    if (m_domaine == DOMAINE_POINT) {
        m_noms_colonnes.ajoute("P.x");
        m_noms_colonnes.ajoute("P.y");
        m_noms_colonnes.ajoute("P.z");
    }

    auto attributs = donne_liste_attribut();
    int nombre_de_colonnes = 0;

    for (auto attribut : attributs) {
        nombre_de_colonnes += nombre_de_colonnes_pour_type_attribut(attribut.donne_type());
    }

    m_noms_colonnes.reserve(m_noms_colonnes.taille() + nombre_de_colonnes);
    m_index_colonne_vers_index_attribut.reserve(nombre_de_colonnes);
    m_index_colonne_vers_dimension_valeur.reserve(nombre_de_colonnes);

    int index_attribut = 0;
    for (auto attribut : attributs) {
        accumule_nom_attribut(
            m_noms_colonnes, attribut.donne_type(), attribut.donne_nom().vers_std_string());

        const int colonnes_pour_attribut = nombre_de_colonnes_pour_type_attribut(
            attribut.donne_type());

        for (int i = 0; i < colonnes_pour_attribut; i++) {
            m_index_colonne_vers_index_attribut.ajoute(index_attribut);
            m_index_colonne_vers_dimension_valeur.ajoute(i);
        }

        m_attributs.ajoute(attribut);
        index_attribut += 1;
    }
}

int ModèleTableAttribut::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        /* Retourne 0 si le parent est valide.
         * https://doc.qt.io/qt-6/qabstractitemmodel.html#rowCount */
        return 0;
    }

    if (m_domaine == DOMAINE_POINT) {
        return static_cast<int>(m_corps.nombre_de_points());
    }

    return static_cast<int>(m_corps.nombre_de_primitives());
}

int ModèleTableAttribut::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        /* Retourne 0 si le parent est valide.
         * https://doc.qt.io/qt-6/qabstractitemmodel.html#columnCount */
        return 0;
    }

    return static_cast<int>(m_noms_colonnes.taille());
}

QVariant ModèleTableAttribut::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation == Qt::Orientation::Horizontal) {
        return m_noms_colonnes[section];
    }

    return QString::number(section);
}

QVariant ModèleTableAttribut::data(const QModelIndex &index, int role) const
{
    /* https://doc.qt.io/qt-6/qt.html#ItemDataRole-enum */
    if (role != Qt::DisplayRole) {
        return {};
    }

    int colonne = index.column();

    if (m_domaine == DOMAINE_POINT) {
        if (colonne < 3) {
            /* Nous devons afficher les valeurs des points. */
            auto point = m_corps.donne_point_local(index.row());

            if (colonne == 0) {
                return QString::number(static_cast<double>(point.donne_x()));
            }

            if (colonne == 1) {
                return QString::number(static_cast<double>(point.donne_y()));
            }

            return QString::number(static_cast<double>(point.donne_z()));
        }

        /* Nous sommes en dehors des points, ajust l'index pour n'inclure que les attributs. */
        colonne -= 3;
    }

    const int index_attribut = m_index_colonne_vers_index_attribut[colonne];
    const int dimension = m_index_colonne_vers_dimension_valeur[colonne];

    return qstring_pour_attribut(m_attributs[index_attribut], index.row(), dimension);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ÉditriceAttributs
 * \{ */

EditriceAttributs::EditriceAttributs(JJL::Jorjala &jorjala, QWidget *parent)
    : BaseEditrice("attributs", jorjala, parent), m_table(new QTableView(this)),
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

void EditriceAttributs::ajourne_état(JJL::TypeÉvènement évènement)
{
    auto creation = (évènement == (JJL::TypeÉvènement::NOEUD | JJL::TypeÉvènement::AJOUTÉ));
    creation |= (évènement == (JJL::TypeÉvènement::NOEUD | JJL::TypeÉvènement::ENLEVÉ));
    creation |= (évènement == (JJL::TypeÉvènement::NOEUD | JJL::TypeÉvènement::MODIFIÉ));
    creation |= (évènement == (JJL::TypeÉvènement::RAFRAICHISSEMENT));

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

    m_table->setModel(new ModèleTableAttribut(corps, m_domaine));

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
    /* Invalide le cache. */
    m_noeud = {};
    ajourne_état(JJL::TypeÉvènement::RAFRAICHISSEMENT);
}

/** \} */
