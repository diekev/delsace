/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QCheckBox>
#include <QComboBox>
#include <QGLWidget>
#include <QGraphicsView>
#include <QTreeWidget>
#include <QWidget>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "biblinternes/outils/definitions.h"

#include "qt_ipa_c.h"

/* ------------------------------------------------------------------------- */
/** \name Widget
 * \{ */

class Widget : public QWidget {
    Q_OBJECT

    QT_Rappels_Widget *m_rappels = nullptr;

  public:
    Widget(QT_Rappels_Widget *rappels, QWidget *parent = nullptr);

    EMPECHE_COPIE(Widget);

    ~Widget() override;

    bool event(QEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool focusNextPrevChild(bool next) override;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name GLWidget
 * \{ */

class GLWidget : public QGLWidget {
    Q_OBJECT

    QT_Rappels_GLWidget *m_rappels = nullptr;

  public:
    GLWidget(QT_Rappels_GLWidget *rappels, QWidget *parent = nullptr);

    EMPECHE_COPIE(GLWidget);

    ~GLWidget() override;

    /* Évènements communs. */
    bool event(QEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    /* Évènements spécifiques. */
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    QT_Rappels_GLWidget *donne_rappels() const
    {
        return m_rappels;
    }
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name CheckBox
 * \{ */

class CheckBox : public QCheckBox {
    Q_OBJECT

    QT_Rappels_CheckBox *m_rappels = nullptr;

  public:
    CheckBox(QT_Rappels_CheckBox *rappel, QWidget *parent);

    EMPECHE_COPIE(CheckBox);

    ~CheckBox() override;

  public Q_SLOTS:
    void sur_changement_etat(int state);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ComboBox
 * \{ */

class ComboBox : public QComboBox {
    Q_OBJECT

  public:
    ComboBox(QWidget *parent);

  public Q_SLOTS:
    void sur_changement_index(int state);

  Q_SIGNALS:
    void index_courant_modifie();
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name TreeWidgetItem
 * \{ */

class TreeWidgetItem : public QTreeWidgetItem {
    void *m_données = nullptr;

  public:
    TreeWidgetItem(void *données, QTreeWidgetItem *parent = nullptr)
        : QTreeWidgetItem(parent), m_données(données)
    {
    }

    EMPECHE_COPIE(TreeWidgetItem);

    void *donne_données()
    {
        return m_données;
    }
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name TreeWidget
 * \{ */

class TreeWidget : public QTreeWidget {
    Q_OBJECT

    QT_Rappels_TreeWidget *m_rappels = nullptr;

  public:
    TreeWidget(QT_Rappels_TreeWidget *rappels, QWidget *parent = nullptr);

    EMPECHE_COPIE(TreeWidget);

    ~TreeWidget() override;

  public Q_SLOTS:
    void sur_selection_item(QTreeWidgetItem *item, int colonne);
    void sur_changement_item_courant(QTreeWidgetItem *courant, QTreeWidgetItem *précédent);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name GraphicsView
 * \{ */

class GraphicsView final : public QGraphicsView {
  public:
    GraphicsView(QWidget *parent);
    GraphicsView(QGraphicsScene *scene, QWidget *parent);
    ~GraphicsView() override;

    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool focusNextPrevChild(bool next) override;
};

/** \} */
