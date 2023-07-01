/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "fenetre_principale.h"

#include "danjo/danjo.h"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QCloseEvent>
#include <QCoreApplication>
#include <QCursor>
#include <QDockWidget>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/fichier.hh"
#include "biblinternes/patrons_conception/repondant_commande.h"

#include "coeur/jorjala.hh"

#include "barre_progres.hh"
#include "chef_execution.hh"
#include "gestion_entreface.hh"
#include "tache.h"
#include "vue_region.hh"

/* ------------------------------------------------------------------------- */

/* Sous-classe de QEvent pour ajouter les évnènements de Jorjala à la boucle
 * d'évènements de Qt. */
class EvenementJorjala : public QEvent {
    JJL::TypeEvenement m_type;

  public:
    static QEvent::Type id_type_qt;

    EvenementJorjala(JJL::TypeEvenement type_evenenemt_jorjala)
        : QEvent(id_type_qt), m_type(type_evenenemt_jorjala)
    {
    }

    JJL::TypeEvenement pour_quoi() const
    {
        return m_type;
    }
};

QEvent::Type EvenementJorjala::id_type_qt;

/* ------------------------------------------------------------------------- */

static Qt::CursorShape convertis_type_curseur(JJL::TypeCurseur curseur)
{
    switch (curseur) {
        case JJL::TypeCurseur::NORMAL:
            return Qt::CursorShape::ArrowCursor;
        case JJL::TypeCurseur::ATTENTE_BLOQUÉ:
            return Qt::CursorShape::WaitCursor;
        case JJL::TypeCurseur::TÂCHE_ARRIÈRE_PLAN_EN_COURS:
            return Qt::CursorShape::BusyCursor;
        case JJL::TypeCurseur::MAIN_OUVERTE:
            return Qt::CursorShape::OpenHandCursor;
        case JJL::TypeCurseur::MAIN_FERMÉE:
            return Qt::CursorShape::ClosedHandCursor;
    }

    return Qt::CursorShape::ArrowCursor;
}

class GestionnaireInterface final : public JJL::GestionnaireFenetre {
    FenetrePrincipale &m_fenêtre_principale;
    TaskNotifier *m_task_notifier = nullptr;

  public:
    GestionnaireInterface(FenetrePrincipale &fenêtre_principale)
        : JJL::GestionnaireFenetre(), m_fenêtre_principale(fenêtre_principale),
          m_task_notifier(memoire::loge<TaskNotifier>("TaskNotifier", &m_fenêtre_principale))
    {
    }

    GestionnaireInterface(GestionnaireInterface const &) = delete;
    GestionnaireInterface &operator=(GestionnaireInterface const &) = delete;

    void notifie_observatrices(JJL::TypeEvenement evenement) override
    {
        auto event = new EvenementJorjala(evenement);
        QCoreApplication::postEvent(&m_fenêtre_principale, event);
    }

    void notifie_erreur(JJL::Chaine message) override
    {
        QMessageBox boite_message;
        boite_message.critical(&m_fenêtre_principale, "Erreur", message.vers_std_string().c_str());
        boite_message.setFixedSize(500, 200);
    }

    void change_curseur(JJL::TypeCurseur curseur) override
    {
        QGuiApplication::setOverrideCursor(QCursor(convertis_type_curseur(curseur)));
    }

    void restaure_curseur() override
    {
        QGuiApplication::restoreOverrideCursor();
    }

    void définit_titre_application(JJL::Chaine titre) override
    {
        m_fenêtre_principale.setWindowTitle(titre.vers_std_string().c_str());
    }

    void définit_texte_état_logiciel(JJL::Chaine texte) override
    {
        m_fenêtre_principale.définit_texte_état(texte.vers_std_string().c_str());
    }

    void notifie_tâche_démarrée() override
    {
        m_task_notifier->signale_debut_evaluation("", 0, 0);
    }

    void notifie_tâche_terminée() override
    {
        m_task_notifier->signale_fin_tache();
    }

    bool demande_permission_avant_de_fermer() override
    {
        return m_fenêtre_principale.demande_permission_avant_de_fermer();
    }

    TaskNotifier *donne_task_notifier()
    {
        return m_task_notifier;
    }
};

static void initialise_evenements(JJL::Jorjala &jorjala, FenetrePrincipale *fenetre_principale)
{
    EvenementJorjala::id_type_qt = static_cast<QEvent::Type>(QEvent::registerEventType());

    /* À FAIRE : libère la mémoire. */
    auto gestionnaire_fenêtre = memoire::loge<GestionnaireInterface>("GestionnaireInterface",
                                                                     *fenetre_principale);
    jorjala.définit_gestionnaire_fenêtre(gestionnaire_fenêtre);

    auto données_programme = accède_données_programme(jorjala);
    données_programme->task_notifier = gestionnaire_fenêtre->donne_task_notifier();
    données_programme->fenetre_principale = fenetre_principale;
    données_programme->gestionnaire_danjo->parent_dialogue(fenetre_principale);
}

static void initialise_chef_execution(JJL::Jorjala &jorjala, FenetrePrincipale *fenetre_principale)
{
    auto données_programme = accède_données_programme(jorjala);

    /* À FAIRE : libère la mémoire. */
    auto chef = memoire::loge<ChefExecution>(
        "ChefExecution", jorjala, données_programme->task_notifier);

    jorjala.définit_chef_exécution(chef);
}

/* ------------------------------------------------------------------------- */

static const char *chemins_scripts[] = {
    "entreface/menu_fichier.jo",
    "entreface/menu_edition.jo",
    "entreface/menu_ajouter_noeud_composite.jo",
    "entreface/menu_ajouter_noeud_detail.jo",
    "entreface/menu_ajouter_noeud_objet.jo",
    "entreface/menu_ajouter_noeud_simulation.jo",
    "entreface/menu_debogage.jo",
};

FenetrePrincipale::FenetrePrincipale(JJL::Jorjala &jorjala, QWidget *parent)
    : QMainWindow(parent), m_jorjala(jorjala),
      m_barre_progres(new BarreDeProgres(m_jorjala, this)), m_texte_état(new QLabel("", this))
{
    initialise_evenements(m_jorjala, this);
    initialise_chef_execution(m_jorjala, this);
    initialise_fournisseuse_icône(m_jorjala);

    setWindowTitle("Projet Sans Titre - Jorjala");

    genere_barre_menu();
    genere_menu_prereglages();

    statusBar()->addWidget(m_texte_état);
    définit_texte_état("");

    statusBar()->addWidget(m_barre_progres);
    m_barre_progres->setVisible(false);

    charge_reglages();

    construit_interface_depuis_jorjala();

    /* Afin d'utiliser eventFilter pour filtrer les évènements de Jorjala. */
    qApp->installEventFilter(this);
}

void FenetrePrincipale::charge_reglages()
{
    QSettings settings;

    auto const &recent_files = settings.value("projet_récents").toStringList();

    for (auto const &file : recent_files) {
        if (QFile(file).exists()) {
            m_jorjala.ajoute_fichier_récent(file.toStdString().c_str());
        }
    }
}

void FenetrePrincipale::ecrit_reglages() const
{
    QSettings settings;
    QStringList recent;

    for (auto fichier_recent : m_jorjala.fichiers_récents()) {
        recent.push_front(fichier_recent.vers_std_string().c_str());
    }

    settings.setValue("projet_récents", recent);
}

void FenetrePrincipale::mis_a_jour_menu_fichier_recent()
{
    dls::tableau<danjo::DonneesAction> donnees_actions;

    danjo::DonneesAction donnees{};
    donnees.attache = "ouvrir_fichier";
    donnees.repondant_bouton = repondant_commande(m_jorjala);

    for (auto fichier_recent : m_jorjala.fichiers_récents()) {
        auto string_fichier_recent = fichier_recent.vers_std_string();
        auto name = QFileInfo(string_fichier_recent.c_str()).fileName();

        donnees.nom = name.toStdString();
        donnees.metadonnee = string_fichier_recent.c_str();

        donnees_actions.ajoute(donnees);
    }

    gestionnaire_danjo(m_jorjala)->recree_menu("Projets Récents", donnees_actions);
}

void FenetrePrincipale::closeEvent(QCloseEvent *event)
{
    if (!demande_permission_avant_de_fermer()) {
        event->ignore();
        return;
    }

    ecrit_reglages();
    event->accept();
}

static QLayout *qlayout_depuis_disposition(JJL::Disposition disposition)
{
    switch (disposition.direction()) {
        case JJL::DirectionDisposition::HORIZONTAL:
        {
            return new QHBoxLayout();
        }
        case JJL::DirectionDisposition::VERTICAL:
        {
            return new QVBoxLayout();
        }
    }
    return nullptr;
}

static Qt::Orientation donne_orientation_qsplitter_disposition(JJL::Disposition disposition)
{
    switch (disposition.direction()) {
        case JJL::DirectionDisposition::HORIZONTAL:
        {
            return Qt::Horizontal;
        }
        case JJL::DirectionDisposition::VERTICAL:
        {
            return Qt::Vertical;
        }
    }
    return Qt::Vertical;
}

static QWidget *génère_interface_disposition(JJL::Jorjala &jorjala,
                                             JJL::Disposition région,
                                             QVector<VueRegion *> &régions);

static void génère_interface_région(JJL::Jorjala &jorjala,
                                    JJL::RegionInterface &région,
                                    QSplitter *layout,
                                    QVector<VueRegion *> &régions)
{
    auto qwidget_région = new QWidget();
    layout->addWidget(qwidget_région);

    auto qwidget_région_layout = new QVBoxLayout();
    qwidget_région_layout->setMargin(0);
    qwidget_région->setLayout(qwidget_région_layout);

    if (région.type() == JJL::TypeRegion::CONTENEUR_ÉDITRICE) {
        auto vue_région = new VueRegion(jorjala, région, qwidget_région);
        régions.append(vue_région);
        qwidget_région_layout->addWidget(vue_région);
    }
    else {
        auto widget = génère_interface_disposition(jorjala, région.disposition(), régions);
        qwidget_région_layout->addWidget(widget);
    }
}

static QWidget *génère_interface_disposition(JJL::Jorjala &jorjala,
                                             JJL::Disposition disposition,
                                             QVector<VueRegion *> &régions)
{
    auto qsplitter = new QSplitter();
    qsplitter->setOrientation(donne_orientation_qsplitter_disposition(disposition));

    for (auto région : disposition.régions()) {
        génère_interface_région(jorjala, région, qsplitter, régions);
    }

    auto qlayout = qlayout_depuis_disposition(disposition);
    qlayout->setMargin(0);
    qlayout->addWidget(qsplitter);

    auto qwidget = new QWidget();
    qwidget->setLayout(qlayout);

    return qwidget;
}

void FenetrePrincipale::construit_interface_depuis_jorjala()
{
    auto interface = m_jorjala.donne_interface();
    m_régions.clear();

    auto disposition = interface.disposition();

    auto qwidget = génère_interface_disposition(m_jorjala, disposition, m_régions);
    setCentralWidget(qwidget);

    m_jorjala.notifie_observatrices(JJL::TypeEvenement::RAFRAICHISSEMENT);
}

bool FenetrePrincipale::demande_permission_avant_de_fermer()
{
    if (!m_jorjala.possède_changement()) {
        /* Rien ne fut changé, nous pouvons fermer automatiquement. */
        return true;
    }

    const QMessageBox::StandardButton ret = QMessageBox::warning(
        this,
        tr("Fermeture Jorjala"),
        tr("Le document fut modifié.\n"
           "Voulez-vous sauvegarder les changements ?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret) {
        case QMessageBox::Save:
            appele_commande(m_jorjala, "sauvegarder", "");
            return true;
        case QMessageBox::Cancel:
            return false;
        default:
            break;
    }

    return true;
}

bool FenetrePrincipale::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() != EvenementJorjala::id_type_qt) {
        return QWidget::eventFilter(object, event);
    }

    auto event_jjl = static_cast<EvenementJorjala *>(event);

    if (event_jjl->pour_quoi() == (JJL::TypeEvenement::PROJET | JJL::TypeEvenement::OUVERT)) {
        construit_interface_depuis_jorjala();
        return true;
    }

    for (auto région : m_régions) {
        région->ajourne_éditrice_active(event_jjl->pour_quoi());
    }

    return true;
}

void FenetrePrincipale::genere_barre_menu()
{
    auto donnees = cree_donnees_interface_danjo(m_jorjala, nullptr, nullptr);
    auto gestionnaire = gestionnaire_danjo(m_jorjala);

    for (auto const &chemin : chemins_scripts) {
        auto menu = gestionnaire->compile_menu_fichier(donnees, chemin);
        menuBar()->addMenu(menu);
    }

    auto menu_fichiers_recents = gestionnaire_danjo(m_jorjala)->pointeur_menu("Projets Récents");
    connect(menu_fichiers_recents,
            SIGNAL(aboutToShow()),
            this,
            SLOT(mis_a_jour_menu_fichier_recent()));
}

void FenetrePrincipale::genere_menu_prereglages()
{
    auto donnees = cree_donnees_interface_danjo(m_jorjala, nullptr, nullptr);
    auto gestionnaire = gestionnaire_danjo(m_jorjala);
    m_barre_outil = gestionnaire->compile_barre_outils_fichier(donnees,
                                                               "entreface/menu_prereglage.jo");
    addToolBar(Qt::TopToolBarArea, m_barre_outil);
}

void FenetrePrincipale::image_traitee()
{
    m_jorjala.notifie_observatrices(JJL::TypeEvenement::IMAGE | JJL::TypeEvenement::TRAITÉ);
}

void FenetrePrincipale::signale_proces(int quoi)
{
    m_jorjala.notifie_observatrices(static_cast<JJL::TypeEvenement>(quoi));
}

void FenetrePrincipale::tache_demarree()
{
    m_barre_progres->ajourne_valeur(0);
    m_barre_progres->setVisible(true);
}

void FenetrePrincipale::ajourne_progres(float progres)
{
    m_barre_progres->ajourne_valeur(static_cast<int>(progres * 100.0f));
}

void FenetrePrincipale::tache_terminee()
{
    m_barre_progres->setVisible(false);
}

void FenetrePrincipale::evaluation_debutee(const QString &message, int execution, int total)
{
    m_barre_progres->ajourne_valeur(0);
    m_barre_progres->setVisible(true);
    m_barre_progres->ajourne_message(message, execution, total);
}

void FenetrePrincipale::définit_texte_état(const QString &texte)
{
    m_texte_état->setVisible(texte != "");
    m_texte_état->setText(texte);
}
