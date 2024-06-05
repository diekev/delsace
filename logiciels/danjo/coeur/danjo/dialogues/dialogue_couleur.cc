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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "dialogue_couleur.h"

#include <QBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>

#include "controles_proprietes/commun.hh"

#include "controles/controle_couleur_tsv.h"
#include "controles/controle_nombre_decimal.h"

#include "types/outils.h"

namespace danjo {

/* ************************************************************************** */

class AffichageCouleur final : public QWidget {
    QColor m_couleur;

  public:
    explicit AffichageCouleur(const QColor &c, QWidget *parent = nullptr)
        : QWidget(parent), m_couleur(c)
    {
        auto metriques = this->fontMetrics();
        setFixedHeight(static_cast<int>(static_cast<float>(metriques.height()) * 1.5f));
        setFixedWidth(metriques.horizontalAdvance("#000000"));
    }

    void couleur(const QColor &c)
    {
        m_couleur = c;
        update();
    }

    void paintEvent(QPaintEvent * /*event*/) override
    {
        QPainter peintre(this);
        peintre.fillRect(this->rect(), m_couleur);
        peintre.setPen(QPen(Qt::black));
        peintre.drawRect(0, 0, this->rect().width() - 1, this->rect().height() - 1);
    }
};

/* ************************************************************************** */

DialogueCouleur::DialogueCouleur(QWidget *parent)
    : QDialog(parent), m_disposition(crée_vbox_layout()), m_disposition_sel_ct(crée_vbox_layout()),
      m_disposition_sel_cv(crée_hbox_layout()), m_disposition_horiz(crée_hbox_layout()),
      m_disposition_rvba(new QGridLayout()), m_disposition_boutons(crée_hbox_layout()),
      m_selecteur_sat_val(new ControleSatVal(this)), m_selecteur_teinte(new SelecteurTeinte(this)),
      m_selecteur_valeur(new ControleValeurCouleur(this)), m_r(new ControleNombreDecimal(this)),
      m_v(new ControleNombreDecimal(this)), m_b(new ControleNombreDecimal(this)),
      m_h(new ControleNombreDecimal(this)), m_s(new ControleNombreDecimal(this)),
      m_v0(new ControleNombreDecimal(this)), m_a(new ControleNombreDecimal(this)),
      m_affichage_couleur_nouvelle(
          new AffichageCouleur(converti_couleur(m_couleur_nouvelle), this)),
      m_affichage_couleur_originale(
          new AffichageCouleur(converti_couleur(m_couleur_origine), this)),
      m_contraste(new QLabel("", this))
{
    m_disposition_sel_ct->addWidget(m_selecteur_sat_val);
    m_disposition_sel_ct->addWidget(m_selecteur_teinte);

    m_disposition_sel_cv->addLayout(m_disposition_sel_ct);
    m_disposition_sel_cv->addWidget(m_selecteur_valeur);

    m_disposition_rvba->addWidget(new QLabel("R"), 0, 0);
    m_disposition_rvba->addWidget(m_r, 0, 1);
    m_disposition_rvba->addWidget(new QLabel("V"), 1, 0);
    m_disposition_rvba->addWidget(m_v, 1, 1);
    m_disposition_rvba->addWidget(new QLabel("B"), 2, 0);
    m_disposition_rvba->addWidget(m_b, 2, 1);
    m_disposition_rvba->addWidget(new QLabel("H"), 3, 0);
    m_disposition_rvba->addWidget(m_h, 3, 1);
    m_disposition_rvba->addWidget(new QLabel("S"), 4, 0);
    m_disposition_rvba->addWidget(m_s, 4, 1);
    m_disposition_rvba->addWidget(new QLabel("V"), 5, 0);
    m_disposition_rvba->addWidget(m_v0, 5, 1);
    m_disposition_rvba->addWidget(new QLabel("A"), 6, 0);
    m_disposition_rvba->addWidget(m_a, 6, 1);
    m_disposition_rvba->addWidget(m_affichage_couleur_originale, 7, 0);
    m_disposition_rvba->addWidget(m_affichage_couleur_nouvelle, 7, 1);
    m_disposition_rvba->addWidget(new QLabel("Contraste"), 8, 0);
    m_disposition_rvba->addWidget(m_contraste, 8, 1);

    m_disposition_horiz->addLayout(m_disposition_sel_cv);
    m_disposition_horiz->addLayout(m_disposition_rvba);

    connect(m_r, &ControleNombreDecimal::valeur_changee, this, &DialogueCouleur::ajourne_couleur);
    connect(m_v, &ControleNombreDecimal::valeur_changee, this, &DialogueCouleur::ajourne_couleur);
    connect(m_b, &ControleNombreDecimal::valeur_changee, this, &DialogueCouleur::ajourne_couleur);
    connect(
        m_h, &ControleNombreDecimal::valeur_changee, this, &DialogueCouleur::ajourne_couleur_hsv);
    connect(
        m_s, &ControleNombreDecimal::valeur_changee, this, &DialogueCouleur::ajourne_couleur_hsv);
    connect(
        m_v0, &ControleNombreDecimal::valeur_changee, this, &DialogueCouleur::ajourne_couleur_hsv);
    connect(m_a, &ControleNombreDecimal::valeur_changee, this, &DialogueCouleur::ajourne_couleur);
    connect(m_selecteur_sat_val,
            &ControleSatVal::valeur_changee,
            this,
            &DialogueCouleur::ajourne_couleur_sat_val);
    connect(m_selecteur_teinte,
            &SelecteurTeinte::valeur_changee,
            this,
            &DialogueCouleur::ajourne_couleur_teinte);
    connect(m_selecteur_valeur,
            &ControleValeurCouleur::valeur_changee,
            this,
            &DialogueCouleur::ajourne_couleur_valeur);

    auto bouton_accepter = new QPushButton("Accepter");
    auto bouton_annuler = new QPushButton("Annuler");

    connect(bouton_accepter, &QPushButton::pressed, this, &DialogueCouleur::accept);
    connect(bouton_annuler, &QPushButton::pressed, this, &DialogueCouleur::reject);

    m_disposition_boutons->addWidget(bouton_accepter);
    m_disposition_boutons->addWidget(bouton_annuler);

    m_disposition->addLayout(m_disposition_horiz);
    m_disposition->addLayout(m_disposition_boutons);

    setLayout(m_disposition);
}

void DialogueCouleur::couleur_originale(const dls::phys::couleur32 &c)
{
    for (int i = 0; i < 4; ++i) {
        m_couleur_origine[i] = c[i];
    }

    m_couleur_nouvelle = m_couleur_origine;

    m_affichage_couleur_nouvelle->couleur(converti_couleur(m_couleur_nouvelle));
    m_affichage_couleur_originale->couleur(converti_couleur(m_couleur_origine));

    dls::phys::couleur32 hsv;
    rvb_vers_hsv(m_couleur_nouvelle, hsv);

    m_selecteur_sat_val->couleur(hsv);
    m_selecteur_teinte->teinte(hsv.r);
    m_selecteur_valeur->valeur(hsv.b);

    m_r->valeur(c[0]);
    m_v->valeur(c[1]);
    m_b->valeur(c[2]);
    m_h->valeur(hsv.r);
    m_s->valeur(hsv.v);
    m_v0->valeur(hsv.b);
    m_a->valeur(c[3]);

    ajourne_label_contraste();
}

dls::phys::couleur32 DialogueCouleur::couleur_originale()
{
    return m_couleur_origine;
}

dls::phys::couleur32 DialogueCouleur::couleur_nouvelle()
{
    return m_couleur_nouvelle;
}

void DialogueCouleur::ajourne_plage(float min, float max)
{
    m_r->ajourne_plage(min, max);
    m_v->ajourne_plage(min, max);
    m_b->ajourne_plage(min, max);
    m_h->ajourne_plage(0.0f, 1.0f);
    m_s->ajourne_plage(0.0f, 1.0f);
    m_v0->ajourne_plage(0.0f, 1.0f);
    m_a->ajourne_plage(0.0f, 1.0f);
}

dls::phys::couleur32 DialogueCouleur::requiers_couleur(const dls::phys::couleur32 &couleur_origine)
{
    DialogueCouleur dialogue;
    dialogue.couleur_originale(couleur_origine);

    dialogue.show();
    auto ok = dialogue.exec();

    if (ok == QDialog::Accepted) {
        return dialogue.couleur_nouvelle();
    }

    return dialogue.couleur_originale();
}

void DialogueCouleur::ajourne_couleur()
{
    m_couleur_nouvelle.r = m_r->valeur();
    m_couleur_nouvelle.v = m_v->valeur();
    m_couleur_nouvelle.b = m_b->valeur();
    m_couleur_nouvelle.a = m_a->valeur();

    m_affichage_couleur_nouvelle->couleur(converti_couleur(m_couleur_nouvelle));

    dls::phys::couleur32 hsv;
    rvb_vers_hsv(m_couleur_nouvelle, hsv);

    m_selecteur_sat_val->couleur(hsv);
    m_selecteur_teinte->teinte(hsv.r);
    m_selecteur_valeur->valeur(hsv.b);

    m_h->valeur(hsv.r);
    m_s->valeur(hsv.v);
    m_v0->valeur(hsv.b);

    ajourne_label_contraste();

    Q_EMIT(couleur_changee());
}

void DialogueCouleur::ajourne_couleur_hsv()
{
    dls::phys::couleur32 hsv;
    hsv.r = m_h->valeur();
    hsv.v = m_s->valeur();
    hsv.b = m_v0->valeur();
    m_couleur_nouvelle.a = m_a->valeur();

    hsv_vers_rvb(hsv, m_couleur_nouvelle);

    m_affichage_couleur_nouvelle->couleur(converti_couleur(m_couleur_nouvelle));

    m_selecteur_sat_val->couleur(hsv);
    m_selecteur_teinte->teinte(hsv.r);
    m_selecteur_valeur->valeur(hsv.b);

    m_r->valeur(m_couleur_nouvelle.r);
    m_v->valeur(m_couleur_nouvelle.v);
    m_b->valeur(m_couleur_nouvelle.b);

    ajourne_label_contraste();

    Q_EMIT(couleur_changee());
}

void DialogueCouleur::ajourne_couleur_sat_val()
{
    dls::phys::couleur32 hsv;
    hsv.r = m_h->valeur();
    hsv.v = m_selecteur_sat_val->saturation();
    hsv.b = m_selecteur_sat_val->valeur();
    m_couleur_nouvelle.a = m_a->valeur();

    hsv_vers_rvb(hsv, m_couleur_nouvelle);

    m_affichage_couleur_nouvelle->couleur(converti_couleur(m_couleur_nouvelle));

    m_s->valeur(hsv.v);
    m_v0->valeur(hsv.b);
    m_selecteur_valeur->valeur(hsv.b);

    m_r->valeur(m_couleur_nouvelle.r);
    m_v->valeur(m_couleur_nouvelle.v);
    m_b->valeur(m_couleur_nouvelle.b);

    ajourne_label_contraste();

    Q_EMIT(couleur_changee());
}

void DialogueCouleur::ajourne_couleur_teinte()
{
    dls::phys::couleur32 hsv;
    hsv.r = m_selecteur_teinte->teinte();
    hsv.v = m_s->valeur();
    hsv.b = m_v0->valeur();
    m_couleur_nouvelle.a = m_a->valeur();

    hsv_vers_rvb(hsv, m_couleur_nouvelle);

    m_affichage_couleur_nouvelle->couleur(converti_couleur(m_couleur_nouvelle));

    m_selecteur_sat_val->couleur(hsv);
    m_h->valeur(hsv.r);
    m_selecteur_valeur->valeur(hsv.b);

    m_r->valeur(m_couleur_nouvelle.r);
    m_v->valeur(m_couleur_nouvelle.v);
    m_b->valeur(m_couleur_nouvelle.b);

    ajourne_label_contraste();

    Q_EMIT(couleur_changee());
}

void DialogueCouleur::ajourne_couleur_valeur()
{
    dls::phys::couleur32 hsv;
    hsv.r = m_h->valeur();
    hsv.v = m_s->valeur();
    hsv.b = m_selecteur_valeur->valeur();
    m_couleur_nouvelle.a = m_a->valeur();

    hsv_vers_rvb(hsv, m_couleur_nouvelle);

    m_affichage_couleur_nouvelle->couleur(converti_couleur(m_couleur_nouvelle));

    m_selecteur_sat_val->couleur(hsv);
    m_selecteur_teinte->teinte(hsv.r);
    m_v0->valeur(hsv.b);

    m_r->valeur(m_couleur_nouvelle.r);
    m_v->valeur(m_couleur_nouvelle.v);
    m_b->valeur(m_couleur_nouvelle.b);

    ajourne_label_contraste();

    Q_EMIT(couleur_changee());
}

void DialogueCouleur::ajourne_label_contraste()
{
    auto contraste = calcul_contraste_local(m_couleur_origine, m_couleur_nouvelle);
    m_contraste->setText(QString::number(contraste));
}

} /* namespace danjo */
