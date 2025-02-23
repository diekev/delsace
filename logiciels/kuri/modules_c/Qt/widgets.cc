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
            Q##classe::nom_qt(event);                                                             \
        }                                                                                         \
    }

#define IMPLEMENTE_METHODES_EVENEMENTS(classe)                                                    \
    bool classe::event(QEvent *event)                                                             \
    {                                                                                             \
        if (m_rappels && m_rappels->sur_evenement) {                                              \
            QT_Generic_Event generic_event;                                                       \
            generic_event.event = reinterpret_cast<QT_Evenement *>(event);                        \
            if (m_rappels->sur_evenement(m_rappels, generic_event)) {                             \
                return true;                                                                      \
            }                                                                                     \
        }                                                                                         \
        return Q##classe::event(event);                                                           \
    }                                                                                             \
    IMPLEMENTE_METHODE_EVENEMENT(classe, QEnterEvent, enterEvent, QT_Evenement, sur_entree)       \
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
        classe, QResizeEvent, resizeEvent, QT_ResizeEvent, sur_redimensionnement)                 \
    IMPLEMENTE_METHODE_EVENEMENT(classe, QKeyEvent, keyPressEvent, QT_KeyEvent, sur_pression_cle) \
    IMPLEMENTE_METHODE_EVENEMENT(                                                                 \
        classe, QKeyEvent, keyReleaseEvent, QT_KeyEvent, sur_relachement_cle)                     \
    IMPLEMENTE_METHODE_EVENEMENT(                                                                 \
        classe, QContextMenuEvent, contextMenuEvent, QT_ContextMenuEvent, sur_menu_contexte)      \
    IMPLEMENTE_METHODE_EVENEMENT(                                                                 \
        classe, QDragEnterEvent, dragEnterEvent, QT_DragEnterEvent, sur_entree_drag)              \
    IMPLEMENTE_METHODE_EVENEMENT(classe, QCloseEvent, closeEvent, QT_CloseEvent, sur_fermeture)   \
    IMPLEMENTE_METHODE_EVENEMENT(classe, QDropEvent, dropEvent, QT_DropEvent, sur_drop)

template <typename TypeRappels>
static void autodefinis_supporte_drag_drop(QWidget *widget, TypeRappels *rappels)
{
    if (!rappels) {
        return;
    }
    widget->setAcceptDrops(rappels->sur_drop != nullptr);
}

/* ------------------------------------------------------------------------- */
/** \name Widget
 * \{ */

Widget::Widget(QT_Rappels_Widget *rappels, QWidget *parent) : QWidget(parent), m_rappels(rappels)
{
    autodefinis_supporte_drag_drop(this, m_rappels);
}

Widget::~Widget()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

bool Widget::focusNextPrevChild(bool /*next*/)
{
    /* Pour pouvoir utiliser la touche tab, il faut désactiver la focalisation
     * sur les éléments enfants du conteneur de contrôles. */
    return false;
}

inline QRect vers_qt(QT_Rect rect)
{
    return QRect(rect.x, rect.y, rect.largeur, rect.hauteur);
}

inline QColor vers_qt(QT_Color color)
{
    auto résultat = QColor();
    résultat.setRedF(float(color.r));
    résultat.setGreenF(float(color.g));
    résultat.setBlueF(float(color.b));
    résultat.setAlphaF(float(color.a));
    return résultat;
}

inline QPen vers_qt(QT_Pen pen)
{
    auto résultat = QPen();
    résultat.setColor(vers_qt(pen.color));
    résultat.setWidthF(pen.width);
    return résultat;
}

static Qt::AlignmentFlag convertis_alignement(QT_Alignment drapeaux)
{
    uint résultat = Qt::AlignLeft;
    ENEMERE_ALIGNEMENT_TEXTE(ENUMERE_TRANSLATION_ENUM_DRAPEAU_IPA_VERS_QT);
    return Qt::AlignmentFlag(résultat);
}

class Painter : public QT_Painter {
    QPainter *m_painter = nullptr;

    static void fill_rect_impl(QT_Painter *qt_painter, QT_Rect *rect, QT_Color *color)
    {
        auto painter = static_cast<Painter *>(qt_painter);
        painter->m_painter->fillRect(vers_qt(*rect), vers_qt(*color));
    }

    static void draw_rect_impl(QT_Painter *qt_painter, QT_Rect *rect)
    {
        auto painter = static_cast<Painter *>(qt_painter);
        painter->m_painter->drawRect(vers_qt(*rect));
    }

    static void set_pen_impl(QT_Painter *qt_painter, QT_Pen *pen)
    {
        auto painter = static_cast<Painter *>(qt_painter);
        painter->m_painter->setPen(vers_qt(*pen));
    }

    static void draw_text_impl(QT_Painter *qt_painter,
                               QT_Rect *rect,
                               QT_Alignment alignment,
                               QT_Chaine *chn)
    {
        auto painter = static_cast<Painter *>(qt_painter);
        painter->m_painter->drawText(
            vers_qt(*rect), convertis_alignement(alignment), chn->vers_std_string().c_str());
    }

  public:
    Painter(QPainter *painter) : m_painter(painter)
    {
        this->set_pen = set_pen_impl;
        this->fill_rect = fill_rect_impl;
        this->draw_rect = draw_rect_impl;
        this->draw_text = draw_text_impl;
    }

    EMPECHE_COPIE(Painter);
};

void Widget::paintEvent(QPaintEvent *event)
{
    if (m_rappels && m_rappels->sur_peinture) {
        QPainter qpainter(this);
        Painter enveloppe_painter(&qpainter);
        m_rappels->sur_peinture(m_rappels, &enveloppe_painter);
        return;
    }

    QWidget::paintEvent(event);
}

IMPLEMENTE_METHODES_EVENEMENTS(Widget)

/** \} */

/* ------------------------------------------------------------------------- */
/** \name GLWidget
 * \{ */

OpenGLWidget::OpenGLWidget(QT_Rappels_GLWidget *rappels, QWidget *parent)
    : QOpenGLWidget(parent), m_rappels(rappels)
{
    autodefinis_supporte_drag_drop(this, m_rappels);
}

OpenGLWidget::~OpenGLWidget()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

IMPLEMENTE_METHODES_EVENEMENTS(OpenGLWidget)

void OpenGLWidget::initializeGL()
{
    if (m_rappels && m_rappels->sur_initialisation_gl) {
        m_rappels->sur_initialisation_gl(m_rappels);
    }
    else {
        QOpenGLWidget::initializeGL();
    }
}

void OpenGLWidget::paintGL()
{
    if (m_rappels && m_rappels->sur_peinture_gl) {
        m_rappels->sur_peinture_gl(m_rappels);
    }
    else {
        QOpenGLWidget::paintGL();
    }
}

void OpenGLWidget::resizeGL(int w, int h)
{
    if (m_rappels && m_rappels->sur_redimensionnement_gl) {
        auto taille = QT_Taille{w, h};
        m_rappels->sur_redimensionnement_gl(m_rappels, taille);
    }
    else {
        QOpenGLWidget::resizeGL(w, h);
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
        connect(this, &CheckBox::checkStateChanged, this, &CheckBox::sur_changement_etat);
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

/* ------------------------------------------------------------------------- */
/** \name GraphicsView
 * \{ */

GraphicsView::GraphicsView(QWidget *parent) : QGraphicsView(parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

GraphicsView::GraphicsView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

GraphicsView::~GraphicsView()
{
}

void GraphicsView::keyPressEvent(QKeyEvent *event)
{
    static_cast<Widget *>(parent())->keyPressEvent(event);
}

void GraphicsView::wheelEvent(QWheelEvent *event)
{
    static_cast<Widget *>(parent())->wheelEvent(event);
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    static_cast<Widget *>(parent())->mouseMoveEvent(event);
}

void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    static_cast<Widget *>(parent())->mousePressEvent(event);
}

void GraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    static_cast<Widget *>(parent())->mouseDoubleClickEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    static_cast<Widget *>(parent())->mouseReleaseEvent(event);
}

bool GraphicsView::focusNextPrevChild(bool /*next*/)
{
    /* Pour pouvoir utiliser la touche tab, il faut désactiver la focalisation
     * sur les éléments enfants du conteneur de contrôles. */
    return false;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Widget
 * \{ */

PlainTextEdit::PlainTextEdit(QT_Rappels_PlainTextEdit *rappels, QWidget *parent)
    : QPlainTextEdit(parent), m_rappels(rappels)
{
    autodefinis_supporte_drag_drop(this, m_rappels);
}

PlainTextEdit::~PlainTextEdit()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

IMPLEMENTE_METHODES_EVENEMENTS(PlainTextEdit)

QTextCursor *PlainTextEdit::donne_cursor()
{
    m_cached_cursor = this->textCursor();
    return &m_cached_cursor;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Dialog
 * \{ */

Dialog::Dialog(QT_Rappels_Dialog *rappels, QWidget *parent) : QDialog(parent), m_rappels(rappels)
{
}

Dialog::~Dialog()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

IMPLEMENTE_METHODES_EVENEMENTS(Dialog)

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ToolButton
 * \{ */

ToolButton::ToolButton(QT_Rappels_ToolButton *rappels, QWidget *parent)
    : QToolButton(parent), m_rappels(rappels)
{
    autodefinis_supporte_drag_drop(this, m_rappels);
}

ToolButton::~ToolButton()
{
    if (m_rappels && m_rappels->sur_destruction) {
        m_rappels->sur_destruction(m_rappels);
    }
}

IMPLEMENTE_METHODES_EVENEMENTS(ToolButton)

/** \} */
