/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
#    include <string>
extern "C" {
#else
typedef unsigned char bool;
#endif

/* ------------------------------------------------------------------------- */
/** \name Macros pour les énumérations.
 * \{ */

#define ENUMERE_DECLARATION_ENUM_IPA(nom_ipa, nom_qt) nom_ipa,

#define ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT(nom_ipa, nom_qt)                                     \
    case nom_ipa:                                                                                 \
    {                                                                                             \
        return nom_qt;                                                                            \
    }

#define ENUMERE_TRANSLATION_ENUM_QT_VERS_IPA(nom_ipa, nom_qt)                                     \
    case nom_qt:                                                                                  \
    {                                                                                             \
        return nom_ipa;                                                                           \
    }

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Chaine
 * \{ */

struct QT_Chaine {
    char *caractères;
    int64_t taille;

#ifdef __cplusplus
    std::string vers_std_string() const
    {
        if (!caractères) {
            return "";
        }

        return std::string(caractères, size_t(taille));
    }
#endif
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Taille
 * \{ */

struct QT_Taille {
    int largeur;
    int hauteur;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Pixmap
 * \{ */

struct QT_Pixmap;

struct QT_Pixmap *QT_cree_pixmap(struct QT_Chaine chemin);
void QT_detruit_pixmap(struct QT_Pixmap *pixmap);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_CursorShape
 * \{ */

/* À FAIRE : implémente tous les cas. */
#define ENUMERE_CURSOR_SHAPE(O)                                                                   \
    O(QT_CURSORSHAPE_ARROW, Qt::ArrowCursor)                                                      \
    O(QT_CURSORSHAPE_UP_ARROW, Qt::UpArrowCursor)                                                 \
    O(QT_CURSORSHAPE_CROSS, Qt::CrossCursor)                                                      \
    O(QT_CURSORSHAPE_WAIT, Qt::WaitCursor)                                                        \
    O(QT_CURSORSHAPE_IBEAM, Qt::IBeamCursor)                                                      \
    O(QT_CURSORSHAPE_SIZE_VERTICAL, Qt::SizeVerCursor)                                            \
    O(QT_CURSORSHAPE_SIZE_HORIZONTAL, Qt::SizeHorCursor)                                          \
    O(QT_CURSORSHAPE_SIZE_BDIALOG, Qt::SizeBDiagCursor)                                           \
    O(QT_CURSORSHAPE_SIZE_FDIALOG, Qt::SizeFDiagCursor)                                           \
    O(QT_CURSORSHAPE_SIZE_ALL, Qt::SizeAllCursor)                                                 \
    O(QT_CURSORSHAPE_BLANK, Qt::BlankCursor)                                                      \
    O(QT_CURSORSHAPE_SPLIT_VERTICAL, Qt::SplitVCursor)                                            \
    O(QT_CURSORSHAPE_SPLIT_HORIZONTAL, Qt::SplitHCursor)                                          \
    O(QT_CURSORSHAPE_POINTING_HAND, Qt::PointingHandCursor)                                       \
    O(QT_CURSORSHAPE_FORBIDDEN, Qt::ForbiddenCursor)                                              \
    O(QT_CURSORSHAPE_WHATS_THIS, Qt::WhatsThisCursor)                                             \
    O(QT_CURSORSHAPE_BUSY, Qt::BusyCursor)                                                        \
    O(QT_CURSORSHAPE_OPEN_HAND, Qt::OpenHandCursor)                                               \
    O(QT_CURSORSHAPE_CLOSED_HAND, Qt::ClosedHandCursor)                                           \
    O(QT_CURSORSHAPE_DRAG_COPY, Qt::DragCopyCursor)                                               \
    O(QT_CURSORSHAPE_DRAG_MOVE, Qt::DragMoveCursor)                                               \
    O(QT_CURSORSHAPE_DRAG_LINK, Qt::DragLinkCursor)                                               \
    O(QT_CURSORSHAPE_BITMAP, Qt::BitmapCursor)                                                    \
    O(QT_CURSORSHAPE_CUSTOM, Qt::CustomCursor)

enum QT_CursorShape { ENUMERE_CURSOR_SHAPE(ENUMERE_DECLARATION_ENUM_IPA) };

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Rappel_Generique
 * \{ */

struct QT_Rappel_Generique {
    void (*sur_rappel)(struct QT_Rappel_Generique *);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Generic_Widget
 * \{ */

#define ENUMERE_TYPES_WIDGETS(O)                                                                  \
    O(Widget, QT_Widget, widget)                                                                  \
    O(GLWidget, QT_GLWidget, glwidget)                                                            \
    O(ComboBox, QT_ComboBox, combo_box)                                                           \
    O(QSplitter, QT_Splitter, splitter)                                                           \
    O(QFrame, QT_Frame, frame)                                                                    \
    O(FenetrePrincipale, QT_Fenetre_Principale, fenetre_principale)                               \
    O(TabWidget, QT_TabWidget, tab_widget)                                                        \
    O(QScrollArea, QT_ScrollArea, scroll_area)                                                    \
    O(QCheckBox, QT_CheckBox, check_box)                                                          \
    O(QLabel, QT_Label, label)                                                                    \
    O(QLineEdit, QT_LineEdit, line_edit)                                                          \
    O(QTreeWidget, QT_TreeWidget, tree_widget)                                                    \
    O(TreeWidgetItem, QT_TreeWidgetItem, tree_widget_item)                                        \
    O(ConteneurControles, DNJ_Conteneur_Controles, conteneur_controles)                           \
    O(QPushButton, QT_PushButton, button)                                                         \
    O(QDialog, QT_Dialog, dialogue)                                                               \
    O(QGraphicsView, QT_GraphicsView, graphics_view)                                              \
    O(QStatusBar, QT_StatusBar, status_bar)

#define PRODECLARE_TYPES_WIDGETS(nom_qt, nom_classe, nom_union) struct nom_classe;
ENUMERE_TYPES_WIDGETS(PRODECLARE_TYPES_WIDGETS)
#undef PRODECLARE_TYPES_WIDGETS

/** Type générique pour passer des widgets de type dérivé aux fonctions devant prendre un QWidget.
 */
union QT_Generic_Widget {
#define DECLARE_TYPES_WIDGETS(nom_qt, nom_classe, nom_union) struct nom_classe *nom_union;
    ENUMERE_TYPES_WIDGETS(DECLARE_TYPES_WIDGETS)
#undef DECLARE_TYPES_WIDGETS
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Generic_Object
 * \{ */

#define ENUMERE_TYPES_OBJETS(O)                                                                   \
    O(QObject, QT_Object, object)                                                                 \
    O(QTimer, QT_Timer, timer)                                                                    \
    O(QStyle, QT_Style, style)                                                                    \
    O(QScreen, QT_Screen, screen)                                                                 \
    O(QGraphicsScene, QT_GraphicsScene, graphics_scene)

#define PRODECLARE_TYPES_OBJETS(nom_qt, nom_classe, nom_union) struct nom_classe;
ENUMERE_TYPES_OBJETS(PRODECLARE_TYPES_OBJETS)
#undef PRODECLARE_TYPES_OBJETS

/** Type générique pour passer des objects de type dérivé aux fonctions devant prendre un QWidget.
 */
union QT_Generic_Object {
#define DECLARE_TYPES_WIDGETS(nom_qt, nom_classe, nom_union) struct nom_classe *nom_union;
    /* Objets. */
    ENUMERE_TYPES_OBJETS(DECLARE_TYPES_WIDGETS)
    /* Widgets. */
    ENUMERE_TYPES_WIDGETS(DECLARE_TYPES_WIDGETS)
#undef DECLARE_TYPES_WIDGETS
};

void QT_object_definis_propriete_chaine(union QT_Generic_Object object,
                                        struct QT_Chaine *nom,
                                        struct QT_Chaine *valeur);

bool QT_object_bloque_signaux(union QT_Generic_Object object, bool ouinon);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Generic_Layout
 * \{ */

#define ENUMERE_TYPES_LAYOUTS(O)                                                                  \
    O(QLayout, QT_Layout, layout)                                                                 \
    O(QVBoxLayout, QT_VBoxLayout, vbox)                                                           \
    O(QHBoxLayout, QT_HBoxLayout, hbox)                                                           \
    O(QBoxLayout, QT_BoxLayout, box)                                                              \
    O(QFormLayout, QT_FormLayout, form)                                                           \
    O(QGridLayout, QT_GridLayout, grid)

#define PRODECLARE_TYPES_LAYOUTS(nom_qt, nom_classe, nom_union) struct nom_classe;
ENUMERE_TYPES_LAYOUTS(PRODECLARE_TYPES_LAYOUTS)
#undef PRODECLARE_TYPES_LAYOUTS

/** Type générique pour passer des layouts de type dérivé aux fonctions devant prendre un QLayout.
 */
union QT_Generic_Layout {
#define DECLARE_TYPES_LAYOUTS(nom_qt, nom_classe, nom_union) struct nom_classe *nom_union;
    ENUMERE_TYPES_LAYOUTS(DECLARE_TYPES_LAYOUTS)
#undef DECLARE_TYPES_LAYOUTS
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Generic_Event
 * \{ */

#define ENUMERE_TYPES_EVENTS(O)                                                                   \
    O(QEvent, QT_Evenement, event)                                                                \
    O(QMouseEvent, QT_MouseEvent, mouse_event)                                                    \
    O(QWheelEvent, QT_WheelEvent, wheel_event)                                                    \
    O(QResizeEvent, QT_ResizeEvent, resize_event)

#define PRODECLARE_TYPES_EVENTS(nom_qt, nom_classe, nom_union) struct nom_classe;
ENUMERE_TYPES_EVENTS(PRODECLARE_TYPES_EVENTS)
#undef PRODECLARE_TYPES_EVENTS

/** Type générique pour passer des layouts de type dérivé aux fonctions devant prendre un QLayout.
 */
union QT_Generic_Event {
#define DECLARE_TYPES_EVENTS(nom_qt, nom_classe, nom_union) struct nom_classe *nom_union;
    ENUMERE_TYPES_EVENTS(DECLARE_TYPES_EVENTS)
#undef DECLARE_TYPES_EVENTS
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Generic_GraphicsItem
 * \{ */

#define ENUMERE_TYPES_GRAPHICS_ITEM(O)                                                            \
    O(QGraphicsItem, QT_GraphicsItem, item)                                                       \
    O(QGraphicsLineItem, QT_GraphicsLineItem, line_item)                                          \
    O(QGraphicsRectItem, QT_GraphicsRectItem, rect_item)                                          \
    O(QGraphicsTextItem, QT_GraphicsTextItem, text_item)

#define PRODECLARE_TYPES_GRAPHICS_ITEM(nom_qt, nom_classe, nom_union) struct nom_classe;
ENUMERE_TYPES_GRAPHICS_ITEM(PRODECLARE_TYPES_GRAPHICS_ITEM)
#undef PRODECLARE_TYPES_GRAPHICS_ITEM

/** Type générique pour passer des layouts de type dérivé aux fonctions devant prendre un QLayout.
 */
union QT_Generic_GraphicsItem {
#define DECLARE_TYPES_GRAPHICS_ITEM(nom_qt, nom_classe, nom_union) struct nom_classe *nom_union;
    ENUMERE_TYPES_GRAPHICS_ITEM(DECLARE_TYPES_GRAPHICS_ITEM)
#undef DECLARE_TYPES_GRAPHICS_ITEM
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Creatrice_Barre_Menu
 * \{ */

struct QT_Creatrice_Barre_Menu {
    void (*commence_menu)(struct QT_Creatrice_Barre_Menu *, struct QT_Chaine *);
    void (*ajoute_action)(struct QT_Creatrice_Barre_Menu *,
                          struct QT_Chaine *,
                          struct QT_Chaine *);
    void (*ajoute_separateur)(struct QT_Creatrice_Barre_Menu *);
    void (*termine_menu)(struct QT_Creatrice_Barre_Menu *);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Fenetre_Principale
 * \{ */

struct QT_Rappels_Fenetre_Principale {
    int (*sur_filtre_evenement)(struct QT_Rappels_Fenetre_Principale *, struct QT_Evenement *);
    void (*sur_creation_barre_menu)(struct QT_Rappels_Fenetre_Principale *,
                                    struct QT_Creatrice_Barre_Menu *);
    void (*sur_clique_action_menu)(struct QT_Rappels_Fenetre_Principale *, struct QT_Chaine *);
};

struct QT_Fenetre_Principale *QT_cree_fenetre_principale(
    struct QT_Rappels_Fenetre_Principale *rappels);
void QT_detruit_fenetre_principale(struct QT_Fenetre_Principale *fenetre);

void QT_fenetre_principale_definis_titre_fenetre(struct QT_Fenetre_Principale *fenetre,
                                                 struct QT_Chaine nom);
void QT_fenetre_principale_definis_widget_central(struct QT_Fenetre_Principale *fenetre,
                                                  union QT_Generic_Widget widget);

struct QT_Rappels_Fenetre_Principale *QT_fenetre_principale_donne_rappels(
    struct QT_Fenetre_Principale *fenetre);

struct QT_StatusBar *QT_fenetre_principale_donne_barre_etat(struct QT_Fenetre_Principale *fenetre);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Application
 * \{ */

struct QT_Application;

struct QT_Application *QT_cree_application(int *argc, char **argv);
void QT_detruit_application(struct QT_Application *app);
int QT_application_exec(struct QT_Application *app);

void QT_core_application_definis_nom_organisation(struct QT_Chaine nom);
void QT_core_application_definis_nom_application(struct QT_Chaine nom);

struct QT_Application *QT_donne_application(void);

void QT_application_poste_evenement(union QT_Generic_Object receveur, int type_evenement);
void QT_application_poste_evenement_et_donnees(union QT_Generic_Object receveur,
                                               int type_evenement,
                                               void *donnees);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GuiApplication
 * \{ */

void QT_gui_application_definis_curseur(enum QT_CursorShape cursor);
void QT_gui_application_restaure_curseur();

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Style
 * \{ */

struct QT_Style *QT_application_donne_style();

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Timer
 * \{ */

struct QT_Rappels_Timer {
    void (*sur_timeout)(struct QT_Rappels_Timer *);
};

struct QT_Timer *QT_cree_timer(struct QT_Rappels_Timer *rappels);
void QT_timer_debute(struct QT_Timer *timer, int millisecondes);
void QT_timer_arrete(struct QT_Timer *timer);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Screen
 * \{ */

struct QT_Screen *QT_application_donne_ecran_principal();
struct QT_Taille QT_screen_donne_taille_disponible(struct QT_Screen *screen);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Position
 * \{ */

struct QT_Position {
    int x;
    int y;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Point
 * \{ */

struct QT_Point {
    int x;
    int y;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_PointF
 * \{ */

struct QT_PointF {
    double x;
    double y;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_RectF
 * \{ */

struct QT_RectF {
    double x;
    double y;
    double largeur;
    double hauteur;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Color
 * \{ */

struct QT_Color {
    double r;
    double g;
    double b;
    double a;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Pen
 * \{ */

struct QT_Pen {
    struct QT_Color color;
    double width;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Brush
 * \{ */

struct QT_Brush {
    struct QT_Color color;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Event
 * \{ */

int QT_evenement_donne_type(union QT_Generic_Event evenement);
void QT_evenement_accepte(union QT_Generic_Event evenement);
void QT_evenement_marque_accepte(union QT_Generic_Event evenement, int accepte);
int QT_enregistre_evenement_personnel(void);
void *QT_event_perso_donne_donnees(struct QT_Evenement *event);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_MouseButton
 * \{ */

/* À FAIRE : implémente tous les cas. */
#define ENUMERE_BOUTON_SOURIS(O)                                                                  \
    O(QT_MOUSEBUTTON_GAUCHE, Qt::LeftButton)                                                      \
    O(QT_MOUSEBUTTON_DROIT, Qt::RightButton)                                                      \
    O(QT_MOUSEBUTTON_MILLIEU, Qt::MiddleButton)                                                   \
    O(QT_MOUSEBUTTON_ARRIERE, Qt::BackButton)                                                     \
    O(QT_MOUSEBUTTON_AUCUN, Qt::NoButton)

enum QT_MouseButton { ENUMERE_BOUTON_SOURIS(ENUMERE_DECLARATION_ENUM_IPA) };

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Keyboard_Modifier
 * \{ */

#define ENUMERE_MODIFICATEURS_CLAVIER(O)                                                          \
    O(QT_KEYBOARD_MODIFIER_AUCUN, Qt::NoModifier)                                                 \
    O(QT_KEYBOARD_MODIFIER_MAJ, Qt::ShiftModifier)                                                \
    O(QT_KEYBOARD_MODIFIER_CTRL, Qt::ControlModifier)                                             \
    O(QT_KEYBOARD_MODIFIER_ALT, Qt::AltModifier)                                                  \
    O(QT_KEYBOARD_MODIFIER_META, Qt::MetaModifier)                                                \
    O(QT_KEYBOARD_MODIFIER_KEYPAD, Qt::KeypadModifier)                                            \
    O(QT_KEYBOARD_MODIFIER_GROUP_SWITCH, Qt::GroupSwitchModifier)

enum QT_Keyboard_Modifier {
    QT_KEYBOARD_MODIFIER_AUCUN = 0,
    QT_KEYBOARD_MODIFIER_MAJ = 1,
    QT_KEYBOARD_MODIFIER_CTRL = 2,
    QT_KEYBOARD_MODIFIER_ALT = 4,
    QT_KEYBOARD_MODIFIER_META = 8,
    QT_KEYBOARD_MODIFIER_KEYPAD = 16,
    QT_KEYBOARD_MODIFIER_GROUP_SWITCH = 32,
};

enum QT_Keyboard_Modifier QT_application_donne_modificateurs_clavier(void);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_MouseEvent
 * \{ */

void QT_mouse_event_donne_position(struct QT_MouseEvent *event, struct QT_Position *r_position);
enum QT_MouseButton QT_mouse_event_donne_bouton(struct QT_MouseEvent *event);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_WheelEvent
 * \{ */

void QT_wheel_event_donne_position(struct QT_WheelEvent *event, struct QT_Position *r_position);
int QT_wheel_event_donne_delta(struct QT_WheelEvent *event);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Widget
 * \{ */

#define RAPPELS_EVENEMENTS_COMMUNS(type_classe)                                                   \
    void (*sur_entree)(struct type_classe *, struct QT_Evenement *);                              \
    void (*sur_sortie)(struct type_classe *, struct QT_Evenement *);                              \
    void (*sur_pression_souris)(struct type_classe *, struct QT_MouseEvent *);                    \
    void (*sur_deplacement_souris)(struct type_classe *, struct QT_MouseEvent *);                 \
    void (*sur_relachement_souris)(struct type_classe *, struct QT_MouseEvent *);                 \
    void (*sur_molette_souris)(struct type_classe *, struct QT_WheelEvent *);                     \
    void (*sur_redimensionnement)(struct type_classe *, struct QT_ResizeEvent *);                 \
    void (*sur_destruction)(struct type_classe *)

struct QT_Rappels_Widget {
    RAPPELS_EVENEMENTS_COMMUNS(QT_Rappels_Widget);
};

struct QT_Widget *QT_cree_widget(struct QT_Rappels_Widget *rappels,
                                 union QT_Generic_Widget parent);
/* Pour les parents, etc. */
struct QT_Widget *QT_widget_nul(void);
void QT_widget_definis_layout(union QT_Generic_Widget widget, union QT_Generic_Layout layout);
void QT_widget_remplace_layout(union QT_Generic_Widget widget, union QT_Generic_Layout layout);
void QT_widget_affiche_maximisee(union QT_Generic_Widget widget);
void QT_widget_affiche_minimisee(union QT_Generic_Widget widget);
void QT_widget_affiche_normal(union QT_Generic_Widget widget);
void QT_widget_affiche_plein_ecran(union QT_Generic_Widget widget);
void QT_widget_definis_taille_de_base(union QT_Generic_Widget widget, struct QT_Taille taille);
void QT_widget_definis_taille_minimum(union QT_Generic_Widget widget, struct QT_Taille taille);
void QT_widget_definis_taille_fixe(union QT_Generic_Widget widget, struct QT_Taille taille);
void QT_widget_definis_largeur_fixe(union QT_Generic_Widget widget, int largeur);
void QT_widget_definis_hauteur_fixe(union QT_Generic_Widget widget, int hauteur);
void QT_widget_affiche(union QT_Generic_Widget widget);
void QT_widget_cache(union QT_Generic_Widget widget);
void QT_widget_definis_actif(union QT_Generic_Widget widget, bool ouinon);
union QT_Generic_Widget QT_widget_donne_widget_parent(union QT_Generic_Widget widget);
void QT_widget_copie_comportement_taille(union QT_Generic_Widget widget,
                                         union QT_Generic_Widget widget_source);
void QT_widget_definis_feuille_de_style(union QT_Generic_Widget widget, struct QT_Chaine *texte);
void QT_widget_definis_style(union QT_Generic_Widget widget, struct QT_Style *style);
void QT_widget_ajourne(union QT_Generic_Widget widget);
void QT_widget_definis_trackage_souris(union QT_Generic_Widget widget, bool ouinon);

#define ENUMERE_COMPORTEMENT_TAILLE(O)                                                            \
    O(QT_COMPORTEMENT_TAILLE_FIXE, QSizePolicy::Fixed)                                            \
    O(QT_COMPORTEMENT_TAILLE_MINIMUM, QSizePolicy::Minimum)                                       \
    O(QT_COMPORTEMENT_TAILLE_MAXIMUM, QSizePolicy::Maximum)                                       \
    O(QT_COMPORTEMENT_TAILLE_PREFERE, QSizePolicy::Preferred)                                     \
    O(QT_COMPORTEMENT_TAILLE_AGRANDISSAGE_MINIMUM, QSizePolicy::MinimumExpanding)                 \
    O(QT_COMPORTEMENT_TAILLE_AGRANSSSAGE, QSizePolicy::Expanding)                                 \
    O(QT_COMPORTEMENT_TAILLE_IGNORE, QSizePolicy::Ignored)

enum QT_Comportement_Taille { ENUMERE_COMPORTEMENT_TAILLE(ENUMERE_DECLARATION_ENUM_IPA) };

void QT_widget_definis_comportement_taille(union QT_Generic_Widget widget,
                                           enum QT_Comportement_Taille horizontal,
                                           enum QT_Comportement_Taille vertical);
void QT_widget_definis_etirement_comportement_taille(union QT_Generic_Widget widget,
                                                     int horizontal,
                                                     int vertical);

void QT_widget_definis_hauteur_pour_largeur_comportement_taille(union QT_Generic_Widget widget,
                                                                int oui_non);

int QT_widget_donne_hauteur_pour_largeur_comportement_taille(union QT_Generic_Widget widget);

void QT_widget_definis_curseur(union QT_Generic_Widget widget, enum QT_CursorShape cursor);

void QT_widget_restore_curseur(union QT_Generic_Widget widget);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GLWidget
 * \{ */

struct QT_Rappels_GLWidget {
    RAPPELS_EVENEMENTS_COMMUNS(QT_Rappels_GLWidget);

    void (*sur_initialisation_gl)(struct QT_Rappels_GLWidget *);
    void (*sur_peinture_gl)(struct QT_Rappels_GLWidget *);
    /** Les dimensions sont passées dans l'ordre : largeur, hauteur. */
    void (*sur_redimensionnement_gl)(struct QT_Rappels_GLWidget *, struct QT_Taille);

    /** Le widget pour lequel les rappels sont mis en place. */
    struct QT_GLWidget *widget;
};

struct QT_GLWidget *QT_cree_glwidget(struct QT_Rappels_GLWidget *rappels,
                                     union QT_Generic_Widget parent);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_StatusBar
 * \{ */

void QT_status_bar_ajoute_widget(struct QT_StatusBar *status_bar, union QT_Generic_Widget widget);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Layout
 * \{ */

struct QT_HBoxLayout *QT_cree_hbox_layout(union QT_Generic_Widget parent);
struct QT_VBoxLayout *QT_cree_vbox_layout(union QT_Generic_Widget parent);
struct QT_FormLayout *QT_cree_form_layout(union QT_Generic_Widget parent);
struct QT_GridLayout *QT_cree_grid_layout(union QT_Generic_Widget parent);
void QT_layout_definis_marge(union QT_Generic_Layout layout, int taille);
void QT_layout_ajoute_widget(union QT_Generic_Layout layout, union QT_Generic_Widget widget);
void QT_layout_ajoute_layout(union QT_Generic_Layout layout, union QT_Generic_Layout sous_layout);

void QT_form_layout_ajoute_ligne_chaine(struct QT_FormLayout *layout,
                                        struct QT_Chaine label,
                                        union QT_Generic_Widget widget);
void QT_form_layout_ajoute_ligne(struct QT_FormLayout *layout,
                                 struct QT_Label *label,
                                 union QT_Generic_Widget widget);
void QT_form_layout_ajoute_disposition(struct QT_FormLayout *form, union QT_Generic_Layout layout);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ComboBox
 * \{ */

struct QT_ComboBox *QT_cree_combobox(union QT_Generic_Widget parent);
void QT_combobox_reinitialise(struct QT_ComboBox *combo);
void QT_combobox_ajoute_item(struct QT_ComboBox *combo,
                             struct QT_Chaine texte,
                             struct QT_Chaine valeur);
void QT_combobox_definis_index_courant(struct QT_ComboBox *combo, int index);
int QT_combobox_donne_index_courant(struct QT_ComboBox *combo);
void QT_combobox_connecte_sur_changement_index(struct QT_ComboBox *combo,
                                               struct QT_Rappel_Generique *rappel);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Splitter
 * \{ */

enum QT_Orientation_Splitter {
    QT_ORIENTATION_SPLITTER_HORIZONTALE,
    QT_ORIENTATION_SPLITTER_VERTICALE,
};

struct QT_Splitter *QT_cree_splitter();
void QT_splitter_definis_orientation(struct QT_Splitter *splitter,
                                     enum QT_Orientation_Splitter orientation);
void QT_splitter_ajoute_widget(struct QT_Splitter *splitter, union QT_Generic_Widget widget);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_TabWidget
 * \{ */

struct QT_Rappels_TabWidget {
    void (*sur_changement_page)(struct QT_Rappels_TabWidget *, int index);
    void (*sur_fermeture_page)(struct QT_Rappels_TabWidget *, int index);
    void (*sur_destruction)(struct QT_Rappels_TabWidget *);
};

struct QT_TabWidget *QT_cree_tab_widget(struct QT_Rappels_TabWidget *rappels,
                                        union QT_Generic_Widget parent);

void QT_tab_widget_definis_tabs_fermable(struct QT_TabWidget *tab_widget, int fermable);
void QT_tab_widget_widget_de_coin(struct QT_TabWidget *tab_widget, union QT_Generic_Widget widget);
void QT_tab_widget_ajoute_tab(struct QT_TabWidget *tab_widget,
                              union QT_Generic_Widget widget,
                              struct QT_Chaine *nom);
void QT_tab_widget_supprime_tab(struct QT_TabWidget *tab_widget, int index);
void QT_tab_widget_definis_index_courant(struct QT_TabWidget *tab_widget, int index);
int QT_tab_widget_donne_compte_tabs(struct QT_TabWidget *tab_widget);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ScrollArea
 * \{ */

enum QT_Comportement_Barre_Defilement {
    QT_COMPORTEMENT_BARRE_DEFILEMENT_AU_BESOIN,
    QT_COMPORTEMENT_BARRE_DEFILEMENT_TOUJOURS_INACTIF,
    QT_COMPORTEMENT_BARRE_DEFILEMENT_TOUJOURS_ACTIF,
};

struct QT_ScrollArea *QT_cree_scroll_area(union QT_Generic_Widget parent);
void QT_scroll_area_definis_widget(struct QT_ScrollArea *scroll_area,
                                   union QT_Generic_Widget widget);
void QT_scroll_area_definis_comportement_vertical(
    struct QT_ScrollArea *scroll_area, enum QT_Comportement_Barre_Defilement comportement);
void QT_scroll_area_permet_redimensionnement_widget(struct QT_ScrollArea *scroll_area,
                                                    int redimensionnable);
void QT_scroll_area_definis_style_frame(struct QT_ScrollArea *scroll_area, int style);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_CheckBox
 * \{ */

enum QT_Etat_CheckBox {
    QT_ETAT_CHECKBOX_NON_COCHE,
    QT_ETAT_CHECKBOX_PARTIELLEMENT_COCHE,
    QT_ETAT_CHECKBOX_COCHE,
};

struct QT_Rappels_CheckBox {
    void (*sur_destruction)(struct QT_Rappels_CheckBox *);
    void (*sur_changement_etat)(struct QT_Rappels_CheckBox *, enum QT_Etat_CheckBox);
};

struct QT_CheckBox *QT_checkbox_cree(struct QT_Rappels_CheckBox *rappels,
                                     union QT_Generic_Widget parent);
void QT_checkbox_definis_coche(struct QT_CheckBox *checkbox, int coche);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Label
 * \{ */

struct QT_Label *QT_cree_label(struct QT_Chaine *texte, union QT_Generic_Widget parent);

void QT_label_definis_texte(struct QT_Label *label, struct QT_Chaine texte);

void QT_label_definis_pixmap(struct QT_Label *label,
                             struct QT_Pixmap *pixmap,
                             struct QT_Taille taille);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QLineEdit
 * \{ */

struct QT_LineEdit *QT_cree_line_edit(union QT_Generic_Widget parent);
void QT_line_edit_definis_texte(struct QT_LineEdit *line_edit, struct QT_Chaine texte);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_PushButton
 * \{ */

struct QT_PushButton *QT_cree_push_button(struct QT_Chaine texte, union QT_Generic_Widget parent);

void QT_push_button_connecte_sur_pression(struct QT_PushButton *button,
                                          struct QT_Rappel_Generique *rappel);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Dialog
 * \{ */

struct QT_Dialog *QT_cree_dialog(union QT_Generic_Widget parent);
void QT_dialog_definis_bouton_accepter(struct QT_Dialog *dialog, struct QT_PushButton *bouton);
void QT_dialog_definis_bouton_annuler(struct QT_Dialog *dialog, struct QT_PushButton *bouton);
int QT_dialog_exec(struct QT_Dialog *dialog);
void QT_dialog_definis_modal(struct QT_Dialog *dialog, bool ouinon);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_TreeWidgetItem
 * \{ */

#define ENUMERE_INDICATEUR_ENFANT_TREE_WIDGET(O)                                                  \
    O(QT_INDICATEUR_ENFANT_ARBRE_VISIBLE, QTreeWidgetItem::ChildIndicatorPolicy::ShowIndicator)   \
    O(QT_INDICATEUR_ENFANT_ARBRE_INVISIBLE,                                                       \
      QTreeWidgetItem::ChildIndicatorPolicy::DontShowIndicator)                                   \
    O(QT_INDICATEUR_ENFANT_ARBRE_INVISIBLE_SI_SANS_ENFANT,                                        \
      QTreeWidgetItem::ChildIndicatorPolicy::DontShowIndicatorWhenChildless)

enum QT_Indicateur_Enfant_Arbre {
    ENUMERE_INDICATEUR_ENFANT_TREE_WIDGET(ENUMERE_DECLARATION_ENUM_IPA)
};

struct QT_TreeWidgetItem *QT_cree_treewidgetitem(void *donnees, struct QT_TreeWidgetItem *parent);
void *QT_treewidgetitem_donne_donnees(struct QT_TreeWidgetItem *widget);
void QT_treewidgetitem_definis_indicateur_enfant(struct QT_TreeWidgetItem *widget,
                                                 enum QT_Indicateur_Enfant_Arbre indicateur);
void QT_treewidgetitem_definis_texte(struct QT_TreeWidgetItem *widget,
                                     int colonne,
                                     struct QT_Chaine *texte);
void QT_treewidgetitem_ajoute_enfant(struct QT_TreeWidgetItem *widget,
                                     struct QT_TreeWidgetItem *enfant);
void QT_treewidgetitem_definis_selectionne(struct QT_TreeWidgetItem *widget, bool ouinon);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_TreeWidget
 * \{ */

#define ENUMERE_MODE_SELECTION(O)                                                                 \
    O(QT_MODE_SELECTION_AUCUNE_SELECTION, QAbstractItemView::SelectionMode::NoSelection)          \
    O(QT_MODE_SELECTION_SELECTION_UNIQUE, QAbstractItemView::SelectionMode::SingleSelection)      \
    O(QT_MODE_SELECTION_SELECTION_MULTIPLE, QAbstractItemView::SelectionMode::MultiSelection)     \
    O(QT_MODE_SELECTION_SELECTION_ETENDUE, QAbstractItemView::SelectionMode::ExtendedSelection)   \
    O(QT_MODE_SELECTION_CONTIGUE, QAbstractItemView::SelectionMode::ContiguousSelection)

enum QT_Mode_Selection { ENUMERE_MODE_SELECTION(ENUMERE_DECLARATION_ENUM_IPA) };

#define ENUMERE_COMPORTEMENT_FOCUS(O)                                                             \
    O(QT_COMPORTEMENT_FOCUS_AUCUN_FOCUS, Qt::NoFocus)                                             \
    O(QT_COMPORTEMENT_FOCUS_FOCUS_SUR_TAB, Qt::TabFocus)                                          \
    O(QT_COMPORTEMENT_FOCUS_FOCUS_SUR_CLIQUE, Qt::ClickFocus)                                     \
    O(QT_COMPORTEMENT_FOCUS_FOCUS_FORT, Qt::StrongFocus)                                          \
    O(QT_COMPORTEMENT_FOCUS_FOCUS_SUR_MOLETTE, Qt::WheelFocus)

enum QT_Comportement_Focus { ENUMERE_COMPORTEMENT_FOCUS(ENUMERE_DECLARATION_ENUM_IPA) };

#define ENUMERE_COMPORTEMENT_MENU_CONTEXTUEL(O)                                                   \
    O(QT_COMPORTEMENT_MENU_CONTEXTUEL_AUCUN_MENU, Qt::NoContextMenu)                              \
    O(QT_COMPORTEMENT_MENU_CONTEXTUEL_MENU_DEFAUT, Qt::DefaultContextMenu)                        \
    O(QT_COMPORTEMENT_MENU_CONTEXTUEL_MENU_ACTIONS, Qt::ActionsContextMenu)                       \
    O(QT_COMPORTEMENT_MENU_CONTEXTUEL_MENU_PERSONNALISE, Qt::CustomContextMenu)                   \
    O(QT_COMPORTEMENT_MENU_CONTEXTUEL_PREVIENS_MENU, Qt::PreventContextMenu)

enum QT_Comportement_Menu_Contextuel {
    ENUMERE_COMPORTEMENT_MENU_CONTEXTUEL(ENUMERE_DECLARATION_ENUM_IPA)
};

#define ENUMERE_MODE_DRAG_DROP(O)                                                                 \
    O(QT_MODE_DRAGDROP_AUCUN_DRAG_DROP, QAbstractItemView::DragDropMode::NoDragDrop)              \
    O(QT_MODE_DRAGDROP_DRAG_SEUL, QAbstractItemView::DragDropMode::DragOnly)                      \
    O(QT_MODE_DRAGDROP_DROP_SEUL, QAbstractItemView::DragDropMode::DropOnly)                      \
    O(QT_MODE_DRAGDROP_DRAG_DROP, QAbstractItemView::DragDropMode::DragDrop)                      \
    O(QT_MODE_DRAGDROP_MOUVEMENT_INTERNE, QAbstractItemView::DragDropMode::InternalMove)

enum QT_Mode_DragDrop { ENUMERE_MODE_DRAG_DROP(ENUMERE_DECLARATION_ENUM_IPA) };

struct QT_Rappels_TreeWidget {
    void (*sur_destruction)(struct QT_Rappels_TreeWidget *);
    void (*sur_selection_item)(struct QT_Rappels_TreeWidget *, struct QT_TreeWidgetItem *, int);
    /** Les QT_TreeWidgetIem sont donnés dans l'ordre : courant, précédent. */
    void (*sur_changement_item_courant)(struct QT_Rappels_TreeWidget *,
                                        struct QT_TreeWidgetItem *,
                                        struct QT_TreeWidgetItem *);
};

struct QT_TreeWidget *QT_cree_treewidget(struct QT_Rappels_TreeWidget *rappels,
                                         union QT_Generic_Widget parent);

void QT_treewidget_efface(struct QT_TreeWidget *tree_widget);
void QT_treewidget_definis_nombre_colonne(struct QT_TreeWidget *tree_widget, int compte);
void QT_treewidget_ajoute_item_racine(struct QT_TreeWidget *tree_widget,
                                      struct QT_TreeWidgetItem *item);
void QT_treewidget_definis_item_widget(struct QT_TreeWidget *tree_widget,
                                       struct QT_TreeWidgetItem *item,
                                       int colonne,
                                       union QT_Generic_Widget widget);
void QT_treewidget_definis_taille_icone(struct QT_TreeWidget *tree_widget,
                                        int largeur,
                                        int hauteur);
void QT_treewidget_definis_toutes_les_colonnes_montre_focus(struct QT_TreeWidget *tree_widget,
                                                            int oui_non);
void QT_treewidget_definis_anime(struct QT_TreeWidget *tree_widget, int oui_non);
void QT_treewidget_definis_auto_defilement(struct QT_TreeWidget *tree_widget, int oui_non);
void QT_treewidget_definis_hauteurs_uniformes_lignes(struct QT_TreeWidget *tree_widget,
                                                     int oui_non);
void QT_treewidget_definis_mode_selection(struct QT_TreeWidget *tree_widget,
                                          enum QT_Mode_Selection mode);
void QT_treewidget_definis_comportement_focus(struct QT_TreeWidget *tree_widget,
                                              enum QT_Comportement_Focus comportement);
void QT_treewidget_definis_comportement_menu_contextuel(
    struct QT_TreeWidget *tree_widget, enum QT_Comportement_Menu_Contextuel comportement);
void QT_treewidget_definis_entete_visible(struct QT_TreeWidget *tree_widget, int oui_non);
void QT_treewidget_definis_mode_drag_drop(struct QT_TreeWidget *tree_widget,
                                          enum QT_Mode_DragDrop mode);
void QT_treewidget_definis_activation_drag(struct QT_TreeWidget *tree_widget, int oui_non);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Frame
 * \{ */

#define ENEMERE_OMBRAGE_FRAME(O)                                                                  \
    O(QT_FRAME_SHADOW_SIMPLE, QFrame::Plain)                                                      \
    O(QT_FRAME_SHADOW_REHAUSSE, QFrame::Raised)                                                   \
    O(QT_FRAME_SHADOW_ABAISSE, QFrame::Sunken)

enum QT_Frame_Shadow { ENEMERE_OMBRAGE_FRAME(ENUMERE_DECLARATION_ENUM_IPA) };

#define ENEMERE_FORME_FRAME(O)                                                                    \
    O(QT_FRAME_SHAPE_NULLE, QFrame::NoFrame)                                                      \
    O(QT_FRAME_SHAPE_BOITE, QFrame::Box)                                                          \
    O(QT_FRAME_SHAPE_PANNEAU, QFrame::Panel)                                                      \
    O(QT_FRAME_SHAPE_PANNEAU_WINDOWS, QFrame::WinPanel)                                           \
    O(QT_FRAME_SHAPE_LIGNE_HORIZONTALE, QFrame::HLine)                                            \
    O(QT_FRAME_SHAPE_LIGNE_VERTICALE, QFrame::VLine)                                              \
    O(QT_FRAME_SHAPE_PANNEAU_STYLISE, QFrame::StyledPanel)

enum QT_Frame_Shape { ENEMERE_FORME_FRAME(ENUMERE_DECLARATION_ENUM_IPA) };

struct QT_Frame *QT_cree_frame(union QT_Generic_Widget parent);
void QT_frame_definis_forme(struct QT_Frame *frame, enum QT_Frame_Shape forme);
void QT_frame_definis_ombrage(struct QT_Frame *frame, enum QT_Frame_Shadow ombrage);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsRectItem
 * \{ */

struct QT_GraphicsRectItem *QT_cree_graphics_rect_item(union QT_Generic_GraphicsItem parent);
void QT_graphics_rect_item_definis_pinceau(struct QT_GraphicsRectItem *item,
                                           struct QT_Pen pinceau);
void QT_graphics_rect_item_definis_brosse(struct QT_GraphicsRectItem *item, struct QT_Brush brush);
void QT_graphics_rect_item_definis_rect(struct QT_GraphicsRectItem *item, struct QT_RectF rect);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsTextItem
 * \{ */

struct QT_GraphicsTextItem *QT_cree_graphics_text_item(struct QT_Chaine texte,
                                                       union QT_Generic_GraphicsItem parent);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsLineItem
 * \{ */

struct QT_GraphicsLineItem *QT_cree_graphics_line_item(union QT_Generic_GraphicsItem parent);
void QT_graphics_rect_line_definis_pinceau(struct QT_GraphicsLineItem *item,
                                           struct QT_Pen pinceau);
void QT_line_graphics_item_definis_ligne(
    struct QT_GraphicsLineItem *line, double x1, double y1, double x2, double y2);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsScene
 * \{ */

struct QT_GraphicsScene *QT_cree_graphics_scene(union QT_Generic_Object parent);
void QT_graphics_scene_detruit(struct QT_GraphicsScene *scene);
struct QT_GraphicsView *QT_graphics_scene_cree_graphics_view(struct QT_GraphicsScene *scene,
                                                             union QT_Generic_Widget parent);
void QT_graphics_scene_efface(struct QT_GraphicsScene *scene);
struct QT_RectF QT_graphics_scene_donne_rect_scene(struct QT_GraphicsScene *scene);
void QT_graphics_scene_definis_rect_scene(struct QT_GraphicsScene *scene, struct QT_RectF rect);
void QT_graphics_scene_ajoute_item(struct QT_GraphicsScene *scene,
                                   union QT_Generic_GraphicsItem item);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsView
 * \{ */

struct QT_GraphicsView *QT_cree_graphics_view(union QT_Generic_Widget parent);
void QT_graphics_view_definis_scene(struct QT_GraphicsView *graphics_view,
                                    struct QT_GraphicsScene *scene);
void QT_graphics_view_reinit_transforme(struct QT_GraphicsView *graphics_view);
void QT_graphics_view_definis_echelle_taille(struct QT_GraphicsView *graphics_view,
                                             float x,
                                             float y);
struct QT_PointF QT_graphics_view_mappe_vers_scene(struct QT_GraphicsView *graphics_view,
                                                   struct QT_Point point);
struct QT_Point QT_graphics_view_mappe_depuis_scene(struct QT_GraphicsView *graphics_view,
                                                    struct QT_PointF point);
struct QT_Point QT_graphics_view_mappe_vers_global(struct QT_GraphicsView *graphics_view,
                                                   struct QT_Point point);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Constructrice_Parametre_Enum
 * \{ */

struct DNJ_Constructrice_Parametre_Enum {
    void (*ajoute_item)(struct DNJ_Constructrice_Parametre_Enum *,
                        struct QT_Chaine,
                        struct QT_Chaine);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Rappels_Enveloppe_Parametre
 * \{ */

#define ENEMERE_TYPE_PARAMETRE_DANJO(O)                                                           \
    O(DNJ_TYPE_PARAMETRE_ENTIER, danjo::TypePropriete::ENTIER)                                    \
    O(DNJ_TYPE_PARAMETRE_DECIMAL, danjo::TypePropriete::DECIMAL)                                  \
    O(DNJ_TYPE_PARAMETRE_VECTEUR_DECIMAL, danjo::TypePropriete::VECTEUR_DECIMAL)                  \
    O(DNJ_TYPE_PARAMETRE_VECTEUR_ENTIER, danjo::TypePropriete::VECTEUR_ENTIER)                    \
    O(DNJ_TYPE_PARAMETRE_COULEUR, danjo::TypePropriete::COULEUR)                                  \
    O(DNJ_TYPE_PARAMETRE_FICHIER_ENTREE, danjo::TypePropriete::FICHIER_ENTREE)                    \
    O(DNJ_TYPE_PARAMETRE_FICHIER_SORTIE, danjo::TypePropriete::FICHIER_SORTIE)                    \
    O(DNJ_TYPE_PARAMETRE_CHAINE_CARACTERE, danjo::TypePropriete::CHAINE_CARACTERE)                \
    O(DNJ_TYPE_PARAMETRE_BOOL, danjo::TypePropriete::BOOL)                                        \
    O(DNJ_TYPE_PARAMETRE_ENUM, danjo::TypePropriete::ENUM)                                        \
    O(DNJ_TYPE_PARAMETRE_COURBE_COULEUR, danjo::TypePropriete::COURBE_COULEUR)                    \
    O(DNJ_TYPE_PARAMETRE_COURBE_VALEUR, danjo::TypePropriete::COURBE_VALEUR)                      \
    O(DNJ_TYPE_PARAMETRE_RAMPE_COULEUR, danjo::TypePropriete::RAMPE_COULEUR)                      \
    O(DNJ_TYPE_PARAMETRE_TEXTE, danjo::TypePropriete::TEXTE)                                      \
    O(DNJ_TYPE_PARAMETRE_LISTE, danjo::TypePropriete::LISTE)                                      \
    O(DNJ_TYPE_PARAMETRE_LISTE_MANIP, danjo::TypePropriete::LISTE_MANIP)

enum DNJ_Type_Parametre { ENEMERE_TYPE_PARAMETRE_DANJO(ENUMERE_DECLARATION_ENUM_IPA) };

struct DNJ_Rappels_Enveloppe_Parametre {
    enum DNJ_Type_Parametre (*donne_type_parametre)(struct DNJ_Rappels_Enveloppe_Parametre *);
    bool (*est_extra)(struct DNJ_Rappels_Enveloppe_Parametre *);
    void (*definis_visibilite)(struct DNJ_Rappels_Enveloppe_Parametre *, bool);
    bool (*est_visible)(struct DNJ_Rappels_Enveloppe_Parametre *);
    void (*donne_infobulle)(struct DNJ_Rappels_Enveloppe_Parametre *, struct QT_Chaine *);
    int (*donne_dimensions_vecteur)(struct DNJ_Rappels_Enveloppe_Parametre *);

    void (*cree_items_enum)(struct DNJ_Rappels_Enveloppe_Parametre *,
                            struct DNJ_Constructrice_Parametre_Enum *);

    /* Évaluation des valeurs. */
    bool (*evalue_bool)(struct DNJ_Rappels_Enveloppe_Parametre *, int);
    int (*evalue_entier)(struct DNJ_Rappels_Enveloppe_Parametre *, int);
    float (*evalue_decimal)(struct DNJ_Rappels_Enveloppe_Parametre *, int);
    void (*evalue_vecteur_decimal)(struct DNJ_Rappels_Enveloppe_Parametre *, int, float *);
    void (*evalue_vecteur_entier)(struct DNJ_Rappels_Enveloppe_Parametre *, int, int *);
    void (*evalue_couleur)(struct DNJ_Rappels_Enveloppe_Parametre *, int, float *);
    void (*evalue_chaine)(struct DNJ_Rappels_Enveloppe_Parametre *, int, struct QT_Chaine *);
    void (*evalue_enum)(struct DNJ_Rappels_Enveloppe_Parametre *, int, struct QT_Chaine *);

    /* Définition des valeurs. */
    void (*definis_bool)(struct DNJ_Rappels_Enveloppe_Parametre *, bool);
    void (*definis_entier)(struct DNJ_Rappels_Enveloppe_Parametre *, int);
    void (*definis_decimal)(struct DNJ_Rappels_Enveloppe_Parametre *, float);
    void (*definis_vecteur_decimal)(struct DNJ_Rappels_Enveloppe_Parametre *, float *);
    void (*definis_vecteur_entier)(struct DNJ_Rappels_Enveloppe_Parametre *, int *);
    void (*definis_couleur)(struct DNJ_Rappels_Enveloppe_Parametre *, float *);
    void (*definis_chaine)(struct DNJ_Rappels_Enveloppe_Parametre *, struct QT_Chaine *);
    void (*definis_enum)(struct DNJ_Rappels_Enveloppe_Parametre *, struct QT_Chaine *);

    /* Plage des valeurs. */
    void (*donne_plage_entier)(struct DNJ_Rappels_Enveloppe_Parametre *, int *, int *);
    void (*donne_plage_decimal)(struct DNJ_Rappels_Enveloppe_Parametre *, float *, float *);
    void (*donne_plage_vecteur_entier)(struct DNJ_Rappels_Enveloppe_Parametre *, int *, int *);
    void (*donne_plage_vecteur_decimal)(struct DNJ_Rappels_Enveloppe_Parametre *,
                                        float *,
                                        float *);
    void (*donne_plage_couleur)(struct DNJ_Rappels_Enveloppe_Parametre *, float *, float *);

    /* Animation. */
    void (*ajoute_image_cle_bool)(struct DNJ_Rappels_Enveloppe_Parametre *, bool, int);
    void (*ajoute_image_cle_entier)(struct DNJ_Rappels_Enveloppe_Parametre *, int, int);
    void (*ajoute_image_cle_decimal)(struct DNJ_Rappels_Enveloppe_Parametre *, float, int);
    void (*ajoute_image_cle_vecteur_decimal)(struct DNJ_Rappels_Enveloppe_Parametre *,
                                             float *,
                                             int);
    void (*ajoute_image_cle_vecteur_entier)(struct DNJ_Rappels_Enveloppe_Parametre *, int *, int);
    void (*ajoute_image_cle_couleur)(struct DNJ_Rappels_Enveloppe_Parametre *, float *, int);
    void (*ajoute_image_cle_chaine)(struct DNJ_Rappels_Enveloppe_Parametre *,
                                    struct QT_Chaine *,
                                    int);
    void (*ajoute_image_cle_enum)(struct DNJ_Rappels_Enveloppe_Parametre *,
                                  struct QT_Chaine *,
                                  int);

    void (*supprime_animation)(struct DNJ_Rappels_Enveloppe_Parametre *);
    bool (*est_anime)(struct DNJ_Rappels_Enveloppe_Parametre *);
    bool (*est_animable)(struct DNJ_Rappels_Enveloppe_Parametre *);
    bool (*possede_image_cle)(struct DNJ_Rappels_Enveloppe_Parametre *, int);
};
/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_ConstructriceInterfaceParametres
 * \{ */

enum DNJ_Type_Disposition {
    DNJ_TYPE_DISPOSITION_LIGNE,
    DNJ_TYPE_DISPOSITION_COLONNE,
};

struct DNJ_ConstructriceInterfaceParametres {
    void (*commence_disposition)(struct DNJ_ConstructriceInterfaceParametres *,
                                 enum DNJ_Type_Disposition);
    void (*termine_disposition)(struct DNJ_ConstructriceInterfaceParametres *);
    void (*ajoute_etiquette)(struct DNJ_ConstructriceInterfaceParametres *, struct QT_Chaine);
    void (*ajoute_propriete)(struct DNJ_ConstructriceInterfaceParametres *,
                             struct QT_Chaine,
                             struct DNJ_Rappels_Enveloppe_Parametre *);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Constructrice_Liste
 * \{ */

struct DNJ_Constructrice_Liste {
    void (*ajoute_element)(struct DNJ_Constructrice_Liste *, struct QT_Chaine);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Pilote_Clique
 * \{ */

struct DNJ_Pilote_Clique;

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Rappels_Widget
 * \{ */

struct DNJ_Rappels_Widget {
    void (*sur_changement_parametre)(struct DNJ_Rappels_Widget *);
    void (*sur_pre_changement_parametre)(struct DNJ_Rappels_Widget *);
    void (*sur_post_changement_parametre)(struct DNJ_Rappels_Widget *);
    void (*sur_changement_onglet_dossier)(struct DNJ_Rappels_Widget *, int);
    void (*sur_requete_liste)(struct DNJ_Rappels_Widget *,
                              struct QT_Chaine,
                              struct DNJ_Constructrice_Liste *);
    struct DNJ_Pilote_Clique *(*donne_pilote_clique)(struct DNJ_Rappels_Widget *);
    void (*sur_creation_interface)(struct DNJ_Rappels_Widget *,
                                   struct DNJ_ConstructriceInterfaceParametres *);
    void (*sur_destruction)(struct DNJ_Rappels_Widget *);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Conteneur_Controles
 * \{ */

struct DNJ_Conteneur_Controles *DNJ_cree_conteneur_controle(struct DNJ_Rappels_Widget *rappels,
                                                            union QT_Generic_Widget parent);
struct QT_Layout *DNJ_conteneur_cree_interface(struct DNJ_Conteneur_Controles *conteneur);

/** \} */

#ifdef __cplusplus
}
#endif
