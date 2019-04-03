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

#include "swizzler.hh"

/* Clang se plaint des structures anonymes dans les unions. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

namespace dls::math {

template <int, typename, int...>
struct vecteur;

namespace detail {

/**
 * Cette structure est là pour aider à définir un type de vecteur équivalent
 * pour le swizzler, et ainsi avoir des expressions du type :
 *  vec3.xy == vec2.xy
 *  vec3.xxx == vec4.xxx
 *
 * sinon vec3.xy serait d'un type différent de vec2.xy, etc...
 */
template <int, typename, unsigned long>
struct equivec;

template <int O, typename T>
struct equivec<O, T, 2> {
	using type = vecteur<O, T, 0, 1>;
};

template <int O, typename T>
struct equivec<O, T, 3> {
	using type = vecteur<O, T, 0, 1, 2>;
};

template <int O, typename T>
struct equivec<O, T, 4> {
	using type = vecteur<O, T, 0, 1, 2, 3>;
};

/**
 * Classe de base pour les vecteurs dont la spécialisation définie les membres
 * des vecteurs, et leurs méthodes d'accès.
 */
template <typename T, unsigned long N, template <int...> class type_swizzler>
struct base_vecteur;


template <int O, typename T, int... Ns>
struct selecteur_base_vecteur {
	static_assert (sizeof...(Ns) > 0, "Il doit y avoir au moins 1 composant !");

	template <int... index>
	struct usine_enveloppe_swizzler {
		using type = swizzler<typename equivec<O, T, sizeof...(index)>::type, T, sizeof...(Ns), index...>;
	};

	template <int x>
	struct usine_enveloppe_swizzler<x> {
		using type = T;
	};

	using type_base = base_vecteur<T, sizeof...(Ns), usine_enveloppe_swizzler>;
};

/**
 * Spécialisation de base_vecteur pour les vecteurs 2D.
 */
template <typename T, template<int...> class enveloppe_swizzler>
struct base_vecteur<T, 2, enveloppe_swizzler> {
	union {
		T data[2];

		struct {
			typename enveloppe_swizzler<0>::type x;
			typename enveloppe_swizzler<0>::type y;
		};
		struct {
			typename enveloppe_swizzler<0>::type r;
			typename enveloppe_swizzler<0>::type g;
		};
		struct {
			typename enveloppe_swizzler<0>::type s;
			typename enveloppe_swizzler<0>::type t;
		};

		typename enveloppe_swizzler<0, 0>::type xx, rr, ss;
		typename enveloppe_swizzler<0, 1>::type xy, rg, st;
		typename enveloppe_swizzler<1, 0>::type yx, gr, ts;
		typename enveloppe_swizzler<1, 1>::type yy, gg, tt;
		typename enveloppe_swizzler<0, 0, 0>::type xxx, rrr, sss;
		typename enveloppe_swizzler<0, 0, 1>::type xxy, rrg, sst;
		typename enveloppe_swizzler<0, 1, 0>::type xyx, rgr, sts;
		typename enveloppe_swizzler<0, 1, 1>::type xyy, rgg, stt;
		typename enveloppe_swizzler<1, 0, 0>::type yxx, grr, tss;
		typename enveloppe_swizzler<1, 0, 1>::type yxy, grg, tst;
		typename enveloppe_swizzler<1, 1, 0>::type yyx, ggr, tts;
		typename enveloppe_swizzler<1, 1, 1>::type yyy, ggg, ttt;
		typename enveloppe_swizzler<0, 0, 0, 0>::type xxxx, rrrr, ssss;
		typename enveloppe_swizzler<0, 0, 0, 1>::type xxxy, rrrg, ssst;
		typename enveloppe_swizzler<0, 0, 1, 0>::type xxyx, rrgr, ssts;
		typename enveloppe_swizzler<0, 0, 1, 1>::type xxyy, rrgg, sstt;
		typename enveloppe_swizzler<0, 1, 0, 0>::type xyxx, rgrr, stss;
		typename enveloppe_swizzler<0, 1, 0, 1>::type xyxy, rgrg, stst;
		typename enveloppe_swizzler<0, 1, 1, 0>::type xyyx, rggr, stts;
		typename enveloppe_swizzler<0, 1, 1, 1>::type xyyy, rggg, sttt;
		typename enveloppe_swizzler<1, 0, 0, 0>::type yxxx, grrr, tsss;
		typename enveloppe_swizzler<1, 0, 0, 1>::type yxxy, grrg, tsst;
		typename enveloppe_swizzler<1, 0, 1, 0>::type yxyx, grgr, tsts;
		typename enveloppe_swizzler<1, 0, 1, 1>::type yxyy, grgg, tstt;
		typename enveloppe_swizzler<1, 1, 0, 0>::type yyxx, ggrr, ttss;
		typename enveloppe_swizzler<1, 1, 0, 1>::type yyxy, ggrg, ttst;
		typename enveloppe_swizzler<1, 1, 1, 0>::type yyyx, gggr, ttts;
		typename enveloppe_swizzler<1, 1, 1, 1>::type yyyy, gggg, tttt;
	};
};

/**
 * Spécialisation de base_vecteur pour les vecteurs 3D.
 */
template <typename T, template<int...> class enveloppe_swizzler>
struct base_vecteur<T, 3, enveloppe_swizzler> {
	union {
		T data[3];

		struct {
			typename enveloppe_swizzler<0>::type x;
			typename enveloppe_swizzler<1>::type y;
			typename enveloppe_swizzler<2>::type z;
		};
		struct {
			typename enveloppe_swizzler<0>::type r;
			typename enveloppe_swizzler<1>::type g;
			typename enveloppe_swizzler<2>::type b;
		};
		struct {
			typename enveloppe_swizzler<0>::type s;
			typename enveloppe_swizzler<1>::type t;
			typename enveloppe_swizzler<2>::type p;
		};

		typename enveloppe_swizzler<0, 0>::type xx, rr, ss;
		typename enveloppe_swizzler<0, 1>::type xy, rg, st;
		typename enveloppe_swizzler<0, 2>::type xz, rb, sp;
		typename enveloppe_swizzler<1, 0>::type yx, gr, ts;
		typename enveloppe_swizzler<1, 1>::type yy, gg, tt;
		typename enveloppe_swizzler<1, 2>::type yz, gb, tp;
		typename enveloppe_swizzler<2, 0>::type zx, br, ps;
		typename enveloppe_swizzler<2, 1>::type zy, bg, pt;
		typename enveloppe_swizzler<2, 2>::type zz, bb, pp;
		typename enveloppe_swizzler<0, 0, 0>::type xxx, rrr, sss;
		typename enveloppe_swizzler<0, 0, 1>::type xxy, rrg, sst;
		typename enveloppe_swizzler<0, 0, 2>::type xxz, rrb, ssp;
		typename enveloppe_swizzler<0, 1, 0>::type xyx, rgr, sts;
		typename enveloppe_swizzler<0, 1, 1>::type xyy, rgg, stt;
		typename enveloppe_swizzler<0, 1, 2>::type xyz, rgb, stp;
		typename enveloppe_swizzler<0, 2, 0>::type xzx, rbr, sps;
		typename enveloppe_swizzler<0, 2, 1>::type xzy, rbg, spt;
		typename enveloppe_swizzler<0, 2, 2>::type xzz, rbb, spp;
		typename enveloppe_swizzler<1, 0, 0>::type yxx, grr, tss;
		typename enveloppe_swizzler<1, 0, 1>::type yxy, grg, tst;
		typename enveloppe_swizzler<1, 0, 2>::type yxz, grb, tsp;
		typename enveloppe_swizzler<1, 1, 0>::type yyx, ggr, tts;
		typename enveloppe_swizzler<1, 1, 1>::type yyy, ggg, ttt;
		typename enveloppe_swizzler<1, 1, 2>::type yyz, ggb, ttp;
		typename enveloppe_swizzler<1, 2, 0>::type yzx, gbr, tps;
		typename enveloppe_swizzler<1, 2, 1>::type yzy, gbg, tpt;
		typename enveloppe_swizzler<1, 2, 2>::type yzz, gbb, tpp;
		typename enveloppe_swizzler<2, 0, 0>::type zxx, brr, pss;
		typename enveloppe_swizzler<2, 0, 1>::type zxy, brg, pst;
		typename enveloppe_swizzler<2, 0, 2>::type zxz, brb, psp;
		typename enveloppe_swizzler<2, 1, 0>::type zyx, bgr, pts;
		typename enveloppe_swizzler<2, 1, 1>::type zyy, bgg, ptt;
		typename enveloppe_swizzler<2, 1, 2>::type zyz, bgb, ptp;
		typename enveloppe_swizzler<2, 2, 0>::type zzx, bbr, pps;
		typename enveloppe_swizzler<2, 2, 1>::type zzy, bbg, ppt;
		typename enveloppe_swizzler<2, 2, 2>::type zzz, bbb, ppp;
		typename enveloppe_swizzler<0, 0, 0, 0>::type xxxx, rrrr, ssss;
		typename enveloppe_swizzler<0, 0, 0, 1>::type xxxy, rrrg, ssst;
		typename enveloppe_swizzler<0, 0, 0, 2>::type xxxz, rrrb, sssp;
		typename enveloppe_swizzler<0, 0, 1, 0>::type xxyx, rrgr, ssts;
		typename enveloppe_swizzler<0, 0, 1, 1>::type xxyy, rrgg, sstt;
		typename enveloppe_swizzler<0, 0, 1, 2>::type xxyz, rrgb, sstp;
		typename enveloppe_swizzler<0, 0, 2, 0>::type xxzx, rrbr, ssps;
		typename enveloppe_swizzler<0, 0, 2, 1>::type xxzy, rrbg, sspt;
		typename enveloppe_swizzler<0, 0, 2, 2>::type xxzz, rrbb, sspp;
		typename enveloppe_swizzler<0, 1, 0, 0>::type xyxx, rgrr, stss;
		typename enveloppe_swizzler<0, 1, 0, 1>::type xyxy, rgrg, stst;
		typename enveloppe_swizzler<0, 1, 0, 2>::type xyxz, rgrb, stsp;
		typename enveloppe_swizzler<0, 1, 1, 0>::type xyyx, rggr, stts;
		typename enveloppe_swizzler<0, 1, 1, 1>::type xyyy, rggg, sttt;
		typename enveloppe_swizzler<0, 1, 1, 2>::type xyyz, rggb, sttp;
		typename enveloppe_swizzler<0, 1, 2, 0>::type xyzx, rgbr, stps;
		typename enveloppe_swizzler<0, 1, 2, 1>::type xyzy, rgbg, stpt;
		typename enveloppe_swizzler<0, 1, 2, 2>::type xyzz, rgbb, stpp;
		typename enveloppe_swizzler<0, 2, 0, 0>::type xzxx, rbrr, spss;
		typename enveloppe_swizzler<0, 2, 0, 1>::type xzxy, rbrg, spst;
		typename enveloppe_swizzler<0, 2, 0, 2>::type xzxz, rbrb, spsp;
		typename enveloppe_swizzler<0, 2, 1, 0>::type xzyx, rbgr, spts;
		typename enveloppe_swizzler<0, 2, 1, 1>::type xzyy, rbgg, sptt;
		typename enveloppe_swizzler<0, 2, 1, 2>::type xzyz, rbgb, sptp;
		typename enveloppe_swizzler<0, 2, 2, 0>::type xzzx, rbbr, spps;
		typename enveloppe_swizzler<0, 2, 2, 1>::type xzzy, rbbg, sppt;
		typename enveloppe_swizzler<0, 2, 2, 2>::type xzzz, rbbb, sppp;
		typename enveloppe_swizzler<1, 0, 0, 0>::type yxxx, grrr, tsss;
		typename enveloppe_swizzler<1, 0, 0, 1>::type yxxy, grrg, tsst;
		typename enveloppe_swizzler<1, 0, 0, 2>::type yxxz, grrb, tssp;
		typename enveloppe_swizzler<1, 0, 1, 0>::type yxyx, grgr, tsts;
		typename enveloppe_swizzler<1, 0, 1, 1>::type yxyy, grgg, tstt;
		typename enveloppe_swizzler<1, 0, 1, 2>::type yxyz, grgb, tstp;
		typename enveloppe_swizzler<1, 0, 2, 0>::type yxzx, grbr, tsps;
		typename enveloppe_swizzler<1, 0, 2, 1>::type yxzy, grbg, tspt;
		typename enveloppe_swizzler<1, 0, 2, 2>::type yxzz, grbb, tspp;
		typename enveloppe_swizzler<1, 1, 0, 0>::type yyxx, ggrr, ttss;
		typename enveloppe_swizzler<1, 1, 0, 1>::type yyxy, ggrg, ttst;
		typename enveloppe_swizzler<1, 1, 0, 2>::type yyxz, ggrb, ttsp;
		typename enveloppe_swizzler<1, 1, 1, 0>::type yyyx, gggr, ttts;
		typename enveloppe_swizzler<1, 1, 1, 1>::type yyyy, gggg, tttt;
		typename enveloppe_swizzler<1, 1, 1, 2>::type yyyz, gggb, tttp;
		typename enveloppe_swizzler<1, 1, 2, 0>::type yyzx, ggbr, ttps;
		typename enveloppe_swizzler<1, 1, 2, 1>::type yyzy, ggbg, ttpt;
		typename enveloppe_swizzler<1, 1, 2, 2>::type yyzz, ggbb, ttpp;
		typename enveloppe_swizzler<1, 2, 0, 0>::type yzxx, gbrr, tpss;
		typename enveloppe_swizzler<1, 2, 0, 1>::type yzxy, gbrg, tpst;
		typename enveloppe_swizzler<1, 2, 0, 2>::type yzxz, gbrb, tpsp;
		typename enveloppe_swizzler<1, 2, 1, 0>::type yzyx, gbgr, tpts;
		typename enveloppe_swizzler<1, 2, 1, 1>::type yzyy, gbgg, tptt;
		typename enveloppe_swizzler<1, 2, 1, 2>::type yzyz, gbgb, tptp;
		typename enveloppe_swizzler<1, 2, 2, 0>::type yzzx, gbbr, tpps;
		typename enveloppe_swizzler<1, 2, 2, 1>::type yzzy, gbbg, tppt;
		typename enveloppe_swizzler<1, 2, 2, 2>::type yzzz, gbbb, tppp;
		typename enveloppe_swizzler<2, 0, 0, 0>::type zxxx, brrr, psss;
		typename enveloppe_swizzler<2, 0, 0, 1>::type zxxy, brrg, psst;
		typename enveloppe_swizzler<2, 0, 0, 2>::type zxxz, brrb, pssp;
		typename enveloppe_swizzler<2, 0, 1, 0>::type zxyx, brgr, psts;
		typename enveloppe_swizzler<2, 0, 1, 1>::type zxyy, brgg, pstt;
		typename enveloppe_swizzler<2, 0, 1, 2>::type zxyz, brgb, pstp;
		typename enveloppe_swizzler<2, 0, 2, 0>::type zxzx, brbr, psps;
		typename enveloppe_swizzler<2, 0, 2, 1>::type zxzy, brbg, pspt;
		typename enveloppe_swizzler<2, 0, 2, 2>::type zxzz, brbb, pspp;
		typename enveloppe_swizzler<2, 1, 0, 0>::type zyxx, bgrr, ptss;
		typename enveloppe_swizzler<2, 1, 0, 1>::type zyxy, bgrg, ptst;
		typename enveloppe_swizzler<2, 1, 0, 2>::type zyxz, bgrb, ptsp;
		typename enveloppe_swizzler<2, 1, 1, 0>::type zyyx, bggr, ptts;
		typename enveloppe_swizzler<2, 1, 1, 1>::type zyyy, bggg, pttt;
		typename enveloppe_swizzler<2, 1, 1, 2>::type zyyz, bggb, pttp;
		typename enveloppe_swizzler<2, 1, 2, 0>::type zyzx, bgbr, ptps;
		typename enveloppe_swizzler<2, 1, 2, 1>::type zyzy, bgbg, ptpt;
		typename enveloppe_swizzler<2, 1, 2, 2>::type zyzz, bgbb, ptpp;
		typename enveloppe_swizzler<2, 2, 0, 0>::type zzxx, bbrr, ppss;
		typename enveloppe_swizzler<2, 2, 0, 1>::type zzxy, bbrg, ppst;
		typename enveloppe_swizzler<2, 2, 0, 2>::type zzxz, bbrb, ppsp;
		typename enveloppe_swizzler<2, 2, 1, 0>::type zzyx, bbgr, ppts;
		typename enveloppe_swizzler<2, 2, 1, 1>::type zzyy, bbgg, pptt;
		typename enveloppe_swizzler<2, 2, 1, 2>::type zzyz, bbgb, pptp;
		typename enveloppe_swizzler<2, 2, 2, 0>::type zzzx, bbbr, ppps;
		typename enveloppe_swizzler<2, 2, 2, 1>::type zzzy, bbbg, pppt;
		typename enveloppe_swizzler<2, 2, 2, 2>::type zzzz, bbbb, pppp;
	};
};

/**
 * Spécialisation de base_vecteur pour les vecteurs 4D.
 */
template <typename T, template<int...> class enveloppe_swizzler>
struct base_vecteur<T, 4, enveloppe_swizzler> {
	union {
		T data[4];

		struct {
			typename enveloppe_swizzler<0>::type x;
			typename enveloppe_swizzler<1>::type y;
			typename enveloppe_swizzler<2>::type z;
			typename enveloppe_swizzler<3>::type w;
		};

		struct {
			typename enveloppe_swizzler<0>::type r;
			typename enveloppe_swizzler<1>::type g;
			typename enveloppe_swizzler<2>::type b;
			typename enveloppe_swizzler<3>::type a;
		};

		struct {
			typename enveloppe_swizzler<0>::type s;
			typename enveloppe_swizzler<1>::type t;
			typename enveloppe_swizzler<2>::type p;
			typename enveloppe_swizzler<3>::type q;
		};

		typename enveloppe_swizzler<0, 0>::type xx, rr, ss;
		typename enveloppe_swizzler<0, 1>::type xy, rg, st;
		typename enveloppe_swizzler<0, 2>::type xz, rb, sp;
		typename enveloppe_swizzler<0, 3>::type xw, ra, sq;
		typename enveloppe_swizzler<1, 0>::type yx, gr, ts;
		typename enveloppe_swizzler<1, 1>::type yy, gg, tt;
		typename enveloppe_swizzler<1, 2>::type yz, gb, tp;
		typename enveloppe_swizzler<1, 3>::type yw, ga, tq;
		typename enveloppe_swizzler<2, 0>::type zx, br, ps;
		typename enveloppe_swizzler<2, 1>::type zy, bg, pt;
		typename enveloppe_swizzler<2, 2>::type zz, bb, pp;
		typename enveloppe_swizzler<2, 3>::type zw, ba, pq;
		typename enveloppe_swizzler<3, 0>::type wx, ar, qs;
		typename enveloppe_swizzler<3, 1>::type wy, ag, qt;
		typename enveloppe_swizzler<3, 2>::type wz, ab, qp;
		typename enveloppe_swizzler<3, 3>::type ww, aa, qq;
		typename enveloppe_swizzler<0, 0, 0>::type xxx, rrr, sss;
		typename enveloppe_swizzler<0, 0, 1>::type xxy, rrg, sst;
		typename enveloppe_swizzler<0, 0, 2>::type xxz, rrb, ssp;
		typename enveloppe_swizzler<0, 0, 3>::type xxw, rra, ssq;
		typename enveloppe_swizzler<0, 1, 0>::type xyx, rgr, sts;
		typename enveloppe_swizzler<0, 1, 1>::type xyy, rgg, stt;
		typename enveloppe_swizzler<0, 1, 2>::type xyz, rgb, stp;
		typename enveloppe_swizzler<0, 1, 3>::type xyw, rga, stq;
		typename enveloppe_swizzler<0, 2, 0>::type xzx, rbr, sps;
		typename enveloppe_swizzler<0, 2, 1>::type xzy, rbg, spt;
		typename enveloppe_swizzler<0, 2, 2>::type xzz, rbb, spp;
		typename enveloppe_swizzler<0, 2, 3>::type xzw, rba, spq;
		typename enveloppe_swizzler<0, 3, 0>::type xwx, rar, sqs;
		typename enveloppe_swizzler<0, 3, 1>::type xwy, rag, sqt;
		typename enveloppe_swizzler<0, 3, 2>::type xwz, rab, sqp;
		typename enveloppe_swizzler<0, 3, 3>::type xww, raa, sqq;
		typename enveloppe_swizzler<1, 0, 0>::type yxx, grr, tss;
		typename enveloppe_swizzler<1, 0, 1>::type yxy, grg, tst;
		typename enveloppe_swizzler<1, 0, 2>::type yxz, grb, tsp;
		typename enveloppe_swizzler<1, 0, 3>::type yxw, gra, tsq;
		typename enveloppe_swizzler<1, 1, 0>::type yyx, ggr, tts;
		typename enveloppe_swizzler<1, 1, 1>::type yyy, ggg, ttt;
		typename enveloppe_swizzler<1, 1, 2>::type yyz, ggb, ttp;
		typename enveloppe_swizzler<1, 1, 3>::type yyw, gga, ttq;
		typename enveloppe_swizzler<1, 2, 0>::type yzx, gbr, tps;
		typename enveloppe_swizzler<1, 2, 1>::type yzy, gbg, tpt;
		typename enveloppe_swizzler<1, 2, 2>::type yzz, gbb, tpp;
		typename enveloppe_swizzler<1, 2, 3>::type yzw, gba, tpq;
		typename enveloppe_swizzler<1, 3, 0>::type ywx, gar, tqs;
		typename enveloppe_swizzler<1, 3, 1>::type ywy, gag, tqt;
		typename enveloppe_swizzler<1, 3, 2>::type ywz, gab, tqp;
		typename enveloppe_swizzler<1, 3, 3>::type yww, gaa, tqq;
		typename enveloppe_swizzler<2, 0, 0>::type zxx, brr, pss;
		typename enveloppe_swizzler<2, 0, 1>::type zxy, brg, pst;
		typename enveloppe_swizzler<2, 0, 2>::type zxz, brb, psp;
		typename enveloppe_swizzler<2, 0, 3>::type zxw, bra, psq;
		typename enveloppe_swizzler<2, 1, 0>::type zyx, bgr, pts;
		typename enveloppe_swizzler<2, 1, 1>::type zyy, bgg, ptt;
		typename enveloppe_swizzler<2, 1, 2>::type zyz, bgb, ptp;
		typename enveloppe_swizzler<2, 1, 3>::type zyw, bga, ptq;
		typename enveloppe_swizzler<2, 2, 0>::type zzx, bbr, pps;
		typename enveloppe_swizzler<2, 2, 1>::type zzy, bbg, ppt;
		typename enveloppe_swizzler<2, 2, 2>::type zzz, bbb, ppp;
		typename enveloppe_swizzler<2, 2, 3>::type zzw, bba, ppq;
		typename enveloppe_swizzler<2, 3, 0>::type zwx, bar, pqs;
		typename enveloppe_swizzler<2, 3, 1>::type zwy, bag, pqt;
		typename enveloppe_swizzler<2, 3, 2>::type zwz, bab, pqp;
		typename enveloppe_swizzler<2, 3, 3>::type zww, baa, pqq;
		typename enveloppe_swizzler<3, 0, 0>::type wxx, arr, qss;
		typename enveloppe_swizzler<3, 0, 1>::type wxy, arg, qst;
		typename enveloppe_swizzler<3, 0, 2>::type wxz, arb, qsp;
		typename enveloppe_swizzler<3, 0, 3>::type wxw, ara, qsq;
		typename enveloppe_swizzler<3, 1, 0>::type wyx, agr, qts;
		typename enveloppe_swizzler<3, 1, 1>::type wyy, agg, qtt;
		typename enveloppe_swizzler<3, 1, 2>::type wyz, agb, qtp;
		typename enveloppe_swizzler<3, 1, 3>::type wyw, aga, qtq;
		typename enveloppe_swizzler<3, 2, 0>::type wzx, abr, qps;
		typename enveloppe_swizzler<3, 2, 1>::type wzy, abg, qpt;
		typename enveloppe_swizzler<3, 2, 2>::type wzz, abb, qpp;
		typename enveloppe_swizzler<3, 2, 3>::type wzw, aba, qpq;
		typename enveloppe_swizzler<3, 3, 0>::type wwx, aar, qqs;
		typename enveloppe_swizzler<3, 3, 1>::type wwy, aag, qqt;
		typename enveloppe_swizzler<3, 3, 2>::type wwz, aab, qqp;
		typename enveloppe_swizzler<3, 3, 3>::type www, aaa, qqq;
		typename enveloppe_swizzler<0, 0, 0, 0>::type xxxx, rrrr, ssss;
		typename enveloppe_swizzler<0, 0, 0, 1>::type xxxy, rrrg, ssst;
		typename enveloppe_swizzler<0, 0, 0, 2>::type xxxz, rrrb, sssp;
		typename enveloppe_swizzler<0, 0, 0, 3>::type xxxw, rrra, sssq;
		typename enveloppe_swizzler<0, 0, 1, 0>::type xxyx, rrgr, ssts;
		typename enveloppe_swizzler<0, 0, 1, 1>::type xxyy, rrgg, sstt;
		typename enveloppe_swizzler<0, 0, 1, 2>::type xxyz, rrgb, sstp;
		typename enveloppe_swizzler<0, 0, 1, 3>::type xxyw, rrga, sstq;
		typename enveloppe_swizzler<0, 0, 2, 0>::type xxzx, rrbr, ssps;
		typename enveloppe_swizzler<0, 0, 2, 1>::type xxzy, rrbg, sspt;
		typename enveloppe_swizzler<0, 0, 2, 2>::type xxzz, rrbb, sspp;
		typename enveloppe_swizzler<0, 0, 2, 3>::type xxzw, rrba, sspq;
		typename enveloppe_swizzler<0, 0, 3, 0>::type xxwx, rrar, ssqs;
		typename enveloppe_swizzler<0, 0, 3, 1>::type xxwy, rrag, ssqt;
		typename enveloppe_swizzler<0, 0, 3, 2>::type xxwz, rrab, ssqp;
		typename enveloppe_swizzler<0, 0, 3, 3>::type xxww, rraa, ssqq;
		typename enveloppe_swizzler<0, 1, 0, 0>::type xyxx, rgrr, stss;
		typename enveloppe_swizzler<0, 1, 0, 1>::type xyxy, rgrg, stst;
		typename enveloppe_swizzler<0, 1, 0, 2>::type xyxz, rgrb, stsp;
		typename enveloppe_swizzler<0, 1, 0, 3>::type xyxw, rgra, stsq;
		typename enveloppe_swizzler<0, 1, 1, 0>::type xyyx, rggr, stts;
		typename enveloppe_swizzler<0, 1, 1, 1>::type xyyy, rggg, sttt;
		typename enveloppe_swizzler<0, 1, 1, 2>::type xyyz, rggb, sttp;
		typename enveloppe_swizzler<0, 1, 1, 3>::type xyyw, rgga, sttq;
		typename enveloppe_swizzler<0, 1, 2, 0>::type xyzx, rgbr, stps;
		typename enveloppe_swizzler<0, 1, 2, 1>::type xyzy, rgbg, stpt;
		typename enveloppe_swizzler<0, 1, 2, 2>::type xyzz, rgbb, stpp;
		typename enveloppe_swizzler<0, 1, 2, 3>::type xyzw, rgba, stpq;
		typename enveloppe_swizzler<0, 1, 3, 0>::type xywx, rgar, stqs;
		typename enveloppe_swizzler<0, 1, 3, 1>::type xywy, rgag, stqt;
		typename enveloppe_swizzler<0, 1, 3, 2>::type xywz, rgab, stqp;
		typename enveloppe_swizzler<0, 1, 3, 3>::type xyww, rgaa, stqq;
		typename enveloppe_swizzler<0, 2, 0, 0>::type xzxx, rbrr, spss;
		typename enveloppe_swizzler<0, 2, 0, 1>::type xzxy, rbrg, spst;
		typename enveloppe_swizzler<0, 2, 0, 2>::type xzxz, rbrb, spsp;
		typename enveloppe_swizzler<0, 2, 0, 3>::type xzxw, rbra, spsq;
		typename enveloppe_swizzler<0, 2, 1, 0>::type xzyx, rbgr, spts;
		typename enveloppe_swizzler<0, 2, 1, 1>::type xzyy, rbgg, sptt;
		typename enveloppe_swizzler<0, 2, 1, 2>::type xzyz, rbgb, sptp;
		typename enveloppe_swizzler<0, 2, 1, 3>::type xzyw, rbga, sptq;
		typename enveloppe_swizzler<0, 2, 2, 0>::type xzzx, rbbr, spps;
		typename enveloppe_swizzler<0, 2, 2, 1>::type xzzy, rbbg, sppt;
		typename enveloppe_swizzler<0, 2, 2, 2>::type xzzz, rbbb, sppp;
		typename enveloppe_swizzler<0, 2, 2, 3>::type xzzw, rbba, sppq;
		typename enveloppe_swizzler<0, 2, 3, 0>::type xzwx, rbar, spqs;
		typename enveloppe_swizzler<0, 2, 3, 1>::type xzwy, rbag, spqt;
		typename enveloppe_swizzler<0, 2, 3, 2>::type xzwz, rbab, spqp;
		typename enveloppe_swizzler<0, 2, 3, 3>::type xzww, rbaa, spqq;
		typename enveloppe_swizzler<0, 3, 0, 0>::type xwxx, rarr, sqss;
		typename enveloppe_swizzler<0, 3, 0, 1>::type xwxy, rarg, sqst;
		typename enveloppe_swizzler<0, 3, 0, 2>::type xwxz, rarb, sqsp;
		typename enveloppe_swizzler<0, 3, 0, 3>::type xwxw, rara, sqsq;
		typename enveloppe_swizzler<0, 3, 1, 0>::type xwyx, ragr, sqts;
		typename enveloppe_swizzler<0, 3, 1, 1>::type xwyy, ragg, sqtt;
		typename enveloppe_swizzler<0, 3, 1, 2>::type xwyz, ragb, sqtp;
		typename enveloppe_swizzler<0, 3, 1, 3>::type xwyw, raga, sqtq;
		typename enveloppe_swizzler<0, 3, 2, 0>::type xwzx, rabr, sqps;
		typename enveloppe_swizzler<0, 3, 2, 1>::type xwzy, rabg, sqpt;
		typename enveloppe_swizzler<0, 3, 2, 2>::type xwzz, rabb, sqpp;
		typename enveloppe_swizzler<0, 3, 2, 3>::type xwzw, raba, sqpq;
		typename enveloppe_swizzler<0, 3, 3, 0>::type xwwx, raar, sqqs;
		typename enveloppe_swizzler<0, 3, 3, 1>::type xwwy, raag, sqqt;
		typename enveloppe_swizzler<0, 3, 3, 2>::type xwwz, raab, sqqp;
		typename enveloppe_swizzler<0, 3, 3, 3>::type xwww, raaa, sqqq;
		typename enveloppe_swizzler<1, 0, 0, 0>::type yxxx, grrr, tsss;
		typename enveloppe_swizzler<1, 0, 0, 1>::type yxxy, grrg, tsst;
		typename enveloppe_swizzler<1, 0, 0, 2>::type yxxz, grrb, tssp;
		typename enveloppe_swizzler<1, 0, 0, 3>::type yxxw, grra, tssq;
		typename enveloppe_swizzler<1, 0, 1, 0>::type yxyx, grgr, tsts;
		typename enveloppe_swizzler<1, 0, 1, 1>::type yxyy, grgg, tstt;
		typename enveloppe_swizzler<1, 0, 1, 2>::type yxyz, grgb, tstp;
		typename enveloppe_swizzler<1, 0, 1, 3>::type yxyw, grga, tstq;
		typename enveloppe_swizzler<1, 0, 2, 0>::type yxzx, grbr, tsps;
		typename enveloppe_swizzler<1, 0, 2, 1>::type yxzy, grbg, tspt;
		typename enveloppe_swizzler<1, 0, 2, 2>::type yxzz, grbb, tspp;
		typename enveloppe_swizzler<1, 0, 2, 3>::type yxzw, grba, tspq;
		typename enveloppe_swizzler<1, 0, 3, 0>::type yxwx, grar, tsqs;
		typename enveloppe_swizzler<1, 0, 3, 1>::type yxwy, grag, tsqt;
		typename enveloppe_swizzler<1, 0, 3, 2>::type yxwz, grab, tsqp;
		typename enveloppe_swizzler<1, 0, 3, 3>::type yxww, graa, tsqq;
		typename enveloppe_swizzler<1, 1, 0, 0>::type yyxx, ggrr, ttss;
		typename enveloppe_swizzler<1, 1, 0, 1>::type yyxy, ggrg, ttst;
		typename enveloppe_swizzler<1, 1, 0, 2>::type yyxz, ggrb, ttsp;
		typename enveloppe_swizzler<1, 1, 0, 3>::type yyxw, ggra, ttsq;
		typename enveloppe_swizzler<1, 1, 1, 0>::type yyyx, gggr, ttts;
		typename enveloppe_swizzler<1, 1, 1, 1>::type yyyy, gggg, tttt;
		typename enveloppe_swizzler<1, 1, 1, 2>::type yyyz, gggb, tttp;
		typename enveloppe_swizzler<1, 1, 1, 3>::type yyyw, ggga, tttq;
		typename enveloppe_swizzler<1, 1, 2, 0>::type yyzx, ggbr, ttps;
		typename enveloppe_swizzler<1, 1, 2, 1>::type yyzy, ggbg, ttpt;
		typename enveloppe_swizzler<1, 1, 2, 2>::type yyzz, ggbb, ttpp;
		typename enveloppe_swizzler<1, 1, 2, 3>::type yyzw, ggba, ttpq;
		typename enveloppe_swizzler<1, 1, 3, 0>::type yywx, ggar, ttqs;
		typename enveloppe_swizzler<1, 1, 3, 1>::type yywy, ggag, ttqt;
		typename enveloppe_swizzler<1, 1, 3, 2>::type yywz, ggab, ttqp;
		typename enveloppe_swizzler<1, 1, 3, 3>::type yyww, ggaa, ttqq;
		typename enveloppe_swizzler<1, 2, 0, 0>::type yzxx, gbrr, tpss;
		typename enveloppe_swizzler<1, 2, 0, 1>::type yzxy, gbrg, tpst;
		typename enveloppe_swizzler<1, 2, 0, 2>::type yzxz, gbrb, tpsp;
		typename enveloppe_swizzler<1, 2, 0, 3>::type yzxw, gbra, tpsq;
		typename enveloppe_swizzler<1, 2, 1, 0>::type yzyx, gbgr, tpts;
		typename enveloppe_swizzler<1, 2, 1, 1>::type yzyy, gbgg, tptt;
		typename enveloppe_swizzler<1, 2, 1, 2>::type yzyz, gbgb, tptp;
		typename enveloppe_swizzler<1, 2, 1, 3>::type yzyw, gbga, tptq;
		typename enveloppe_swizzler<1, 2, 2, 0>::type yzzx, gbbr, tpps;
		typename enveloppe_swizzler<1, 2, 2, 1>::type yzzy, gbbg, tppt;
		typename enveloppe_swizzler<1, 2, 2, 2>::type yzzz, gbbb, tppp;
		typename enveloppe_swizzler<1, 2, 2, 3>::type yzzw, gbba, tppq;
		typename enveloppe_swizzler<1, 2, 3, 0>::type yzwx, gbar, tpqs;
		typename enveloppe_swizzler<1, 2, 3, 1>::type yzwy, gbag, tpqt;
		typename enveloppe_swizzler<1, 2, 3, 2>::type yzwz, gbab, tpqp;
		typename enveloppe_swizzler<1, 2, 3, 3>::type yzww, gbaa, tpqq;
		typename enveloppe_swizzler<1, 3, 0, 0>::type ywxx, garr, tqss;
		typename enveloppe_swizzler<1, 3, 0, 1>::type ywxy, garg, tqst;
		typename enveloppe_swizzler<1, 3, 0, 2>::type ywxz, garb, tqsp;
		typename enveloppe_swizzler<1, 3, 0, 3>::type ywxw, gara, tqsq;
		typename enveloppe_swizzler<1, 3, 1, 0>::type ywyx, gagr, tqts;
		typename enveloppe_swizzler<1, 3, 1, 1>::type ywyy, gagg, tqtt;
		typename enveloppe_swizzler<1, 3, 1, 2>::type ywyz, gagb, tqtp;
		typename enveloppe_swizzler<1, 3, 1, 3>::type ywyw, gaga, tqtq;
		typename enveloppe_swizzler<1, 3, 2, 0>::type ywzx, gabr, tqps;
		typename enveloppe_swizzler<1, 3, 2, 1>::type ywzy, gabg, tqpt;
		typename enveloppe_swizzler<1, 3, 2, 2>::type ywzz, gabb, tqpp;
		typename enveloppe_swizzler<1, 3, 2, 3>::type ywzw, gaba, tqpq;
		typename enveloppe_swizzler<1, 3, 3, 0>::type ywwx, gaar, tqqs;
		typename enveloppe_swizzler<1, 3, 3, 1>::type ywwy, gaag, tqqt;
		typename enveloppe_swizzler<1, 3, 3, 2>::type ywwz, gaab, tqqp;
		typename enveloppe_swizzler<1, 3, 3, 3>::type ywww, gaaa, tqqq;
		typename enveloppe_swizzler<2, 0, 0, 0>::type zxxx, brrr, psss;
		typename enveloppe_swizzler<2, 0, 0, 1>::type zxxy, brrg, psst;
		typename enveloppe_swizzler<2, 0, 0, 2>::type zxxz, brrb, pssp;
		typename enveloppe_swizzler<2, 0, 0, 3>::type zxxw, brra, pssq;
		typename enveloppe_swizzler<2, 0, 1, 0>::type zxyx, brgr, psts;
		typename enveloppe_swizzler<2, 0, 1, 1>::type zxyy, brgg, pstt;
		typename enveloppe_swizzler<2, 0, 1, 2>::type zxyz, brgb, pstp;
		typename enveloppe_swizzler<2, 0, 1, 3>::type zxyw, brga, pstq;
		typename enveloppe_swizzler<2, 0, 2, 0>::type zxzx, brbr, psps;
		typename enveloppe_swizzler<2, 0, 2, 1>::type zxzy, brbg, pspt;
		typename enveloppe_swizzler<2, 0, 2, 2>::type zxzz, brbb, pspp;
		typename enveloppe_swizzler<2, 0, 2, 3>::type zxzw, brba, pspq;
		typename enveloppe_swizzler<2, 0, 3, 0>::type zxwx, brar, psqs;
		typename enveloppe_swizzler<2, 0, 3, 1>::type zxwy, brag, psqt;
		typename enveloppe_swizzler<2, 0, 3, 2>::type zxwz, brab, psqp;
		typename enveloppe_swizzler<2, 0, 3, 3>::type zxww, braa, psqq;
		typename enveloppe_swizzler<2, 1, 0, 0>::type zyxx, bgrr, ptss;
		typename enveloppe_swizzler<2, 1, 0, 1>::type zyxy, bgrg, ptst;
		typename enveloppe_swizzler<2, 1, 0, 2>::type zyxz, bgrb, ptsp;
		typename enveloppe_swizzler<2, 1, 0, 3>::type zyxw, bgra, ptsq;
		typename enveloppe_swizzler<2, 1, 1, 0>::type zyyx, bggr, ptts;
		typename enveloppe_swizzler<2, 1, 1, 1>::type zyyy, bggg, pttt;
		typename enveloppe_swizzler<2, 1, 1, 2>::type zyyz, bggb, pttp;
		typename enveloppe_swizzler<2, 1, 1, 3>::type zyyw, bgga, pttq;
		typename enveloppe_swizzler<2, 1, 2, 0>::type zyzx, bgbr, ptps;
		typename enveloppe_swizzler<2, 1, 2, 1>::type zyzy, bgbg, ptpt;
		typename enveloppe_swizzler<2, 1, 2, 2>::type zyzz, bgbb, ptpp;
		typename enveloppe_swizzler<2, 1, 2, 3>::type zyzw, bgba, ptpq;
		typename enveloppe_swizzler<2, 1, 3, 0>::type zywx, bgar, ptqs;
		typename enveloppe_swizzler<2, 1, 3, 1>::type zywy, bgag, ptqt;
		typename enveloppe_swizzler<2, 1, 3, 2>::type zywz, bgab, ptqp;
		typename enveloppe_swizzler<2, 1, 3, 3>::type zyww, bgaa, ptqq;
		typename enveloppe_swizzler<2, 2, 0, 0>::type zzxx, bbrr, ppss;
		typename enveloppe_swizzler<2, 2, 0, 1>::type zzxy, bbrg, ppst;
		typename enveloppe_swizzler<2, 2, 0, 2>::type zzxz, bbrb, ppsp;
		typename enveloppe_swizzler<2, 2, 0, 3>::type zzxw, bbra, ppsq;
		typename enveloppe_swizzler<2, 2, 1, 0>::type zzyx, bbgr, ppts;
		typename enveloppe_swizzler<2, 2, 1, 1>::type zzyy, bbgg, pptt;
		typename enveloppe_swizzler<2, 2, 1, 2>::type zzyz, bbgb, pptp;
		typename enveloppe_swizzler<2, 2, 1, 3>::type zzyw, bbga, pptq;
		typename enveloppe_swizzler<2, 2, 2, 0>::type zzzx, bbbr, ppps;
		typename enveloppe_swizzler<2, 2, 2, 1>::type zzzy, bbbg, pppt;
		typename enveloppe_swizzler<2, 2, 2, 2>::type zzzz, bbbb, pppp;
		typename enveloppe_swizzler<2, 2, 2, 3>::type zzzw, bbba, pppq;
		typename enveloppe_swizzler<2, 2, 3, 0>::type zzwx, bbar, ppqs;
		typename enveloppe_swizzler<2, 2, 3, 1>::type zzwy, bbag, ppqt;
		typename enveloppe_swizzler<2, 2, 3, 2>::type zzwz, bbab, ppqp;
		typename enveloppe_swizzler<2, 2, 3, 3>::type zzww, bbaa, ppqq;
		typename enveloppe_swizzler<2, 3, 0, 0>::type zwxx, barr, pqss;
		typename enveloppe_swizzler<2, 3, 0, 1>::type zwxy, barg, pqst;
		typename enveloppe_swizzler<2, 3, 0, 2>::type zwxz, barb, pqsp;
		typename enveloppe_swizzler<2, 3, 0, 3>::type zwxw, bara, pqsq;
		typename enveloppe_swizzler<2, 3, 1, 0>::type zwyx, bagr, pqts;
		typename enveloppe_swizzler<2, 3, 1, 1>::type zwyy, bagg, pqtt;
		typename enveloppe_swizzler<2, 3, 1, 2>::type zwyz, bagb, pqtp;
		typename enveloppe_swizzler<2, 3, 1, 3>::type zwyw, baga, pqtq;
		typename enveloppe_swizzler<2, 3, 2, 0>::type zwzx, babr, pqps;
		typename enveloppe_swizzler<2, 3, 2, 1>::type zwzy, babg, pqpt;
		typename enveloppe_swizzler<2, 3, 2, 2>::type zwzz, babb, pqpp;
		typename enveloppe_swizzler<2, 3, 2, 3>::type zwzw, baba, pqpq;
		typename enveloppe_swizzler<2, 3, 3, 0>::type zwwx, baar, pqqs;
		typename enveloppe_swizzler<2, 3, 3, 1>::type zwwy, baag, pqqt;
		typename enveloppe_swizzler<2, 3, 3, 2>::type zwwz, baab, pqqp;
		typename enveloppe_swizzler<2, 3, 3, 3>::type zwww, baaa, pqqq;
		typename enveloppe_swizzler<3, 0, 0, 0>::type wxxx, arrr, qsss;
		typename enveloppe_swizzler<3, 0, 0, 1>::type wxxy, arrg, qsst;
		typename enveloppe_swizzler<3, 0, 0, 2>::type wxxz, arrb, qssp;
		typename enveloppe_swizzler<3, 0, 0, 3>::type wxxw, arra, qssq;
		typename enveloppe_swizzler<3, 0, 1, 0>::type wxyx, argr, qsts;
		typename enveloppe_swizzler<3, 0, 1, 1>::type wxyy, argg, qstt;
		typename enveloppe_swizzler<3, 0, 1, 2>::type wxyz, argb, qstp;
		typename enveloppe_swizzler<3, 0, 1, 3>::type wxyw, arga, qstq;
		typename enveloppe_swizzler<3, 0, 2, 0>::type wxzx, arbr, qsps;
		typename enveloppe_swizzler<3, 0, 2, 1>::type wxzy, arbg, qspt;
		typename enveloppe_swizzler<3, 0, 2, 2>::type wxzz, arbb, qspp;
		typename enveloppe_swizzler<3, 0, 2, 3>::type wxzw, arba, qspq;
		typename enveloppe_swizzler<3, 0, 3, 0>::type wxwx, arar, qsqs;
		typename enveloppe_swizzler<3, 0, 3, 1>::type wxwy, arag, qsqt;
		typename enveloppe_swizzler<3, 0, 3, 2>::type wxwz, arab, qsqp;
		typename enveloppe_swizzler<3, 0, 3, 3>::type wxww, araa, qsqq;
		typename enveloppe_swizzler<3, 1, 0, 0>::type wyxx, agrr, qtss;
		typename enveloppe_swizzler<3, 1, 0, 1>::type wyxy, agrg, qtst;
		typename enveloppe_swizzler<3, 1, 0, 2>::type wyxz, agrb, qtsp;
		typename enveloppe_swizzler<3, 1, 0, 3>::type wyxw, agra, qtsq;
		typename enveloppe_swizzler<3, 1, 1, 0>::type wyyx, aggr, qtts;
		typename enveloppe_swizzler<3, 1, 1, 1>::type wyyy, aggg, qttt;
		typename enveloppe_swizzler<3, 1, 1, 2>::type wyyz, aggb, qttp;
		typename enveloppe_swizzler<3, 1, 1, 3>::type wyyw, agga, qttq;
		typename enveloppe_swizzler<3, 1, 2, 0>::type wyzx, agbr, qtps;
		typename enveloppe_swizzler<3, 1, 2, 1>::type wyzy, agbg, qtpt;
		typename enveloppe_swizzler<3, 1, 2, 2>::type wyzz, agbb, qtpp;
		typename enveloppe_swizzler<3, 1, 2, 3>::type wyzw, agba, qtpq;
		typename enveloppe_swizzler<3, 1, 3, 0>::type wywx, agar, qtqs;
		typename enveloppe_swizzler<3, 1, 3, 1>::type wywy, agag, qtqt;
		typename enveloppe_swizzler<3, 1, 3, 2>::type wywz, agab, qtqp;
		typename enveloppe_swizzler<3, 1, 3, 3>::type wyww, agaa, qtqq;
		typename enveloppe_swizzler<3, 2, 0, 0>::type wzxx, abrr, qpss;
		typename enveloppe_swizzler<3, 2, 0, 1>::type wzxy, abrg, qpst;
		typename enveloppe_swizzler<3, 2, 0, 2>::type wzxz, abrb, qpsp;
		typename enveloppe_swizzler<3, 2, 0, 3>::type wzxw, abra, qpsq;
		typename enveloppe_swizzler<3, 2, 1, 0>::type wzyx, abgr, qpts;
		typename enveloppe_swizzler<3, 2, 1, 1>::type wzyy, abgg, qptt;
		typename enveloppe_swizzler<3, 2, 1, 2>::type wzyz, abgb, qptp;
		typename enveloppe_swizzler<3, 2, 1, 3>::type wzyw, abga, qptq;
		typename enveloppe_swizzler<3, 2, 2, 0>::type wzzx, abbr, qpps;
		typename enveloppe_swizzler<3, 2, 2, 1>::type wzzy, abbg, qppt;
		typename enveloppe_swizzler<3, 2, 2, 2>::type wzzz, abbb, qppp;
		typename enveloppe_swizzler<3, 2, 2, 3>::type wzzw, abba, qppq;
		typename enveloppe_swizzler<3, 2, 3, 0>::type wzwx, abar, qpqs;
		typename enveloppe_swizzler<3, 2, 3, 1>::type wzwy, abag, qpqt;
		typename enveloppe_swizzler<3, 2, 3, 2>::type wzwz, abab, qpqp;
		typename enveloppe_swizzler<3, 2, 3, 3>::type wzww, abaa, qpqq;
		typename enveloppe_swizzler<3, 3, 0, 0>::type wwxx, aarr, qqss;
		typename enveloppe_swizzler<3, 3, 0, 1>::type wwxy, aarg, qqst;
		typename enveloppe_swizzler<3, 3, 0, 2>::type wwxz, aarb, qqsp;
		typename enveloppe_swizzler<3, 3, 0, 3>::type wwxw, aara, qqsq;
		typename enveloppe_swizzler<3, 3, 1, 0>::type wwyx, aagr, qqts;
		typename enveloppe_swizzler<3, 3, 1, 1>::type wwyy, aagg, qqtt;
		typename enveloppe_swizzler<3, 3, 1, 2>::type wwyz, aagb, qqtp;
		typename enveloppe_swizzler<3, 3, 1, 3>::type wwyw, aaga, qqtq;
		typename enveloppe_swizzler<3, 3, 2, 0>::type wwzx, aabr, qqps;
		typename enveloppe_swizzler<3, 3, 2, 1>::type wwzy, aabg, qqpt;
		typename enveloppe_swizzler<3, 3, 2, 2>::type wwzz, aabb, qqpp;
		typename enveloppe_swizzler<3, 3, 2, 3>::type wwzw, aaba, qqpq;
		typename enveloppe_swizzler<3, 3, 3, 0>::type wwwx, aaar, qqqs;
		typename enveloppe_swizzler<3, 3, 3, 1>::type wwwy, aaag, qqqt;
		typename enveloppe_swizzler<3, 3, 3, 2>::type wwwz, aaab, qqqp;
		typename enveloppe_swizzler<3, 3, 3, 3>::type wwww, aaaa, qqqq;
	};
};

}  /* namespace detail */
}  /* namespace dls::math */

#pragma GCC diagnostic pop

#pragma clang diagnostic pop
