/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include "widgets.hh"

/* ------------------------------------------------------------------------- */
/** \name Widget
 * \{ */

Widget::Widget(QT_Rappels_Widget *rappels, QWidget *parent) : QWidget(parent), m_rappels(rappels)
{
}

void Widget::enterEvent(QEvent *event)
{
    if (m_rappels && m_rappels->sur_entree) {
        m_rappels->sur_entree(m_rappels, reinterpret_cast<QT_Evenement *>(event));
    }
    else {
        QWidget::enterEvent(event);
    }
}

void Widget::leaveEvent(QEvent *event)
{
    if (m_rappels && m_rappels->sur_sortie) {
        m_rappels->sur_sortie(m_rappels, reinterpret_cast<QT_Evenement *>(event));
    }
    else {
        QWidget::leaveEvent(event);
    }
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    if (m_rappels && m_rappels->sur_pression_souris) {
        m_rappels->sur_pression_souris(m_rappels, reinterpret_cast<QT_MouseEvent *>(event));
    }
    else {
        QWidget::mousePressEvent(event);
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_rappels && m_rappels->sur_deplacement_souris) {
        m_rappels->sur_deplacement_souris(m_rappels, reinterpret_cast<QT_MouseEvent *>(event));
    }
    else {
        QWidget::mouseMoveEvent(event);
    }
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_rappels && m_rappels->sur_relachement_souris) {
        m_rappels->sur_relachement_souris(m_rappels, reinterpret_cast<QT_MouseEvent *>(event));
    }
    else {
        QWidget::mouseReleaseEvent(event);
    }
}

void Widget::wheelEvent(QWheelEvent *event)
{
    if (m_rappels && m_rappels->sur_molette_souris) {
        m_rappels->sur_molette_souris(m_rappels, reinterpret_cast<QT_WheelEvent *>(event));
    }
    else {
        QWidget::wheelEvent(event);
    }
}

void Widget::resizeEvent(QResizeEvent *event)
{
    if (m_rappels && m_rappels->sur_redimensionnement) {
        m_rappels->sur_redimensionnement(m_rappels, reinterpret_cast<QT_ResizeEvent *>(event));
    }
    else {
        QWidget::resizeEvent(event);
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name GLWidget
 * \{ */

GLWidget::GLWidget(QT_Rappels_GLWidget *rappels, QWidget *parent)
    : QGLWidget(parent), m_rappels(rappels)
{
}

void GLWidget::enterEvent(QEvent *event)
{
    if (m_rappels && m_rappels->sur_entree) {
        m_rappels->sur_entree(m_rappels, reinterpret_cast<QT_Evenement *>(event));
    }
    else {
        QGLWidget::enterEvent(event);
    }
}

void GLWidget::leaveEvent(QEvent *event)
{
    if (m_rappels && m_rappels->sur_sortie) {
        m_rappels->sur_sortie(m_rappels, reinterpret_cast<QT_Evenement *>(event));
    }
    else {
        QGLWidget::leaveEvent(event);
    }
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_rappels && m_rappels->sur_pression_souris) {
        m_rappels->sur_pression_souris(m_rappels, reinterpret_cast<QT_MouseEvent *>(event));
    }
    else {
        QGLWidget::mousePressEvent(event);
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_rappels && m_rappels->sur_deplacement_souris) {
        m_rappels->sur_deplacement_souris(m_rappels, reinterpret_cast<QT_MouseEvent *>(event));
    }
    else {
        QGLWidget::mouseMoveEvent(event);
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_rappels && m_rappels->sur_relachement_souris) {
        m_rappels->sur_relachement_souris(m_rappels, reinterpret_cast<QT_MouseEvent *>(event));
    }
    else {
        QGLWidget::mouseReleaseEvent(event);
    }
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    if (m_rappels && m_rappels->sur_molette_souris) {
        m_rappels->sur_molette_souris(m_rappels, reinterpret_cast<QT_WheelEvent *>(event));
    }
    else {
        QGLWidget::wheelEvent(event);
    }
}

void GLWidget::resizeEvent(QResizeEvent *event)
{
    if (m_rappels && m_rappels->sur_redimensionnement) {
        m_rappels->sur_redimensionnement(m_rappels, reinterpret_cast<QT_ResizeEvent *>(event));
    }
    else {
        QGLWidget::resizeEvent(event);
    }
}

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

void TreeWidget::sur_selection_item(QTreeWidgetItem *item, int colonne)
{
    if (m_rappels && m_rappels->sur_selection_item) {
        m_rappels->sur_selection_item(
            m_rappels, reinterpret_cast<QT_TreeWidgetItem *>(item), colonne);
    }
}

void TreeWidget::sur_changement_item_courant(QTreeWidgetItem *courant, QTreeWidgetItem *precedent)
{
    if (m_rappels && m_rappels->sur_changement_item_courant) {
        m_rappels->sur_changement_item_courant(m_rappels,
                                               reinterpret_cast<QT_TreeWidgetItem *>(courant),
                                               reinterpret_cast<QT_TreeWidgetItem *>(precedent));
    }
}

/** \} */
