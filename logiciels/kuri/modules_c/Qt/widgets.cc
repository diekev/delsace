/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include "widgets.hh"

#define IMPLEMENTE_METHODE_EVENEMENT(classe, type_qt, nom_qt, type_ipa, nom_rappel)               \
    void classe::nom_qt(type_qt *event)                                                           \
    {                                                                                             \
        if (m_rappels && m_rappels->nom_rappel) {                                                 \
            m_rappels->nom_rappel(m_rappels, reinterpret_cast<type_ipa *>(event));                \
        }                                                                                         \
        else {                                                                                    \
            classe::nom_qt(event);                                                                \
        }                                                                                         \
    }

#define IMPLEMENTE_METHODES_EVENEMENTS(classe)                                                    \
    IMPLEMENTE_METHODE_EVENEMENT(classe, QEvent, enterEvent, QT_Evenement, sur_entree)            \
    IMPLEMENTE_METHODE_EVENEMENT(classe, QEvent, leaveEvent, QT_Evenement, sur_sortie)            \
    IMPLEMENTE_METHODE_EVENEMENT(                                                                 \
        classe, QMouseEvent, mousePressEvent, QT_MouseEvent, sur_pression_souris)                 \
    IMPLEMENTE_METHODE_EVENEMENT(                                                                 \
        classe, QMouseEvent, mouseMoveEvent, QT_MouseEvent, sur_deplacement_souris)               \
    IMPLEMENTE_METHODE_EVENEMENT(                                                                 \
        classe, QMouseEvent, mouseReleaseEvent, QT_MouseEvent, sur_relachement_souris)            \
    IMPLEMENTE_METHODE_EVENEMENT(                                                                 \
        classe, QMouseEvent, mouseDoubleClickEvent, QT_MouseEvent, sur_double_clique_souris)      \
    IMPLEMENTE_METHODE_EVENEMENT(                                                                 \
        classe, QWheelEvent, wheelEvent, QT_WheelEvent, sur_molette_souris)                       \
    IMPLEMENTE_METHODE_EVENEMENT(                                                                 \
        classe, QResizeEvent, resizeEvent, QT_ResizeEvent, sur_redimensionnement)

/* ------------------------------------------------------------------------- */
/** \name Widget
 * \{ */

Widget::Widget(QT_Rappels_Widget *rappels, QWidget *parent) : QWidget(parent), m_rappels(rappels)
{
}

Widget::~Widget()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

IMPLEMENTE_METHODES_EVENEMENTS(Widget)

/** \} */

/* ------------------------------------------------------------------------- */
/** \name GLWidget
 * \{ */

GLWidget::GLWidget(QT_Rappels_GLWidget *rappels, QWidget *parent)
    : QGLWidget(parent), m_rappels(rappels)
{
}

GLWidget::~GLWidget()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

IMPLEMENTE_METHODES_EVENEMENTS(GLWidget)

void GLWidget::initializeGL()
{
    if (m_rappels && m_rappels->sur_initialisation_gl) {
        m_rappels->sur_initialisation_gl(m_rappels);
    }
    else {
        QGLWidget::initializeGL();
    }
}

void GLWidget::paintGL()
{
    if (m_rappels && m_rappels->sur_peinture_gl) {
        m_rappels->sur_peinture_gl(m_rappels);
    }
    else {
        QGLWidget::paintGL();
    }
}

void GLWidget::resizeGL(int w, int h)
{
    if (m_rappels && m_rappels->sur_redimensionnement_gl) {
        auto taille = QT_Taille{w, h};
        m_rappels->sur_redimensionnement_gl(m_rappels, taille);
    }
    else {
        QGLWidget::resizeGL(w, h);
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name CheckBox
 * \{ */

CheckBox::CheckBox(QT_Rappels_CheckBox *rappels, QWidget *parent)
    : QCheckBox(parent), m_rappels(rappels)
{
    if (m_rappels && m_rappels->sur_changement_etat) {
        connect(this, &CheckBox::stateChanged, this, &CheckBox::sur_changement_etat);
    }
}

CheckBox::~CheckBox()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

static QT_Etat_CheckBox convertis_état_checkbox(int state)
{
    if (state == Qt::CheckState::Unchecked) {
        return QT_ETAT_CHECKBOX_NON_COCHE;
    }

    if (state == Qt::CheckState::PartiallyChecked) {
        return QT_ETAT_CHECKBOX_PARTIELLEMENT_COCHE;
    }

    return QT_ETAT_CHECKBOX_COCHE;
}

void CheckBox::sur_changement_etat(int state)
{
    if (m_rappels && m_rappels->sur_changement_etat) {
        m_rappels->sur_changement_etat(m_rappels, convertis_état_checkbox(state));
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ComboBox
 * \{ */

ComboBox::ComboBox(QWidget *parent) : QComboBox(parent)
{
    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(sur_changement_index(int)));
}

void ComboBox::sur_changement_index(int /*state*/)
{
    emit index_courant_modifie();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name TreeWidget
 * \{ */

TreeWidget::TreeWidget(QT_Rappels_TreeWidget *rappels, QWidget *parent)
    : QTreeWidget(parent), m_rappels(rappels)
{
    if (m_rappels && m_rappels->sur_selection_item) {
        connect(this, &QTreeWidget::itemPressed, this, &TreeWidget::sur_selection_item);
    }

    if (m_rappels && m_rappels->sur_changement_item_courant) {
        connect(this,
                &QTreeWidget::currentItemChanged,
                this,
                &TreeWidget::sur_changement_item_courant);
    }
}

TreeWidget::~TreeWidget()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

void TreeWidget::sur_selection_item(QTreeWidgetItem *item, int colonne)
{
    if (m_rappels && m_rappels->sur_selection_item) {
        m_rappels->sur_selection_item(
            m_rappels, reinterpret_cast<QT_TreeWidgetItem *>(item), colonne);
    }
}

void TreeWidget::sur_changement_item_courant(QTreeWidgetItem *courant, QTreeWidgetItem *précédent)
{
    if (m_rappels && m_rappels->sur_changement_item_courant) {
        m_rappels->sur_changement_item_courant(m_rappels,
                                               reinterpret_cast<QT_TreeWidgetItem *>(courant),
                                               reinterpret_cast<QT_TreeWidgetItem *>(précédent));
    }
}

/** \} */
