/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include "qt_ipa_c.h"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QTimer>
#include <QToolTip>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include <iostream>

#include "biblinternes/outils/definitions.h"

#include "danjo/manipulable.h"

#include "fenetre_principale.hh"
#include "ipa_danjo.hh"
#include "tabs.hh"
#include "widgets.hh"

#define VERS_QT(x) auto q##x = vers_qt(x)

class EvenementPerso : public QEvent {
    void *m_données = nullptr;

  public:
    EvenementPerso(void *données, int type) : QEvent(Type(type)), m_données(données)
    {
    }

    EMPECHE_COPIE(EvenementPerso);

    void *donne_données()
    {
        return m_données;
    }
};

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

inline QColor vers_qt(QT_Color color)
{
    auto résultat = QColor();
    résultat.setRedF(color.r);
    résultat.setGreenF(color.g);
    résultat.setBlueF(color.b);
    résultat.setAlphaF(color.a);
    return résultat;
}

inline QT_Color vers_ipa(QColor color)
{
    auto résultat = QT_Color();
    résultat.r = color.redF();
    résultat.g = color.greenF();
    résultat.b = color.blueF();
    résultat.a = color.alphaF();
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

TRANSTYPAGE_OBJET_SIMPLE(QPixmap, QT_Pixmap)
TRANSTYPAGE_OBJET_SIMPLE(QTextCursor, QT_TextCursor)

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

extern "C" {

/* ------------------------------------------------------------------------- */
/** \name QT_Chaine
 * \{ */

void QT_chaine_detruit(struct QT_Chaine *chn)
{
    delete[] chn->caractères;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Pixmap
 * \{ */

QT_Pixmap *QT_cree_pixmap(QT_Chaine chemin)
{
    return vers_ipa(new QPixmap(vers_qt(chemin)));
}

void QT_detruit_pixmap(QT_Pixmap *pixmap)
{
    auto qpixmap = vers_qt(pixmap);
    delete (qpixmap);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Generic_Object
 * \{ */

void QT_object_definis_propriete_chaine(QT_Generic_Object object,
                                        QT_Chaine *nom,
                                        QT_Chaine *valeur)
{
    auto qobject = vers_qt(object);
    qobject->setProperty(nom->vers_std_string().c_str(), vers_qt(valeur));
}

void QT_object_definis_propriete_bool(QT_Generic_Object object, QT_Chaine nom, bool valeur)
{
    auto qobject = vers_qt(object);
    qobject->setProperty(nom.vers_std_string().c_str(), valeur);
}

bool QT_object_bloque_signaux(QT_Generic_Object object, bool ouinon)
{
    auto qobject = vers_qt(object);
    return qobject->blockSignals(ouinon);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ToolBarArea
 * \{ */

static Qt::ToolBarArea convertis_toolbararea(QT_ToolBarArea area)
{
    switch (area) {
        ENUMERE_TOOLBARAREA(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return Qt::TopToolBarArea;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Fenetre_Principale
 * \{ */

QT_Fenetre_Principale *QT_cree_fenetre_principale(QT_Rappels_Fenetre_Principale *rappels)
{
    auto résultat = new FenetrePrincipale(rappels);
    rappels->fenetre = vers_ipa(résultat);
    return vers_ipa(résultat);
}

void QT_detruit_fenetre_principale(QT_Fenetre_Principale *fenetre)
{
    auto fenêtre_qt = vers_qt(fenetre);
    delete fenêtre_qt;
}

void QT_fenetre_principale_definis_widget_central(QT_Fenetre_Principale *fenetre,
                                                  QT_Generic_Widget widget)
{
    auto fenêtre_qt = vers_qt(fenetre);
    auto qwidget = vers_qt(widget);
    fenêtre_qt->setCentralWidget(qwidget);
}

QT_Rappels_Fenetre_Principale *QT_fenetre_principale_donne_rappels(QT_Fenetre_Principale *fenetre)
{
    auto qfenêtre = vers_qt(fenetre);
    return qfenêtre->donne_rappels();
}

QT_MenuBar *QT_fenetre_principale_donne_barre_menu(QT_Fenetre_Principale *fenetre)
{
    auto qfenêtre = vers_qt(fenetre);
    return vers_ipa(qfenêtre->menuBar());
}

QT_StatusBar *QT_fenetre_principale_donne_barre_etat(QT_Fenetre_Principale *fenetre)
{
    auto qfenêtre = vers_qt(fenetre);
    return vers_ipa(qfenêtre->statusBar());
}

void QT_fenetre_principale_ajoute_barre_a_outils(QT_Fenetre_Principale *fenetre,
                                                 QT_ToolBar *tool_bar,
                                                 QT_ToolBarArea area)
{
    auto qfenêtre = vers_qt(fenetre);
    auto qtool_bar = vers_qt(tool_bar);
    qfenêtre->addToolBar(convertis_toolbararea(area), qtool_bar);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Application
 * \{ */

QT_Application *QT_cree_application(int *argc, char **argv)
{
    auto résultat = new QApplication(*argc, argv);
    return reinterpret_cast<QT_Application *>(résultat);
}

void QT_detruit_application(QT_Application *app)
{
    auto app_qt = reinterpret_cast<QApplication *>(app);
    delete app_qt;
}

int QT_application_exec(QT_Application *app)
{
    auto app_qt = reinterpret_cast<QApplication *>(app);
    return app_qt->exec();
}

void QT_core_application_definis_nom_organisation(QT_Chaine nom)
{
    QCoreApplication::setOrganizationName(vers_qt(nom));
}

void QT_core_application_definis_nom_application(QT_Chaine nom)
{
    QCoreApplication::setApplicationName(vers_qt(nom));
}

void QT_core_application_definis_feuille_de_style(QT_Chaine feuille)
{
    qApp->setStyleSheet(vers_qt(feuille));
}

QT_Application *QT_donne_application()
{
    return reinterpret_cast<QT_Application *>(qApp);
}

void QT_application_poste_evenement(QT_Generic_Object receveur, int type_evenement)
{
    auto qreceveur = vers_qt(receveur);
    auto event = new QEvent(QEvent::Type(type_evenement));
    QCoreApplication::postEvent(qreceveur, event);
}

void QT_application_poste_evenement_et_donnees(QT_Generic_Object receveur,
                                               int type_evenement,
                                               void *donnees)
{
    auto qreceveur = vers_qt(receveur);
    auto event = new EvenementPerso(donnees, type_evenement);
    QCoreApplication::postEvent(qreceveur, event);
}

void QT_application_sur_fin_boucle_evenement(QT_Application *app, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    auto qapp = reinterpret_cast<QApplication *>(app);
    QObject::connect(qapp, &QCoreApplication::aboutToQuit, [=]() { rappel->sur_rappel(rappel); });
}

void QT_application_beep()
{
    QApplication::beep();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Application
 * \{ */

void QT_gui_application_definis_curseur(QT_CursorShape cursor)
{
    QGuiApplication::setOverrideCursor(convertis_forme_curseur(cursor));
}

void QT_gui_application_restaure_curseur()
{
    QGuiApplication::restoreOverrideCursor();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Style
 * \{ */

QT_Style *QT_application_donne_style()
{
    return vers_ipa(QApplication::style());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Timer
 * \{ */

class Timer : public QTimer {
    QT_Rappels_Timer *m_rappels = nullptr;

  public:
    Timer(QT_Rappels_Timer *rappels) : QTimer(), m_rappels(rappels)
    {
        if (m_rappels && m_rappels->sur_timeout) {
            callOnTimeout([&]() { m_rappels->sur_timeout(m_rappels); });
        }
    }

    EMPECHE_COPIE(Timer);
};

QT_Timer *QT_cree_timer(QT_Rappels_Timer *rappels)
{
    return vers_ipa(new Timer(rappels));
}

void QT_timer_debute(QT_Timer *timer, int millisecondes)
{
    auto qtimer = vers_qt(timer);
    qtimer->start(millisecondes);
}

void QT_timer_arrete(QT_Timer *timer)
{
    auto qtimer = vers_qt(timer);
    qtimer->stop();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Screen
 * \{ */

QT_Screen *QT_application_donne_ecran_principal()
{
    return vers_ipa(QApplication::primaryScreen());
}

QT_Taille QT_screen_donne_taille_disponible(QT_Screen *screen)
{
    auto qscreen = vers_qt(screen);
    auto size = qscreen->availableSize();
    QT_Taille résultat;
    résultat.hauteur = size.height();
    résultat.largeur = size.width();
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Settings
 * \{ */

QT_Settings *QT_donne_parametres()
{
    return vers_ipa(new QSettings());
}

void QT_detruit_parametres(QT_Settings *settings)
{
    auto qsettings = vers_qt(settings);
    delete qsettings;
}

void QT_settings_lis_liste_chaine(QT_Settings *settings,
                                  QT_Chaine nom_paramètre,
                                  QT_Exportrice_Liste_Chaine *exportrice)
{
    if (!exportrice || !exportrice->ajoute) {
        return;
    }

    auto qsettings = vers_qt(settings);
    auto clé = nom_paramètre.vers_std_string();

    if (!qsettings->contains(clé.c_str())) {
        return;
    }

    auto const &liste = qsettings->value(clé.c_str()).toStringList();

    for (auto const &élément : liste) {
        auto std_élément = élément.toStdString();

        QT_Chaine chaine_élément;
        chaine_élément.caractères = const_cast<char *>(std_élément.c_str());
        chaine_élément.taille = int64_t(std_élément.size());

        exportrice->ajoute(exportrice, chaine_élément);
    }
}

void QT_settings_ecris_liste_chaine(QT_Settings *settings,
                                    QT_Chaine nom_paramètre,
                                    QT_Chaine *liste,
                                    int64_t taille_liste)
{
    if (!liste || taille_liste == 0) {
        return;
    }

    QStringList q_string_liste;
    for (auto i = 0; i < taille_liste; i++) {
        q_string_liste.append(vers_qt(liste[i]));
    }

    auto qsettings = vers_qt(settings);
    auto clé = nom_paramètre.vers_std_string();
    qsettings->setValue(clé.c_str(), q_string_liste);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Action
 * \{ */

QT_Action *QT_cree_action(QT_Chaine texte, QT_Generic_Object parent)
{
    VERS_QT(parent);
    return vers_ipa(new QAction(vers_qt(texte), qparent));
}

void QT_action_definis_donnee_z32(QT_Action *action, int donnee)
{
    VERS_QT(action);
    qaction->setData(QVariant(donnee));
}

int QT_action_donne_donnee_z32(QT_Action *action)
{
    VERS_QT(action);
    return qaction->data().toInt();
}

void QT_action_sur_declenchage(QT_Action *action, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }
    VERS_QT(action);
    QObject::connect(qaction, &QAction::triggered, [=]() { rappel->sur_rappel(rappel); });
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Rect
 * \{ */

QT_Point QT_rect_donne_bas_gauche(QT_Rect rect)
{
    VERS_QT(rect);
    return vers_ipa(qrect.bottomLeft());
}

QT_Point QT_rect_donne_bas_droit(QT_Rect rect)
{
    VERS_QT(rect);
    return vers_ipa(qrect.bottomRight());
}

QT_Point QT_rect_donne_haut_gauche(QT_Rect rect)
{
    VERS_QT(rect);
    return vers_ipa(qrect.topLeft());
}

QT_Point QT_rect_donne_haut_droit(QT_Rect rect)
{
    VERS_QT(rect);
    return vers_ipa(qrect.topRight());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Color
 * \{ */

void QT_color_depuis_tsl(double t, double s, double l, double a, struct QT_Color *r_color)
{
    auto résultat = QColor::fromHslF(t, s, l, a);
    *r_color = vers_ipa(résultat);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Cursor
 * \{ */

QT_Point QT_cursor_pos()
{
    auto résultat = QCursor::pos();
    return QT_Point{résultat.x(), résultat.y()};
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Evenement
 * \{ */

int QT_evenement_donne_type(QT_Generic_Event evenement)
{
    auto event = vers_qt(evenement);
    return event->type();
}

void QT_evenement_accepte(QT_Generic_Event evenement)
{
    auto event = vers_qt(evenement);
    event->accept();
}

void QT_evenement_marque_accepte(QT_Generic_Event evenement, int accepte)
{
    auto event = vers_qt(evenement);
    event->setAccepted(static_cast<bool>(accepte));
}

int QT_enregistre_evenement_personnel()
{
    return QEvent::registerEventType();
}

void *QT_event_perso_donne_donnees(QT_Evenement *event)
{
    auto qevent = reinterpret_cast<EvenementPerso *>(event);
    return qevent->donne_données();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Keyboard_Modifier
 * \{ */

static QT_Keyboard_Modifier modificateurs_vers_ipa(Qt::KeyboardModifiers drapeaux)
{
    int résultat = QT_KEYBOARD_MODIFIER_AUCUN;
    ENUMERE_MODIFICATEURS_CLAVIER(ENUMERE_TRANSLATION_ENUM_DRAPEAU_QT_VERS_IPA);
    return QT_Keyboard_Modifier(résultat);
}

QT_Keyboard_Modifier QT_application_donne_modificateurs_clavier(void)
{
    auto drapeaux = QApplication::keyboardModifiers();
    return modificateurs_vers_ipa(drapeaux);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_MouseEvent
 * \{ */

void QT_mouse_event_donne_position(QT_MouseEvent *event, QT_Position *r_position)
{
    auto qevent = vers_qt(event);
    if (r_position) {
        r_position->x = qevent->pos().x();
        r_position->y = qevent->pos().y();
    }
}

QT_MouseButton QT_mouse_event_donne_bouton(QT_MouseEvent *event)
{
    auto qevent = vers_qt(event);
    switch (qevent->button()) {
        ENUMERE_BOUTON_SOURIS(ENUMERE_TRANSLATION_ENUM_QT_VERS_IPA)
        default:
        {
            return QT_MOUSEBUTTON_AUCUN;
        }
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_WheelEvent
 * \{ */

void QT_wheel_event_donne_position(QT_WheelEvent *event, QT_Position *r_position)
{
    auto qevent = vers_qt(event);
    if (r_position) {
        r_position->x = qevent->pos().x();
        r_position->y = qevent->pos().y();
    }
}

int QT_wheel_event_donne_delta(QT_WheelEvent *event)
{
    auto qevent = vers_qt(event);
    return qevent->delta();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_KeyEvent
 * \{ */

static QT_Key qt_key_vers_ipa(Qt::Key key)
{
    switch (key) {
        ENUMERE_CLE_CLAVIER(ENUMERE_TRANSLATION_ENUM_QT_VERS_IPA)
    }
    return QT_KEY_unknown;
}

QT_Key QT_key_event_donne_cle(QT_KeyEvent *event)
{
    auto qevent = vers_qt(event);
    return qt_key_vers_ipa(Qt::Key(qevent->key()));
}

QT_Keyboard_Modifier QT_key_event_donne_modificateurs_clavier(QT_KeyEvent *event)
{
    auto qevent = vers_qt(event);
    return modificateurs_vers_ipa(qevent->modifiers());
}

int QT_key_event_donne_compte(QT_KeyEvent *event)
{
    auto qevent = vers_qt(event);
    return qevent->count();
}

bool QT_key_event_est_auto_repete(QT_KeyEvent *event)
{
    auto qevent = vers_qt(event);
    return qevent->isAutoRepeat();
}

uint32_t QT_key_event_donne_cle_virtuelle_native(QT_KeyEvent *event)
{
    auto qevent = vers_qt(event);
    return qevent->nativeVirtualKey();
}

uint32_t QT_key_event_donne_code_scan_natif(QT_KeyEvent *event)
{
    auto qevent = vers_qt(event);
    return qevent->nativeScanCode();
}

uint32_t QT_key_event_donne_modificateurs_natifs(QT_KeyEvent *event)
{
    auto qevent = vers_qt(event);
    return qevent->nativeModifiers();
}

QT_Chaine QT_key_event_donne_texte(QT_KeyEvent *event)
{
    static char tampon[128];

    auto qevent = vers_qt(event);
    auto text = qevent->text().toStdString();

    QT_Chaine résultat;

    if (text.size() < 128) {
        memcpy(tampon, text.c_str(), text.size());
        résultat.caractères = tampon;
        résultat.taille = int64_t(text.size());
    }
    else {
        résultat.caractères = nullptr;
        résultat.taille = 0;
    }

    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Widget
 * \{ */

QT_Widget *QT_cree_widget(QT_Rappels_Widget *rappels, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = vers_ipa(new Widget(rappels, qparent));
    if (rappels) {
        rappels->widget = résultat;
    }
    return résultat;
}

QT_Widget *QT_widget_nul()
{
    return nullptr;
}

void QT_widget_definis_titre_fenetre(QT_Generic_Widget widget, QT_Chaine nom)
{
    auto qwidget = vers_qt(widget);
    qwidget->setWindowTitle(vers_qt(nom));
}

void QT_widget_definis_layout(QT_Generic_Widget widget, QT_Generic_Layout layout)
{
    auto qwidget = vers_qt(widget);
    auto qlayout = vers_qt(layout);
    qwidget->setLayout(qlayout);
}

void QT_widget_remplace_layout(QT_Generic_Widget widget, QT_Generic_Layout layout)
{
    auto qwidget = vers_qt(widget);
    auto qlayout = vers_qt(layout);

    if (qwidget->layout()) {
        QWidget tmp;
        tmp.setLayout(qwidget->layout());
    }

    if (qlayout) {
        qwidget->setLayout(qlayout);
    }
}

void QT_widget_affiche_maximisee(QT_Generic_Widget widget)
{
    auto qwidget = vers_qt(widget);
    qwidget->showMaximized();
}

void QT_widget_affiche_minimisee(QT_Generic_Widget widget)
{
    auto qwidget = vers_qt(widget);
    qwidget->showMinimized();
}

void QT_widget_affiche_normal(QT_Generic_Widget widget)
{
    auto qwidget = vers_qt(widget);
    qwidget->showNormal();
}

void QT_widget_affiche_plein_ecran(QT_Generic_Widget widget)
{
    auto qwidget = vers_qt(widget);
    qwidget->showFullScreen();
}

void QT_widget_definis_taille_de_base(QT_Generic_Widget widget, QT_Taille taille)
{
    auto qwidget = vers_qt(widget);
    qwidget->setBaseSize(taille.largeur, taille.hauteur);
}

void QT_widget_definis_taille_minimum(QT_Generic_Widget widget, QT_Taille taille)
{
    auto qwidget = vers_qt(widget);
    qwidget->setMinimumSize(taille.largeur, taille.hauteur);
}

void QT_widget_definis_taille_fixe(QT_Generic_Widget widget, QT_Taille taille)
{
    auto qwidget = vers_qt(widget);
    qwidget->setFixedSize(taille.largeur, taille.hauteur);
}

void QT_widget_definis_largeur_fixe(QT_Generic_Widget widget, int largeur)
{
    auto qwidget = vers_qt(widget);
    qwidget->setFixedWidth(largeur);
}

void QT_widget_definis_hauteur_fixe(QT_Generic_Widget widget, int hauteur)
{
    auto qwidget = vers_qt(widget);
    qwidget->setFixedHeight(hauteur);
}

void QT_widget_redimensionne(QT_Generic_Widget widget, QT_Taille taille)
{
    VERS_QT(widget);
    qwidget->resize(taille.largeur, taille.hauteur);
}

QT_Taille QT_widget_donne_taille(QT_Generic_Widget widget)
{
    VERS_QT(widget);
    auto size = qwidget->size();
    return QT_Taille{size.width(), size.height()};
}

void QT_widget_affiche(QT_Generic_Widget widget)
{
    auto qwidget = vers_qt(widget);
    qwidget->show();
}

void QT_widget_cache(QT_Generic_Widget widget)
{
    auto qwidget = vers_qt(widget);
    qwidget->hide();
}

void QT_widget_definis_visible(QT_Generic_Widget widget, bool ouinon)
{
    auto qwidget = vers_qt(widget);
    qwidget->setVisible(ouinon);
}

void QT_widget_definis_actif(QT_Generic_Widget widget, bool ouinon)
{
    auto qwidget = vers_qt(widget);
    qwidget->setEnabled(ouinon);
}

QT_Generic_Widget QT_widget_donne_widget_parent(QT_Generic_Widget widget)
{
    auto qwidget = vers_qt(widget);
    /* XXX - ce peut ne pas être un Widget mais un QWidget. */
    auto widget_parent = static_cast<Widget *>(qwidget->parent());
    QT_Generic_Widget résultat;
    résultat.widget = vers_ipa(widget_parent);
    return résultat;
}

void QT_widget_copie_comportement_taille(QT_Generic_Widget widget, QT_Generic_Widget widget_source)
{
    auto qwidget = vers_qt(widget);
    auto qwidget_source = vers_qt(widget_source);
    qwidget->setSizePolicy(qwidget_source->sizePolicy());
}

void QT_widget_definis_feuille_de_style(QT_Generic_Widget widget, QT_Chaine *texte)
{
    auto qwidget = vers_qt(widget);
    qwidget->setStyleSheet(vers_qt(texte));
}

void QT_widget_definis_style(QT_Generic_Widget widget, QT_Style *style)
{
    auto qwidget = vers_qt(widget);
    auto qstyle = vers_qt(style);
    qwidget->setStyle(qstyle);
}

void QT_widget_ajourne(QT_Generic_Widget widget)
{
    auto qwidget = vers_qt(widget);
    qwidget->update();
}

void QT_widget_definis_trackage_souris(QT_Generic_Widget widget, bool ouinon)
{
    auto qwidget = vers_qt(widget);
    qwidget->setMouseTracking(ouinon);
}

static QSizePolicy::Policy convertis_comportement_taille(QT_Comportement_Taille comportement)
{
    switch (comportement) {
        ENUMERE_COMPORTEMENT_TAILLE(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QSizePolicy::Fixed;
}

void QT_widget_definis_comportement_taille(QT_Generic_Widget widget,
                                           QT_Comportement_Taille horizontal,
                                           QT_Comportement_Taille vertical)
{
    auto qwidget = vers_qt(widget);
    auto policy = qwidget->sizePolicy();
    policy.setHorizontalPolicy(convertis_comportement_taille(horizontal));
    policy.setVerticalPolicy(convertis_comportement_taille(vertical));
    qwidget->setSizePolicy(policy);
}

void QT_widget_definis_etirement_comportement_taille(QT_Generic_Widget widget,
                                                     int horizontal,
                                                     int vertical)
{
    auto qwidget = vers_qt(widget);
    auto policy = qwidget->sizePolicy();
    policy.setHorizontalStretch(horizontal);
    policy.setVerticalStretch(vertical);
    qwidget->setSizePolicy(policy);
}

void QT_widget_definis_hauteur_pour_largeur_comportement_taille(QT_Generic_Widget widget,
                                                                int oui_non)
{
    auto qwidget = vers_qt(widget);
    auto policy = qwidget->sizePolicy();
    policy.setHeightForWidth(bool(oui_non));
    qwidget->setSizePolicy(policy);
}

int QT_widget_donne_hauteur_pour_largeur_comportement_taille(QT_Generic_Widget widget)
{
    auto qwidget = vers_qt(widget);
    auto policy = qwidget->sizePolicy();
    return policy.hasHeightForWidth();
}

void QT_widget_definis_curseur(QT_Generic_Widget widget, QT_CursorShape cursor)
{
    auto qwidget = vers_qt(widget);
    qwidget->setCursor(convertis_forme_curseur(cursor));
}

void QT_widget_restore_curseur(QT_Generic_Widget widget)
{
    auto qwidget = vers_qt(widget);
    qwidget->unsetCursor();
}

void QT_widget_transforme_point_vers_global(QT_Generic_Widget widget,
                                            QT_Point *point,
                                            QT_Point *r_point)
{
    if (!r_point) {
        return;
    }

    VERS_QT(widget);
    auto résultat = qwidget->mapToGlobal(QPoint(point->x, point->y));
    *r_point = QT_Point{résultat.x(), résultat.y()};
}

void QT_widget_transforme_point_vers_local(QT_Generic_Widget widget,
                                           QT_Point *point,
                                           QT_Point *r_point)
{
    if (!r_point) {
        return;
    }

    VERS_QT(widget);
    auto résultat = qwidget->mapFromGlobal(QPoint(point->x, point->y));
    *r_point = QT_Point{résultat.x(), résultat.y()};
}

QT_Rect QT_widget_donne_geometrie(QT_Generic_Widget widget)
{
    VERS_QT(widget);
    return vers_ipa(qwidget->geometry());
}

void QT_widget_definis_infobulle(QT_Generic_Widget widget, QT_Chaine texte)
{
    VERS_QT(widget);
    VERS_QT(texte);
    qwidget->setToolTip(qtexte);
}

void QT_widget_definis_fonte(union QT_Generic_Widget widget, struct QT_Font *fonte)
{
    VERS_QT(widget);
    VERS_QT(fonte);
    qwidget->setFont(qfonte);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GLWidget
 * \{ */

QT_GLWidget *QT_cree_glwidget(QT_Rappels_GLWidget *rappels, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = vers_ipa(new GLWidget(rappels, qparent));
    if (rappels->widget) {
        rappels->widget = résultat;
    }
    return résultat;
}

QT_Rappels_GLWidget *QT_glwidget_donne_rappels(QT_GLWidget *widget)
{
    VERS_QT(widget);
    return qwidget->donne_rappels();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_StatusBar
 * \{ */

void QT_status_bar_ajoute_widget(QT_StatusBar *status_bar, QT_Generic_Widget widget)
{
    auto qstatus_bar = vers_qt(status_bar);
    auto qwidget = vers_qt(widget);
    qstatus_bar->addWidget(qwidget);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_MenuBar
 * \{ */

void QT_menu_bar_ajoute_menu(QT_MenuBar *menu_bar, QT_Menu *menu)
{
    auto qmenu_bar = vers_qt(menu_bar);
    auto qmenu = vers_qt(menu);
    qmenu_bar->addMenu(qmenu);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Menu
 * \{ */

QT_Menu *QT_cree_menu(QT_Generic_Widget parent)
{
    VERS_QT(parent);
    return vers_ipa(new QMenu(qparent));
}

QT_Menu *QT_cree_menu_titre(QT_Chaine titre, QT_Generic_Widget parent)
{
    VERS_QT(parent);
    VERS_QT(titre);
    return vers_ipa(new QMenu(qtitre, qparent));
}

void QT_menu_connecte_sur_pret_a_montrer(QT_Menu *menu, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    auto qmenu = vers_qt(menu);
    QObject::connect(qmenu, &QMenu::aboutToShow, [=]() { rappel->sur_rappel(rappel); });
}

void QT_menu_popup(QT_Menu *menu, QT_Point pos)
{
    VERS_QT(menu);
    qmenu->popup(QPoint(pos.x, pos.y));
}

void QT_menu_ajoute_action(QT_Menu *menu, QT_Action *action)
{
    VERS_QT(menu);
    VERS_QT(action);
    qmenu->addAction(qaction);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Alignment
 * \{ */

static Qt::AlignmentFlag convertis_alignement(QT_Alignment alignment)
{
    switch (alignment) {
        ENEMERE_ALIGNEMENT_TEXTE(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }

    return Qt::AlignLeft;
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
/** \name QT_Layout
 * \{ */

QT_HBoxLayout *QT_cree_hbox_layout(QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QHBoxLayout(qparent));
}

QT_VBoxLayout *QT_cree_vbox_layout(QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QVBoxLayout(qparent));
}

QT_FormLayout *QT_cree_form_layout(QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QFormLayout(qparent));
}

QT_GridLayout *QT_cree_grid_layout(QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QGridLayout(qparent));
}

void QT_layout_definis_marge(QT_Generic_Layout layout, int taille)
{
    auto qlayout = vers_qt(layout);
    qlayout->setMargin(taille);
}

void QT_layout_ajoute_widget(QT_Generic_Layout layout, QT_Generic_Widget widget)
{
    auto qlayout = vers_qt(layout);
    auto qwidget = vers_qt(widget);
    qlayout->addWidget(qwidget);
}

bool QT_layout_aligne_widget(QT_Generic_Layout layout,
                             QT_Generic_Widget widget,
                             QT_Alignment alignement)
{
    auto qlayout = vers_qt(layout);
    auto qwidget = vers_qt(widget);
    return qlayout->setAlignment(qwidget, convertis_alignement(alignement));
}

bool QT_layout_aligne_layout(QT_Generic_Layout layout,
                             QT_Generic_Layout sous_layout,
                             QT_Alignment alignement)
{
    auto qlayout = vers_qt(layout);
    auto qsous_layout = vers_qt(sous_layout);
    return qlayout->setAlignment(qsous_layout, convertis_alignement(alignement));
}

void QT_layout_definis_contrainte_taille(QT_Generic_Layout layout,
                                         QT_Layout_Size_Constraint contrainte)
{
    VERS_QT(layout);
    qlayout->setSizeConstraint(convertis_contrainte_taille(contrainte));
}

void QT_box_layout_ajoute_layout(QT_Generic_BoxLayout layout, QT_Generic_Layout sous_layout)
{
    VERS_QT(layout);
    auto qsous_layout = vers_qt(sous_layout);
    qlayout->addLayout(qsous_layout);
}

void QT_box_layout_ajoute_etirement(QT_Generic_BoxLayout layout, int etirement)
{
    VERS_QT(layout);
    qlayout->addStretch(etirement);
}

void QT_box_layout_ajoute_espacage(QT_Generic_BoxLayout layout, int espacage)
{
    VERS_QT(layout);
    qlayout->addSpacing(espacage);
}

void QT_form_layout_ajoute_ligne_chaine(QT_FormLayout *layout,
                                        QT_Chaine label,
                                        QT_Generic_Widget widget)
{
    auto qlayout = vers_qt(layout);
    auto qwidget = vers_qt(widget);

    if (qwidget) {
        if (label.taille != 0) {
            qlayout->addRow(vers_qt(label), qwidget);
        }
        else {
            qlayout->addRow(qwidget);
        }
    }
    else {
        auto qlabel = new QLabel(vers_qt(label));
        qlayout->addRow(qlabel);
    }
}

void QT_form_layout_ajoute_ligne(QT_FormLayout *layout, QT_Label *label, QT_Generic_Widget widget)
{
    auto qlayout = vers_qt(layout);
    auto qlabel = vers_qt(label);
    auto qwidget = vers_qt(widget);

    if (qwidget) {
        qlayout->addRow(qlabel, qwidget);
    }
    else {
        qlayout->addRow(qlabel);
    }
}

void QT_form_layout_ajoute_disposition(QT_FormLayout *form, QT_Generic_Layout layout)
{
    auto qform = vers_qt(form);
    auto qlayout = vers_qt(layout);
    qform->addRow(qlayout);
}

void QT_grid_layout_ajoute_widget(QT_GridLayout *layout,
                                  QT_Generic_Widget widget,
                                  int ligne,
                                  int colonne,
                                  QT_Alignment alignement)
{
    VERS_QT(layout);
    VERS_QT(widget);

    qlayout->addWidget(qwidget, ligne, colonne, convertis_alignement(alignement));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ComboBox
 * \{ */

QT_ComboBox *QT_cree_combobox(QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = new ComboBox(qparent);
    return vers_ipa(résultat);
}

void QT_combobox_reinitialise(QT_ComboBox *combo)
{
    auto qcombo = vers_qt(combo);
    qcombo->clear();
}

void QT_combobox_ajoute_item(QT_ComboBox *combo, QT_Chaine texte, QT_Chaine valeur)
{
    auto qcombo = vers_qt(combo);
    qcombo->addItem(vers_qt(texte), vers_qt(valeur));
}

void QT_combobox_definis_index_courant(QT_ComboBox *combo, int index)
{
    auto qcombo = vers_qt(combo);
    auto furent_bloqués = qcombo->blockSignals(true);
    qcombo->setCurrentIndex(index);
    qcombo->blockSignals(furent_bloqués);
}

int QT_combobox_donne_index_courant(QT_ComboBox *combo)
{
    auto qcombo = vers_qt(combo);
    return qcombo->currentIndex();
}

void QT_combobox_connecte_sur_changement_index(QT_ComboBox *combo, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    auto qcombo = vers_qt(combo);
    QObject::connect(
        qcombo, &ComboBox::index_courant_modifie, [=]() { rappel->sur_rappel(rappel); });
}

QT_Chaine QT_combobox_donne_valeur_courante_chaine(QT_ComboBox *combo)
{
    VERS_QT(combo);
    auto chaine = qcombo->currentData().toString().toStdString();

    static char tampon[FILENAME_MAX];

    QT_Chaine résultat;

    if (chaine.size() < FILENAME_MAX) {
        memcpy(tampon, chaine.c_str(), chaine.size());
        résultat.caractères = tampon;
        résultat.taille = int64_t(chaine.size());
    }
    else {
        résultat.caractères = nullptr;
        résultat.taille = 0;
    }

    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Splitter
 * \{ */

QT_Splitter *QT_cree_splitter()
{
    return vers_ipa(new QSplitter());
}

void QT_splitter_definis_orientation(QT_Splitter *splitter, QT_Orientation orientation)
{
    auto qsplitter = vers_qt(splitter);
    qsplitter->setOrientation(convertis_orientation(orientation));
}

void QT_splitter_ajoute_widget(QT_Splitter *splitter, QT_Generic_Widget widget)
{
    auto qsplitter = vers_qt(splitter);
    auto qwidget = vers_qt(widget);
    qsplitter->addWidget(qwidget);
}

void QT_splitter_definis_enfants_collapsables(QT_Splitter *splitter, bool ouinon)
{
    VERS_QT(splitter);
    qsplitter->setChildrenCollapsible(ouinon);
}

void QT_splitter_definis_tailles(QT_Splitter *splitter, int *éléments, int nombre_tailles)
{
    VERS_QT(splitter);

    QList<int> tailles;
    for (int i = 0; i < nombre_tailles; i++) {
        tailles.push_back(éléments[i]);
    }

    qsplitter->setSizes(tailles);
}

void QT_splitter_donne_tailles(QT_Splitter *splitter, int *r_éléments, int nombre_tailles)
{
    VERS_QT(splitter);

    int i = 0;
    for (auto size : qsplitter->sizes()) {
        if (i >= nombre_tailles) {
            break;
        }

        r_éléments[i++] = size;
    }
}

void QT_splitter_connecte_sur_mouvement_splitter(QT_Splitter *splitter,
                                                 QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(splitter);

    QObject::connect(
        qsplitter, &QSplitter::splitterMoved, [=](int, int) { rappel->sur_rappel(rappel); });
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_TabWidget
 * \{ */

QT_TabWidget *QT_cree_tab_widget(QT_Rappels_TabWidget *rappels, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = vers_ipa(new TabWidget(rappels, qparent));
    if (rappels) {
        rappels->widget = résultat;
    }
    return résultat;
}

QT_Rappels_TabWidget *QT_tab_widget_donne_rappels(QT_TabWidget *tab)
{
    auto qtab = reinterpret_cast<QTabWidget *>(tab);
    if (auto widget = dynamic_cast<TabWidget *>(qtab)) {
        return widget->donne_rappels();
    }
    return nullptr;
}

void QT_tab_widget_definis_tabs_fermable(QT_TabWidget *tab_widget, int fermable)
{
    auto qtab_widget = vers_qt(tab_widget);
    qtab_widget->setTabsClosable(bool(fermable));
}

void QT_tab_widget_widget_de_coin(QT_TabWidget *tab_widget, QT_Generic_Widget widget)
{
    auto qtab_widget = vers_qt(tab_widget);
    auto qwidget = vers_qt(widget);
    qtab_widget->setCornerWidget(qwidget);
}

void QT_tab_widget_ajoute_tab(QT_TabWidget *tab_widget, QT_Generic_Widget widget, QT_Chaine *nom)
{
    auto qtab_widget = vers_qt(tab_widget);
    auto qwidget = vers_qt(widget);
    qtab_widget->addTab(qwidget, vers_qt(nom));
}

void QT_tab_widget_definis_infobulle_tab(QT_TabWidget *tab_widget, int index, QT_Chaine infobulle)
{
    VERS_QT(tab_widget);
    VERS_QT(infobulle);
    qtab_widget->setTabToolTip(index, qinfobulle);
}

void QT_tab_widget_supprime_tab(QT_TabWidget *tab_widget, int index)
{
    auto qtab_widget = vers_qt(tab_widget);
    qtab_widget->removeTab(index);
}

void QT_tab_widget_definis_index_courant(QT_TabWidget *tab_widget, int index)
{
    auto qtab_widget = vers_qt(tab_widget);
    qtab_widget->setCurrentIndex(index);
}

int QT_tab_widget_donne_compte_tabs(QT_TabWidget *tab_widget)
{
    auto qtab_widget = vers_qt(tab_widget);
    return qtab_widget->count();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ScrollArea
 * \{ */

QT_ScrollArea *QT_cree_scroll_area(QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QScrollArea(qparent));
}

void QT_scroll_area_definis_widget(QT_ScrollArea *scroll_area, QT_Generic_Widget widget)
{
    auto qscroll = vers_qt(scroll_area);
    auto qwidget = vers_qt(widget);
    qscroll->setWidget(qwidget);
}

static Qt::ScrollBarPolicy convertis_comportement_défilement(
    enum QT_Comportement_Barre_Defilement comportement)
{
    switch (comportement) {
        case QT_COMPORTEMENT_BARRE_DEFILEMENT_AU_BESOIN:
        {
            return Qt::ScrollBarAsNeeded;
        }
        case QT_COMPORTEMENT_BARRE_DEFILEMENT_TOUJOURS_INACTIF:
        {
            return Qt::ScrollBarAlwaysOff;
        }
        case QT_COMPORTEMENT_BARRE_DEFILEMENT_TOUJOURS_ACTIF:
        {
            return Qt::ScrollBarAlwaysOn;
        }
    }

    return Qt::ScrollBarAsNeeded;
}

void QT_scroll_area_definis_comportement_vertical(
    QT_ScrollArea *scroll_area, enum QT_Comportement_Barre_Defilement comportement)
{
    auto qscroll = vers_qt(scroll_area);
    qscroll->setVerticalScrollBarPolicy(convertis_comportement_défilement(comportement));
}

void QT_scroll_area_permet_redimensionnement_widget(QT_ScrollArea *scroll_area,
                                                    int redimensionnable)
{
    auto qscroll = vers_qt(scroll_area);
    qscroll->setWidgetResizable(bool(redimensionnable));
}

void QT_scroll_area_definis_style_frame(QT_ScrollArea *scroll_area, int style)
{
    auto qscroll = vers_qt(scroll_area);
    qscroll->setFrameStyle(style);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_CheckBox
 * \{ */

QT_CheckBox *QT_checkbox_cree(QT_Rappels_CheckBox *rappels, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = new CheckBox(rappels, qparent);
    return vers_ipa(résultat);
}

void QT_checkbox_definis_coche(QT_CheckBox *checkbox, int coche)
{
    auto qcheckbox = vers_qt(checkbox);
    qcheckbox->setChecked(bool(coche));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Label
 * \{ */

QT_Label *QT_cree_label(QT_Chaine *texte, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    VERS_QT(texte);
    auto résultat = new QLabel(qtexte, qparent);
    return vers_ipa(résultat);
}

void QT_label_definis_texte(QT_Label *label, QT_Chaine texte)
{
    auto qlabel = vers_qt(label);
    qlabel->setText(vers_qt(texte));
}

void QT_label_definis_pixmap(QT_Label *label, QT_Pixmap *pixmap, QT_Taille taille)
{
    auto qlabel = vers_qt(label);
    auto qpixmap = vers_qt(pixmap);
    qlabel->setPixmap(qpixmap->scaled(16, 16));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ToolTip
 * \{ */

void QT_tooltip_montre_texte(QT_Point point, QT_Chaine texte)
{
    QToolTip::showText(QPoint(point.x, point.y), vers_qt(texte));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QLineEdit
 * \{ */

QT_LineEdit *QT_cree_line_edit(QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QLineEdit(qparent));
}

void QT_line_edit_definis_texte(QT_LineEdit *line_edit, QT_Chaine texte)
{
    auto qline = vers_qt(line_edit);
    qline->setText(vers_qt(texte));
}

void QT_line_edit_definis_texte_lieutenant(QT_LineEdit *line_edit, QT_Chaine texte)
{
    auto qline = vers_qt(line_edit);
    qline->setPlaceholderText(vers_qt(texte));
}

void QT_line_edit_connecte_sur_changement(QT_LineEdit *line_edit, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(line_edit);
    QObject::connect(qline_edit, &QLineEdit::textChanged, [=]() { rappel->sur_rappel(rappel); });
}

void QT_line_edit_connecte_sur_pression_retour(QT_LineEdit *line_edit, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(line_edit);
    QObject::connect(qline_edit, &QLineEdit::returnPressed, [=]() { rappel->sur_rappel(rappel); });
}

QT_Chaine QT_line_edit_donne_texte(QT_LineEdit *line_edit)
{
    VERS_QT(line_edit);
    static char tampon[FILENAME_MAX];

    auto texte = qline_edit->text().toStdString();
    QT_Chaine résultat;

    if (texte.size() < FILENAME_MAX) {
        memcpy(tampon, texte.c_str(), texte.size());
        résultat.caractères = tampon;
        résultat.taille = int64_t(texte.size());
    }
    else {
        résultat.caractères = nullptr;
        résultat.taille = 0;
    }

    return résultat;
}

void QT_line_edit_definis_lecture_seule(QT_LineEdit *line_edit, bool ouinon)
{
    VERS_QT(line_edit);
    qline_edit->setReadOnly(ouinon);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_PushButton
 * \{ */

QT_PushButton *QT_cree_push_button(QT_Chaine texte, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = new QPushButton(vers_qt(texte), qparent);
    return vers_ipa(résultat);
}

void QT_push_button_connecte_sur_pression(QT_PushButton *button, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    auto qbutton = vers_qt(button);
    QObject::connect(qbutton, &QPushButton::pressed, [=]() { rappel->sur_rappel(rappel); });
}

void QT_push_button_connecte_sur_clic(QT_PushButton *button, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    auto qbutton = vers_qt(button);
    QObject::connect(qbutton, &QPushButton::clicked, [=]() { rappel->sur_rappel(rappel); });
}

void QT_push_button_definis_autodefaut(QT_PushButton *button, bool ouinon)
{
    VERS_QT(button);
    qbutton->setAutoDefault(ouinon);
}

void QT_push_button_definis_defaut(QT_PushButton *button, bool ouinon)
{
    VERS_QT(button);
    qbutton->setDefault(ouinon);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_StandardButton
 * \{ */

static QMessageBox::StandardButton vers_qt(QT_StandardButton drapeaux)
{
    int résultat = QMessageBox::StandardButton::NoButton;
    ENUMERE_BOUTON_STANDARD(ENUMERE_TRANSLATION_ENUM_DRAPEAU_IPA_VERS_QT)
    return QMessageBox::StandardButton(résultat);
}

static QT_StandardButton vers_ipa(QMessageBox::StandardButton drapeaux)
{
    int résultat = QMessageBox::StandardButton::NoButton;
    ENUMERE_BOUTON_STANDARD(ENUMERE_TRANSLATION_ENUM_DRAPEAU_QT_VERS_IPA)
    return QT_StandardButton(résultat);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_DialogButtonBox
 * \{ */

QT_DialogButtonBox *QT_cree_dialog_button_box(QT_Generic_Widget parent)
{
    VERS_QT(parent);
    return vers_ipa(new QDialogButtonBox(qparent));
}

void QT_dialog_button_box_definis_orientation(QT_DialogButtonBox *box, QT_Orientation orientation)
{
    VERS_QT(box);
    qbox->setOrientation(convertis_orientation(orientation));
}

QT_PushButton *QT_dialog_button_box_ajoute_bouton_standard(QT_DialogButtonBox *box,
                                                           QT_StandardButton button)
{
    VERS_QT(box);
    auto résultat = qbox->addButton(QDialogButtonBox::StandardButton(vers_qt(button)));
    return vers_ipa(résultat);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Dialog
 * \{ */

QT_Dialog *QT_cree_dialog(QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = new QDialog(qparent);
    return vers_ipa(résultat);
}

QT_Dialog *QT_cree_dialog_rappels(QT_Rappels_Dialog *rappels, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = new Dialog(rappels, qparent);
    if (rappels) {
        rappels->widget = vers_ipa(résultat);
    }
    return vers_ipa(résultat);
}

void QT_dialog_detruit(QT_Dialog *dialog)
{
    VERS_QT(dialog);
    delete qdialog;
}

void QT_dialog_definis_bouton_accepter(QT_Dialog *dialog, QT_PushButton *bouton)
{
    auto qdialog = vers_qt(dialog);
    auto qbouton = vers_qt(bouton);
    QObject::connect(qbouton, &QPushButton::clicked, qdialog, &QDialog::accept);
}

void QT_dialog_definis_bouton_annuler(QT_Dialog *dialog, QT_PushButton *bouton)
{
    auto qdialog = vers_qt(dialog);
    auto qbouton = vers_qt(bouton);
    QObject::connect(qbouton, &QPushButton::clicked, qdialog, &QDialog::reject);
}

int QT_dialog_exec(QT_Dialog *dialog)
{
    return vers_qt(dialog)->exec();
}

void QT_dialog_definis_modal(QT_Dialog *dialog, bool ouinon)
{
    vers_qt(dialog)->setModal(ouinon);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_FileDialog
 * \{ */

QT_Chaine QT_file_dialog_donne_chemin_pour_lecture(QT_Generic_Widget parent,
                                                   QT_Chaine titre,
                                                   QT_Chaine dossier,
                                                   QT_Chaine filtre)
{
    auto qparent = vers_qt(parent);
    auto qtitre = titre.vers_std_string();
    auto qdossier = dossier.vers_std_string();
    auto qfiltre = filtre.vers_std_string();

    auto chemin = QFileDialog::getOpenFileName(
        qparent, qtitre.c_str(), qdossier.c_str(), qfiltre.c_str());

    auto std_chemin = chemin.toStdString();

    static char tampon[FILENAME_MAX];

    QT_Chaine résultat;

    if (std_chemin.size() < FILENAME_MAX) {
        memcpy(tampon, std_chemin.c_str(), std_chemin.size());
        résultat.caractères = tampon;
        résultat.taille = int64_t(std_chemin.size());
    }
    else {
        résultat.caractères = nullptr;
        résultat.taille = 0;
    }

    return résultat;
}

QT_Chaine QT_file_dialog_donne_chemin_pour_ecriture(QT_Generic_Widget parent,
                                                    QT_Chaine titre,
                                                    QT_Chaine dossier,
                                                    QT_Chaine filtre)
{
    auto qparent = vers_qt(parent);
    auto qtitre = titre.vers_std_string();
    auto qdossier = dossier.vers_std_string();
    auto qfiltre = filtre.vers_std_string();

    auto chemin = QFileDialog::getSaveFileName(
        qparent, qtitre.c_str(), qdossier.c_str(), qfiltre.c_str());

    auto std_chemin = chemin.toStdString();

    static char tampon[FILENAME_MAX];

    QT_Chaine résultat;

    if (std_chemin.size() < FILENAME_MAX) {
        memcpy(tampon, std_chemin.c_str(), std_chemin.size());
        résultat.caractères = tampon;
        résultat.taille = int64_t(std_chemin.size());
    }
    else {
        résultat.caractères = nullptr;
        résultat.taille = 0;
    }

    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_MessageBox
 * \{ */

QT_StandardButton QT_message_box_affiche_avertissement(QT_Generic_Widget parent,
                                                       QT_Chaine titre,
                                                       QT_Chaine message,
                                                       QT_StandardButton boutons)
{
    auto qparent = vers_qt(parent);
    auto qtitre = titre.vers_std_string();
    auto qmessage = message.vers_std_string();
    auto qboutons = vers_qt(boutons);

    auto résultat = QMessageBox::warning(qparent, qtitre.c_str(), qmessage.c_str(), qboutons);
    return vers_ipa(résultat);
}

QT_StandardButton QT_message_box_affiche_erreur(QT_Generic_Widget parent,
                                                QT_Chaine titre,
                                                QT_Chaine message,
                                                QT_StandardButton boutons)
{
    auto qparent = vers_qt(parent);
    auto qtitre = titre.vers_std_string();
    auto qmessage = message.vers_std_string();
    auto qboutons = vers_qt(boutons);

    auto résultat = QMessageBox::critical(qparent, qtitre.c_str(), qmessage.c_str(), qboutons);
    return vers_ipa(résultat);
}

QT_StandardButton QT_message_box_affiche_question(QT_Generic_Widget parent,
                                                  QT_Chaine titre,
                                                  QT_Chaine message,
                                                  QT_StandardButton boutons)
{
    auto qparent = vers_qt(parent);
    auto qtitre = titre.vers_std_string();
    auto qmessage = message.vers_std_string();
    auto qboutons = vers_qt(boutons);

    auto résultat = QMessageBox::question(qparent, qtitre.c_str(), qmessage.c_str(), qboutons);
    return vers_ipa(QMessageBox::StandardButton(résultat));
}

QT_StandardButton QT_message_box_affiche_information(QT_Generic_Widget parent,
                                                     QT_Chaine titre,
                                                     QT_Chaine message,
                                                     QT_StandardButton boutons)
{
    auto qparent = vers_qt(parent);
    auto qtitre = titre.vers_std_string();
    auto qmessage = message.vers_std_string();
    auto qboutons = vers_qt(boutons);

    auto résultat = QMessageBox::information(qparent, qtitre.c_str(), qmessage.c_str(), qboutons);
    return vers_ipa(résultat);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_TreeWidgetItem
 * \{ */

QT_TreeWidgetItem *QT_cree_treewidgetitem(void *donnees, QT_TreeWidgetItem *parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new TreeWidgetItem(donnees, qparent));
}

void *QT_treewidgetitem_donne_donnees(QT_TreeWidgetItem *widget)
{
    auto qwidget = vers_qt(widget);
    return qwidget->donne_données();
}

static QTreeWidgetItem::ChildIndicatorPolicy convertis_mode_indicateur(
    QT_Indicateur_Enfant_Arbre mode)
{
    switch (mode) {
        ENUMERE_INDICATEUR_ENFANT_TREE_WIDGET(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QTreeWidgetItem::ChildIndicatorPolicy::ShowIndicator;
}

void QT_treewidgetitem_definis_indicateur_enfant(QT_TreeWidgetItem *widget,
                                                 QT_Indicateur_Enfant_Arbre indicateur)
{
    auto qwidget = vers_qt(widget);
    qwidget->setChildIndicatorPolicy(convertis_mode_indicateur(indicateur));
}

void QT_treewidgetitem_definis_texte(QT_TreeWidgetItem *widget, int colonne, QT_Chaine *texte)
{
    auto qwidget = vers_qt(widget);
    qwidget->setText(colonne, vers_qt(texte));
}

void QT_treewidgetitem_ajoute_enfant(QT_TreeWidgetItem *widget, QT_TreeWidgetItem *enfant)
{
    auto qwidget = vers_qt(widget);
    auto qenfant = vers_qt(enfant);
    qwidget->addChild(qenfant);
}

void QT_treewidgetitem_definis_selectionne(QT_TreeWidgetItem *widget, bool ouinon)
{
    auto qwidget = vers_qt(widget);
    qwidget->setSelected(ouinon);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_TreeWidget
 * \{ */

static QAbstractItemView::SelectionMode convertis_mode_sélection(QT_Mode_Selection mode)
{
    switch (mode) {
        ENUMERE_MODE_SELECTION(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QAbstractItemView::SelectionMode::NoSelection;
}

static Qt::FocusPolicy convertis_comportement_focus(QT_Comportement_Focus comportement)
{
    switch (comportement) {
        ENUMERE_COMPORTEMENT_FOCUS(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return Qt::NoFocus;
}

static Qt::ContextMenuPolicy convertis_comportement_menu_contextuel(
    QT_Comportement_Menu_Contextuel comportement)
{
    switch (comportement) {
        ENUMERE_COMPORTEMENT_MENU_CONTEXTUEL(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return Qt::NoContextMenu;
}

static QAbstractItemView::DragDropMode convertis_mode_drag_drop(QT_Mode_DragDrop mode)
{
    switch (mode) {
        ENUMERE_MODE_DRAG_DROP(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QAbstractItemView::DragDropMode::NoDragDrop;
}

QT_TreeWidget *QT_cree_treewidget(QT_Rappels_TreeWidget *rappels, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new TreeWidget(rappels, qparent));
}

void QT_treewidget_efface(QT_TreeWidget *tree_widget)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->clear();
}

void QT_treewidget_definis_nombre_colonne(QT_TreeWidget *tree_widget, int compte)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setColumnCount(compte);
}

void QT_treewidget_ajoute_item_racine(QT_TreeWidget *tree_widget, QT_TreeWidgetItem *item)
{
    auto qtree_widget = vers_qt(tree_widget);
    auto qitem = vers_qt(item);
    qtree_widget->addTopLevelItem(qitem);
}

void QT_treewidget_definis_item_widget(QT_TreeWidget *tree_widget,
                                       QT_TreeWidgetItem *item,
                                       int colonne,
                                       QT_Generic_Widget widget)
{
    auto qtree_widget = vers_qt(tree_widget);
    auto qitem = vers_qt(item);
    auto qwidget = vers_qt(widget);
    qtree_widget->setItemWidget(qitem, colonne, qwidget);
}

void QT_treewidget_definis_taille_icone(QT_TreeWidget *tree_widget, int largeur, int hauteur)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setIconSize(QSize(largeur, hauteur));
}

void QT_treewidget_definis_toutes_les_colonnes_montre_focus(QT_TreeWidget *tree_widget,
                                                            int oui_non)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setAllColumnsShowFocus(bool(oui_non));
}

void QT_treewidget_definis_anime(QT_TreeWidget *tree_widget, int oui_non)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setAnimated(bool(oui_non));
}

void QT_treewidget_definis_auto_defilement(QT_TreeWidget *tree_widget, int oui_non)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setAutoScroll(bool(oui_non));
}

void QT_treewidget_definis_hauteurs_uniformes_lignes(QT_TreeWidget *tree_widget, int oui_non)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setUniformRowHeights(bool(oui_non));
}

void QT_treewidget_definis_mode_selection(QT_TreeWidget *tree_widget, QT_Mode_Selection mode)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setSelectionMode(convertis_mode_sélection(mode));
}

void QT_treewidget_definis_comportement_focus(QT_TreeWidget *tree_widget,
                                              QT_Comportement_Focus comportement)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setFocusPolicy(convertis_comportement_focus(comportement));
}

void QT_treewidget_definis_comportement_menu_contextuel(
    QT_TreeWidget *tree_widget, QT_Comportement_Menu_Contextuel comportement)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setContextMenuPolicy(convertis_comportement_menu_contextuel(comportement));
}

void QT_treewidget_definis_entete_visible(QT_TreeWidget *tree_widget, int oui_non)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setHeaderHidden(bool(oui_non));
}

void QT_treewidget_definis_mode_drag_drop(QT_TreeWidget *tree_widget, QT_Mode_DragDrop mode)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setDragDropMode(convertis_mode_drag_drop(mode));
}

void QT_treewidget_definis_activation_drag(QT_TreeWidget *tree_widget, int oui_non)
{
    auto qtree_widget = vers_qt(tree_widget);
    qtree_widget->setDragEnabled(bool(oui_non));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Frame
 * \{ */

static QFrame::Shadow convertis_ombrage_frame(QT_Frame_Shadow ombrage)
{
    switch (ombrage) {
        ENEMERE_OMBRAGE_FRAME(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QFrame::Plain;
}

static QFrame::Shape convertis_forme_frame(QT_Frame_Shape forme)
{
    switch (forme) {
        ENEMERE_FORME_FRAME(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QFrame::NoFrame;
}

QT_Frame *QT_cree_frame(QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QFrame(qparent));
}

void QT_frame_definis_forme(QT_Frame *frame, enum QT_Frame_Shape forme)
{
    auto qframe = vers_qt(frame);
    qframe->setFrameShape(convertis_forme_frame(forme));
}

void QT_frame_definis_ombrage(QT_Frame *frame, enum QT_Frame_Shadow ombrage)
{
    auto qframe = vers_qt(frame);
    qframe->setFrameShadow(convertis_ombrage_frame(ombrage));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsItem
 * \{ */

void QT_graphics_item_definis_position(QT_Generic_GraphicsItem item, QT_PointF pos)
{
    VERS_QT(item);
    VERS_QT(pos);
    qitem->setPos(qpos);
}

QT_RectF QT_graphics_item_donne_rect(QT_Generic_GraphicsItem item)
{
    VERS_QT(item);
    auto rect = qitem->boundingRect();
    return vers_ipa(rect);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsRectItem
 * \{ */

QT_GraphicsRectItem *QT_cree_graphics_rect_item(QT_Generic_GraphicsItem parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QGraphicsRectItem(qparent));
}

void QT_graphics_rect_item_definis_pinceau(QT_GraphicsRectItem *item, QT_Pen *pinceau)
{
    auto qitem = vers_qt(item);
    auto qpen = vers_qt(*pinceau);
    qitem->setPen(qpen);
}

void QT_graphics_rect_item_definis_brosse(QT_GraphicsRectItem *item, QT_Brush *brush)
{
    auto qitem = vers_qt(item);
    auto qbrush = vers_qt(*brush);
    qitem->setBrush(qbrush);
}

void QT_graphics_rect_item_definis_rect(QT_GraphicsRectItem *item, QT_RectF *rect)
{
    auto qitem = vers_qt(item);
    auto qrect = QRectF(rect->x, rect->y, rect->largeur, rect->hauteur);
    qitem->setRect(qrect);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsTextItem
 * \{ */

QT_GraphicsTextItem *QT_cree_graphics_text_item(QT_Chaine texte, QT_Generic_GraphicsItem parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QGraphicsTextItem(vers_qt(texte), qparent));
}

void QT_graphics_text_item_definis_police(QT_GraphicsTextItem *text_item, QT_Font *font)
{
    VERS_QT(text_item);
    VERS_QT(font);
    qtext_item->setFont(qfont);
}

void QT_graphics_text_item_definis_couleur_defaut(QT_GraphicsTextItem *text_item, QT_Color *color)
{
    VERS_QT(text_item);
    auto qcolor = vers_qt(*color);
    qtext_item->setDefaultTextColor(qcolor);
}

void QT_graphics_text_item_donne_rect(QT_GraphicsTextItem *item, QT_RectF *r_rect)
{
    VERS_QT(item);
    auto rect = qitem->boundingRect();
    *r_rect = vers_ipa(rect);
}

void QT_graphics_text_item_definis_position(QT_GraphicsTextItem *item, QT_PointF *pos)
{
    VERS_QT(item);
    auto qpos = vers_qt(*pos);
    qitem->setPos(qpos);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsLineItem
 * \{ */

QT_GraphicsLineItem *QT_cree_graphics_line_item(QT_Generic_GraphicsItem parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QGraphicsLineItem(qparent));
}

void QT_graphics_line_item_definis_pinceau(QT_GraphicsLineItem *item, QT_Pen *pinceau)
{
    auto qitem = vers_qt(item);
    auto qpen = vers_qt(*pinceau);
    qitem->setPen(qpen);
}

void QT_graphics_line_item_definis_ligne(
    QT_GraphicsLineItem *line, double x1, double y1, double x2, double y2)
{
    auto qline = vers_qt(line);
    qline->setLine(x1, y1, x2, y2);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsScene
 * \{ */

QT_GraphicsScene *QT_cree_graphics_scene(QT_Generic_Object parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QGraphicsScene(qparent));
}

void QT_graphics_scene_detruit(QT_GraphicsScene *scene)
{
    auto qscene = vers_qt(scene);
    delete qscene;
}

QT_GraphicsView *QT_graphics_scene_cree_graphics_view(QT_GraphicsScene *scene,
                                                      QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto qscene = vers_qt(scene);
    return vers_ipa(new GraphicsView(qscene, qparent));
}

void QT_graphics_scene_efface(QT_GraphicsScene *scene)
{
    auto qscene = vers_qt(scene);
    qscene->clear();
}

void QT_graphics_scene_donne_rect_scene(QT_GraphicsScene *scene, QT_RectF *r_rect)
{
    auto qscene = vers_qt(scene);
    auto rect = qscene->sceneRect();
    *r_rect = QT_RectF{rect.x(), rect.y(), rect.width(), rect.height()};
}

void QT_graphics_scene_definis_rect_scene(QT_GraphicsScene *scene, QT_RectF rect)
{
    auto qscene = vers_qt(scene);
    auto qrect = QRectF(rect.x, rect.y, rect.largeur, rect.hauteur);
    qscene->setSceneRect(qrect);
}

void QT_graphics_scene_ajoute_item(QT_GraphicsScene *scene, QT_Generic_GraphicsItem item)
{
    auto qscene = vers_qt(scene);
    auto qitem = vers_qt(item);
    qscene->addItem(qitem);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsView
 * \{ */

QT_GraphicsView *QT_cree_graphics_view(QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new GraphicsView(qparent));
}

void QT_graphics_view_definis_scene(QT_GraphicsView *graphics_view, QT_GraphicsScene *scene)
{
    auto qgraphics_view = vers_qt(graphics_view);
    auto qscene = vers_qt(scene);
    qgraphics_view->setScene(qscene);
}

void QT_graphics_view_reinit_transforme(QT_GraphicsView *graphics_view)
{
    auto qgraphics_view = vers_qt(graphics_view);
    qgraphics_view->resetTransform();
}

void QT_graphics_view_definis_echelle_taille(QT_GraphicsView *graphics_view, float x, float y)
{
    auto qgraphics_view = vers_qt(graphics_view);
    qgraphics_view->scale(double(x), double(y));
}

void QT_graphics_view_mappe_vers_scene(QT_GraphicsView *graphics_view,
                                       QT_Point *point,
                                       QT_PointF *r_point)
{
    auto qgraphics_view = vers_qt(graphics_view);
    auto résultat = qgraphics_view->mapToScene(QPoint(point->x, point->y));
    if (r_point) {
        *r_point = QT_PointF{résultat.x(), résultat.y()};
    }
}

void QT_graphics_view_mappe_depuis_scene(QT_GraphicsView *graphics_view,
                                         QT_PointF *point,
                                         QT_Point *r_point)
{
    auto qgraphics_view = vers_qt(graphics_view);
    auto résultat = qgraphics_view->mapFromScene(QPointF(point->x, point->y));
    if (r_point) {
        *r_point = QT_Point{résultat.x(), résultat.y()};
    }
}

void QT_graphics_view_mappe_vers_global(QT_GraphicsView *graphics_view,
                                        QT_Point point,
                                        QT_Point *r_point)
{
    auto qgraphics_view = vers_qt(graphics_view);
    auto résultat = qgraphics_view->mapToGlobal(QPoint(point.x, point.y));
    if (r_point) {
        *r_point = QT_Point{résultat.x(), résultat.y()};
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Variant
 * \{ */

class EnveloppeVariant : public QT_Variant {
    QVariant m_variant{};

    static void sur_définis_chaine(QT_Variant *variant, QT_Chaine chaine)
    {
        auto enveloppe = static_cast<EnveloppeVariant *>(variant);
        enveloppe->m_variant = vers_qt(chaine);
    }

#define ENUMERE_RAPPEL_TYPE_STANDARD(type_kuri, type_cpp)                                         \
    static void sur_définis_##type_kuri(QT_Variant *variant, type_cpp valeur)                     \
    {                                                                                             \
        auto enveloppe = static_cast<EnveloppeVariant *>(variant);                                \
        enveloppe->m_variant.setValue(valeur);                                                    \
    }

    ENUMERE_TYPE_STANDARD(ENUMERE_RAPPEL_TYPE_STANDARD)
#undef ENUMERE_RAPPEL_TYPE_STANDARD

  public:
    EnveloppeVariant()
    {
        definis_chaine = sur_définis_chaine;
#define ENUMERE_RAPPEL_TYPE_STANDARD(type_kuri, type_cpp)                                         \
    definis_##type_kuri = sur_définis_##type_kuri;
        ENUMERE_TYPE_STANDARD(ENUMERE_RAPPEL_TYPE_STANDARD)
#undef ENUMERE_RAPPEL_TYPE_STANDARD
    }

    QVariant donne_variant() const
    {
        return m_variant;
    }
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Item_Data_Role
 * \{ */

static QT_Item_Data_Role convertis_role(Qt::ItemDataRole role)
{
    switch (role) {
        ENUMERE_ITEM_DATA_ROLE(ENUMERE_TRANSLATION_ENUM_QT_VERS_IPA)
    }

    return QT_ITEM_DATA_ROLE_Display;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_TableModel
 * \{ */

class ModèleTable final : public QAbstractTableModel {
    QT_Rappels_TableModel *m_rappels = nullptr;

  public:
    ModèleTable(QT_Rappels_TableModel *rappels, QObject *parent)
        : QAbstractTableModel(parent), m_rappels(rappels)
    {
    }

    EMPECHE_COPIE(ModèleTable);

    ~ModèleTable() override
    {
        if (m_rappels && m_rappels->sur_destruction) {
            m_rappels->sur_destruction(m_rappels);
        }
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (m_rappels && m_rappels->donne_nombre_lignes) {
            auto model = vers_ipa(parent);
            return m_rappels->donne_nombre_lignes(m_rappels, &model);
        }
        return 0;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (m_rappels && m_rappels->donne_nombre_colonnes) {
            auto model = vers_ipa(parent);
            return m_rappels->donne_nombre_colonnes(m_rappels, &model);
        }
        return 0;
    }

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override
    {
        if (!m_rappels || !m_rappels->donne_donnee_entete) {
            return {};
        }

        auto enveloppe_variant = EnveloppeVariant();
        m_rappels->donne_donnee_entete(m_rappels,
                                       section,
                                       convertis_orientation(orientation),
                                       convertis_role(Qt::ItemDataRole(role)),
                                       &enveloppe_variant);
        return enveloppe_variant.donne_variant();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!m_rappels || !m_rappels->donne_donnee_cellule) {
            return {};
        }

        auto enveloppe_variant = EnveloppeVariant();
        auto model = vers_ipa(index);
        m_rappels->donne_donnee_cellule(
            m_rappels, &model, convertis_role(Qt::ItemDataRole(role)), &enveloppe_variant);
        return enveloppe_variant.donne_variant();
    }
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_AbstractTableModel
 * \{ */

QT_AbstractTableModel *QT_cree_table_model(QT_Rappels_TableModel *rappels,
                                           QT_Generic_Object parent)
{
    VERS_QT(parent);
    auto modèle_table = new ModèleTable(rappels, qparent);
    rappels->table_model = vers_ipa(modèle_table);
    return vers_ipa(modèle_table);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_SortFilterProxyModel
 * \{ */

QT_SortFilterProxyModel *QT_cree_sort_filter_proxy_model(QT_Generic_Object parent)
{
    VERS_QT(parent);
    auto résultat = new QSortFilterProxyModel(qparent);
    return vers_ipa(résultat);
}

void QT_sort_filter_proxy_model_definis_model_source(QT_SortFilterProxyModel *sfpm,
                                                     QT_Generic_ItemModel model)
{
    VERS_QT(model);
    VERS_QT(sfpm);
    qsfpm->setSourceModel(qmodel);
}

void QT_sort_filter_proxy_model_definis_regex_filtre(QT_SortFilterProxyModel *sfpm,
                                                     QT_Chaine *regex)
{
    VERS_QT(sfpm);
    VERS_QT(regex);
    qsfpm->setFilterRegExp(qregex);
}

void QT_sort_filter_proxy_model_definis_colonne_filtre(QT_SortFilterProxyModel *sfpm, int colonne)
{
    VERS_QT(sfpm);
    qsfpm->setFilterKeyColumn(colonne);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_TableView
 * \{ */

static QAbstractItemView::SelectionBehavior convertis_comportement_selection(
    QT_Item_View_Selection_Behavior comportement)
{
    switch (comportement) {
        ENUMERE_ITEM_VIEW_SELECTION_BEHAVIOR(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QAbstractItemView::SelectItems;
}

QT_TableView *QT_cree_table_view(QT_Generic_Widget parent)
{
    VERS_QT(parent);
    return vers_ipa(new QTableView(qparent));
}

void QT_table_view_definis_model(QT_TableView *view,
                                 QT_Generic_ItemModel model,
                                 bool detruit_model_existant)
{
    VERS_QT(view);
    VERS_QT(model);
    if (detruit_model_existant) {
        auto model_existant = qview->model();
        delete model_existant;
    }

    qview->setModel(qmodel);
}

void QT_table_view_cache_colonne(QT_TableView *view, int colonne)
{
    VERS_QT(view);
    qview->setColumnHidden(colonne, true);
}

QT_HeaderView *QT_table_view_donne_entete_horizontale(QT_TableView *view)
{
    VERS_QT(view);
    auto résultat = qview->horizontalHeader();
    return vers_ipa(résultat);
}

QT_HeaderView *QT_table_view_donne_entete_verticale(QT_TableView *view)
{
    VERS_QT(view);
    auto résultat = qview->verticalHeader();
    return vers_ipa(résultat);
}

void QT_table_view_definis_comportement_selection(QT_TableView *view,
                                                  QT_Item_View_Selection_Behavior comportement)
{
    VERS_QT(view);
    qview->setSelectionBehavior(convertis_comportement_selection(comportement));
}

void QT_table_view_connecte_sur_changement_item(QT_TableView *view, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(view);
    auto model = qview->selectionModel();
    if (!model) {
        return;
    }

    QObject::connect(
        model,
        &QItemSelectionModel::currentChanged,
        [=](const QModelIndex &, const QModelIndex &) { rappel->sur_rappel(rappel); });
}

void QT_table_view_connecte_sur_changement_selection(QT_TableView *view,
                                                     QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(view);
    auto model = qview->selectionModel();
    if (!model) {
        return;
    }

    QObject::connect(
        model,
        &QItemSelectionModel::selectionChanged,
        [=](const QItemSelection &, const QItemSelection &) { rappel->sur_rappel(rappel); });
}

void QT_table_view_donne_item_courant(QT_TableView *view, QT_ModelIndex *r_index)
{
    if (!r_index) {
        return;
    }

    VERS_QT(view);
    auto model = qview->selectionModel();
    if (!model) {
        return;
    }

    *r_index = vers_ipa(model->currentIndex());
}

bool QT_table_view_possede_selection(QT_TableView *view)
{
    VERS_QT(view);
    auto model = qview->selectionModel();
    if (!model) {
        return false;
    }
    return model->hasSelection();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_HeaderView
 * \{ */

static QHeaderView::ResizeMode convertis_resize_mode(QT_Header_View_Resize_Mode mode)
{
    switch (mode) {
        ENUMERE_HEADER_VIEW_RESIZE_MODE(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QHeaderView::Interactive;
}

void QT_header_view_cache_colonne(QT_HeaderView *view, int colonne)
{
    VERS_QT(view);
    qview->setSectionHidden(colonne, true);
}

void QT_header_view_definis_mode_redimension(QT_HeaderView *view,
                                             QT_Header_View_Resize_Mode mode,
                                             int colonne)
{
    VERS_QT(view);
    qview->setSectionResizeMode(colonne, convertis_resize_mode(mode));
}

void QT_header_view_definis_mode_redimension_section(QT_HeaderView *view,
                                                     QT_Header_View_Resize_Mode mode)
{
    VERS_QT(view);
    qview->setSectionResizeMode(convertis_resize_mode(mode));
}

void QT_header_view_definis_alignement_defaut(QT_HeaderView *view, QT_Alignment alignement)
{
    VERS_QT(view);
    qview->setDefaultAlignment(convertis_alignement(alignement));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Slider_Tick_Position
 * \{ */

static QSlider::TickPosition convertis_position_ticks(QT_Slider_Tick_Position position)
{
    switch (position) {
        ENUMERE_SLIDER_TICK_POSITION(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QSlider::NoTicks;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Slider
 * \{ */

QT_Slider *QT_cree_slider(QT_Generic_Widget parent)
{
    VERS_QT(parent);
    return vers_ipa(new QSlider(qparent));
}

void QT_slider_sur_changement_valeur(QT_Slider *slider, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(slider);
    QObject::connect(qslider, &QSlider::valueChanged, [=](int) { rappel->sur_rappel(rappel); });
}

void QT_slider_definis_valeur(QT_Slider *slider, int valeur)
{
    VERS_QT(slider);
    qslider->setValue(valeur);
}

int QT_slider_donne_valeur(QT_Slider *slider)
{
    VERS_QT(slider);
    return qslider->value();
}

void QT_slider_definis_orientation(QT_Slider *slider, QT_Orientation orientation)
{
    VERS_QT(slider);
    qslider->setOrientation(convertis_orientation(orientation));
}

void QT_slider_definis_position_tick(QT_Slider *slider, QT_Slider_Tick_Position position)
{
    VERS_QT(slider);
    qslider->setTickPosition(convertis_position_ticks(position));
}

void QT_slider_definis_interval_tick(QT_Slider *slider, int valeur)
{
    VERS_QT(slider);
    qslider->setTickInterval(valeur);
}

void QT_slider_definis_plage(QT_Slider *slider, int minimum, int maximum)
{
    VERS_QT(slider);
    qslider->setMinimum(minimum);
    qslider->setMaximum(maximum);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ProgressBar
 * \{ */

QT_ProgressBar *QT_cree_progress_bar(QT_Generic_Widget parent)
{
    VERS_QT(parent);
    return vers_ipa(new QProgressBar(qparent));
}

void QT_progress_bar_definis_plage(QT_ProgressBar *progress_bar, int minimum, int maximum)
{
    VERS_QT(progress_bar);
    qprogress_bar->setRange(minimum, maximum);
}

void QT_progress_bar_definis_valeur(QT_ProgressBar *progress_bar, int valeur)
{
    VERS_QT(progress_bar);
    qprogress_bar->setValue(valeur);
}

void QT_progress_bar_definis_orientation(QT_ProgressBar *progress_bar, QT_Orientation orientation)
{
    VERS_QT(progress_bar);
    qprogress_bar->setOrientation(convertis_orientation(orientation));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_TextCursor
 * \{ */

static QTextCursor::MoveOperation convertis_move_operation(QT_Text_Cursor_Move_Operation op)
{
    switch (op) {
        ENUMERE_TEXT_CURSOR_MOVE_OPERATION(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QTextCursor::NoMove;
}

static QTextCursor::MoveMode convertis_move_mode(QT_Text_Cursor_Move_Mode mode)
{
    switch (mode) {
        ENUMERE_TEXT_CURSOR_MOVE_MODE(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QTextCursor::MoveAnchor;
}

int QT_text_cursor_donne_position(QT_TextCursor *cursor)
{
    VERS_QT(cursor);
    return qcursor->position();
}

void QT_text_cursor_definis_position(QT_TextCursor *cursor,
                                     int position,
                                     QT_Text_Cursor_Move_Mode mode)
{
    VERS_QT(cursor);
    qcursor->setPosition(position, convertis_move_mode(mode));
}

void QT_text_cursor_deplace_vers(QT_TextCursor *cursor,
                                 QT_Text_Cursor_Move_Operation op,
                                 QT_Text_Cursor_Move_Mode mode,
                                 int n)
{
    VERS_QT(cursor);
    qcursor->movePosition(convertis_move_operation(op), convertis_move_mode(mode), n);
}

void QT_text_cursor_donne_texte_selection(QT_TextCursor *cursor, QT_Chaine *résultat)
{
    if (!résultat) {
        return;
    }

    VERS_QT(cursor);

    static char tampon[128];

    auto text = qcursor->selectedText().toStdString();

    if (text.size() < 128) {
        memcpy(tampon, text.c_str(), text.size());
        résultat->caractères = tampon;
        résultat->taille = int64_t(text.size());
    }
    else {
        résultat->caractères = nullptr;
        résultat->taille = 0;
    }
}

void QT_text_cursor_insere_texte(QT_TextCursor *cursor, QT_Chaine texte)
{
    VERS_QT(cursor);
    VERS_QT(texte);
    qcursor->insertText(qtexte);
}

bool QT_text_cursor_possede_selection_apres(QT_TextCursor *cursor, int position)
{
    VERS_QT(cursor);
    if (!qcursor->hasSelection()) {
        return false;
    }
    auto début = qcursor->selectionStart();
    auto fin = qcursor->selectionEnd();
    return début >= position && fin >= position;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_PlainTextEdit
 * \{ */

QT_PlainTextEdit *QT_cree_plain_text_edit(QT_Rappels_PlainTextEdit *rappels,
                                          QT_Generic_Widget parent)
{
    VERS_QT(parent);
    auto résultat = new PlainTextEdit(rappels, qparent);
    if (rappels) {
        rappels->widget = vers_ipa(résultat);
    }
    return vers_ipa(résultat);
}

QT_Rappels_PlainTextEdit *QT_plain_text_edit_donne_rappels(QT_PlainTextEdit *text_edit)
{
    auto base_text_edit = reinterpret_cast<QPlainTextEdit *>(text_edit);
    if (auto qtext_edit = dynamic_cast<PlainTextEdit *>(base_text_edit)) {
        return qtext_edit->donne_rappels();
    }
    return nullptr;
}

QT_Chaine QT_plain_text_edit_donne_texte(QT_PlainTextEdit *text_edit)
{
    VERS_QT(text_edit);
    auto texte = qtext_edit->toPlainText().toStdString();

    QT_Chaine résultat;
    résultat.caractères = new char[texte.size()];
    résultat.taille = int64_t(texte.size());
    memcpy(résultat.caractères, texte.c_str(), texte.size());
    return résultat;
}

void QT_plain_text_edit_definis_texte(struct QT_PlainTextEdit *text_edit, struct QT_Chaine *texte)
{
    VERS_QT(text_edit);
    VERS_QT(texte);
    qtext_edit->setPlainText(qtexte);
}

QT_TextCursor *QT_plain_text_edit_donne_curseur(QT_PlainTextEdit *text_edit)
{
    auto base_text_edit = reinterpret_cast<QPlainTextEdit *>(text_edit);
    if (auto qtext_edit = dynamic_cast<PlainTextEdit *>(base_text_edit)) {
        return vers_ipa(qtext_edit->donne_cursor());
    }
    return nullptr;
}

void QT_plain_text_edit_definis_curseur(QT_PlainTextEdit *text_edit, QT_TextCursor *cursor)
{
    VERS_QT(text_edit);
    VERS_QT(cursor);
    if (cursor) {
        qtext_edit->setTextCursor(*qcursor);
    }
}

void QT_plain_text_edit_coupe(QT_PlainTextEdit *text_edit)
{
    VERS_QT(text_edit);
    qtext_edit->cut();
}

void QT_plain_text_edit_copie(QT_PlainTextEdit *text_edit)
{
    VERS_QT(text_edit);
    qtext_edit->copy();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_SpinBoxButtonSymbols
 * \{ */

static QAbstractSpinBox::ButtonSymbols convertis_spinbox_button_symbols(
    QT_SpinBox_Button_Symbols symbols)
{
    switch (symbols) {
        ENUMERE_SPINBOX_BUTTON_SYMBOLS(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QAbstractSpinBox::UpDownArrows;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_SpinBox
 * \{ */

QT_SpinBox *QT_cree_spinbox(QT_Generic_Widget parent)
{
    VERS_QT(parent);
    return vers_ipa(new QSpinBox(qparent));
}

void QT_spinbox_sur_changement_valeur(QT_SpinBox *spinbox, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(spinbox);
    QObject::connect(qspinbox, qOverload<int>(&QSpinBox::valueChanged), [=](int) {
        rappel->sur_rappel(rappel);
    });
}

void QT_spinbox_definis_alignement(QT_SpinBox *spinbox, QT_Alignment alignement)
{
    VERS_QT(spinbox);
    qspinbox->setAlignment(convertis_alignement(alignement));
}

void QT_spinbox_definis_plage(QT_SpinBox *spinbox, int minimum, int maximum)
{
    VERS_QT(spinbox);
    qspinbox->setMinimum(minimum);
    qspinbox->setMaximum(maximum);
}

void QT_spinbox_definis_valeur(QT_SpinBox *spinbox, int valeur)
{
    VERS_QT(spinbox);
    qspinbox->setValue(valeur);
}

int QT_spinbox_donne_valeur(QT_SpinBox *spinbox)
{
    VERS_QT(spinbox);
    return qspinbox->value();
}

void QT_spinbox_definis_lecture_seule(QT_SpinBox *spinbox, bool ouinon)
{
    VERS_QT(spinbox);
    qspinbox->setReadOnly(ouinon);
}

void QT_spinbox_definis_symboles_boutons(QT_SpinBox *spinbox, QT_SpinBox_Button_Symbols symbols)
{
    VERS_QT(spinbox);
    qspinbox->setButtonSymbols(convertis_spinbox_button_symbols(symbols));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_DoubleSpinBox
 * \{ */

QT_DoubleSpinBox *QT_cree_doublespinbox(QT_Generic_Widget parent)
{
    VERS_QT(parent);
    return vers_ipa(new QDoubleSpinBox(qparent));
}

void QT_doublespinbox_sur_changement_valeur(QT_DoubleSpinBox *doublespinbox,
                                            QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(doublespinbox);
    QObject::connect(qdoublespinbox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=](int) {
        rappel->sur_rappel(rappel);
    });
}

void QT_doublespinbox_definis_alignement(QT_DoubleSpinBox *doublespinbox, QT_Alignment alignement)
{
    VERS_QT(doublespinbox);
    qdoublespinbox->setAlignment(convertis_alignement(alignement));
}

void QT_doublespinbox_definis_plage(QT_DoubleSpinBox *doublespinbox,
                                    double minimum,
                                    double maximum)
{
    VERS_QT(doublespinbox);
    qdoublespinbox->setMinimum(minimum);
    qdoublespinbox->setMaximum(maximum);
}

void QT_doublespinbox_definis_valeur(QT_DoubleSpinBox *doublespinbox, double valeur)
{
    VERS_QT(doublespinbox);
    qdoublespinbox->setValue(valeur);
}

double QT_doublespinbox_donne_valeur(QT_DoubleSpinBox *doublespinbox)
{
    VERS_QT(doublespinbox);
    return qdoublespinbox->value();
}

void QT_doublespinbox_definis_lecture_seule(QT_DoubleSpinBox *doublespinbox, bool ouinon)
{
    VERS_QT(doublespinbox);
    qdoublespinbox->setReadOnly(ouinon);
}

void QT_doublespinbox_definis_symboles_boutons(QT_DoubleSpinBox *doublespinbox,
                                               QT_SpinBox_Button_Symbols symbols)
{
    VERS_QT(doublespinbox);
    qdoublespinbox->setButtonSymbols(convertis_spinbox_button_symbols(symbols));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Pilote_Clique
 * \{ */

DNJ_Pilote_Clique *DNJ_cree_pilote_clique(DNJ_Rappels_Pilote_Clique *rappels)
{
    auto résultat = new PiloteClique(rappels);
    return reinterpret_cast<DNJ_Pilote_Clique *>(résultat);
}

void DNJ_detruit_pilote_clique(DNJ_Pilote_Clique *pilote)
{
    auto qpilote = reinterpret_cast<PiloteClique *>(pilote);
    delete qpilote;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Conteneur_Controles
 * \{ */

DNJ_Conteneur_Controles *DNJ_cree_conteneur_controle(DNJ_Rappels_Widget *rappels,
                                                     QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = new ConteneurControles(rappels, qparent);
    if (rappels) {
        rappels->widget = vers_ipa(résultat);
    }
    return vers_ipa(résultat);
}

QT_Layout *DNJ_conteneur_cree_interface(DNJ_Conteneur_Controles *conteneur)
{
    auto qconteneur = vers_qt(conteneur);
    auto résultat = qconteneur->crée_interface();
    return vers_ipa(résultat);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Gestionnaire_Interface
 * \{ */

static danjo::DonneesInterface convertis_contexte(DNJ_Contexte_Interface *context)
{
    auto résultat = danjo::DonneesInterface();
    résultat.repondant_bouton = reinterpret_cast<PiloteClique *>(context->pilote_clique);
    résultat.conteneur = vers_qt(context->conteneur);
    résultat.parent_menu = vers_qt(context->parent_menu);
    résultat.parent_barre_outils = vers_qt(context->parent_barre_outils);
    return résultat;
}

DNJ_Gestionnaire_Interface *DNJ_cree_gestionnaire_interface()
{
    auto résultat = new danjo::GestionnaireInterface();
    return reinterpret_cast<DNJ_Gestionnaire_Interface *>(résultat);
}

void DNJ_detruit_gestionnaire_interface(DNJ_Gestionnaire_Interface *gestionnaire)
{
    auto dnj_gestionnaire = reinterpret_cast<danjo::GestionnaireInterface *>(gestionnaire);
    delete dnj_gestionnaire;
}

QT_Menu *DNJ_gestionaire_compile_menu_fichier(DNJ_Gestionnaire_Interface *gestionnaire,
                                              DNJ_Contexte_Interface *context,
                                              QT_Chaine chemin)
{
    if (!context) {
        return nullptr;
    }

    auto données = convertis_contexte(context);
    auto dnj_gestionnaire = reinterpret_cast<danjo::GestionnaireInterface *>(gestionnaire);
    auto résultat = dnj_gestionnaire->compile_menu_fichier(données, chemin.vers_std_string());
    return vers_ipa(résultat);
}

QT_Menu *DNJ_gestionaire_compile_menu_texte(DNJ_Gestionnaire_Interface *gestionnaire,
                                            DNJ_Contexte_Interface *context,
                                            QT_Chaine texte)
{
    if (!context) {
        return nullptr;
    }

    auto données = convertis_contexte(context);
    auto dnj_gestionnaire = reinterpret_cast<danjo::GestionnaireInterface *>(gestionnaire);
    auto résultat = dnj_gestionnaire->compile_menu_texte(données, texte.vers_std_string());
    return vers_ipa(résultat);
}

QT_Menu *DNJ_gestionnaire_donne_menu(DNJ_Gestionnaire_Interface *gestionnaire, QT_Chaine nom_menu)
{
    auto dnj_gestionnaire = reinterpret_cast<danjo::GestionnaireInterface *>(gestionnaire);
    auto résultat = dnj_gestionnaire->pointeur_menu(nom_menu.vers_std_string());
    return vers_ipa(résultat);
}

void DNJ_gestionnaire_recree_menu(DNJ_Gestionnaire_Interface *gestionnaire,
                                  QT_Chaine nom_menu,
                                  DNJ_Donnees_Action *actions,
                                  int64_t nombre_actions)
{
    if (!actions || nombre_actions == 0) {
        return;
    }

    auto données_actions = dls::tableau<danjo::DonneesAction>();

    for (int i = 0; i < nombre_actions; i++) {
        auto action = actions[i];

        auto donnée = danjo::DonneesAction{};
        donnée.attache = action.attache.vers_std_string();
        donnée.metadonnee = action.metadonnee.vers_std_string();
        donnée.nom = action.nom.vers_std_string();
        donnée.repondant_bouton = reinterpret_cast<PiloteClique *>(action.pilote_clique);

        données_actions.ajoute(donnée);
    }

    auto dnj_gestionnaire = reinterpret_cast<danjo::GestionnaireInterface *>(gestionnaire);
    dnj_gestionnaire->recree_menu(nom_menu.vers_std_string(), données_actions);
}

QT_ToolBar *DNJ_gestionaire_compile_barre_a_outils_fichier(
    DNJ_Gestionnaire_Interface *gestionnaire, DNJ_Contexte_Interface *context, QT_Chaine chemin)
{
    if (!context) {
        return nullptr;
    }

    auto données = convertis_contexte(context);
    auto dnj_gestionnaire = reinterpret_cast<danjo::GestionnaireInterface *>(gestionnaire);
    auto résultat = dnj_gestionnaire->compile_barre_outils_fichier(données,
                                                                   chemin.vers_std_string());
    return vers_ipa(résultat);
}

QT_BoxLayout *DNJ_gestionnaire_compile_entreface_fichier(DNJ_Gestionnaire_Interface *gestionnaire,
                                                         DNJ_Contexte_Interface *context,
                                                         QT_Chaine chemin)
{
    if (!context) {
        return nullptr;
    }

    auto manipulable = danjo::Manipulable{};
    auto données = convertis_contexte(context);
    données.manipulable = &manipulable;
    auto dnj_gestionnaire = reinterpret_cast<danjo::GestionnaireInterface *>(gestionnaire);
    auto résultat = dnj_gestionnaire->compile_entreface_fichier(données, chemin.vers_std_string());
    return vers_ipa(résultat);
}

void DNJ_gestionnaire_ajourne_controles(DNJ_Gestionnaire_Interface *gestionnaire)
{
    auto dnj_gestionnaire = reinterpret_cast<danjo::GestionnaireInterface *>(gestionnaire);
    dnj_gestionnaire->ajourne_controles();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_FournisseuseIcone
 * \{ */

DNJ_FournisseuseIcone *DNJ_cree_fournisseuse_icone(DNJ_Rappels_Fournisseuse_Icone *rappels)
{
    auto résultat = new FournisseuseIcône(rappels);
    return reinterpret_cast<DNJ_FournisseuseIcone *>(résultat);
}

void DNJ_detruit_fournisseuse_icone(DNJ_FournisseuseIcone *fournisseuse)
{
    auto qfournisseuse = reinterpret_cast<FournisseuseIcône *>(fournisseuse);
    delete qfournisseuse;
}

void DNJ_definis_fournisseuse_icone(DNJ_FournisseuseIcone *fournisseuse)
{
    auto qfournisseuse = reinterpret_cast<FournisseuseIcône *>(fournisseuse);

    if (qfournisseuse) {
        danjo::définis_fournisseuse_icone(*qfournisseuse);
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Rappels_DialoguesChemins
 * \{ */

struct DNJ_DialoguesChemins *DNJ_cree_dialogues_chemins(
    struct DNJ_Rappels_DialoguesChemins *rappels)
{
    auto résultat = new DialoguesChemins(rappels);
    return reinterpret_cast<DNJ_DialoguesChemins *>(résultat);
}

void DNJ_detruit_dialogues_chemins(struct DNJ_DialoguesChemins *dialogues)
{
    auto qdialogues = reinterpret_cast<DialoguesChemins *>(dialogues);
    delete qdialogues;
}

void DNJ_definis_dialogues_chemins(struct DNJ_DialoguesChemins *dialogues)
{
    auto qdialogues = reinterpret_cast<DialoguesChemins *>(dialogues);
    if (qdialogues) {
        danjo::définis_dialogues_chemins(*qdialogues);
    }
}

/** \} */
}
