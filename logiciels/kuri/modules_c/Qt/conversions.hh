/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#pragma once

#include "fenetre_principale.hh"
#include "ipa_danjo.hh"
#include "qt_ipa_c.h"
#include "widgets.hh"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QApplication>
#include <QAudioDevice>
#include <QAudioSink>
#include <QAudioSource>
#include <QClipboard>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDrag>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMediaDevices>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QMutex>
#include <QOpenGLContext>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QSplitter>
#include <QSslSocket>
#include <QStatusBar>
#include <QTableView>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QWaitCondition>
#include <QWindow>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#define VERS_QT(x) auto q##x = vers_qt(x)

inline QObject *vers_qt(QT_Generic_Object object)
{
    return reinterpret_cast<QObject *>(object.object);
}

inline QWidget *vers_qt(QT_Generic_Widget widget)
{
    return reinterpret_cast<QWidget *>(widget.widget);
}

inline QEvent *vers_qt(QT_Generic_Event event)
{
    return reinterpret_cast<QEvent *>(event.event);
}

inline QLayout *vers_qt(QT_Generic_Layout layout)
{
    return reinterpret_cast<QLayout *>(layout.layout);
}

inline QBoxLayout *vers_qt(QT_Generic_BoxLayout layout)
{
    return reinterpret_cast<QBoxLayout *>(layout.box);
}

inline QGraphicsItem *vers_qt(QT_Generic_GraphicsItem item)
{
    return reinterpret_cast<QGraphicsItem *>(item.item);
}

inline QAbstractItemModel *vers_qt(QT_Generic_ItemModel model)
{
    return reinterpret_cast<QAbstractItemModel *>(model.item_model);
}

inline QAbstractSocket *vers_qt(QT_AbstractSocket socket)
{
    return reinterpret_cast<QAbstractSocket *>(socket.tcp);
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

inline QT_Color vers_ipa(QColor color)
{
    auto résultat = QT_Color();
    résultat.r = double(color.redF());
    résultat.g = double(color.greenF());
    résultat.b = double(color.blueF());
    résultat.a = double(color.alphaF());
    return résultat;
}

inline QT_Point vers_ipa(QPoint point)
{
    return QT_Point{point.x(), point.y()};
}

inline QPointF vers_qt(QT_PointF point)
{
    return QPointF(point.x, point.y);
}

inline QT_RectF vers_ipa(QRectF rect)
{
    return QT_RectF{rect.x(), rect.y(), rect.width(), rect.height()};
}

inline QT_Rect vers_ipa(QRect rect)
{
    return QT_Rect{rect.x(), rect.y(), rect.width(), rect.height()};
}

inline QRect vers_qt(QT_Rect rect)
{
    return QRect(rect.x, rect.y, rect.largeur, rect.hauteur);
}

inline QString vers_qt(QT_Chaine const *chaine)
{
    if (!chaine) {
        return "";
    }
    return chaine->vers_std_string().c_str();
}

inline QString vers_qt(QT_Chaine const chaine)
{
    return chaine.vers_std_string().c_str();
}

inline QT_Chaine vers_ipa(dls::chaine const &chaine)
{
    QT_Chaine résultat;
    résultat.caractères = const_cast<char *>(chaine.c_str());
    résultat.taille = chaine.taille();
    return résultat;
}

inline QFont vers_qt(QT_Font *font)
{
    if (!font) {
        return {};
    }

    auto famille = vers_qt(font->famille);

    auto résultat = QFont(famille);
    if (font->taille_point != 0) {
        résultat.setPointSize(font->taille_point);
    }
    return résultat;
}

inline QBrush vers_qt(QT_Brush brush)
{
    return QBrush(vers_qt(brush.color));
}

inline QPen vers_qt(QT_Pen pen)
{
    auto résultat = QPen();
    résultat.setColor(vers_qt(pen.color));
    résultat.setWidthF(pen.width);
    return résultat;
}

#define TRANSTYPAGE_WIDGETS(nom_qt, nom_classe, nom_union)                                        \
    inline nom_classe *vers_ipa(nom_qt *widget)                                                   \
    {                                                                                             \
        return reinterpret_cast<nom_classe *>(widget);                                            \
    }                                                                                             \
    inline nom_qt *vers_qt(nom_classe *widget)                                                    \
    {                                                                                             \
        return reinterpret_cast<nom_qt *>(widget);                                                \
    }

ENUMERE_TYPES_OBJETS(TRANSTYPAGE_WIDGETS)
ENUMERE_TYPES_WIDGETS(TRANSTYPAGE_WIDGETS)
ENUMERE_TYPES_LAYOUTS(TRANSTYPAGE_WIDGETS)
ENUMERE_TYPES_EVENTS(TRANSTYPAGE_WIDGETS)
ENUMERE_TYPES_GRAPHICS_ITEM(TRANSTYPAGE_WIDGETS)
ENUMERE_TYPES_BOX_LAYOUTS(TRANSTYPAGE_WIDGETS)
ENUMERE_TYPES_ITEM_MODEL(TRANSTYPAGE_WIDGETS)
ENUMERE_TYPES_SOCKETS(TRANSTYPAGE_WIDGETS)

#undef TRANSTYPAGE_WIDGETS

#define TRANSTYPAGE_OBJET_SIMPLE(nom_qt, nom_ipa)                                                 \
    inline nom_ipa *vers_ipa(nom_qt *widget)                                                      \
    {                                                                                             \
        return reinterpret_cast<nom_ipa *>(widget);                                               \
    }                                                                                             \
    inline nom_qt *vers_qt(nom_ipa *widget)                                                       \
    {                                                                                             \
        return reinterpret_cast<nom_qt *>(widget);                                                \
    }

TRANSTYPAGE_OBJET_SIMPLE(QIcon, QT_Icon)
TRANSTYPAGE_OBJET_SIMPLE(QPixmap, QT_Pixmap)
TRANSTYPAGE_OBJET_SIMPLE(QTextCursor, QT_TextCursor)
TRANSTYPAGE_OBJET_SIMPLE(QMutex, QT_Mutex)
TRANSTYPAGE_OBJET_SIMPLE(QWaitCondition, QT_WaitCondition)

#undef TRANSTYPAGE_WIDGETS

static Qt::CursorShape convertis_forme_curseur(QT_CursorShape cursor)
{
    switch (cursor) {
        ENUMERE_CURSOR_SHAPE(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return Qt::ArrowCursor;
}

static Qt::Orientation convertis_orientation(QT_Orientation orientation)
{
    switch (orientation) {
        case QT_ORIENTATION_HORIZONTALE:
        {
            return Qt::Horizontal;
        }
        case QT_ORIENTATION_VERTICALE:
        {
            return Qt::Vertical;
        }
    }
    return Qt::Horizontal;
}

static QT_Orientation convertis_orientation(Qt::Orientation orientation)
{
    switch (orientation) {
        case Qt::Horizontal:
        {
            return QT_ORIENTATION_HORIZONTALE;
        }
        case Qt::Vertical:
        {
            return QT_ORIENTATION_VERTICALE;
        }
    }
    return QT_ORIENTATION_HORIZONTALE;
}

/* ------------------------------------------------------------------------- */
/** \name QT_ModelIndex
 * \{ */

static QT_ModelIndex vers_ipa(const QModelIndex &model)
{
    auto résultat = QT_ModelIndex{};
    résultat.est_valide = model.isValid();
    if (résultat.est_valide) {
        résultat.colonne = model.column();
        résultat.ligne = model.row();
    }
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Layout_Size_Constraint
 * \{ */

static QLayout::SizeConstraint convertis_contrainte_taille(QT_Layout_Size_Constraint contrainte)
{
    switch (contrainte) {
        ENUMERE_LAYOUT_SIZE_CONSTRAINT(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QLayout::SetDefaultConstraint;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Alignment
 * \{ */

static Qt::AlignmentFlag convertis_alignement(QT_Alignment drapeaux)
{
    uint résultat = Qt::AlignLeft;
    ENEMERE_ALIGNEMENT_TEXTE(ENUMERE_TRANSLATION_ENUM_DRAPEAU_IPA_VERS_QT);
    return Qt::AlignmentFlag(résultat);
}

/** \} */
