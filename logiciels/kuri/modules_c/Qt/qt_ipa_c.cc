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
#include <QAudioDevice>
#include <QAudioSink>
#include <QAudioSource>
#include <QClipboard>
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
#include <QLocalServer>
#include <QLocalSocket>
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

#include <iostream>

#include "biblinternes/outils/definitions.h"

#include "danjo/manipulable.h"

#include "conversions.hh"
#include "fenetre_principale.hh"
#include "ipa_danjo.hh"
#include "tabs.hh"
#include "widgets.hh"

#define CONVERTIS_ET_APPEL(objet, fonction, ...)                                                  \
    VERS_QT(objet);                                                                               \
    q##objet->fonction(__VA_ARGS__);

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

extern "C" {

/* ------------------------------------------------------------------------- */
/** \name QT_Chaine
 * \{ */

static QT_Chaine crée_qt_chaine(const QString &string)
{
    auto const std_string = string.toStdString();
    QT_Chaine résultat;
    résultat.caractères = new char[std_string.size()];
    résultat.taille = int64_t(std_string.size());
    memcpy(résultat.caractères, std_string.c_str(), std_string.size());
    return résultat;
}

static QT_Chaine crée_qt_chaine_tampon(const QString &string, char *tampon, size_t taille_tampon)
{
    auto const std_string = string.toStdString();
    QT_Chaine résultat;

    if (std_string.size() < taille_tampon) {
        memcpy(tampon, std_string.c_str(), std_string.size());
        résultat.caractères = tampon;
        résultat.taille = int64_t(std_string.size());
    }
    else {
        résultat.caractères = nullptr;
        résultat.taille = 0;
    }

    return résultat;
}

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
/** \name QT_Icon
 * \{ */

QT_Icon *QT_cree_icon_chemin(QT_Chaine chemin)
{
    return vers_ipa(new QIcon(vers_qt(chemin)));
}

void QT_detruit_icon(QT_Icon *icon)
{
    auto qicon = vers_qt(icon);
    delete qicon;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_AudioFormat
 * \{ */

static QAudioFormat::ChannelConfig convertis_channel_config(QT_Audio_Format_Channel_Config config)
{
    switch (config) {
        ENUMERE_CHANNEL_CONFIG(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QAudioFormat::ChannelConfigUnknown;
}

static QAudioFormat::SampleFormat convertis_sample_format(QT_Audio_Format_Sample_Format format)
{
    switch (format) {
        ENUMERE_SAMPLE_FORMAT(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QAudioFormat::Unknown;
}

static QAudioFormat vers_qt(QT_AudioFormat *format)
{
    QAudioFormat résultat;
    résultat.setSampleRate(format->sample_rate);
    résultat.setSampleFormat(convertis_sample_format(format->sample_format));
    résultat.setChannelCount(format->channel_count);
    résultat.setChannelConfig(convertis_channel_config(format->channel_config));
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_AudioDevice
 * \{ */

static std::optional<QAudioDevice> donne_audio_device(struct QT_AudioDevice *device)
{
    auto devices = QMediaDevices::audioOutputs();

    for (auto qdevice : devices) {
        if (qdevice.id() == vers_qt(device->id)) {
            return qdevice;
        }
    }
    return {};
}

void QT_detruit_audio_device(struct QT_AudioDevice *device)
{
    if (!device) {
        return;
    }
    QT_chaine_detruit(&device->id);
    QT_chaine_detruit(&device->description);
}

bool QT_audio_device_is_format_supported(struct QT_AudioDevice *device,
                                         struct QT_AudioFormat *format)
{
    if (!device || !format) {
        return false;
    }

    auto opt_qdevice = donne_audio_device(device);
    if (!opt_qdevice.has_value()) {
        return false;
    }

    auto qdevice = opt_qdevice.value();
    auto qformat = vers_qt(format);
    return qdevice.isFormatSupported(qformat);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_MediaDevices
 * \{ */

void QT_media_devices_default_audio_output(struct QT_AudioDevice *resultat)
{
    if (!resultat) {
        return;
    }

    QT_detruit_audio_device(resultat);

    auto default_output_device = QMediaDevices::defaultAudioOutput();
    resultat->id = crée_qt_chaine(default_output_device.id());
    resultat->description = crée_qt_chaine(default_output_device.description());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_AudioSink
 * \{ */

static QT_AudioState convertis_audio_state_vers_ipa(QtAudio::State state)
{
    switch (state) {
        ENUMERE_AUDIO_STATE(ENUMERE_TRANSLATION_ENUM_QT_VERS_IPA)
    }
    return QT_AudioState(0);
}

static QT_AudioError convertis_audio_error_vers_ipa(QtAudio::Error error)
{
    switch (error) {
        ENUMERE_AUDIO_ERROR(ENUMERE_TRANSLATION_ENUM_QT_VERS_IPA)
    }
    return QT_AudioError(0);
}

QT_AudioSink *QT_audio_sink_cree(struct QT_AudioFormat *format, union QT_Generic_Object parent)
{
    VERS_QT(parent);
    VERS_QT(format);

    auto résultat = new QAudioSink(qformat, qparent);
    return vers_ipa(résultat);
}

void QT_audio_sink_detruit(struct QT_AudioSink *sink)
{
    VERS_QT(sink);
    delete qsink;
}

QT_AudioError QT_audio_sink_error(struct QT_AudioSink *sink)
{
    VERS_QT(sink);
    return convertis_audio_error_vers_ipa(qsink->error());
}

void QT_audio_sink_reset(struct QT_AudioSink *sink)
{
    CONVERTIS_ET_APPEL(sink, reset);
}

void QT_audio_sink_resume(struct QT_AudioSink *sink)
{
    CONVERTIS_ET_APPEL(sink, resume);
}

void QT_audio_sink_stop(struct QT_AudioSink *sink)
{
    CONVERTIS_ET_APPEL(sink, stop);
}

void QT_audio_sink_suspend(struct QT_AudioSink *sink)
{
    CONVERTIS_ET_APPEL(sink, suspend);
}

bool QT_audio_sink_is_null(struct QT_AudioSink *sink)
{
    VERS_QT(sink);
    return qsink->isNull();
}

void QT_audio_sink_set_volume(struct QT_AudioSink *sink, double volume)
{
    VERS_QT(sink);
    qsink->setVolume(volume);
}

double QT_audio_sink_get_volume(struct QT_AudioSink *sink)
{
    VERS_QT(sink);
    return qsink->volume();
}

void QT_audio_sink_start(struct QT_AudioSink *sink, struct QT_IODevice *device)
{
    VERS_QT(sink);
    VERS_QT(device);
    qsink->start(qdevice);
}

enum QT_AudioState QT_audio_sink_state(struct QT_AudioSink *sink)
{
    VERS_QT(sink);
    return convertis_audio_state_vers_ipa(qsink->state());
}

void QT_audio_sink_sur_state_changed(struct QT_AudioSink *sink, struct QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(sink);
    QObject::connect(
        qsink, &QAudioSink::stateChanged, [=](QtAudio::State) { rappel->sur_rappel(rappel); });
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_AudioSource
 * \{ */

QT_AudioSource *QT_audio_source_cree(struct QT_AudioFormat *format, union QT_Generic_Object parent)
{
    VERS_QT(parent);
    VERS_QT(format);

    auto résultat = new QAudioSource(qformat, qparent);
    return vers_ipa(résultat);
}

void QT_audio_source_detruit(struct QT_AudioSource *source)
{
    VERS_QT(source);
    delete qsource;
}

QT_AudioError QT_audio_source_error(struct QT_AudioSource *source)
{
    VERS_QT(source);
    return convertis_audio_error_vers_ipa(qsource->error());
}

void QT_audio_source_reset(struct QT_AudioSource *source)
{
    CONVERTIS_ET_APPEL(source, reset);
}

void QT_audio_source_resume(struct QT_AudioSource *source)
{
    CONVERTIS_ET_APPEL(source, resume);
}

void QT_audio_source_stop(struct QT_AudioSource *source)
{
    CONVERTIS_ET_APPEL(source, stop);
}

void QT_audio_source_suspend(struct QT_AudioSource *source)
{
    CONVERTIS_ET_APPEL(source, suspend);
}

bool QT_audio_source_is_null(struct QT_AudioSource *source)
{
    VERS_QT(source);
    return qsource->isNull();
}

void QT_audio_source_set_volume(struct QT_AudioSource *source, double volume)
{
    VERS_QT(source);
    qsource->setVolume(volume);
}

double QT_audio_source_get_volume(struct QT_AudioSource *source)
{
    VERS_QT(source);
    return qsource->volume();
}

void QT_audio_source_start(struct QT_AudioSource *source, struct QT_IODevice *device)
{
    VERS_QT(source);
    VERS_QT(device);
    qsource->start(qdevice);
}

enum QT_AudioState QT_audio_source_state(struct QT_AudioSource *source)
{
    VERS_QT(source);
    return convertis_audio_state_vers_ipa(qsource->state());
}

void QT_audio_source_sur_state_changed(struct QT_AudioSource *source,
                                       struct QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(source);
    QObject::connect(
        qsource, &QAudioSource::stateChanged, [=](QtAudio::State) { rappel->sur_rappel(rappel); });
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Object
 * \{ */

class Object final : public QObject {
    QT_Rappels_Object *m_rappels;

  public:
    Object(QT_Rappels_Object *rappels, QObject *parent = nullptr)
        : QObject(parent), m_rappels(rappels)
    {
    }

    EMPECHE_COPIE(Object);

    bool event(QEvent *event) override
    {
        if (!m_rappels || !m_rappels->sur_evenement) {
            return QObject::event(event);
        }

        if (m_rappels->sur_evenement(m_rappels, vers_ipa(event))) {
            return true;
        }

        return QObject::event(event);
    }
};

QT_Object *QT_object_cree(QT_Rappels_Object *rappels, QT_Generic_Object parent)
{
    VERS_QT(parent);
    auto résultat = vers_ipa(new Object(rappels, qparent));
    if (rappels) {
        rappels->object = résultat;
    }
    return résultat;
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

void QT_object_move_to_thread(QT_Generic_Object object, QT_Thread *thread)
{
    VERS_QT(object);
    VERS_QT(thread);
    qobject->moveToThread(qthread);
}

void QT_Object_install_event_filter(QT_Generic_Object object, QT_Generic_Object filter)
{
    VERS_QT(object);
    VERS_QT(filter);
    qobject->installEventFilter(qfilter);
}

void QT_Object_remove_event_filter(QT_Generic_Object object, QT_Generic_Object filter)
{
    VERS_QT(object);
    VERS_QT(filter);
    qobject->removeEventFilter(qfilter);
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

void QT_core_application_set_attribute(QT_ApplicationAttribute attribute)
{
    QCoreApplication::setAttribute((Qt::ApplicationAttribute)attribute);
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

QT_Clipboard *QT_application_donne_clipboard()
{
    return vers_ipa(QApplication::clipboard());
}

void QT_application_process_events()
{
    QCoreApplication::processEvents();
}

QT_Thread *QT_application_thread()
{
    return vers_ipa(QApplication::instance()->thread());
}

int QT_application_screen_count()
{
    return static_cast<int>(QApplication::screens().size());
}

void QT_application_screen_geometry(int index, QT_Rect *rect)
{
    *rect = vers_ipa(QApplication::screens().at(index)->geometry());
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

static QStyle::StandardPixmap convertis_standard_pixmap(QT_Standard_Pixmap standard_icon)
{
    switch (standard_icon) {
        ENUMERE_STANDARD_PIXMAP(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QStyle::SP_TitleBarMenuButton;
}

QT_Style *QT_application_donne_style()
{
    return vers_ipa(QApplication::style());
}

QT_Icon *QT_style_donne_standard_icon(QT_Style *style, QT_Standard_Pixmap standard_icon)
{
    VERS_QT(style);
    auto icon = qstyle->standardIcon(convertis_standard_pixmap(standard_icon));
    auto résultat = new QIcon(icon);
    return vers_ipa(résultat);
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

void QT_timer_definis_se_repete(QT_Timer *timer, bool ouinon)
{
    auto qtimer = vers_qt(timer);
    qtimer->setSingleShot(!ouinon);
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

void QT_action_definis_icone(QT_Action *action, QT_Icon *icon)
{
    VERS_QT(action);
    VERS_QT(icon);
    if (qicon) {
        qaction->setIcon(*qicon);
    }
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

void QT_action_definis_cochable(QT_Action *action, bool ouinon)
{
    VERS_QT(action);
    qaction->setCheckable(ouinon);
}

void QT_action_definis_coche(QT_Action *action, bool ouinon)
{
    VERS_QT(action);
    qaction->setChecked(ouinon);
}

bool QT_action_est_coche(QT_Action *action)
{
    VERS_QT(action);
    return qaction->isChecked();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Drag
 * \{ */

static QT_DropAction convertis_drop_action(Qt::DropAction drapeaux)
{
    int résultat = 0;
    ENUMERE_DROP_ACTION(ENUMERE_TRANSLATION_ENUM_DRAPEAU_QT_VERS_IPA);
    return QT_DropAction(résultat);
}

QT_Drag *QT_cree_drag(QT_Generic_Object source)
{
    VERS_QT(source);
    return vers_ipa(new QDrag(qsource));
}

void QT_drag_definis_mimedata(QT_Drag *drag, QT_MimeData *mimedata)
{
    VERS_QT(drag);
    VERS_QT(mimedata);
    qdrag->setMimeData(qmimedata);
}

void QT_drag_definis_pixmap(QT_Drag *drag, QT_Pixmap *pixmap)
{
    VERS_QT(drag);
    VERS_QT(pixmap);
    if (qpixmap) {
        qdrag->setPixmap(*qpixmap);
    }
}

QT_DropAction QT_drag_exec(QT_Drag *drag)
{
    VERS_QT(drag);
    return convertis_drop_action(qdrag->exec());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ByteArray
 * \{ */

void QT_byte_array_detruit(QT_ByteArray *array)
{
    if (array) {
        delete[] array->donnees;
        array->donnees = nullptr;
        array->taille_donnees = 0;
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_MimeData
 * \{ */

QT_MimeData *QT_cree_mimedata()
{
    return vers_ipa(new QMimeData);
}

void QT_mimedata_definis_donnee(QT_MimeData *mimedata,
                                QT_Chaine mimetype,
                                uint8_t *donnees,
                                uint64_t taille_donnees)
{
    VERS_QT(mimedata);
    VERS_QT(mimetype);
    qmimedata->setData(qmimetype,
                       QByteArray(reinterpret_cast<const char *>(donnees), int(taille_donnees)));
}

QT_ByteArray QT_mimedata_donne_donnee(QT_MimeData *mimedata, QT_Chaine mimetype)
{
    VERS_QT(mimedata);
    VERS_QT(mimetype);

    auto array = qmimedata->data(qmimetype);
    auto taille = uint64_t(array.size());

    QT_ByteArray résultat;
    résultat.donnees = new uint8_t[taille];
    résultat.taille_donnees = taille;
    memcpy(résultat.donnees, array.data(), taille);
    return résultat;
}

bool QT_mimedata_a_format(QT_MimeData *mimedata, QT_Chaine mimetype)
{
    VERS_QT(mimedata);
    VERS_QT(mimetype);
    return qmimedata->hasFormat(qmimetype);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Clipboard
 * \{ */

QT_MimeData *QT_clipboard_donne_mimedata(QT_Clipboard *clipboard)
{
    VERS_QT(clipboard);
    return vers_ipa(const_cast<QMimeData *>(qclipboard->mimeData()));
}

void QT_clipboard_definis_mimedata(QT_Clipboard *clipboard, QT_MimeData *mimedata)
{
    VERS_QT(clipboard);
    VERS_QT(mimedata);
    qclipboard->setMimeData(qmimedata);
}

void QT_clipboard_efface(QT_Clipboard *clipboard)
{
    VERS_QT(clipboard);
    qclipboard->clear();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Keyboard_Modifier
 * \{ */

static Qt::KeyboardModifiers convertis_modificateurs_clavier(QT_Keyboard_Modifier drapeaux)
{
    uint résultat = Qt::KeyboardModifier::NoModifier;
    ENUMERE_MODIFICATEURS_CLAVIER(ENUMERE_TRANSLATION_ENUM_DRAPEAU_IPA_VERS_QT);
    return Qt::KeyboardModifiers(résultat);
}

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
/** \name QT_Key
 * \{ */

static Qt::Key convertis_touche_clavier(QT_Key key)
{
    switch (key) {
        ENUMERE_CLE_CLAVIER(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return Qt::Key_unknown;
}

static QT_Key qt_key_vers_ipa(Qt::Key key)
{
    switch (key) {
        ENUMERE_CLE_CLAVIER(ENUMERE_TRANSLATION_ENUM_QT_VERS_IPA)
    }
    return QT_KEY_unknown;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Shortcut
 * \{ */

static Qt::ShortcutContext convertis_shortcut_context(QT_Shortcut_Context context)
{
    switch (context) {
        ENUMERE_SHORTCUT_CONTEXTS(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return Qt::WindowShortcut;
}

QT_Shortcut *QT_shortcut_cree(QT_Generic_Widget parent)
{
    VERS_QT(parent);
    return vers_ipa(new QShortcut(qparent));
}

void QT_shortcut_definis_touches(QT_Shortcut *shortcut, QT_Keyboard_Modifier mod, QT_Key key)
{
    VERS_QT(shortcut);
    auto qmod = convertis_modificateurs_clavier(mod);
    auto qkey = convertis_touche_clavier(key);
    qshortcut->setKey(int(uint(qmod) | uint(qkey)));
}

void QT_shortcut_sur_activation(QT_Shortcut *shortcut, QT_Rappel_Generique *rappel)
{
    if (!rappel || !rappel->sur_rappel) {
        return;
    }

    VERS_QT(shortcut);
    QObject::connect(qshortcut, &QShortcut::activated, [=]() { rappel->sur_rappel(rappel); });
}

void QT_shortcut_definis_contexte(struct QT_Shortcut *shortcut, enum QT_Shortcut_Context context)
{
    VERS_QT(shortcut);
    qshortcut->setContext(convertis_shortcut_context(context));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Thread
 * \{ */

class Thread final : public QThread {
    QT_Rappels_Thread *m_rappels;

  public:
    Thread(QT_Rappels_Thread *rappels, QObject *parent = nullptr)
        : QThread(parent), m_rappels(rappels)
    {
    }

    EMPECHE_COPIE(Thread);

    ~Thread() override
    {
        if (m_rappels && m_rappels->sur_destruction) {
            m_rappels->sur_destruction(m_rappels);
        }
    }

    void run() override
    {
        if (m_rappels && m_rappels->sur_lancement_thread) {
            m_rappels->sur_lancement_thread(m_rappels);
        }

        QThread::run();
    }
};

QT_Thread *QT_thread_courant()
{
    return vers_ipa(QThread::currentThread());
}

QT_Thread *QT_thread_cree(struct QT_Rappels_Thread *rappels)
{
    auto résultat = new Thread(rappels);
    return vers_ipa(résultat);
}

void QT_thread_start(QT_Thread *thread)
{
    VERS_QT(thread);
    qthread->start();
}

void QT_thread_quit(QT_Thread *thread)
{
    VERS_QT(thread);
    qthread->quit();
}

void QT_thread_wait(QT_Thread *thread)
{
    VERS_QT(thread);
    qthread->wait();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Mutex
 * \{ */

struct QT_Mutex *QT_mutex_cree()
{
    return vers_ipa(new QMutex());
}

void QT_mutex_detruit(struct QT_Mutex *mutex)
{
    VERS_QT(mutex);
    delete (qmutex);
}

void QT_mutex_lock(struct QT_Mutex *mutex)
{
    VERS_QT(mutex);
    qmutex->lock();
}

void QT_mutex_unlock(struct QT_Mutex *mutex)
{
    VERS_QT(mutex);
    qmutex->unlock();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_WaitCondition
 * \{ */

struct QT_WaitCondition *QT_condition_cree()
{
    return vers_ipa(new QWaitCondition());
}

void QT_condition_detruit(struct QT_WaitCondition *cond)
{
    VERS_QT(cond);
    delete (qcond);
}

void QT_condition_wait(struct QT_WaitCondition *cond, struct QT_Mutex *mutex)
{
    VERS_QT(cond);
    VERS_QT(mutex);
    qcond->wait(qmutex);
}

void QT_condition_notify_one(struct QT_WaitCondition *cond)
{
    VERS_QT(cond);
    qcond->notify_one();
}

void QT_condition_notify_all(struct QT_WaitCondition *cond)
{
    VERS_QT(cond);
    qcond->notify_all();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Window
 * \{ */

static QSurface::SurfaceType convertis_surface_type(QT_Surface_Type surface_type)
{
    switch (surface_type) {
        ENUMERE_SURFACE_TYPE(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QSurface::RasterSurface;
}

class Window : public QWindow {
    QT_Rappels_Window *m_rappels = nullptr;

  public:
    Window(QT_Rappels_Window *rappels) : m_rappels(rappels)
    {
        if (!m_rappels) {
            return;
        }

        m_rappels->window = vers_ipa(this);

        if (m_rappels->sur_creation) {
            m_rappels->sur_creation(m_rappels);
        }
    }

    EMPECHE_COPIE(Window);

    ~Window() override
    {
        if (m_rappels && m_rappels->sur_destruction) {
            m_rappels->sur_destruction(m_rappels);
        }
    }

    QT_Rappels_Window *donne_rappels() const
    {
        return m_rappels;
    }

    bool event(QEvent *event) override
    {
        if (m_rappels && m_rappels->sur_evenement) {
            QT_Generic_Event generic_event;
            generic_event.event = reinterpret_cast<QT_Evenement *>(event);
            if (m_rappels->sur_evenement(m_rappels, generic_event)) {
                return true;
            }
        }

        return QWindow::event(event);
    }
};

struct QT_Window *QT_window_cree_avec_rappels(struct QT_Rappels_Window *rappels)
{
    auto résultat = new Window(rappels);
    return vers_ipa(résultat);
}

void QT_window_detruit(struct QT_Window *window)
{
    VERS_QT(window);
    delete qwindow;
}

struct QT_Rappels_Window *QT_window_donne_rappels(struct QT_Window *window)
{
    VERS_QT(window);

    if (auto ipa_window = dynamic_cast<Window *>(qwindow)) {
        return ipa_window->donne_rappels();
    }
    return nullptr;
}

void QT_window_request_update(struct QT_Window *window)
{
    CONVERTIS_ET_APPEL(window, requestUpdate);
}

void QT_window_show(struct QT_Window *window)
{
    CONVERTIS_ET_APPEL(window, show);
}

void QT_window_show_maximized(struct QT_Window *window)
{
    CONVERTIS_ET_APPEL(window, showMaximized);
}

void QT_window_show_minimized(struct QT_Window *window)
{
    CONVERTIS_ET_APPEL(window, showMinimized);
}

void QT_window_set_surface_type(struct QT_Window *window, enum QT_Surface_Type surface_type)
{
    auto qsurface_type = convertis_surface_type(surface_type);
    CONVERTIS_ET_APPEL(window, setSurfaceType, qsurface_type);
}

void QT_window_set_title(struct QT_Window *window, struct QT_Chaine title)
{
    CONVERTIS_ET_APPEL(window, setTitle, vers_qt(title));
}

int QT_window_height(struct QT_Window *window)
{
    VERS_QT(window);
    return qwindow->height();
}

int QT_window_width(struct QT_Window *window)
{
    VERS_QT(window);
    return qwindow->width();
}

void QT_window_resize(struct QT_Window *window, int width, int height)
{
    CONVERTIS_ET_APPEL(window, resize, width, height);
}

bool QT_window_is_exposed(struct QT_Window *window)
{
    VERS_QT(window);
    return qwindow->isExposed();
}

void QT_window_set_position(struct QT_Window *window, int x, int y)
{
    VERS_QT(window);
    qwindow->setPosition(x, y);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Event_Loop
 * \{ */

class EventLoop : public QEventLoop {
    QT_Rappels_Event_Loop *m_rappels = nullptr;

  public:
    EventLoop(QT_Rappels_Event_Loop *rappels) : m_rappels(rappels)
    {
        if (!m_rappels) {
            return;
        }

        m_rappels->event_loop = vers_ipa(this);
    }

    EMPECHE_COPIE(EventLoop);

    ~EventLoop() override
    {
        if (m_rappels && m_rappels->sur_destruction) {
            m_rappels->sur_destruction(m_rappels);
        }
    }

    QT_Rappels_Event_Loop *donne_rappels() const
    {
        return m_rappels;
    }

    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (m_rappels && m_rappels->sur_filtre_evenement) {
            QT_Generic_Event generic_event;
            generic_event.event = reinterpret_cast<QT_Evenement *>(event);
            if (m_rappels->sur_filtre_evenement(m_rappels, generic_event)) {
                return true;
            }
        }

        return QEventLoop::eventFilter(obj, event);
    }

    bool event(QEvent *event) override
    {
        if (m_rappels && m_rappels->sur_evenement) {
            QT_Generic_Event generic_event;
            generic_event.event = reinterpret_cast<QT_Evenement *>(event);
            if (m_rappels->sur_evenement(m_rappels, generic_event)) {
                return true;
            }
        }

        return QEventLoop::event(event);
    }
};

struct QT_Event_Loop *QT_Event_Loop_cree_avec_rappels(struct QT_Rappels_Event_Loop *rappels)
{
    auto résultat = new EventLoop(rappels);
    return vers_ipa(résultat);
}

void QT_Event_Loop_detruit(struct QT_Event_Loop *event_loop)
{
    VERS_QT(event_loop);
    delete qevent_loop;
}

struct QT_Rappels_Event_Loop *QT_Event_Loop_donne_rappels(struct QT_Event_Loop *event_loop)
{
    VERS_QT(event_loop);
    if (auto ipa_event_loop = dynamic_cast<EventLoop *>(qevent_loop)) {
        return ipa_event_loop->donne_rappels();
    }
    return nullptr;
}

int QT_Event_Loop_exec(struct QT_Event_Loop *event_loop)
{
    VERS_QT(event_loop);
    return qevent_loop->exec();
}

void QT_Event_Loop_exit(struct QT_Event_Loop *event_loop)
{
    CONVERTIS_ET_APPEL(event_loop, exit);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Surface_Format
 * \{ */

static void copie_vers_ipa(QSurfaceFormat const &qformat, struct QT_Surface_Format *format)
{
    format->alpha_buffer_size = qformat.alphaBufferSize();
    format->blue_buffer_size = qformat.blueBufferSize();
    format->depth_buffer_size = qformat.depthBufferSize();
    format->green_buffer_size = qformat.greenBufferSize();
    format->red_buffer_size = qformat.redBufferSize();
    format->stencil_buffer_size = qformat.stencilBufferSize();

    format->samples = qformat.samples();
    format->swap_interval = qformat.swapInterval();
    format->major_version = qformat.majorVersion();
    format->minor_version = qformat.minorVersion();

    format->options = int(qformat.options());
    format->profile = int(qformat.profile());
    format->renderable_type = int(qformat.renderableType());
    format->swap_behavior = int(qformat.swapBehavior());
}

static void copie_vers_qt(struct QT_Surface_Format *format, QSurfaceFormat &qformat)
{
    qformat.setAlphaBufferSize(format->alpha_buffer_size);
    qformat.setBlueBufferSize(format->blue_buffer_size);
    qformat.setDepthBufferSize(format->depth_buffer_size);
    qformat.setGreenBufferSize(format->green_buffer_size);
    qformat.setRedBufferSize(format->red_buffer_size);
    qformat.setStencilBufferSize(format->stencil_buffer_size);

    qformat.setSamples(format->samples);
    qformat.setSwapInterval(format->swap_interval);
    qformat.setMajorVersion(format->major_version);
    qformat.setMinorVersion(format->minor_version);

    qformat.setOptions(QSurfaceFormat::FormatOptions(format->options));
    qformat.setProfile(QSurfaceFormat::OpenGLContextProfile(format->profile));
    qformat.setRenderableType(QSurfaceFormat::RenderableType(format->renderable_type));
    qformat.setSwapBehavior(QSurfaceFormat::SwapBehavior(format->swap_behavior));
}

void QT_initialize_surface_format(struct QT_Surface_Format *format)
{
    QSurfaceFormat qformat;
    copie_vers_ipa(qformat, format);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_OpenGL_Context
 * \{ */

QT_OpenGL_Context *QT_OpenGL_Context_cree_avec_parent(QT_Generic_Object parent)
{
    VERS_QT(parent);
    auto résultat = new QOpenGLContext(qparent);
    return vers_ipa(résultat);
}

void QT_OpenGL_detruit(QT_OpenGL_Context *context)
{
    VERS_QT(context);
    delete qcontext;
}

void QT_OpenGL_Context_format(QT_OpenGL_Context *context, QT_Surface_Format *format)
{
    VERS_QT(context);
    copie_vers_ipa(qcontext->format(), format);
}

void QT_OpenGL_Context_set_format(QT_OpenGL_Context *context, QT_Surface_Format *format)
{
    VERS_QT(context);
    QSurfaceFormat surface_format;
    copie_vers_qt(format, surface_format);
    qcontext->setFormat(surface_format);
}

bool QT_OpenGL_Context_create(QT_OpenGL_Context *context)
{
    VERS_QT(context);
    return qcontext->create();
}

bool QT_OpenGL_Context_make_current(QT_OpenGL_Context *context, QT_Window *window)
{
    VERS_QT(context);
    VERS_QT(window);
    return qcontext->makeCurrent(qwindow);
}

void QT_OpenGL_Context_donne_current(QT_OpenGL_Context *context)
{
    CONVERTIS_ET_APPEL(context, doneCurrent);
}

void QT_OpenGL_Context_swap_buffers(QT_OpenGL_Context *context, QT_Window *window)
{
    VERS_QT(context);
    VERS_QT(window);
    qcontext->swapBuffers(qwindow);
}

void QT_OpenGL_Context_set_share_context(struct QT_OpenGL_Context *context,
                                         struct QT_OpenGL_Context *share_context)
{
    VERS_QT(context);
    VERS_QT(share_context);
    qcontext->setShareContext(qshare_context);
}

bool QT_OpenGL_Context_are_sharing(struct QT_OpenGL_Context *context1,
                                   struct QT_OpenGL_Context *context2)
{
    VERS_QT(context1);
    VERS_QT(context2);
    return QOpenGLContext::areSharing(qcontext1, qcontext2);
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
    auto résultat = QColor::fromHslF(float(t), float(s), float(l), float(a));
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

static QT_Event_Type convertis_type_évènement(QEvent::Type type)
{
    switch (type) {
        ENUMERE_EVENT_TYPE(ENUMERE_TRANSLATION_ENUM_QT_VERS_IPA);
        default:
        {
            return QT_Event_Type(type);
        }
    }
}

QT_Event_Type QT_evenement_donne_type(QT_Generic_Event evenement)
{
    auto event = vers_qt(evenement);
    return convertis_type_évènement(event->type());
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
    switch (qevent->buttons()) {
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
        r_position->x = int(qevent->position().x());
        r_position->y = int(qevent->position().y());
    }
}

int QT_wheel_event_donne_delta(QT_WheelEvent *event)
{
    auto qevent = vers_qt(event);
    return qevent->angleDelta().y();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_KeyEvent
 * \{ */

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
    return crée_qt_chaine_tampon(qevent->text(), tampon, sizeof(tampon));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ResizeEvent
 * \{ */

void QT_resize_event_donne_vieille_taille(QT_ResizeEvent *event, QT_Taille *r_taille)
{
    VERS_QT(event);
    if (r_taille) {
        r_taille->hauteur = qevent->oldSize().height();
        r_taille->largeur = qevent->oldSize().width();
    }
}

void QT_resize_event_donne_taille(QT_ResizeEvent *event, QT_Taille *r_taille)
{
    VERS_QT(event);
    if (r_taille) {
        r_taille->hauteur = qevent->size().height();
        r_taille->largeur = qevent->size().width();
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ContextMenuEvent
 * \{ */

void QT_context_menu_event_donne_position_globale(QT_ContextMenuEvent *event,
                                                  QT_Position *r_position)
{
    VERS_QT(event);
    if (r_position) {
        r_position->x = qevent->globalPos().x();
        r_position->y = qevent->globalPos().y();
    }
}

void QT_context_menu_event_donne_position(QT_ContextMenuEvent *event, QT_Position *r_position)
{
    VERS_QT(event);
    if (r_position) {
        r_position->x = qevent->pos().x();
        r_position->y = qevent->pos().y();
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_DragEnterEvent
 * \{ */

QT_MimeData *QT_drag_enter_event_donne_mimedata(QT_DragEnterEvent *event)
{
    VERS_QT(event);
    return vers_ipa(const_cast<QMimeData *>(qevent->mimeData()));
}

void QT_drag_enter_event_accepte_action_propose(QT_DragEnterEvent *event)
{
    VERS_QT(event);
    qevent->acceptProposedAction();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_DropEvent
 * \{ */

QT_MimeData *QT_drop_event_donne_mimedata(QT_DropEvent *event)
{
    VERS_QT(event);
    return vers_ipa(const_cast<QMimeData *>(qevent->mimeData()));
}

void QT_drop_event_accepte_action_propose(QT_DropEvent *event)
{
    VERS_QT(event);
    qevent->acceptProposedAction();
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
    QT_Generic_Widget résultat;

    auto parent = qwidget->parent();
    if (auto qwidget_parent = dynamic_cast<QWidget *>(parent)) {
        résultat.widget = vers_ipa(qwidget_parent);
    }
    else if (auto widget_parent = dynamic_cast<Widget *>(parent)) {
        résultat.widget = vers_ipa(widget_parent);
    }
    else {
        résultat.widget = nullptr;
    }

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

void QT_widget_donne_rect(QT_Generic_Widget widget, QT_Rect *r_rect)
{
    if (r_rect == nullptr) {
        return;
    }

    VERS_QT(widget);
    *r_rect = vers_ipa(qwidget->rect());
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

static Qt::FocusPolicy convertis_focus_policy(QT_Focus_Policy policy)
{
    switch (policy) {
        ENUMERE_FOCUS_POLICY(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return Qt::NoFocus;
}

void QT_widget_definis_comportement_focus(QT_Generic_Widget widget, QT_Focus_Policy policy)
{
    VERS_QT(widget);
    qwidget->setFocusPolicy(convertis_focus_policy(policy));
}

void QT_widget_accepte_drop(union QT_Generic_Widget widget, bool ouinon)
{
    VERS_QT(widget);
    qwidget->setAcceptDrops(ouinon);
}

QT_Style *QT_widget_donne_style(union QT_Generic_Widget widget)
{
    VERS_QT(widget);
    return vers_ipa(qwidget->style());
}

QT_Window *QT_widget_donne_window_handle(union QT_Generic_Widget widget)
{
    VERS_QT(widget);
    return vers_ipa(qwidget->windowHandle());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_GLWidget
 * \{ */

QT_OpenGLWidget *QT_cree_glwidget(QT_Rappels_GLWidget *rappels, QT_Generic_Widget parent)
{
    auto qparent = vers_qt(parent);
    auto résultat = vers_ipa(new OpenGLWidget(rappels, qparent));
    if (rappels) {
        rappels->widget = résultat;
    }
    return résultat;
}

QT_Rappels_GLWidget *QT_glwidget_donne_rappels(QT_OpenGLWidget *widget)
{
    VERS_QT(widget);
    if (auto ipa_widget = dynamic_cast<OpenGLWidget *>(qwidget)) {
        return ipa_widget->donne_rappels();
    }
    return nullptr;
}

QT_OpenGL_Context *QT_glwidget_context(QT_OpenGLWidget *widget)
{
    VERS_QT(widget);
    return vers_ipa(qwidget->context());
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

void QT_menu_detruit(QT_Menu *menu)
{
    VERS_QT(menu);
    delete qmenu;
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

void QT_menu_ajoute_section(QT_Menu *menu, QT_Chaine titre)
{
    VERS_QT(menu);
    VERS_QT(titre);
    qmenu->addSection(qtitre);
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
    qlayout->setContentsMargins(taille, taille, taille, taille);
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

    if (auto qcombo = dynamic_cast<ComboBox *>(vers_qt(combo))) {
        QObject::connect(
            qcombo, &ComboBox::index_courant_modifie, [=]() { rappel->sur_rappel(rappel); });
    }
}

QT_Chaine QT_combobox_donne_valeur_courante_chaine(QT_ComboBox *combo)
{
    VERS_QT(combo);
    static char tampon[FILENAME_MAX];
    return crée_qt_chaine_tampon(qcombo->currentData().toString(), tampon, sizeof(tampon));
}

void QT_combobox_definis_modifiable(QT_ComboBox *combo, bool ouinon)
{
    VERS_QT(combo);
    qcombo->setEditable(ouinon);
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

void QT_tab_widget_remplace_widget_page_courante(QT_TabWidget *tab_widget,
                                                 QT_Generic_Widget widget)
{
    VERS_QT(tab_widget);
    VERS_QT(widget);

    auto index_courant = qtab_widget->currentIndex();

    /* Sauvegarde les données. */
    auto texte = qtab_widget->tabText(index_courant);
    auto icon = qtab_widget->tabIcon(index_courant);
    auto infobulle = qtab_widget->tabToolTip(index_courant);

    qtab_widget->removeTab(index_courant);

    auto nouvel_index = qtab_widget->insertTab(index_courant, qwidget, icon, texte);
    qtab_widget->setTabToolTip(nouvel_index, infobulle);

    qtab_widget->setCurrentIndex(nouvel_index);
}

QT_Widget *QT_tab_widget_donne_widget_courant(QT_TabWidget *tab_widget)
{
    VERS_QT(tab_widget);
    return reinterpret_cast<QT_Widget *>(qtab_widget->currentWidget());
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

QT_ScrollBar *QT_scroll_area_donne_barre_horizontale(QT_ScrollArea *scroll_area)
{
    auto qscroll = vers_qt(scroll_area);
    return vers_ipa(qscroll->horizontalScrollBar());
}

QT_ScrollBar *QT_scroll_area_donne_barre_verticale(QT_ScrollArea *scroll_area)
{
    auto qscroll = vers_qt(scroll_area);
    return vers_ipa(qscroll->verticalScrollBar());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_ScrollBar
 * \{ */

void QT_scroll_bar_definis_plage(QT_ScrollBar *bar, int min, int max)
{
    VERS_QT(bar);
    qbar->setRange(min, max);
}

void QT_scroll_bar_definis_valeur(QT_ScrollBar *bar, int valeur)
{
    VERS_QT(bar);
    qbar->setValue(valeur);
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

void QT_checkbox_definis_texte(QT_CheckBox *checkbox, QT_Chaine texte)
{
    VERS_QT(checkbox);
    VERS_QT(texte);
    qcheckbox->setText(qtexte);
}

void QT_checkbox_definis_coche(QT_CheckBox *checkbox, int coche)
{
    auto qcheckbox = vers_qt(checkbox);
    qcheckbox->setChecked(bool(coche));
}

bool QT_checkbox_est_coche(QT_CheckBox *checkbox)
{
    auto qcheckbox = vers_qt(checkbox);
    return qcheckbox->isChecked();
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
    return crée_qt_chaine_tampon(qline_edit->text(), tampon, sizeof(tampon));
}

void QT_line_edit_definis_lecture_seule(QT_LineEdit *line_edit, bool ouinon)
{
    VERS_QT(line_edit);
    qline_edit->setReadOnly(ouinon);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QToolButton
 * \{ */

static Qt::ToolButtonStyle convertis_tool_button_style(QT_Tool_Button_Style style)
{
    switch (style) {
        ENUMERE_TOOL_BUTTON_STYLE(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return Qt::ToolButtonIconOnly;
}

QT_ToolButton *QT_cree_tool_button(QT_Generic_Widget parent)
{
    VERS_QT(parent);
    return vers_ipa(new QToolButton(qparent));
}

QT_ToolButton *QT_cree_tool_button_rappels(QT_Rappels_ToolButton *rappels,
                                           QT_Generic_Widget parent)
{
    VERS_QT(parent);
    auto résultat = vers_ipa(new ToolButton(rappels, qparent));
    if (rappels) {
        rappels->widget = résultat;
    }
    return résultat;
}

void QT_tool_button_definis_action_defaut(QT_ToolButton *tool_button, QT_Action *action)
{
    VERS_QT(tool_button);
    VERS_QT(action);
    qtool_button->setDefaultAction(qaction);
}

void QT_tool_button_definis_style(QT_ToolButton *tool_button, QT_Tool_Button_Style style)
{
    VERS_QT(tool_button);
    qtool_button->setToolButtonStyle(convertis_tool_button_style(style));
}

void QT_tool_button_definis_taille_icone(QT_ToolButton *tool_button, QT_Taille *taille)
{
    VERS_QT(tool_button);
    qtool_button->setIconSize(QSize(taille->largeur, taille->hauteur));
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

void QT_push_button_definis_icone(QT_PushButton *button, QT_Icon *icon)
{
    VERS_QT(button);
    VERS_QT(icon);
    qbutton->setIcon(*qicon);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_StandardButton
 * \{ */

static QMessageBox::StandardButtons standard_buttons_vers_qt(QT_StandardButton drapeaux)
{
    int résultat = QMessageBox::StandardButton::NoButton;
    ENUMERE_BOUTON_STANDARD(ENUMERE_TRANSLATION_ENUM_DRAPEAU_IPA_VERS_QT)
    return QMessageBox::StandardButton(résultat);
}

static QDialogButtonBox::StandardButtons standard_buttons_vers_message_box(
    QT_StandardButton drapeaux)
{
    auto résultat = standard_buttons_vers_qt(drapeaux).toInt();
    return QDialogButtonBox::StandardButtons::fromInt(uint32_t(résultat));
}

static QT_StandardButton standard_buttons_vers_ipa(QMessageBox::StandardButtons drapeaux)
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
    auto flag = standard_buttons_vers_message_box(button).toInt();
    auto résultat = qbox->addButton(QDialogButtonBox::StandardButton(flag));
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

    static char tampon[FILENAME_MAX];
    return crée_qt_chaine_tampon(chemin, tampon, sizeof(tampon));
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

    static char tampon[FILENAME_MAX];
    return crée_qt_chaine_tampon(chemin, tampon, sizeof(tampon));
}

QT_Chaine QT_file_dialog_donne_dossier_existant(QT_Generic_Widget parent,
                                                QT_Chaine titre,
                                                QT_Chaine dossier)
{
    auto qparent = vers_qt(parent);
    auto qtitre = titre.vers_std_string();
    auto qdossier = dossier.vers_std_string();

    auto chemin = QFileDialog::getExistingDirectory(qparent, qtitre.c_str(), qdossier.c_str());

    static char tampon[FILENAME_MAX];
    return crée_qt_chaine_tampon(chemin, tampon, sizeof(tampon));
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
    auto qboutons = standard_buttons_vers_qt(boutons);

    auto résultat = QMessageBox::warning(qparent, qtitre.c_str(), qmessage.c_str(), qboutons);
    return standard_buttons_vers_ipa(résultat);
}

QT_StandardButton QT_message_box_affiche_erreur(QT_Generic_Widget parent,
                                                QT_Chaine titre,
                                                QT_Chaine message,
                                                QT_StandardButton boutons)
{
    auto qparent = vers_qt(parent);
    auto qtitre = titre.vers_std_string();
    auto qmessage = message.vers_std_string();
    auto qboutons = standard_buttons_vers_qt(boutons);

    auto résultat = QMessageBox::critical(qparent, qtitre.c_str(), qmessage.c_str(), qboutons);
    return standard_buttons_vers_ipa(résultat);
}

QT_StandardButton QT_message_box_affiche_question(QT_Generic_Widget parent,
                                                  QT_Chaine titre,
                                                  QT_Chaine message,
                                                  QT_StandardButton boutons)
{
    auto qparent = vers_qt(parent);
    auto qtitre = titre.vers_std_string();
    auto qmessage = message.vers_std_string();
    auto qboutons = standard_buttons_vers_qt(boutons);

    auto résultat = QMessageBox::question(qparent, qtitre.c_str(), qmessage.c_str(), qboutons);
    return standard_buttons_vers_ipa(résultat);
}

QT_StandardButton QT_message_box_affiche_information(QT_Generic_Widget parent,
                                                     QT_Chaine titre,
                                                     QT_Chaine message,
                                                     QT_StandardButton boutons)
{
    auto qparent = vers_qt(parent);
    auto qtitre = titre.vers_std_string();
    auto qmessage = message.vers_std_string();
    auto qboutons = standard_buttons_vers_qt(boutons);

    auto résultat = QMessageBox::information(qparent, qtitre.c_str(), qmessage.c_str(), qboutons);
    return standard_buttons_vers_ipa(résultat);
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
    if (auto ipa_widget = dynamic_cast<TreeWidgetItem *>(qwidget)) {
        return ipa_widget->donne_données();
    }
    return nullptr;
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

QT_Chaine QT_treewidgetitem_donne_texte(QT_TreeWidgetItem *widget, int colonne)
{
    VERS_QT(widget);
    static char tampon[FILENAME_MAX];
    return crée_qt_chaine_tampon(qwidget->text(colonne), tampon, sizeof(tampon));
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

void QT_treewidgetitem_set_expanded(QT_TreeWidgetItem *widget, bool ouinon)
{
    auto qwidget = vers_qt(widget);
    qwidget->setExpanded(ouinon);
}

QT_TreeWidgetItem *QT_treewidgetitem_parent(QT_TreeWidgetItem *widget)
{
    VERS_QT(widget);
    return vers_ipa(qwidget->parent());
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

void QT_treewidget_set_current_item(QT_TreeWidget *tree_widget, QT_TreeWidgetItem *item)
{
    auto qtree_widget = vers_qt(tree_widget);
    VERS_QT(item);
    qtree_widget->setCurrentItem(qitem);
}

QT_TreeWidgetItem *QT_treewidget_current_item(QT_TreeWidget *tree_widget)
{
    auto qtree_widget = vers_qt(tree_widget);
    return vers_ipa(qtree_widget->currentItem());
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
/** \name QT_GraphicsEllipseItem
 * \{ */

QT_GraphicsEllipseItem *QT_cree_graphics_ellipse_item(QT_Generic_GraphicsItem parent)
{
    auto qparent = vers_qt(parent);
    return vers_ipa(new QGraphicsEllipseItem(qparent));
}

void QT_graphics_ellipse_item_definis_pinceau(QT_GraphicsEllipseItem *item, QT_Pen *pinceau)
{
    auto qitem = vers_qt(item);
    auto qpen = vers_qt(*pinceau);
    qitem->setPen(qpen);
}

void QT_graphics_ellipse_item_definis_brosse(QT_GraphicsEllipseItem *item, QT_Brush *brush)
{
    auto qitem = vers_qt(item);
    auto qbrush = vers_qt(*brush);
    qitem->setBrush(qbrush);
}

void QT_graphics_ellipse_item_definis_rect(QT_GraphicsEllipseItem *item, QT_RectF *rect)
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
/** \name QT_IODevice
 * https://doc.qt.io/qt-5/qiodevice.html
 * \{ */

class IODevice : public QIODevice {
    QT_Rappels_IODevice *m_rappels = nullptr;

  public:
    IODevice(QT_Rappels_IODevice *rappels, QObject *parent) : QIODevice(parent), m_rappels(rappels)
    {
        m_rappels->iodevice = vers_ipa(this);
    }

    EMPECHE_COPIE(IODevice);

    qint64 readData(char *data, qint64 maxlen) override
    {
        return m_rappels->read_data(m_rappels, data, maxlen);
    }

    qint64 writeData(const char *data, qint64 len) override
    {
        return m_rappels->write_data(m_rappels, data, len);
    }
};

QT_IODevice *QT_iodevice_cree_avec_rappels(QT_Rappels_IODevice *rappels, QT_Generic_Object parent)
{
    VERS_QT(parent);
    auto résultat = new IODevice(rappels, qparent);
    return vers_ipa(résultat);
}

void QT_iodevice_ready_read(QT_IODevice *iodevice)
{
    VERS_QT(iodevice);
    Q_EMIT(qiodevice->readyRead());
}

static QIODeviceBase::OpenMode convertis_open_mode(QT_Device_Open_Mode drapeaux)
{
    int résultat = 0;
    ENUMERE_DEVICE_OPEN_MODE(ENUMERE_TRANSLATION_ENUM_DRAPEAU_IPA_VERS_QT);
    return QIODeviceBase::OpenMode(résultat);
}

void QT_iodevice_open(QT_IODevice *iodevice, QT_Device_Open_Mode open_mode)
{
    VERS_QT(iodevice);
    qiodevice->open(convertis_open_mode(open_mode));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_LocalServer
 * \{ */

class LocalServer : public QLocalServer {
    QT_Rappels_LocalServer *m_rappels = nullptr;

  public:
    LocalServer(QT_Rappels_LocalServer *rappels, QObject *parent)
        : QLocalServer(parent), m_rappels(rappels)
    {
        m_rappels->server = vers_ipa(this);

        QObject::connect(this, &QLocalServer::newConnection, [&] {
            auto connection = nextPendingConnection();
            if (m_rappels->sur_connexion) {
                m_rappels->sur_connexion(m_rappels, vers_ipa(connection));
            }

            QObject::connect(connection, &QLocalSocket::readyRead, [this, connection] {
                if (m_rappels->sur_lecture) {
                    m_rappels->sur_lecture(m_rappels, vers_ipa(connection));
                }
            });
        });
    }

    EMPECHE_COPIE(LocalServer);
};

struct QT_LocalServer *QT_local_server_cree(struct QT_Rappels_LocalServer *rappels,
                                            union QT_Generic_Object parent)
{
    VERS_QT(parent);
    auto résultat = new LocalServer(rappels, qparent);
    return vers_ipa(résultat);
}

bool QT_local_server_listen(struct QT_LocalServer *server, struct QT_Chaine nom)
{
    VERS_QT(server);
    return qserver->listen(QString(nom.vers_std_string().c_str()));
}

void QT_local_server_close(struct QT_LocalServer *server)
{
    CONVERTIS_ET_APPEL(server, close);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_LocalSocket
 * \{ */

int64_t QT_local_socket_read(struct QT_LocalSocket *socket, char *data, int64_t maxlen)
{
    VERS_QT(socket);
    return qsocket->read(data, maxlen);
}

bool QT_local_socket_is_valid(struct QT_LocalSocket *socket)
{
    VERS_QT(socket);
    return qsocket->isValid();
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

    static QT_Chaine sur_donne_chaine(QT_Variant *variant)
    {
        auto enveloppe = static_cast<EnveloppeVariant *>(variant);
        auto string = enveloppe->m_variant.toString();
        return crée_qt_chaine(string);
    }

    static bool sur_est_chaine(QT_Variant *variant)
    {
        auto enveloppe = static_cast<EnveloppeVariant *>(variant);
        return enveloppe->m_variant.typeId() == QMetaType::QString;
    }

    static void sur_définis_brosse(QT_Variant *variant, QT_Brush *brosse)
    {
        if (!brosse) {
            return;
        }

        auto enveloppe = static_cast<EnveloppeVariant *>(variant);
        enveloppe->m_variant = vers_qt(*brosse);
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
    EnveloppeVariant() : EnveloppeVariant(QVariant{})
    {
    }

    EnveloppeVariant(const QVariant &variant) : m_variant(variant)
    {
        definis_chaine = sur_définis_chaine;
        donne_chaine = sur_donne_chaine;
        est_chaine = sur_est_chaine;
        definis_brosse = sur_définis_brosse;
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
/** \name QT_Item_Flags
 * \{ */

static QT_Item_Flags item_flags_vers_ipa(Qt::ItemFlags drapeaux)
{
    int résultat = 0;
    ENUMERE_ITEM_FLAGS(ENUMERE_TRANSLATION_ENUM_DRAPEAU_QT_VERS_IPA);
    return QT_Item_Flags(résultat);
}

static Qt::ItemFlags item_flags_vers_qt(QT_Item_Flags drapeaux)
{
    int résultat = 0;
    ENUMERE_ITEM_FLAGS(ENUMERE_TRANSLATION_ENUM_DRAPEAU_IPA_VERS_QT);
    return Qt::ItemFlags(résultat);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name QT_Item_Edit_Triggers
 * \{ */

static QT_Item_Edit_Triggers item_edit_triggers_vers_ipa(QAbstractItemView::EditTriggers drapeaux)
{
    int résultat = 0;
    ENUMERE_ITEM_EDIT_TRIGGERS(ENUMERE_TRANSLATION_ENUM_DRAPEAU_QT_VERS_IPA);
    return QT_Item_Edit_Triggers(résultat);
}

static QAbstractItemView::EditTriggers item_edit_triggers_vers_qt(QT_Item_Edit_Triggers drapeaux)
{
    int résultat = 0;
    ENUMERE_ITEM_EDIT_TRIGGERS(ENUMERE_TRANSLATION_ENUM_DRAPEAU_IPA_VERS_QT);
    return QAbstractItemView::EditTriggers(résultat);
}

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

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (m_rappels && m_rappels->donne_flags) {
            auto model = vers_ipa(index);
            auto résultat = m_rappels->donne_flags(
                m_rappels, &model, item_flags_vers_ipa(QAbstractTableModel::flags(index)));
            return item_flags_vers_qt(résultat);
        }
        return QAbstractTableModel::flags(index);
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

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (m_rappels && m_rappels->definis_donnee_cellule) {
            auto enveloppe_variant = EnveloppeVariant(value);
            auto model = vers_ipa(index);
            return m_rappels->definis_donnee_cellule(
                m_rappels, &model, convertis_role(Qt::ItemDataRole(role)), &enveloppe_variant);
        }
        return QAbstractTableModel::setData(index, value, role);
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
    qsfpm->setFilterRegularExpression(qregex);
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

void QT_table_view_definis_edit_triggers(QT_TableView *view, QT_Item_Edit_Triggers edit_triggers)
{
    VERS_QT(view);
    qview->setEditTriggers(item_edit_triggers_vers_qt(edit_triggers));
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
    *résultat = crée_qt_chaine_tampon(qcursor->selectedText(), tampon, sizeof(tampon));
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

static QPlainTextEdit::LineWrapMode convertis_line_wrap_mode(QT_Line_Wrap_Mode mode)
{
    switch (mode) {
        ENUMERE_MODE_RETOUR_LIGNE(ENUMERE_TRANSLATION_ENUM_IPA_VERS_QT)
    }
    return QPlainTextEdit::LineWrapMode::WidgetWidth;
}

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
    return crée_qt_chaine(qtext_edit->toPlainText());
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

void QT_plain_text_edit_definis_lecture_seule(QT_PlainTextEdit *text_edit, bool ouinon)
{
    VERS_QT(text_edit);
    qtext_edit->setReadOnly(ouinon);
}

void QT_plain_text_edit_definis_mode_retour_ligne(QT_PlainTextEdit *text_edit,
                                                  QT_Line_Wrap_Mode mode)
{
    VERS_QT(text_edit);
    qtext_edit->setLineWrapMode(convertis_line_wrap_mode(mode));
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

void QT_doublespinbox_definis_pas(QT_DoubleSpinBox *doublespinbox, double valeur)
{
    VERS_QT(doublespinbox);
    qdoublespinbox->setSingleStep(valeur);
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
/** \name QT_AbstractSocket
 * \{ */

static QT_Socket_Error convertis_socket_error(QAbstractSocket::SocketError error)
{
    switch (error) {
        ENUMERE_SOCKET_ERROR(ENUMERE_TRANSLATION_ENUM_QT_VERS_IPA)
    }
    return QT_Socket_Error::QT_SOCKET_ERROR_UnknownSocketError;
}

static void connecte_rappels_socket(QTcpSocket *socket, QT_Rappels_Socket *rappels)
{
    if (!rappels) {
        return;
    }

    rappels->socket.tcp = vers_ipa(socket);
    if (rappels->sur_connexion) {
        QObject::connect(
            socket, &QTcpSocket::connected, [=]() { rappels->sur_connexion(rappels); });
    }
    if (rappels->sur_deconnexion) {
        QObject::connect(
            socket, &QTcpSocket::disconnected, [=]() { rappels->sur_deconnexion(rappels); });
    }
    if (rappels->sur_erreur) {
        QObject::connect(socket,
                         qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::errorOccurred),
                         [=](QAbstractSocket::SocketError error) {
                             rappels->sur_erreur(rappels, convertis_socket_error(error));
                         });
    }
    if (rappels->sur_resolution_hote) {
        QObject::connect(
            socket, &QTcpSocket::hostFound, [=]() { rappels->sur_resolution_hote(rappels); });
    }
    if (rappels->sur_pret_a_lire) {
        QObject::connect(
            socket, &QTcpSocket::readyRead, [=]() { rappels->sur_pret_a_lire(rappels); });
    }
}

QT_TcpSocket *QT_cree_tcp_socket_rappels(QT_Generic_Object parent, QT_Rappels_Socket *rappels)
{
    VERS_QT(parent);
    auto résultat = new QTcpSocket(qparent);
    connecte_rappels_socket(résultat, rappels);
    return vers_ipa(résultat);
}

QT_SslSocket *QT_cree_ssl_socket_rappels(QT_Generic_Object parent, QT_Rappels_Socket *rappels)
{
    VERS_QT(parent);
    auto résultat = new QSslSocket(qparent);
    connecte_rappels_socket(résultat, rappels);
    return vers_ipa(résultat);
}

void QT_abstract_socket_connect_to_host(QT_AbstractSocket socket,
                                        QT_Chaine host_name,
                                        uint16_t port)
{
    VERS_QT(socket);
    VERS_QT(host_name);
    qsocket->connectToHost(qhost_name, port);
}

void QT_ssl_socket_connect_to_host_encrypted(QT_SslSocket *socket,
                                             QT_Chaine host_name,
                                             uint16_t port)
{
    VERS_QT(socket);
    VERS_QT(host_name);
    qsocket->connectToHostEncrypted(qhost_name, port);
}

void QT_abstract_socket_close(QT_AbstractSocket socket)
{
    VERS_QT(socket);
    qsocket->close();
}

int64_t QT_abstract_socket_read(QT_AbstractSocket socket, int8_t *donnees, int64_t max)
{
    VERS_QT(socket);
    return qsocket->read(reinterpret_cast<char *>(donnees), max);
}

int64_t QT_abstract_socket_write(QT_AbstractSocket socket, int8_t *donnees, int64_t max)
{
    VERS_QT(socket);
    return qsocket->write(reinterpret_cast<char *>(donnees), max);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Pilote_Clic
 * \{ */

DNJ_Pilote_Clic *DNJ_cree_pilote_clic(DNJ_Rappels_Pilote_Clic *rappels)
{
    auto résultat = new PiloteClic(rappels);
    return reinterpret_cast<DNJ_Pilote_Clic *>(résultat);
}

void DNJ_detruit_pilote_clic(DNJ_Pilote_Clic *pilote)
{
    auto qpilote = reinterpret_cast<PiloteClic *>(pilote);
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

void DNJ_conteneur_cree_interface(DNJ_Conteneur_Controles *conteneur)
{
    auto qconteneur = vers_qt(conteneur);
    qconteneur->crée_interface();
}

void DNJ_conteneur_ajourne_controles(DNJ_Conteneur_Controles *conteneur)
{
    auto qconteneur = vers_qt(conteneur);
    qconteneur->ajourne_controles();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DNJ_Gestionnaire_Interface
 * \{ */

static danjo::DonneesInterface convertis_contexte(DNJ_Contexte_Interface *context)
{
    auto résultat = danjo::DonneesInterface();
    résultat.repondant_bouton = reinterpret_cast<PiloteClic *>(context->pilote_clic);
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
        donnée.repondant_bouton = reinterpret_cast<PiloteClic *>(action.pilote_clic);

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
