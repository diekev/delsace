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

#pragma once

#include <delsace/math/vecteur.hh>

#include "bibliotheques/spectre/spectre.h"

struct ContexteNuancage;
class GNA;
class ParametresRendu;

struct BSDF {
	explicit BSDF(ContexteNuancage &ctx);

	virtual ~BSDF() = default;

	virtual void evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf) = 0;

	virtual void genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, int profondeur) = 0;

	ContexteNuancage &contexte;
};

struct BSDFTrivial : public BSDF {
	explicit BSDFTrivial(ContexteNuancage &ctx);

	void evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf) override;

	void genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, int profondeur) override;
};

struct BSDFAngleVue : public BSDF {
	explicit BSDFAngleVue(ContexteNuancage &ctx);

	void evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf) override;

	void genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, int profondeur) override;
};

struct BSDFDiffus : public BSDF {
	Spectre spectre;

	BSDFDiffus(ContexteNuancage &ctx, Spectre s);

	void evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf) override;

	void genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, int profondeur) override;
};

struct BSDFReflectance : public BSDF {
	explicit BSDFReflectance(ContexteNuancage &ctx);

	void evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf) override;

	void genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, int profondeur) override;
};

struct BSDFVerre : public BSDF {
	double index_refraction = 1.3;

	explicit BSDFVerre(ContexteNuancage &ctx);

	void evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf) override;

	void genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, int profondeur) override;
};

struct BSDFVolume : public BSDF {
	double densite = 1.0;
	Spectre sigma_a = Spectre(1.0);
	Spectre sigma_s = Spectre(1.0);

	explicit BSDFVolume(ContexteNuancage &ctx);

	void evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf) override;

	void genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, int profondeur) override;
};

struct BSDFPhaseIsotropique : public BSDF {
	double densite = 1.0;
	Spectre sigma_a = Spectre(1.0);
	Spectre sigma_s = Spectre(1.0);

	explicit BSDFPhaseIsotropique(ContexteNuancage &ctx);

	void evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf) override;

	void genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, int profondeur) override;
};

struct BSDFPhaseAnisotropique : public BSDF {
	double densite = 1.0;
	Spectre sigma_a = Spectre(1.0);
	Spectre sigma_s = Spectre(1.0);

	bool isotropique = false;
	double g;
	double un_plus_g2;
	double un_moins_g2;
	double un_sur_2g;

	BSDFPhaseAnisotropique(ContexteNuancage &ctx, double g_ = 1.0);

	void evalue_echantillon(GNA &gna, const ParametresRendu &parametres, const dls::math::vec3d &dir, Spectre &L, double &pdf) override;

	void genere_echantillon(GNA &gna, const ParametresRendu &parametres, dls::math::vec3d &dir, Spectre &L, double &pdf, int profondeur) override;

private:
	double calcul_pdf(double cos_theta) const;
	double inverse_cdf(double xi) const;
};
