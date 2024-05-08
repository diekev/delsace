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

#define ENUMERE_DECLARATION_ENUM_DRAPEAU_IPA(nom_ipa, nom_qt, valeur) nom_ipa = valeur,

#define ENUMERE_TRANSLATION_ENUM_DRAPEAU_IPA_VERS_QT(nom_ipa, nom_qt, valeur)                     \
    if ((drapeaux & nom_ipa)) {                                                                   \
        résultat |= nom_qt;                                                                       \
    }

#define ENUMERE_TRANSLATION_ENUM_DRAPEAU_QT_VERS_IPA(nom_ipa, nom_qt, valeur)                     \
    if ((drapeaux & nom_qt)) {                                                                    \
        résultat |= nom_ipa;                                                                      \
    }

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
    O(QStatusBar, QT_StatusBar, status_bar)                                                       \
    O(QMenu, QT_Menu, menu)                                                                       \
    O(QMenuBar, QT_MenuBar, menu_bar)                                                             \
    O(QToolBar, QT_ToolBar, tool_bar)

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
    O(QGraphicsScene, QT_GraphicsScene, graphics_scene)                                           \
    O(QSettings, QT_Settings, settings)

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
    O(QResizeEvent, QT_ResizeEvent, resize_event)                                                 \
    O(QKeyEvent, QT_KeyEvent, key_event)

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
/** \name QT_ToolBarArea
 * \{ */

#define ENUMERE_TOOLBARAREA(O)                                                                    \
    O(QT_TOOLBARAREA_GAUCHE, Qt::LeftToolBarArea)                                                 \
    O(QT_TOOLBARAREA_DROITE, Qt::RightToolBarArea)                                                \
    O(QT_TOOLBARAREA_HAUT, Qt::TopToolBarArea)                                                    \
    O(QT_TOOLBARAREA_BAS, Qt::BottomToolBarArea)

enum QT_ToolBarArea { ENUMERE_TOOLBARAREA(ENUMERE_DECLARATION_ENUM_IPA) };

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Fenetre_Principale
 * \{ */

struct QT_Rappels_Fenetre_Principale {
    int (*sur_filtre_evenement)(struct QT_Rappels_Fenetre_Principale *, struct QT_Evenement *);
    void (*sur_creation_barre_menu)(struct QT_Rappels_Fenetre_Principale *,
                                    struct QT_Creatrice_Barre_Menu *);
    void (*sur_clique_action_menu)(struct QT_Rappels_Fenetre_Principale *, struct QT_Chaine *);
    /** Appelé quand la fenêtre principale est fermée, et permet de controler si la fermeture est
     * avortée. Si faux est retourné, la fenêtre n'est pas fermée. */
    bool (*sur_fermeture)(struct QT_Rappels_Fenetre_Principale *);

    /* La fenetre créée avec ces rappels. */
    struct QT_Fenetre_Principale *fenetre;
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

struct QT_MenuBar *QT_fenetre_principale_donne_barre_menu(struct QT_Fenetre_Principale *fenetre);

struct QT_StatusBar *QT_fenetre_principale_donne_barre_etat(struct QT_Fenetre_Principale *fenetre);

void QT_fenetre_principale_ajoute_barre_a_outils(struct QT_Fenetre_Principale *fenetre,
                                                 struct QT_ToolBar *tool_bar,
                                                 enum QT_ToolBarArea area);

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

/** Définis un rappel à exécuter lorsque l'application sera fermée. */
void QT_application_sur_fin_boucle_evenement(struct QT_Application *app,
                                             struct QT_Rappel_Generique *rappel);

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
/** \name QT_Exportrice_Liste_Chaine
 * \{ */

struct QT_Exportrice_Liste_Chaine {
    void (*ajoute)(struct QT_Exportrice_Liste_Chaine *, struct QT_Chaine);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Settings
 * \{ */

struct QT_Settings *QT_donne_parametres();
void QT_detruit_parametres(struct QT_Settings *settings);

void QT_settings_lis_liste_chaine(struct QT_Settings *settings,
                                  struct QT_Chaine nom_paramètre,
                                  struct QT_Exportrice_Liste_Chaine *exportrice);

void QT_settings_ecris_liste_chaine(struct QT_Settings *settings,
                                    struct QT_Chaine nom_paramètre,
                                    struct QT_Chaine *liste,
                                    int64_t taille_liste);

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

struct QT_Color QT_color_depuis_tsl(double t, double s, double l, double a);

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
/** \name QT_Font
 * \{ */

struct QT_Font {
    int taille_point;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Cursor
 * \{ */

struct QT_Point QT_cursor_pos();

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
/** \name QT_Key
 * \{ */

#define ENUMERE_CLE_CLAVIER(O)                                                                    \
    O(QT_KEY_Escape, Qt::Key_Escape)                                                              \
    O(QT_KEY_Tab, Qt::Key_Tab)                                                                    \
    O(QT_KEY_Backtab, Qt::Key_Backtab)                                                            \
    O(QT_KEY_Backspace, Qt::Key_Backspace)                                                        \
    O(QT_KEY_Return, Qt::Key_Return)                                                              \
    O(QT_KEY_Enter, Qt::Key_Enter)                                                                \
    O(QT_KEY_Insert, Qt::Key_Insert)                                                              \
    O(QT_KEY_Delete, Qt::Key_Delete)                                                              \
    O(QT_KEY_Pause, Qt::Key_Pause)                                                                \
    O(QT_KEY_Print, Qt::Key_Print)                                                                \
    O(QT_KEY_SysReq, Qt::Key_SysReq)                                                              \
    O(QT_KEY_Clear, Qt::Key_Clear)                                                                \
    O(QT_KEY_Home, Qt::Key_Home)                                                                  \
    O(QT_KEY_End, Qt::Key_End)                                                                    \
    O(QT_KEY_Left, Qt::Key_Left)                                                                  \
    O(QT_KEY_Up, Qt::Key_Up)                                                                      \
    O(QT_KEY_Right, Qt::Key_Right)                                                                \
    O(QT_KEY_Down, Qt::Key_Down)                                                                  \
    O(QT_KEY_PageUp, Qt::Key_PageUp)                                                              \
    O(QT_KEY_PageDown, Qt::Key_PageDown)                                                          \
    O(QT_KEY_Shift, Qt::Key_Shift)                                                                \
    O(QT_KEY_Control, Qt::Key_Control)                                                            \
    O(QT_KEY_Meta, Qt::Key_Meta)                                                                  \
    O(QT_KEY_Alt, Qt::Key_Alt)                                                                    \
    O(QT_KEY_CapsLock, Qt::Key_CapsLock)                                                          \
    O(QT_KEY_NumLock, Qt::Key_NumLock)                                                            \
    O(QT_KEY_ScrollLock, Qt::Key_ScrollLock)                                                      \
    O(QT_KEY_F1, Qt::Key_F1)                                                                      \
    O(QT_KEY_F2, Qt::Key_F2)                                                                      \
    O(QT_KEY_F3, Qt::Key_F3)                                                                      \
    O(QT_KEY_F4, Qt::Key_F4)                                                                      \
    O(QT_KEY_F5, Qt::Key_F5)                                                                      \
    O(QT_KEY_F6, Qt::Key_F6)                                                                      \
    O(QT_KEY_F7, Qt::Key_F7)                                                                      \
    O(QT_KEY_F8, Qt::Key_F8)                                                                      \
    O(QT_KEY_F9, Qt::Key_F9)                                                                      \
    O(QT_KEY_F10, Qt::Key_F10)                                                                    \
    O(QT_KEY_F11, Qt::Key_F11)                                                                    \
    O(QT_KEY_F12, Qt::Key_F12)                                                                    \
    O(QT_KEY_F13, Qt::Key_F13)                                                                    \
    O(QT_KEY_F14, Qt::Key_F14)                                                                    \
    O(QT_KEY_F15, Qt::Key_F15)                                                                    \
    O(QT_KEY_F16, Qt::Key_F16)                                                                    \
    O(QT_KEY_F17, Qt::Key_F17)                                                                    \
    O(QT_KEY_F18, Qt::Key_F18)                                                                    \
    O(QT_KEY_F19, Qt::Key_F19)                                                                    \
    O(QT_KEY_F20, Qt::Key_F20)                                                                    \
    O(QT_KEY_F21, Qt::Key_F21)                                                                    \
    O(QT_KEY_F22, Qt::Key_F22)                                                                    \
    O(QT_KEY_F23, Qt::Key_F23)                                                                    \
    O(QT_KEY_F24, Qt::Key_F24)                                                                    \
    O(QT_KEY_F25, Qt::Key_F25)                                                                    \
    O(QT_KEY_F26, Qt::Key_F26)                                                                    \
    O(QT_KEY_F27, Qt::Key_F27)                                                                    \
    O(QT_KEY_F28, Qt::Key_F28)                                                                    \
    O(QT_KEY_F29, Qt::Key_F29)                                                                    \
    O(QT_KEY_F30, Qt::Key_F30)                                                                    \
    O(QT_KEY_F31, Qt::Key_F31)                                                                    \
    O(QT_KEY_F32, Qt::Key_F32)                                                                    \
    O(QT_KEY_F33, Qt::Key_F33)                                                                    \
    O(QT_KEY_F34, Qt::Key_F34)                                                                    \
    O(QT_KEY_F35, Qt::Key_F35)                                                                    \
    O(QT_KEY_Super_L, Qt::Key_Super_L)                                                            \
    O(QT_KEY_Super_R, Qt::Key_Super_R)                                                            \
    O(QT_KEY_Menu, Qt::Key_Menu)                                                                  \
    O(QT_KEY_Hyper_L, Qt::Key_Hyper_L)                                                            \
    O(QT_KEY_Hyper_R, Qt::Key_Hyper_R)                                                            \
    O(QT_KEY_Help, Qt::Key_Help)                                                                  \
    O(QT_KEY_Direction_L, Qt::Key_Direction_L)                                                    \
    O(QT_KEY_Direction_R, Qt::Key_Direction_R)                                                    \
    O(QT_KEY_Space, Qt::Key_Space)                                                                \
    O(QT_KEY_Exclam, Qt::Key_Exclam)                                                              \
    O(QT_KEY_QuoteDbl, Qt::Key_QuoteDbl)                                                          \
    O(QT_KEY_NumberSign, Qt::Key_NumberSign)                                                      \
    O(QT_KEY_Dollar, Qt::Key_Dollar)                                                              \
    O(QT_KEY_Percent, Qt::Key_Percent)                                                            \
    O(QT_KEY_Ampersand, Qt::Key_Ampersand)                                                        \
    O(QT_KEY_Apostrophe, Qt::Key_Apostrophe)                                                      \
    O(QT_KEY_ParenLeft, Qt::Key_ParenLeft)                                                        \
    O(QT_KEY_ParenRight, Qt::Key_ParenRight)                                                      \
    O(QT_KEY_Asterisk, Qt::Key_Asterisk)                                                          \
    O(QT_KEY_Plus, Qt::Key_Plus)                                                                  \
    O(QT_KEY_Comma, Qt::Key_Comma)                                                                \
    O(QT_KEY_Minus, Qt::Key_Minus)                                                                \
    O(QT_KEY_Period, Qt::Key_Period)                                                              \
    O(QT_KEY_Slash, Qt::Key_Slash)                                                                \
    O(QT_KEY_NOMBRE_0, Qt::Key_0)                                                                 \
    O(QT_KEY_NOMBRE_1, Qt::Key_1)                                                                 \
    O(QT_KEY_NOMBRE_2, Qt::Key_2)                                                                 \
    O(QT_KEY_NOMBRE_3, Qt::Key_3)                                                                 \
    O(QT_KEY_NOMBRE_4, Qt::Key_4)                                                                 \
    O(QT_KEY_NOMBRE_5, Qt::Key_5)                                                                 \
    O(QT_KEY_NOMBRE_6, Qt::Key_6)                                                                 \
    O(QT_KEY_NOMBRE_7, Qt::Key_7)                                                                 \
    O(QT_KEY_NOMBRE_8, Qt::Key_8)                                                                 \
    O(QT_KEY_NOMBRE_9, Qt::Key_9)                                                                 \
    O(QT_KEY_Colon, Qt::Key_Colon)                                                                \
    O(QT_KEY_Semicolon, Qt::Key_Semicolon)                                                        \
    O(QT_KEY_Less, Qt::Key_Less)                                                                  \
    O(QT_KEY_Equal, Qt::Key_Equal)                                                                \
    O(QT_KEY_Greater, Qt::Key_Greater)                                                            \
    O(QT_KEY_Question, Qt::Key_Question)                                                          \
    O(QT_KEY_At, Qt::Key_At)                                                                      \
    O(QT_KEY_A, Qt::Key_A)                                                                        \
    O(QT_KEY_B, Qt::Key_B)                                                                        \
    O(QT_KEY_C, Qt::Key_C)                                                                        \
    O(QT_KEY_D, Qt::Key_D)                                                                        \
    O(QT_KEY_E, Qt::Key_E)                                                                        \
    O(QT_KEY_F, Qt::Key_F)                                                                        \
    O(QT_KEY_G, Qt::Key_G)                                                                        \
    O(QT_KEY_H, Qt::Key_H)                                                                        \
    O(QT_KEY_I, Qt::Key_I)                                                                        \
    O(QT_KEY_J, Qt::Key_J)                                                                        \
    O(QT_KEY_K, Qt::Key_K)                                                                        \
    O(QT_KEY_L, Qt::Key_L)                                                                        \
    O(QT_KEY_M, Qt::Key_M)                                                                        \
    O(QT_KEY_N, Qt::Key_N)                                                                        \
    O(QT_KEY_O, Qt::Key_O)                                                                        \
    O(QT_KEY_P, Qt::Key_P)                                                                        \
    O(QT_KEY_Q, Qt::Key_Q)                                                                        \
    O(QT_KEY_R, Qt::Key_R)                                                                        \
    O(QT_KEY_S, Qt::Key_S)                                                                        \
    O(QT_KEY_T, Qt::Key_T)                                                                        \
    O(QT_KEY_U, Qt::Key_U)                                                                        \
    O(QT_KEY_V, Qt::Key_V)                                                                        \
    O(QT_KEY_W, Qt::Key_W)                                                                        \
    O(QT_KEY_X, Qt::Key_X)                                                                        \
    O(QT_KEY_Y, Qt::Key_Y)                                                                        \
    O(QT_KEY_Z, Qt::Key_Z)                                                                        \
    O(QT_KEY_BracketLeft, Qt::Key_BracketLeft)                                                    \
    O(QT_KEY_Backslash, Qt::Key_Backslash)                                                        \
    O(QT_KEY_BracketRight, Qt::Key_BracketRight)                                                  \
    O(QT_KEY_AsciiCircum, Qt::Key_AsciiCircum)                                                    \
    O(QT_KEY_Underscore, Qt::Key_Underscore)                                                      \
    O(QT_KEY_QuoteLeft, Qt::Key_QuoteLeft)                                                        \
    O(QT_KEY_BraceLeft, Qt::Key_BraceLeft)                                                        \
    O(QT_KEY_Bar, Qt::Key_Bar)                                                                    \
    O(QT_KEY_BraceRight, Qt::Key_BraceRight)                                                      \
    O(QT_KEY_AsciiTilde, Qt::Key_AsciiTilde)                                                      \
    O(QT_KEY_nobreakspace, Qt::Key_nobreakspace)                                                  \
    O(QT_KEY_exclamdown, Qt::Key_exclamdown)                                                      \
    O(QT_KEY_cent, Qt::Key_cent)                                                                  \
    O(QT_KEY_sterling, Qt::Key_sterling)                                                          \
    O(QT_KEY_currency, Qt::Key_currency)                                                          \
    O(QT_KEY_yen, Qt::Key_yen)                                                                    \
    O(QT_KEY_brokenbar, Qt::Key_brokenbar)                                                        \
    O(QT_KEY_section, Qt::Key_section)                                                            \
    O(QT_KEY_diaeresis, Qt::Key_diaeresis)                                                        \
    O(QT_KEY_copyright, Qt::Key_copyright)                                                        \
    O(QT_KEY_ordfeminine, Qt::Key_ordfeminine)                                                    \
    O(QT_KEY_guillemotleft, Qt::Key_guillemotleft)                                                \
    O(QT_KEY_notsign, Qt::Key_notsign)                                                            \
    O(QT_KEY_hyphen, Qt::Key_hyphen)                                                              \
    O(QT_KEY_registered, Qt::Key_registered)                                                      \
    O(QT_KEY_macron, Qt::Key_macron)                                                              \
    O(QT_KEY_degree, Qt::Key_degree)                                                              \
    O(QT_KEY_plusminus, Qt::Key_plusminus)                                                        \
    O(QT_KEY_twosuperior, Qt::Key_twosuperior)                                                    \
    O(QT_KEY_threesuperior, Qt::Key_threesuperior)                                                \
    O(QT_KEY_acute, Qt::Key_acute)                                                                \
    O(QT_KEY_mu, Qt::Key_mu)                                                                      \
    O(QT_KEY_paragraph, Qt::Key_paragraph)                                                        \
    O(QT_KEY_periodcentered, Qt::Key_periodcentered)                                              \
    O(QT_KEY_cedilla, Qt::Key_cedilla)                                                            \
    O(QT_KEY_onesuperior, Qt::Key_onesuperior)                                                    \
    O(QT_KEY_masculine, Qt::Key_masculine)                                                        \
    O(QT_KEY_guillemotright, Qt::Key_guillemotright)                                              \
    O(QT_KEY_onequarter, Qt::Key_onequarter)                                                      \
    O(QT_KEY_onehalf, Qt::Key_onehalf)                                                            \
    O(QT_KEY_threequarters, Qt::Key_threequarters)                                                \
    O(QT_KEY_questiondown, Qt::Key_questiondown)                                                  \
    O(QT_KEY_Agrave, Qt::Key_Agrave)                                                              \
    O(QT_KEY_Aacute, Qt::Key_Aacute)                                                              \
    O(QT_KEY_Acircumflex, Qt::Key_Acircumflex)                                                    \
    O(QT_KEY_Atilde, Qt::Key_Atilde)                                                              \
    O(QT_KEY_Adiaeresis, Qt::Key_Adiaeresis)                                                      \
    O(QT_KEY_Aring, Qt::Key_Aring)                                                                \
    O(QT_KEY_AE, Qt::Key_AE)                                                                      \
    O(QT_KEY_Ccedilla, Qt::Key_Ccedilla)                                                          \
    O(QT_KEY_Egrave, Qt::Key_Egrave)                                                              \
    O(QT_KEY_Eacute, Qt::Key_Eacute)                                                              \
    O(QT_KEY_Ecircumflex, Qt::Key_Ecircumflex)                                                    \
    O(QT_KEY_Ediaeresis, Qt::Key_Ediaeresis)                                                      \
    O(QT_KEY_Igrave, Qt::Key_Igrave)                                                              \
    O(QT_KEY_Iacute, Qt::Key_Iacute)                                                              \
    O(QT_KEY_Icircumflex, Qt::Key_Icircumflex)                                                    \
    O(QT_KEY_Idiaeresis, Qt::Key_Idiaeresis)                                                      \
    O(QT_KEY_ETH, Qt::Key_ETH)                                                                    \
    O(QT_KEY_Ntilde, Qt::Key_Ntilde)                                                              \
    O(QT_KEY_Ograve, Qt::Key_Ograve)                                                              \
    O(QT_KEY_Oacute, Qt::Key_Oacute)                                                              \
    O(QT_KEY_Ocircumflex, Qt::Key_Ocircumflex)                                                    \
    O(QT_KEY_Otilde, Qt::Key_Otilde)                                                              \
    O(QT_KEY_Odiaeresis, Qt::Key_Odiaeresis)                                                      \
    O(QT_KEY_multiply, Qt::Key_multiply)                                                          \
    O(QT_KEY_Ooblique, Qt::Key_Ooblique)                                                          \
    O(QT_KEY_Ugrave, Qt::Key_Ugrave)                                                              \
    O(QT_KEY_Uacute, Qt::Key_Uacute)                                                              \
    O(QT_KEY_Ucircumflex, Qt::Key_Ucircumflex)                                                    \
    O(QT_KEY_Udiaeresis, Qt::Key_Udiaeresis)                                                      \
    O(QT_KEY_Yacute, Qt::Key_Yacute)                                                              \
    O(QT_KEY_THORN, Qt::Key_THORN)                                                                \
    O(QT_KEY_ssharp, Qt::Key_ssharp)                                                              \
    O(QT_KEY_division, Qt::Key_division)                                                          \
    O(QT_KEY_ydiaeresis, Qt::Key_ydiaeresis)                                                      \
    O(QT_KEY_AltGr, Qt::Key_AltGr)                                                                \
    O(QT_KEY_Multi_key, Qt::Key_Multi_key)                                                        \
    O(QT_KEY_Codeinput, Qt::Key_Codeinput)                                                        \
    O(QT_KEY_SingleCandidate, Qt::Key_SingleCandidate)                                            \
    O(QT_KEY_MultipleCandidate, Qt::Key_MultipleCandidate)                                        \
    O(QT_KEY_PreviousCandidate, Qt::Key_PreviousCandidate)                                        \
    O(QT_KEY_Mode_switch, Qt::Key_Mode_switch)                                                    \
    O(QT_KEY_Kanji, Qt::Key_Kanji)                                                                \
    O(QT_KEY_Muhenkan, Qt::Key_Muhenkan)                                                          \
    O(QT_KEY_Henkan, Qt::Key_Henkan)                                                              \
    O(QT_KEY_Romaji, Qt::Key_Romaji)                                                              \
    O(QT_KEY_Hiragana, Qt::Key_Hiragana)                                                          \
    O(QT_KEY_Katakana, Qt::Key_Katakana)                                                          \
    O(QT_KEY_Hiragana_Katakana, Qt::Key_Hiragana_Katakana)                                        \
    O(QT_KEY_Zenkaku, Qt::Key_Zenkaku)                                                            \
    O(QT_KEY_Hankaku, Qt::Key_Hankaku)                                                            \
    O(QT_KEY_Zenkaku_Hankaku, Qt::Key_Zenkaku_Hankaku)                                            \
    O(QT_KEY_Touroku, Qt::Key_Touroku)                                                            \
    O(QT_KEY_Massyo, Qt::Key_Massyo)                                                              \
    O(QT_KEY_Kana_Lock, Qt::Key_Kana_Lock)                                                        \
    O(QT_KEY_Kana_Shift, Qt::Key_Kana_Shift)                                                      \
    O(QT_KEY_Eisu_Shift, Qt::Key_Eisu_Shift)                                                      \
    O(QT_KEY_Eisu_toggle, Qt::Key_Eisu_toggle)                                                    \
    O(QT_KEY_Hangul, Qt::Key_Hangul)                                                              \
    O(QT_KEY_Hangul_Start, Qt::Key_Hangul_Start)                                                  \
    O(QT_KEY_Hangul_End, Qt::Key_Hangul_End)                                                      \
    O(QT_KEY_Hangul_Hanja, Qt::Key_Hangul_Hanja)                                                  \
    O(QT_KEY_Hangul_Jamo, Qt::Key_Hangul_Jamo)                                                    \
    O(QT_KEY_Hangul_Romaja, Qt::Key_Hangul_Romaja)                                                \
    O(QT_KEY_Hangul_Jeonja, Qt::Key_Hangul_Jeonja)                                                \
    O(QT_KEY_Hangul_Banja, Qt::Key_Hangul_Banja)                                                  \
    O(QT_KEY_Hangul_PreHanja, Qt::Key_Hangul_PreHanja)                                            \
    O(QT_KEY_Hangul_PostHanja, Qt::Key_Hangul_PostHanja)                                          \
    O(QT_KEY_Hangul_Special, Qt::Key_Hangul_Special)                                              \
    O(QT_KEY_Dead_Grave, Qt::Key_Dead_Grave)                                                      \
    O(QT_KEY_Dead_Acute, Qt::Key_Dead_Acute)                                                      \
    O(QT_KEY_Dead_Circumflex, Qt::Key_Dead_Circumflex)                                            \
    O(QT_KEY_Dead_Tilde, Qt::Key_Dead_Tilde)                                                      \
    O(QT_KEY_Dead_Macron, Qt::Key_Dead_Macron)                                                    \
    O(QT_KEY_Dead_Breve, Qt::Key_Dead_Breve)                                                      \
    O(QT_KEY_Dead_Abovedot, Qt::Key_Dead_Abovedot)                                                \
    O(QT_KEY_Dead_Diaeresis, Qt::Key_Dead_Diaeresis)                                              \
    O(QT_KEY_Dead_Abovering, Qt::Key_Dead_Abovering)                                              \
    O(QT_KEY_Dead_Doubleacute, Qt::Key_Dead_Doubleacute)                                          \
    O(QT_KEY_Dead_Caron, Qt::Key_Dead_Caron)                                                      \
    O(QT_KEY_Dead_Cedilla, Qt::Key_Dead_Cedilla)                                                  \
    O(QT_KEY_Dead_Ogonek, Qt::Key_Dead_Ogonek)                                                    \
    O(QT_KEY_Dead_Iota, Qt::Key_Dead_Iota)                                                        \
    O(QT_KEY_Dead_Voiced_Sound, Qt::Key_Dead_Voiced_Sound)                                        \
    O(QT_KEY_Dead_Semivoiced_Sound, Qt::Key_Dead_Semivoiced_Sound)                                \
    O(QT_KEY_Dead_Belowdot, Qt::Key_Dead_Belowdot)                                                \
    O(QT_KEY_Dead_Hook, Qt::Key_Dead_Hook)                                                        \
    O(QT_KEY_Dead_Horn, Qt::Key_Dead_Horn)                                                        \
    O(QT_KEY_Dead_Stroke, Qt::Key_Dead_Stroke)                                                    \
    O(QT_KEY_Dead_Abovecomma, Qt::Key_Dead_Abovecomma)                                            \
    O(QT_KEY_Dead_Abovereversedcomma, Qt::Key_Dead_Abovereversedcomma)                            \
    O(QT_KEY_Dead_Doublegrave, Qt::Key_Dead_Doublegrave)                                          \
    O(QT_KEY_Dead_Belowring, Qt::Key_Dead_Belowring)                                              \
    O(QT_KEY_Dead_Belowmacron, Qt::Key_Dead_Belowmacron)                                          \
    O(QT_KEY_Dead_Belowcircumflex, Qt::Key_Dead_Belowcircumflex)                                  \
    O(QT_KEY_Dead_Belowtilde, Qt::Key_Dead_Belowtilde)                                            \
    O(QT_KEY_Dead_Belowbreve, Qt::Key_Dead_Belowbreve)                                            \
    O(QT_KEY_Dead_Belowdiaeresis, Qt::Key_Dead_Belowdiaeresis)                                    \
    O(QT_KEY_Dead_Invertedbreve, Qt::Key_Dead_Invertedbreve)                                      \
    O(QT_KEY_Dead_Belowcomma, Qt::Key_Dead_Belowcomma)                                            \
    O(QT_KEY_Dead_Currency, Qt::Key_Dead_Currency)                                                \
    O(QT_KEY_Dead_a, Qt::Key_Dead_a)                                                              \
    O(QT_KEY_Dead_A, Qt::Key_Dead_A)                                                              \
    O(QT_KEY_Dead_e, Qt::Key_Dead_e)                                                              \
    O(QT_KEY_Dead_E, Qt::Key_Dead_E)                                                              \
    O(QT_KEY_Dead_i, Qt::Key_Dead_i)                                                              \
    O(QT_KEY_Dead_I, Qt::Key_Dead_I)                                                              \
    O(QT_KEY_Dead_o, Qt::Key_Dead_o)                                                              \
    O(QT_KEY_Dead_O, Qt::Key_Dead_O)                                                              \
    O(QT_KEY_Dead_u, Qt::Key_Dead_u)                                                              \
    O(QT_KEY_Dead_U, Qt::Key_Dead_U)                                                              \
    O(QT_KEY_Dead_Small_Schwa, Qt::Key_Dead_Small_Schwa)                                          \
    O(QT_KEY_Dead_Capital_Schwa, Qt::Key_Dead_Capital_Schwa)                                      \
    O(QT_KEY_Dead_Greek, Qt::Key_Dead_Greek)                                                      \
    O(QT_KEY_Dead_Lowline, Qt::Key_Dead_Lowline)                                                  \
    O(QT_KEY_Dead_Aboveverticalline, Qt::Key_Dead_Aboveverticalline)                              \
    O(QT_KEY_Dead_Belowverticalline, Qt::Key_Dead_Belowverticalline)                              \
    O(QT_KEY_Dead_Longsolidusoverlay, Qt::Key_Dead_Longsolidusoverlay)                            \
    O(QT_KEY_Back, Qt::Key_Back)                                                                  \
    O(QT_KEY_Forward, Qt::Key_Forward)                                                            \
    O(QT_KEY_Stop, Qt::Key_Stop)                                                                  \
    O(QT_KEY_Refresh, Qt::Key_Refresh)                                                            \
    O(QT_KEY_VolumeDown, Qt::Key_VolumeDown)                                                      \
    O(QT_KEY_VolumeMute, Qt::Key_VolumeMute)                                                      \
    O(QT_KEY_VolumeUp, Qt::Key_VolumeUp)                                                          \
    O(QT_KEY_BassBoost, Qt::Key_BassBoost)                                                        \
    O(QT_KEY_BassUp, Qt::Key_BassUp)                                                              \
    O(QT_KEY_BassDown, Qt::Key_BassDown)                                                          \
    O(QT_KEY_TrebleUp, Qt::Key_TrebleUp)                                                          \
    O(QT_KEY_TrebleDown, Qt::Key_TrebleDown)                                                      \
    O(QT_KEY_MediaPlay, Qt::Key_MediaPlay)                                                        \
    O(QT_KEY_MediaStop, Qt::Key_MediaStop)                                                        \
    O(QT_KEY_MediaPrevious, Qt::Key_MediaPrevious)                                                \
    O(QT_KEY_MediaNext, Qt::Key_MediaNext)                                                        \
    O(QT_KEY_MediaRecord, Qt::Key_MediaRecord)                                                    \
    O(QT_KEY_MediaPause, Qt::Key_MediaPause)                                                      \
    O(QT_KEY_MediaTogglePlayPause, Qt::Key_MediaTogglePlayPause)                                  \
    O(QT_KEY_HomePage, Qt::Key_HomePage)                                                          \
    O(QT_KEY_Favorites, Qt::Key_Favorites)                                                        \
    O(QT_KEY_Search, Qt::Key_Search)                                                              \
    O(QT_KEY_Standby, Qt::Key_Standby)                                                            \
    O(QT_KEY_OpenUrl, Qt::Key_OpenUrl)                                                            \
    O(QT_KEY_LaunchMail, Qt::Key_LaunchMail)                                                      \
    O(QT_KEY_LaunchMedia, Qt::Key_LaunchMedia)                                                    \
    O(QT_KEY_Launch0, Qt::Key_Launch0)                                                            \
    O(QT_KEY_Launch1, Qt::Key_Launch1)                                                            \
    O(QT_KEY_Launch2, Qt::Key_Launch2)                                                            \
    O(QT_KEY_Launch3, Qt::Key_Launch3)                                                            \
    O(QT_KEY_Launch4, Qt::Key_Launch4)                                                            \
    O(QT_KEY_Launch5, Qt::Key_Launch5)                                                            \
    O(QT_KEY_Launch6, Qt::Key_Launch6)                                                            \
    O(QT_KEY_Launch7, Qt::Key_Launch7)                                                            \
    O(QT_KEY_Launch8, Qt::Key_Launch8)                                                            \
    O(QT_KEY_Launch9, Qt::Key_Launch9)                                                            \
    O(QT_KEY_LaunchA, Qt::Key_LaunchA)                                                            \
    O(QT_KEY_LaunchB, Qt::Key_LaunchB)                                                            \
    O(QT_KEY_LaunchC, Qt::Key_LaunchC)                                                            \
    O(QT_KEY_LaunchD, Qt::Key_LaunchD)                                                            \
    O(QT_KEY_LaunchE, Qt::Key_LaunchE)                                                            \
    O(QT_KEY_LaunchF, Qt::Key_LaunchF)                                                            \
    O(QT_KEY_MonBrightnessUp, Qt::Key_MonBrightnessUp)                                            \
    O(QT_KEY_MonBrightnessDown, Qt::Key_MonBrightnessDown)                                        \
    O(QT_KEY_KeyboardLightOnOff, Qt::Key_KeyboardLightOnOff)                                      \
    O(QT_KEY_KeyboardBrightnessUp, Qt::Key_KeyboardBrightnessUp)                                  \
    O(QT_KEY_KeyboardBrightnessDown, Qt::Key_KeyboardBrightnessDown)                              \
    O(QT_KEY_PowerOff, Qt::Key_PowerOff)                                                          \
    O(QT_KEY_WakeUp, Qt::Key_WakeUp)                                                              \
    O(QT_KEY_Eject, Qt::Key_Eject)                                                                \
    O(QT_KEY_ScreenSaver, Qt::Key_ScreenSaver)                                                    \
    O(QT_KEY_WWW, Qt::Key_WWW)                                                                    \
    O(QT_KEY_Memo, Qt::Key_Memo)                                                                  \
    O(QT_KEY_LightBulb, Qt::Key_LightBulb)                                                        \
    O(QT_KEY_Shop, Qt::Key_Shop)                                                                  \
    O(QT_KEY_History, Qt::Key_History)                                                            \
    O(QT_KEY_AddFavorite, Qt::Key_AddFavorite)                                                    \
    O(QT_KEY_HotLinks, Qt::Key_HotLinks)                                                          \
    O(QT_KEY_BrightnessAdjust, Qt::Key_BrightnessAdjust)                                          \
    O(QT_KEY_Finance, Qt::Key_Finance)                                                            \
    O(QT_KEY_Community, Qt::Key_Community)                                                        \
    O(QT_KEY_AudioRewind, Qt::Key_AudioRewind)                                                    \
    O(QT_KEY_BackForward, Qt::Key_BackForward)                                                    \
    O(QT_KEY_ApplicationLeft, Qt::Key_ApplicationLeft)                                            \
    O(QT_KEY_ApplicationRight, Qt::Key_ApplicationRight)                                          \
    O(QT_KEY_Book, Qt::Key_Book)                                                                  \
    O(QT_KEY_CD, Qt::Key_CD)                                                                      \
    O(QT_KEY_Calculator, Qt::Key_Calculator)                                                      \
    O(QT_KEY_ToDoList, Qt::Key_ToDoList)                                                          \
    O(QT_KEY_ClearGrab, Qt::Key_ClearGrab)                                                        \
    O(QT_KEY_Close, Qt::Key_Close)                                                                \
    O(QT_KEY_Copy, Qt::Key_Copy)                                                                  \
    O(QT_KEY_Cut, Qt::Key_Cut)                                                                    \
    O(QT_KEY_Display, Qt::Key_Display)                                                            \
    O(QT_KEY_DOS, Qt::Key_DOS)                                                                    \
    O(QT_KEY_Documents, Qt::Key_Documents)                                                        \
    O(QT_KEY_Excel, Qt::Key_Excel)                                                                \
    O(QT_KEY_Explorer, Qt::Key_Explorer)                                                          \
    O(QT_KEY_Game, Qt::Key_Game)                                                                  \
    O(QT_KEY_Go, Qt::Key_Go)                                                                      \
    O(QT_KEY_iTouch, Qt::Key_iTouch)                                                              \
    O(QT_KEY_LogOff, Qt::Key_LogOff)                                                              \
    O(QT_KEY_Market, Qt::Key_Market)                                                              \
    O(QT_KEY_Meeting, Qt::Key_Meeting)                                                            \
    O(QT_KEY_MenuKB, Qt::Key_MenuKB)                                                              \
    O(QT_KEY_MenuPB, Qt::Key_MenuPB)                                                              \
    O(QT_KEY_MySites, Qt::Key_MySites)                                                            \
    O(QT_KEY_News, Qt::Key_News)                                                                  \
    O(QT_KEY_OfficeHome, Qt::Key_OfficeHome)                                                      \
    O(QT_KEY_Option, Qt::Key_Option)                                                              \
    O(QT_KEY_Paste, Qt::Key_Paste)                                                                \
    O(QT_KEY_Phone, Qt::Key_Phone)                                                                \
    O(QT_KEY_Calendar, Qt::Key_Calendar)                                                          \
    O(QT_KEY_Reply, Qt::Key_Reply)                                                                \
    O(QT_KEY_Reload, Qt::Key_Reload)                                                              \
    O(QT_KEY_RotateWindows, Qt::Key_RotateWindows)                                                \
    O(QT_KEY_RotationPB, Qt::Key_RotationPB)                                                      \
    O(QT_KEY_RotationKB, Qt::Key_RotationKB)                                                      \
    O(QT_KEY_Save, Qt::Key_Save)                                                                  \
    O(QT_KEY_Send, Qt::Key_Send)                                                                  \
    O(QT_KEY_Spell, Qt::Key_Spell)                                                                \
    O(QT_KEY_SplitScreen, Qt::Key_SplitScreen)                                                    \
    O(QT_KEY_Support, Qt::Key_Support)                                                            \
    O(QT_KEY_TaskPane, Qt::Key_TaskPane)                                                          \
    O(QT_KEY_Terminal, Qt::Key_Terminal)                                                          \
    O(QT_KEY_Tools, Qt::Key_Tools)                                                                \
    O(QT_KEY_Travel, Qt::Key_Travel)                                                              \
    O(QT_KEY_Video, Qt::Key_Video)                                                                \
    O(QT_KEY_Word, Qt::Key_Word)                                                                  \
    O(QT_KEY_Xfer, Qt::Key_Xfer)                                                                  \
    O(QT_KEY_ZoomIn, Qt::Key_ZoomIn)                                                              \
    O(QT_KEY_ZoomOut, Qt::Key_ZoomOut)                                                            \
    O(QT_KEY_Away, Qt::Key_Away)                                                                  \
    O(QT_KEY_Messenger, Qt::Key_Messenger)                                                        \
    O(QT_KEY_WebCam, Qt::Key_WebCam)                                                              \
    O(QT_KEY_MailForward, Qt::Key_MailForward)                                                    \
    O(QT_KEY_Pictures, Qt::Key_Pictures)                                                          \
    O(QT_KEY_Music, Qt::Key_Music)                                                                \
    O(QT_KEY_Battery, Qt::Key_Battery)                                                            \
    O(QT_KEY_Bluetooth, Qt::Key_Bluetooth)                                                        \
    O(QT_KEY_WLAN, Qt::Key_WLAN)                                                                  \
    O(QT_KEY_UWB, Qt::Key_UWB)                                                                    \
    O(QT_KEY_AudioForward, Qt::Key_AudioForward)                                                  \
    O(QT_KEY_AudioRepeat, Qt::Key_AudioRepeat)                                                    \
    O(QT_KEY_AudioRandomPlay, Qt::Key_AudioRandomPlay)                                            \
    O(QT_KEY_Subtitle, Qt::Key_Subtitle)                                                          \
    O(QT_KEY_AudioCycleTrack, Qt::Key_AudioCycleTrack)                                            \
    O(QT_KEY_Time, Qt::Key_Time)                                                                  \
    O(QT_KEY_Hibernate, Qt::Key_Hibernate)                                                        \
    O(QT_KEY_View, Qt::Key_View)                                                                  \
    O(QT_KEY_TopMenu, Qt::Key_TopMenu)                                                            \
    O(QT_KEY_PowerDown, Qt::Key_PowerDown)                                                        \
    O(QT_KEY_Suspend, Qt::Key_Suspend)                                                            \
    O(QT_KEY_ContrastAdjust, Qt::Key_ContrastAdjust)                                              \
    O(QT_KEY_LaunchG, Qt::Key_LaunchG)                                                            \
    O(QT_KEY_LaunchH, Qt::Key_LaunchH)                                                            \
    O(QT_KEY_TouchpadToggle, Qt::Key_TouchpadToggle)                                              \
    O(QT_KEY_TouchpadOn, Qt::Key_TouchpadOn)                                                      \
    O(QT_KEY_TouchpadOff, Qt::Key_TouchpadOff)                                                    \
    O(QT_KEY_MicMute, Qt::Key_MicMute)                                                            \
    O(QT_KEY_Red, Qt::Key_Red)                                                                    \
    O(QT_KEY_Green, Qt::Key_Green)                                                                \
    O(QT_KEY_Yellow, Qt::Key_Yellow)                                                              \
    O(QT_KEY_Blue, Qt::Key_Blue)                                                                  \
    O(QT_KEY_ChannelUp, Qt::Key_ChannelUp)                                                        \
    O(QT_KEY_ChannelDown, Qt::Key_ChannelDown)                                                    \
    O(QT_KEY_Guide, Qt::Key_Guide)                                                                \
    O(QT_KEY_Info, Qt::Key_Info)                                                                  \
    O(QT_KEY_Settings, Qt::Key_Settings)                                                          \
    O(QT_KEY_MicVolumeUp, Qt::Key_MicVolumeUp)                                                    \
    O(QT_KEY_MicVolumeDown, Qt::Key_MicVolumeDown)                                                \
    O(QT_KEY_New, Qt::Key_New)                                                                    \
    O(QT_KEY_Open, Qt::Key_Open)                                                                  \
    O(QT_KEY_Find, Qt::Key_Find)                                                                  \
    O(QT_KEY_Undo, Qt::Key_Undo)                                                                  \
    O(QT_KEY_Redo, Qt::Key_Redo)                                                                  \
    O(QT_KEY_MediaLast, Qt::Key_MediaLast)                                                        \
    O(QT_KEY_Select, Qt::Key_Select)                                                              \
    O(QT_KEY_Yes, Qt::Key_Yes)                                                                    \
    O(QT_KEY_No, Qt::Key_No)                                                                      \
    O(QT_KEY_Cancel, Qt::Key_Cancel)                                                              \
    O(QT_KEY_Printer, Qt::Key_Printer)                                                            \
    O(QT_KEY_Execute, Qt::Key_Execute)                                                            \
    O(QT_KEY_Sleep, Qt::Key_Sleep)                                                                \
    O(QT_KEY_Play, Qt::Key_Play)                                                                  \
    O(QT_KEY_Zoom, Qt::Key_Zoom)                                                                  \
    O(QT_KEY_Exit, Qt::Key_Exit)                                                                  \
    O(QT_KEY_Context1, Qt::Key_Context1)                                                          \
    O(QT_KEY_Context2, Qt::Key_Context2)                                                          \
    O(QT_KEY_Context3, Qt::Key_Context3)                                                          \
    O(QT_KEY_Context4, Qt::Key_Context4)                                                          \
    O(QT_KEY_Call, Qt::Key_Call)                                                                  \
    O(QT_KEY_Hangup, Qt::Key_Hangup)                                                              \
    O(QT_KEY_Flip, Qt::Key_Flip)                                                                  \
    O(QT_KEY_ToggleCallHangup, Qt::Key_ToggleCallHangup)                                          \
    O(QT_KEY_VoiceDial, Qt::Key_VoiceDial)                                                        \
    O(QT_KEY_LastNumberRedial, Qt::Key_LastNumberRedial)                                          \
    O(QT_KEY_Camera, Qt::Key_Camera)                                                              \
    O(QT_KEY_CameraFocus, Qt::Key_CameraFocus)                                                    \
    O(QT_KEY_unknown, Qt::Key_unknown)

enum QT_Key { ENUMERE_CLE_CLAVIER(ENUMERE_DECLARATION_ENUM_IPA) };

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Keyboard_Modifier
 * \{ */

#define ENUMERE_MODIFICATEURS_CLAVIER(O)                                                          \
    O(QT_KEYBOARD_MODIFIER_AUCUN, Qt::NoModifier, 0)                                              \
    O(QT_KEYBOARD_MODIFIER_MAJ, Qt::ShiftModifier, 1)                                             \
    O(QT_KEYBOARD_MODIFIER_CTRL, Qt::ControlModifier, 2)                                          \
    O(QT_KEYBOARD_MODIFIER_ALT, Qt::AltModifier, 4)                                               \
    O(QT_KEYBOARD_MODIFIER_META, Qt::MetaModifier, 8)                                             \
    O(QT_KEYBOARD_MODIFIER_KEYPAD, Qt::KeypadModifier, 16)                                        \
    O(QT_KEYBOARD_MODIFIER_GROUP_SWITCH, Qt::GroupSwitchModifier, 32)

enum QT_Keyboard_Modifier { ENUMERE_MODIFICATEURS_CLAVIER(ENUMERE_DECLARATION_ENUM_DRAPEAU_IPA) };

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
/** \name QT_KeyEvent
 * \{ */

enum QT_Key QT_key_event_donne_cle(struct QT_KeyEvent *event);
enum QT_Keyboard_Modifier QT_key_event_donne_modificateurs_clavier(struct QT_KeyEvent *event);
int QT_key_event_donne_compte(struct QT_KeyEvent *event);
bool QT_key_event_est_auto_repete(struct QT_KeyEvent *event);
uint32_t QT_key_event_donne_cle_virtuelle_native(struct QT_KeyEvent *event);
uint32_t QT_key_event_donne_code_scan_natif(struct QT_KeyEvent *event);
uint32_t QT_key_event_donne_modificateurs_natifs(struct QT_KeyEvent *event);
struct QT_Chaine QT_key_event_donne_texte(struct QT_KeyEvent *event);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Widget
 * \{ */

#define RAPPELS_EVENEMENTS_COMMUNS(type_classe)                                                   \
    bool (*sur_evenement)(struct type_classe *, union QT_Generic_Event);                          \
    void (*sur_entree)(struct type_classe *, struct QT_Evenement *);                              \
    void (*sur_sortie)(struct type_classe *, struct QT_Evenement *);                              \
    void (*sur_pression_souris)(struct type_classe *, struct QT_MouseEvent *);                    \
    void (*sur_deplacement_souris)(struct type_classe *, struct QT_MouseEvent *);                 \
    void (*sur_relachement_souris)(struct type_classe *, struct QT_MouseEvent *);                 \
    void (*sur_double_clique_souris)(struct type_classe *, struct QT_MouseEvent *);               \
    void (*sur_molette_souris)(struct type_classe *, struct QT_WheelEvent *);                     \
    void (*sur_redimensionnement)(struct type_classe *, struct QT_ResizeEvent *);                 \
    void (*sur_pression_cle)(struct type_classe *, struct QT_KeyEvent *);                         \
    void (*sur_relachement_cle)(struct type_classe *, struct QT_KeyEvent *);                      \
    void (*sur_destruction)(struct type_classe *)

struct QT_Rappels_Widget {
    RAPPELS_EVENEMENTS_COMMUNS(QT_Rappels_Widget);
    /** Le widget pour lequel les rappels sont mis en place. */
    struct QT_Widget *widget;
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

void QT_widget_transforme_point_vers_global(union QT_Generic_Widget widget,
                                            struct QT_Point point,
                                            struct QT_Point *r_point);

void QT_widget_transforme_point_vers_local(union QT_Generic_Widget widget,
                                           struct QT_Point point,
                                           struct QT_Point *r_point);

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
/** \name QT_MenuBar
 * \{ */

void QT_menu_bar_ajoute_menu(struct QT_MenuBar *menu_bar, struct QT_Menu *menu);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Menu
 * \{ */

void QT_menu_connecte_sur_pret_a_montrer(struct QT_Menu *menu, struct QT_Rappel_Generique *rappel);
void QT_menu_popup(struct QT_Menu *menu, struct QT_Point pos);

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
struct QT_Chaine QT_combobox_donne_valeur_courante_chaine(struct QT_ComboBox *combo);

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
/** \name QT_FileDialog
 * \{ */

struct QT_Chaine QT_file_dialog_donne_chemin_pour_lecture(union QT_Generic_Widget parent,
                                                          struct QT_Chaine titre,
                                                          struct QT_Chaine dossier,
                                                          struct QT_Chaine filtre);
struct QT_Chaine QT_file_dialog_donne_chemin_pour_ecriture(union QT_Generic_Widget parent,
                                                           struct QT_Chaine titre,
                                                           struct QT_Chaine dossier,
                                                           struct QT_Chaine filtre);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Keyboard_Modifier
 * \{ */

#define ENUMERE_BOUTON_STANDARD(O)                                                                \
    O(QT_STANDARDBUTTON_AUCUN, QMessageBox::NoButton, 0x00000000)                                 \
    O(QT_STANDARDBUTTON_OK, QMessageBox::Ok, 0x00000400)                                          \
    O(QT_STANDARDBUTTON_SAVE, QMessageBox::Save, 0x00000800)                                      \
    O(QT_STANDARDBUTTON_SAVE_ALL, QMessageBox::SaveAll, 0x00001000)                               \
    O(QT_STANDARDBUTTON_OPEN, QMessageBox::Open, 0x00002000)                                      \
    O(QT_STANDARDBUTTON_YES, QMessageBox::Yes, 0x00004000)                                        \
    O(QT_STANDARDBUTTON_YES_TO_ALL, QMessageBox::YesToAll, 0x00008000)                            \
    O(QT_STANDARDBUTTON_NO, QMessageBox::No, 0x00010000)                                          \
    O(QT_STANDARDBUTTON_NO_TO_ALL, QMessageBox::NoToAll, 0x00020000)                              \
    O(QT_STANDARDBUTTON_ABORT, QMessageBox::Abort, 0x00040000)                                    \
    O(QT_STANDARDBUTTON_RETRY, QMessageBox::Retry, 0x00080000)                                    \
    O(QT_STANDARDBUTTON_IGNORE, QMessageBox::Ignore, 0x00100000)                                  \
    O(QT_STANDARDBUTTON_CLOSE, QMessageBox::Close, 0x00200000)                                    \
    O(QT_STANDARDBUTTON_CANCEL, QMessageBox::Cancel, 0x00400000)                                  \
    O(QT_STANDARDBUTTON_DISCARD, QMessageBox::Discard, 0x00800000)                                \
    O(QT_STANDARDBUTTON_HELP, QMessageBox::Help, 0x01000000)                                      \
    O(QT_STANDARDBUTTON_APPLY, QMessageBox::Apply, 0x02000000)                                    \
    O(QT_STANDARDBUTTON_RESET, QMessageBox::Reset, 0x04000000)                                    \
    O(QT_STANDARDBUTTON_RESTORE_DEFAULTS, QMessageBox::RestoreDefaults, 0x08000000)

enum QT_StandardButton { ENUMERE_BOUTON_STANDARD(ENUMERE_DECLARATION_ENUM_DRAPEAU_IPA) };

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_MessageBox
 * \{ */

enum QT_StandardButton QT_message_box_affiche_avertissement(union QT_Generic_Widget parent,
                                                            struct QT_Chaine titre,
                                                            struct QT_Chaine message,
                                                            enum QT_StandardButton boutons);

enum QT_StandardButton QT_message_box_affiche_erreur(union QT_Generic_Widget parent,
                                                     struct QT_Chaine titre,
                                                     struct QT_Chaine message,
                                                     enum QT_StandardButton boutons);

enum QT_StandardButton QT_message_box_affiche_question(union QT_Generic_Widget parent,
                                                       struct QT_Chaine titre,
                                                       struct QT_Chaine message,
                                                       enum QT_StandardButton boutons);

enum QT_StandardButton QT_message_box_affiche_information(union QT_Generic_Widget parent,
                                                          struct QT_Chaine titre,
                                                          struct QT_Chaine message,
                                                          enum QT_StandardButton boutons);

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
/** \name QT_GraphicsItem
 * \{ */

void QT_graphics_item_definis_position(union QT_Generic_GraphicsItem item, struct QT_PointF pos);
struct QT_RectF QT_graphics_item_donne_rect(union QT_Generic_GraphicsItem item);

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
void QT_graphics_text_item_definis_police(struct QT_GraphicsTextItem *text_item,
                                          struct QT_Font font);
void QT_graphics_text_item_definis_couleur_defaut(struct QT_GraphicsTextItem *text_item,
                                                  struct QT_Color color);
struct QT_RectF QT_graphics_text_item_donne_rect(struct QT_GraphicsTextItem *item);
void QT_graphics_text_item_definis_position(struct QT_GraphicsTextItem *item,
                                            struct QT_PointF *pos);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GraphicsLineItem
 * \{ */

struct QT_GraphicsLineItem *QT_cree_graphics_line_item(union QT_Generic_GraphicsItem parent);
void QT_graphics_line_item_definis_pinceau(struct QT_GraphicsLineItem *item,
                                           struct QT_Pen pinceau);
void QT_graphics_line_item_definis_ligne(
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
void QT_graphics_view_mappe_vers_scene(struct QT_GraphicsView *graphics_view,
                                       struct QT_Point point,
                                       struct QT_PointF *r_point);
void QT_graphics_view_mappe_depuis_scene(struct QT_GraphicsView *graphics_view,
                                         struct QT_PointF point,
                                         struct QT_Point *r_point);
void QT_graphics_view_mappe_vers_global(struct QT_GraphicsView *graphics_view,
                                        struct QT_Point point,
                                        struct QT_Point *r_point);

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
/** \name DNJ_Rappels_Pilote_Clique
 * \{ */

struct DNJ_Rappels_Pilote_Clique {
    void (*sur_destruction)(struct DNJ_Rappels_Pilote_Clique *);
    bool (*sur_évaluation_prédicat)(struct DNJ_Rappels_Pilote_Clique *,
                                    struct QT_Chaine,
                                    struct QT_Chaine);
    void (*sur_clique)(struct DNJ_Rappels_Pilote_Clique *, struct QT_Chaine, struct QT_Chaine);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Pilote_Clique
 * \{ */

struct DNJ_Pilote_Clique;

struct DNJ_Pilote_Clique *DNJ_cree_pilote_clique(struct DNJ_Rappels_Pilote_Clique *rappels);
void DNJ_detruit_pilote_clique(struct DNJ_Pilote_Clique *pilote);

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

/* ------------------------------------------------------------------------- */
/** \name DNJ_Contexte_Interface
 * \{ */

struct DNJ_Contexte_Interface {
    struct DNJ_Pilote_Clique *pilote_clique;
    struct DNJ_Conteneur_Controles *conteneur;
    union QT_Generic_Widget parent_barre_outils;
    union QT_Generic_Widget parent_menu;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Donnees_Action
 * \{ */

struct DNJ_Donnees_Action {
    struct DNJ_Pilote_Clique *pilote_clique;
    struct QT_Chaine attache;
    struct QT_Chaine nom;
    struct QT_Chaine metadonnee;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Gestionnaire_Interface
 * \{ */

struct DNJ_Gestionnaire_Interface;

struct DNJ_Gestionnaire_Interface *DNJ_cree_gestionnaire_interface();
void DNJ_detruit_gestionnaire_interface(struct DNJ_Gestionnaire_Interface *gestionnaire);
struct QT_Menu *DNJ_gestionaire_compile_menu_fichier(
    struct DNJ_Gestionnaire_Interface *gestionnaire,
    struct DNJ_Contexte_Interface *context,
    struct QT_Chaine chemin);

struct QT_Menu *DNJ_gestionnaire_donne_menu(struct DNJ_Gestionnaire_Interface *gestionnaire,
                                            struct QT_Chaine nom_menu);

void DNJ_gestionnaire_recree_menu(struct DNJ_Gestionnaire_Interface *gestionnaire,
                                  struct QT_Chaine nom_menu,
                                  struct DNJ_Donnees_Action *actions,
                                  int64_t nombre_actions);

struct QT_ToolBar *DNJ_gestionaire_compile_barre_a_outils_fichier(
    struct DNJ_Gestionnaire_Interface *gestionnaire,
    struct DNJ_Contexte_Interface *context,
    struct QT_Chaine chemin);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_FournisseuseIcone
 * \{ */

enum DNJ_Etat_Icone {
    DNJ_ETAT_ICONE_ACTIF,
    DNJ_ETAT_ICONE_INACTIF,
};

struct DNJ_Rappels_Fournisseuse_Icone {
    void (*sur_destruction)(struct DNJ_Rappels_Fournisseuse_Icone *);

    bool (*donne_icone_pour_bouton_animation)(struct DNJ_Rappels_Fournisseuse_Icone *,
                                              enum DNJ_Etat_Icone,
                                              struct QT_Chaine *);

    bool (*donne_icone_pour_echelle_valeur)(struct DNJ_Rappels_Fournisseuse_Icone *,
                                            enum DNJ_Etat_Icone,
                                            struct QT_Chaine *);

    bool (*donne_icone_pour_identifiant)(struct DNJ_Rappels_Fournisseuse_Icone *,
                                         struct QT_Chaine,
                                         enum DNJ_Etat_Icone,
                                         struct QT_Chaine *);
};

struct DNJ_FournisseuseIcone;

struct DNJ_FournisseuseIcone *DNJ_cree_fournisseuse_icone(
    struct DNJ_Rappels_Fournisseuse_Icone *rappels);
void DNJ_detruit_fournisseuse_icone(struct DNJ_FournisseuseIcone *fournisseuse);

void DNJ_definis_fournisseuse_icone(struct DNJ_FournisseuseIcone *fournisseuse);

/** \} */

#ifdef __cplusplus
}
#endif
