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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "couleur.hh"

#include <algorithm>

#include "biblinternes/math/outils.hh"

namespace dls::phys {

/**
 * Pris depuis http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv.
 */
void rvb_vers_hsv(float r, float g, float b, float *lh, float *ls, float *lv)
{
	auto k = 0.0f;

	if (g < b) {
		std::swap(g, b);
		k = -1.0f;
	}

	auto min_gb = b;

	if (r < g) {
		std::swap(r, g);
		k = -2.0f / 6.0f - k;
		min_gb = std::min(g, b);
	}

	auto chroma = r - min_gb;

	*lh = std::abs(k + (g - b) / (6.0f * chroma + 1e-20f));
	*ls = chroma / (r + 1e-20f);
	*lv = r;
}

void rvb_vers_hsv(const couleur32 &rvb, couleur32 &hsv)
{
	rvb_vers_hsv(rvb.r, rvb.v, rvb.b, &hsv.r, &hsv.v, &hsv.b);
}

void hsv_vers_rvb(float h, float s, float v, float *r, float *g, float *b)
{
	auto nr =        std::abs(h * 6.0f - 3.0f) - 1.0f;
	auto ng = 2.0f - std::abs(h * 6.0f - 2.0f);
	auto nb = 2.0f - std::abs(h * 6.0f - 4.0f);

	nr = math::restreint(nr, 0.0f, 1.0f);
	ng = math::restreint(ng, 0.0f, 1.0f);
	nb = math::restreint(nb, 0.0f, 1.0f);

	*r = ((nr - 1.0f) * s + 1.0f) * v;
	*g = ((ng - 1.0f) * s + 1.0f) * v;
	*b = ((nb - 1.0f) * s + 1.0f) * v;
}

void hsv_vers_rvb(const couleur32 &hsv, couleur32 &rvb)
{
	hsv_vers_rvb(hsv.r, hsv.v, hsv.b, &rvb.r, &rvb.v, &rvb.b);
}


/* ****************************** corps noir ******************************** */

/* Calcul de la couleur dans l'interval 800...12000 avec une approximation
 * a/x+bx+c pour R et V, et ((at + b)t + c)t + d) pour B.
 * L'erreur absolue max pour RVB est (0.00095, 0.00077, 0.00057), ce qui est
 * assez pour avoir la même 8bit/canal couleur.
 *
 * Le code provient de Cycles.
 */

static const float table_corps_noir_r[6][3] = {
	{  2.52432244e+03f, -1.06185848e-03f, 3.11067539e+00f },
	{  3.37763626e+03f, -4.34581697e-04f, 1.64843306e+00f },
	{  4.10671449e+03f, -8.61949938e-05f, 6.41423749e-01f },
	{  4.66849800e+03f,  2.85655028e-05f, 1.29075375e-01f },
	{  4.60124770e+03f,  2.89727618e-05f, 1.48001316e-01f },
	{  3.78765709e+03f,  9.36026367e-06f, 3.98995841e-01f },
};

static const float table_corps_noir_v[6][3] = {
	{ -7.50343014e+02f,  3.15679613e-04f, 4.73464526e-01f },
	{ -1.00402363e+03f,  1.29189794e-04f, 9.08181524e-01f },
	{ -1.22075471e+03f,  2.56245413e-05f, 1.20753416e+00f },
	{ -1.42546105e+03f, -4.01730887e-05f, 1.44002695e+00f },
	{ -1.18134453e+03f, -2.18913373e-05f, 1.30656109e+00f },
	{ -5.00279505e+02f, -4.59745390e-06f, 1.09090465e+00f },
};

static const float table_corps_noir_b[6][4] = {
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	{ -2.02524603e-11f,  1.79435860e-07f, -2.60561875e-04f, -1.41761141e-02f },
	{ -2.22463426e-13f, -1.55078698e-08f,  3.81675160e-04f, -7.30646033e-01f },
	{  6.72595954e-13f, -2.73059993e-08f,  4.24068546e-04f, -7.52204323e-01f },
};

couleur32 couleur_depuis_corps_noir(float temperature)
{
	auto rvb = couleur32();

	if (temperature >= 12000.0f) {
		rvb[0] = 0.826270103f;
		rvb[1] = 0.994478524f;
		rvb[2] = 1.56626022f;
	}
	else if (temperature < 965.0f) {
		rvb[0] = 4.70366907f;
		rvb[1] = 0.0f;
		rvb[2] = 0.0f;
	}
	else {
		int i = (temperature >= 6365.0f) ? 5 :
				(temperature >= 3315.0f) ? 4 :
				(temperature >= 1902.0f) ? 3 :
				(temperature >= 1449.0f) ? 2 :
				(temperature >= 1167.0f) ? 1 : 0;

		float const *r = table_corps_noir_r[i];
		float const *g = table_corps_noir_v[i];
		float const *b = table_corps_noir_b[i];

		float const t_inv = 1.0f / temperature;
		rvb[0] = r[0] * t_inv + r[1] * temperature + r[2];
		rvb[1] = g[0] * t_inv + g[1] * temperature + g[2];
		rvb[2] = ((b[0] * temperature + b[1]) * temperature + b[2]) * temperature + b[3];
	}

	rvb.a = 1.0f;

	return rvb;
}

/* ***************************** longueur d'onde **************************** */

// CIE colour matching functions xBar, yBar, and zBar for
//	 wavelengths from 380 through 780 nanometers, every 5
//	 nanometers.  For a wavelength lambda in this range:
//		  cie_colour_match[(lambda - 380) / 5][0] = xBar
//		  cie_colour_match[(lambda - 380) / 5][1] = yBar
//		  cie_colour_match[(lambda - 380) / 5][2] = zBar
static const float cie_colour_match[81][3] = {
	{0.0014f,0.0000f,0.0065f}, {0.0022f,0.0001f,0.0105f}, {0.0042f,0.0001f,0.0201f},
	{0.0076f,0.0002f,0.0362f}, {0.0143f,0.0004f,0.0679f}, {0.0232f,0.0006f,0.1102f},
	{0.0435f,0.0012f,0.2074f}, {0.0776f,0.0022f,0.3713f}, {0.1344f,0.0040f,0.6456f},
	{0.2148f,0.0073f,1.0391f}, {0.2839f,0.0116f,1.3856f}, {0.3285f,0.0168f,1.6230f},
	{0.3483f,0.0230f,1.7471f}, {0.3481f,0.0298f,1.7826f}, {0.3362f,0.0380f,1.7721f},
	{0.3187f,0.0480f,1.7441f}, {0.2908f,0.0600f,1.6692f}, {0.2511f,0.0739f,1.5281f},
	{0.1954f,0.0910f,1.2876f}, {0.1421f,0.1126f,1.0419f}, {0.0956f,0.1390f,0.8130f},
	{0.0580f,0.1693f,0.6162f}, {0.0320f,0.2080f,0.4652f}, {0.0147f,0.2586f,0.3533f},
	{0.0049f,0.3230f,0.2720f}, {0.0024f,0.4073f,0.2123f}, {0.0093f,0.5030f,0.1582f},
	{0.0291f,0.6082f,0.1117f}, {0.0633f,0.7100f,0.0782f}, {0.1096f,0.7932f,0.0573f},
	{0.1655f,0.8620f,0.0422f}, {0.2257f,0.9149f,0.0298f}, {0.2904f,0.9540f,0.0203f},
	{0.3597f,0.9803f,0.0134f}, {0.4334f,0.9950f,0.0087f}, {0.5121f,1.0000f,0.0057f},
	{0.5945f,0.9950f,0.0039f}, {0.6784f,0.9786f,0.0027f}, {0.7621f,0.9520f,0.0021f},
	{0.8425f,0.9154f,0.0018f}, {0.9163f,0.8700f,0.0017f}, {0.9786f,0.8163f,0.0014f},
	{1.0263f,0.7570f,0.0011f}, {1.0567f,0.6949f,0.0010f}, {1.0622f,0.6310f,0.0008f},
	{1.0456f,0.5668f,0.0006f}, {1.0026f,0.5030f,0.0003f}, {0.9384f,0.4412f,0.0002f},
	{0.8544f,0.3810f,0.0002f}, {0.7514f,0.3210f,0.0001f}, {0.6424f,0.2650f,0.0000f},
	{0.5419f,0.2170f,0.0000f}, {0.4479f,0.1750f,0.0000f}, {0.3608f,0.1382f,0.0000f},
	{0.2835f,0.1070f,0.0000f}, {0.2187f,0.0816f,0.0000f}, {0.1649f,0.0610f,0.0000f},
	{0.1212f,0.0446f,0.0000f}, {0.0874f,0.0320f,0.0000f}, {0.0636f,0.0232f,0.0000f},
	{0.0468f,0.0170f,0.0000f}, {0.0329f,0.0119f,0.0000f}, {0.0227f,0.0082f,0.0000f},
	{0.0158f,0.0057f,0.0000f}, {0.0114f,0.0041f,0.0000f}, {0.0081f,0.0029f,0.0000f},
	{0.0058f,0.0021f,0.0000f}, {0.0041f,0.0015f,0.0000f}, {0.0029f,0.0010f,0.0000f},
	{0.0020f,0.0007f,0.0000f}, {0.0014f,0.0005f,0.0000f}, {0.0010f,0.0004f,0.0000f},
	{0.0007f,0.0002f,0.0000f}, {0.0005f,0.0002f,0.0000f}, {0.0003f,0.0001f,0.0000f},
	{0.0002f,0.0001f,0.0000f}, {0.0002f,0.0001f,0.0000f}, {0.0001f,0.0000f,0.0000f},
	{0.0001f,0.0000f,0.0000f}, {0.0001f,0.0000f,0.0000f}, {0.0000f,0.0000f,0.0000f}
};

couleur32 couleur_depuis_longueur_onde(float lambda)
{
	auto clr = couleur32();

	/* converti vers l'interval 0..80 */
	auto ii = (lambda - 380.0f) * (1.0f / 5.0f);
	auto const i = static_cast<int>(ii);

	if(i < 0 || i >= 80) {
		clr = couleur32(0.0f, 0.0f, 0.0f, 1.0f);
	}
	else {
		ii -= static_cast<float>(i);
		float const *c = cie_colour_match[i];
		auto const a = couleur32(c[0], c[1], c[2], 1.0f);
		auto const b = couleur32(c[3], c[4], c[5], 1.0f);
		clr = (1.0f - ii) * a + b * ii;
	}

	clr = xyz_vers_rvb(clr);

	/* Fais en sorte que tous les composants soient <= 1. */
	clr *= 1.0f / 2.52f;

	/* Interdit les valeurs négatives. */
	clr.r = std::max(clr.r, 0.0f);
	clr.v = std::max(clr.v, 0.0f);
	clr.b = std::max(clr.b, 0.0f);
	clr.a = 1.0f;

	return clr;
}

couleur32 couleur_depuis_poids(float poids)
{
	auto clr = dls::phys::couleur32();
	auto const blend = ((poids / 2.0f) + 0.5f);

	/* bleu -> cyan */
	if (poids <= 0.25f) {
		clr[0] = 0.0f;
		clr[1] = blend * poids * 4.0f;
		clr[2] = blend;
	}
	/* cyan -> vert */
	else if (poids <= 0.50f) {
		clr[0] = 0.0f;
		clr[1] = blend;
		clr[2] = blend * (1.0f - ((poids - 0.25f) * 4.0f));
	}
	/* vert -> jaune */
	else if (poids <= 0.75f) {
		clr[0] = blend * ((poids - 0.50f) * 4.0f);
		clr[1] = blend;
		clr[2] = 0.0f;
	}
	/* jaune -> rouge */
	else if (poids <= 1.0f) {
		clr[0] = blend;
		clr[1] = blend * (1.0f - ((poids - 0.75f) * 4.0f));
		clr[2] = 0.0f;
	}
	else {
		/* valeur exceptionnelle, nonrestreinte ou innombrable, assigne une
		 * couleur pour évite la mémoire non-initialisé  */
		clr[0] = 1.0f;
		clr[1] = 0.0f;
		clr[2] = 1.0f;
	}

	clr.a = 1.0f;

	return clr;
}

}  /* dls::phys */
