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
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
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

inline QGraphicsItem *vers_qt(QT_Generic_GraphicsItem item)
{
    return reinterpret_cast<QGraphicsItem *>(item.item);
}

inline QPixmap *vers_qt(QT_Pixmap *pixmap)
{
    return reinterpret_cast<QPixmap *>(pixmap);
}

inline QT_Pixmap *vers_ipa(QPixmap *pixmap)
{
    return reinterpret_cast<QT_Pixmap *>(pixmap);
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

inline QPointF vers_qt(QT_PointF point)
{
    return QPointF(point.x, point.y);
}

inline QT_RectF vers_ipa(QRectF rect)
{
    return QT_RectF{rect.x(), rect.y(), rect.width(), rect.height()};
}

inline QFont vers_qt(QT_Font font)
{
    auto résultat = QFont();
    résultat.setPointSize(font.taille_point);
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
/** \name QT_Pixmap
 * \{ */

struct QT_Pixmap;

QT_Pixmap *QT_cree_pixmap(QT_Chaine chemin)
{
    return vers_ipa(new QPixmap(QString(chemin.vers_std_string().c_str())));
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
    qobject->setProperty(nom->vers_std_string().c_str(),
                         QString(valeur->vers_std_string().c_str()));
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

void QT_fenetre_principale_definis_titre_fenetre(QT_Fenetre_Principale *fenetre, QT_Chaine nom)
{
    auto fenêtre_qt = vers_qt(fenetre);
    fenêtre_qt->setWindowTitle(nom.vers_std_string().c_str());
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
    QCoreApplication::setOrganizationName(nom.vers_std_string().c_str());
}

void QT_core_application_definis_nom_application(QT_Chaine nom)
{
    QCoreApplication::setApplicationName(nom.vers_std_string().c_str());
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
        q_string_liste.append(liste[i].vers_std_string().c_str());
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
    return vers_ipa(new QAction(texte.vers_std_string().c_str(), qparent));
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
/** \name QT_Color
 * \{ */

QT_Color QT_color_depuis_tsl(double t, double s, double l, double a)
{
    auto résultat = QColor::fromHslF(t, s, l, a);
    return vers_ipa(résultat);
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
    qwidget->setStyleSheet(texte->vers_std_string().c_str());
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
                                            QT_Point point,
                                            QT_Point *r_point)
{
    if (!r_point) {
        return;
    }

    VERS_QT(widget);
    auto résultat = qwidget->mapToGlobal(QPoint(point.x, point.y));
    *r_point = QT_Point{résultat.x(), résultat.y()};
}

void QT_widget_transforme_point_vers_local(QT_Generic_Widget widget,
                                           QT_Point point,
                                           QT_Point *r_point)
{
    if (!r_point) {
        return;
    }

    VERS_QT(widget);
    auto résultat = qwidget->mapFromGlobal(QPoint(point.x, point.y));
    *r_point = QT_Point{résultat.x(), résultat.y()};
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

void QT_layout_ajoute_layout(QT_Generic_Layout layout, QT_Generic_Layout sous_layout)
{
    auto qlayout = vers_qt(layout);
    auto qsous_layout = vers_qt(sous_layout);

    if (auto hbox = dynamic_cast<QHBoxLayout *>(qlayout)) {
        hbox->addLayout(qsous_layout);
    }
    else if (auto vbox = dynamic_cast<QVBoxLayout *>(qlayout)) {
        vbox->addLayout(qsous_layout);
    }
}

bool QT_layout_aligne_layout(QT_Generic_Layout layout,
                             QT_Generic_Layout sous_layout,
                             QT_Alignment alignement)
{
    auto qlayout = vers_qt(layout);
    auto qsous_layout = vers_qt(sous_layout);
    return qlayout->setAlignment(qsous_layout, convertis_alignement(alignement));
}

void QT_form_layout_ajoute_ligne_chaine(QT_FormLayout *layout,
                                        QT_Chaine label,
                                        QT_Generic_Widget widget)
{
    auto qlayout = vers_qt(layout);
    auto qwidget = vers_qt(widget);

    if (qwidget) {
        if (label.taille != 0) {
            qlayout->addRow(label.vers_std_string().c_str(), qwidget);
        }
        else {
            qlayout->addRow(qwidget);
        }
    }
    else {
        auto qlabel = new QLabel(label.vers_std_string().c_str());
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
    qcombo->addItem(texte.vers_std_string().c_str(), QString(valeur.vers_std_string().c_str()));
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

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_TabWidget
 * \{ */

QT_TabWidget *QT_cree_tab_widget(QT_Rappels_TabWidget *rappels, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new TabWidget(rappels, qparent));
}

QT_Rappels_TabWidget *QT_tab_widget_donne_rappels(QT_TabWidget *tab)
{
    VERS_QT(tab);
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
    qtab_widget->addTab(qwidget, nom->vers_std_string().c_str());
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
    QString qtexte;
    if (texte) {
        qtexte = texte->vers_std_string().c_str();
    }
    auto résultat = new QLabel(qtexte, qparent);
    return vers_ipa(résultat);
}

void QT_label_definis_texte(QT_Label *label, QT_Chaine texte)
{
    auto qlabel = vers_qt(label);
    qlabel->setText(texte.vers_std_string().c_str());
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
    QToolTip::showText(QPoint(point.x, point.y), texte.vers_std_string().c_str());
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
    qline->setText(texte.vers_std_string().c_str());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_PushButton
 * \{ */

QT_PushButton *QT_cree_push_button(QT_Chaine texte, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = new QPushButton(texte.vers_std_string().c_str(), qparent);
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

void QT_dialog_definis_bouton_accepter(QT_Dialog *dialog, QT_PushButton *bouton)
{
    auto qdialog = vers_qt(dialog);
    auto qbouton = vers_qt(bouton);
    QObject::connect(qbouton, &QPushButton::pressed, qdialog, &QDialog::accept);
}

void QT_dialog_definis_bouton_annuler(QT_Dialog *dialog, QT_PushButton *bouton)
{
    auto qdialog = vers_qt(dialog);
    auto qbouton = vers_qt(bouton);
    QObject::connect(qbouton, &QPushButton::pressed, qdialog, &QDialog::reject);
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

static QMessageBox::StandardButton vers_qt(QT_StandardButton drapeaux)
{
    int résultat = QMessageBox::StandardButton::Ok;
    ENUMERE_BOUTON_STANDARD(ENUMERE_TRANSLATION_ENUM_DRAPEAU_IPA_VERS_QT)
    return QMessageBox::StandardButton(résultat);
}

static QT_StandardButton vers_ipa(QMessageBox::StandardButton drapeaux)
{
    int résultat = QMessageBox::StandardButton::Ok;
    ENUMERE_BOUTON_STANDARD(ENUMERE_TRANSLATION_ENUM_DRAPEAU_QT_VERS_IPA)
    return QT_StandardButton(résultat);
}

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
    qwidget->setText(colonne, texte->vers_std_string().c_str());
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

void QT_graphics_rect_item_definis_pinceau(QT_GraphicsRectItem *item, QT_Pen pinceau)
{
    auto qitem = vers_qt(item);
    auto qpen = vers_qt(pinceau);
    qitem->setPen(qpen);
}

void QT_graphics_rect_item_definis_brosse(QT_GraphicsRectItem *item, QT_Brush brush)
{
    auto qitem = vers_qt(item);
    auto qbrush = vers_qt(brush);
    qitem->setBrush(qbrush);
}

void QT_graphics_rect_item_definis_rect(QT_GraphicsRectItem *item, QT_RectF rect)
{
    auto qitem = vers_qt(item);
    auto qrect = QRectF(rect.x, rect.y, rect.largeur, rect.hauteur);
    qitem->setRect(qrect);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsTextItem
 * \{ */

QT_GraphicsTextItem *QT_cree_graphics_text_item(QT_Chaine texte, QT_Generic_GraphicsItem parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QGraphicsTextItem(texte.vers_std_string().c_str(), qparent));
}

void QT_graphics_text_item_definis_police(QT_GraphicsTextItem *text_item, QT_Font font)
{
    VERS_QT(text_item);
    VERS_QT(font);
    qtext_item->setFont(qfont);
}

void QT_graphics_text_item_definis_couleur_defaut(QT_GraphicsTextItem *text_item, QT_Color color)
{
    VERS_QT(text_item);
    VERS_QT(color);
    qtext_item->setDefaultTextColor(qcolor);
}

QT_RectF QT_graphics_text_item_donne_rect(QT_GraphicsTextItem *item)
{
    VERS_QT(item);
    auto rect = qitem->boundingRect();
    return vers_ipa(rect);
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

void QT_graphics_line_item_definis_pinceau(QT_GraphicsLineItem *item, QT_Pen pinceau)
{
    auto qitem = vers_qt(item);
    auto qpen = vers_qt(pinceau);
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

QT_RectF QT_graphics_scene_donne_rect_scene(QT_GraphicsScene *scene)
{
    auto qscene = vers_qt(scene);
    auto rect = qscene->sceneRect();
    return QT_RectF{rect.x(), rect.y(), rect.width(), rect.height()};
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
    qgraphics_view->scale(x, y);
}

void QT_graphics_view_mappe_vers_scene(QT_GraphicsView *graphics_view,
                                       QT_Point point,
                                       QT_PointF *r_point)
{
    auto qgraphics_view = vers_qt(graphics_view);
    auto résultat = qgraphics_view->mapToScene(QPoint(point.x, point.y));
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
        enveloppe->m_variant = QString(chaine.vers_std_string().c_str());
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
    ModèleTable(QT_Rappels_TableModel *rappels) : m_rappels(rappels)
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
/** \name QT_TableView
 * \{ */

QT_TableView *QT_cree_table_view(QT_Generic_Widget parent)
{
    VERS_QT(parent);
    return vers_ipa(new QTableView(qparent));
}

void QT_table_view_definis_model(QT_TableView *view,
                                 QT_Rappels_TableModel *rappels,
                                 bool detruit_model_existant)
{
    VERS_QT(view);
    if (detruit_model_existant) {
        auto model = qview->model();
        delete model;
    }

    qview->setModel(new ModèleTable(rappels));
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
}
