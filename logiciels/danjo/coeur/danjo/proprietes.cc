/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "proprietes.hh"

#include "types/courbe_bezier.h"
#include "types/liste_manip.hh"
#include "types/rampe_couleur.h"

namespace danjo {

/* ------------------------------------------------------------------------- */
/** \name Propriété
 * \{ */

static std::any donne_valeur_défaut_pour_type(TypePropriete const type)
{
    switch (type) {
        case TypePropriete::ENTIER:
            return std::any(0);
        case TypePropriete::DECIMAL:
            return std::any(0.0f);
        case TypePropriete::VECTEUR_DECIMAL:
            return std::any(dls::math::vec3f(0));
        case TypePropriete::VECTEUR_ENTIER:
            return std::any(dls::math::vec3i(0));
        case TypePropriete::COULEUR:
            return std::any(dls::phys::couleur32(0));
        case TypePropriete::ENUM:
        case TypePropriete::FICHIER_ENTREE:
        case TypePropriete::FICHIER_SORTIE:
        case TypePropriete::CHAINE_CARACTERE:
        case TypePropriete::DOSSIER:
        case TypePropriete::TEXTE:
        case TypePropriete::LISTE:
            return std::any(dls::chaine(""));
        case TypePropriete::BOOL:
            return std::any(false);
        case TypePropriete::COURBE_COULEUR:
            return std::any(CourbeCouleur());
        case TypePropriete::COURBE_VALEUR:
        {
            auto courbe = CourbeBezier();
            cree_courbe_defaut(courbe);
            return std::any(courbe);
        }
        case TypePropriete::RAMPE_COULEUR:
        {
            auto rampe = RampeCouleur();
            cree_rampe_defaut(rampe);
            return std::any(rampe);
        }
        case TypePropriete::LISTE_MANIP:
        {
            auto liste = ListeManipulable();
            return std::any(liste);
        }
    }

    return {};
}

Propriete *Propriete::cree(TypePropriete const type)
{
    return Propriete::cree(type, donne_valeur_défaut_pour_type(type));
}

Propriete *Propriete::cree(TypePropriete type, std::any valeur_)
{
    auto résultat = memoire::loge<Propriete>("danjo::Propriete");
    résultat->type_ = type;
    résultat->valeur = valeur_;

    switch (type) {
        case TypePropriete::ENTIER:
        case TypePropriete::VECTEUR_ENTIER:
        {
            résultat->valeur_min.i = std::numeric_limits<int>::min();
            résultat->valeur_max.i = std::numeric_limits<int>::max();
            break;
        }
        case TypePropriete::DECIMAL:
        case TypePropriete::VECTEUR_DECIMAL:
        {
            résultat->valeur_min.f = -std::numeric_limits<float>::max();
            résultat->valeur_max.f = std::numeric_limits<float>::max();
            break;
        }
        case TypePropriete::COULEUR:
        {
            résultat->valeur_min.f = 0.0f;
            résultat->valeur_max.f = 1.0f;
            break;
        }
        case TypePropriete::ENUM:
        case TypePropriete::BOOL:
        case TypePropriete::LISTE:
        case TypePropriete::CHAINE_CARACTERE:
        case TypePropriete::TEXTE:
        case TypePropriete::FICHIER_ENTREE:
        case TypePropriete::FICHIER_SORTIE:
        case TypePropriete::DOSSIER:
        case TypePropriete::COURBE_COULEUR:
        case TypePropriete::COURBE_VALEUR:
        case TypePropriete::RAMPE_COULEUR:
        case TypePropriete::LISTE_MANIP:
        {
            break;
        }
    }

    return résultat;
}

std::string Propriete::donnne_infobulle() const
{
    return infobulle.c_str();
}

std::string Propriete::donnne_suffixe() const
{
    return suffixe.c_str();
}

std::string Propriete::donne_filtre_extensions() const
{
    return filtre_extension.c_str();
}

/* Évaluation des valeurs. */

bool Propriete::evalue_bool(int /*temps*/) const
{
    assert(type() == TypePropriete::BOOL);
    return std::any_cast<bool>(valeur);
}

int Propriete::evalue_entier(int temps) const
{
    assert(type() == TypePropriete::ENTIER);
    return évalue_valeur_impl<int>(temps);
}

float Propriete::evalue_decimal(int temps) const
{
    assert(type() == TypePropriete::DECIMAL);
    return évalue_valeur_impl<float>(temps);
}

void Propriete::evalue_vecteur_décimal(int temps, float *données) const
{
    // À FAIRE : vec2, vec4
    assert(type() == TypePropriete::VECTEUR_DECIMAL);
    auto val = évalue_valeur_impl<dls::math::vec3f>(temps);
    données[0] = val[0];
    données[1] = val[1];
    données[2] = val[2];
}

void Propriete::evalue_vecteur_entier(int temps, int *données) const
{
    assert(type() == TypePropriete::VECTEUR_ENTIER);
    auto val = évalue_valeur_impl<dls::math::vec3i>(temps);
    données[0] = val[0];
    données[1] = val[1];
    données[2] = val[2];
}

dls::phys::couleur32 Propriete::evalue_couleur(int temps) const
{
    assert(type() == TypePropriete::COULEUR);
    return évalue_valeur_impl<dls::phys::couleur32>(temps);
}

std::string Propriete::evalue_chaine(int /*temps*/) const
{
    dls::chaine chn = std::any_cast<dls::chaine>(valeur);
    return chn.c_str();
}

std::string Propriete::evalue_énum(int temps) const
{
    dls::chaine chn = std::any_cast<dls::chaine>(valeur);
    return chn.c_str();
}

/* Définition des valeurs. */
void Propriete::définis_valeur_entier(int valeur_)
{
    valeur = valeur_;
}
void Propriete::définis_valeur_décimal(float valeur_)
{
    valeur = valeur_;
}
void Propriete::définis_valeur_bool(bool valeur_)
{
    valeur = valeur_;
}
void Propriete::définis_valeur_vec3(dls::math::vec3f valeur_)
{
    valeur = valeur_;
}
void Propriete::définis_valeur_vec3(dls::math::vec3i valeur_)
{
    valeur = valeur_;
}
void Propriete::définis_valeur_couleur(dls::phys::couleur32 valeur_)
{
    valeur = valeur_;
}
void Propriete::définis_valeur_chaine(std::string const &valeur_)
{
    valeur = dls::chaine(valeur_.c_str());
}

void Propriete::définis_valeur_énum(const std::string &valeur_)
{
    valeur = dls::chaine(valeur_.c_str());
}

/* Plage des valeurs. */
plage_valeur<float> Propriete::plage_valeur_decimal() const
{
    return {valeur_min.f, valeur_max.f};
}
plage_valeur<int> Propriete::plage_valeur_entier() const
{
    return {valeur_min.i, valeur_max.i};
}
plage_valeur<float> Propriete::plage_valeur_vecteur_décimal() const
{
    return {valeur_min.f, valeur_max.f};
}
plage_valeur<int> Propriete::plage_valeur_vecteur_entier() const
{
    return {valeur_min.i, valeur_max.i};
}
plage_valeur<float> Propriete::plage_valeur_couleur() const
{
    return {valeur_min.f, valeur_max.f};
}

/* Animation des valeurs. */
void Propriete::ajoute_cle(const int v, int temps)
{
    assert(type() == TypePropriete::ENTIER);
    ajoute_cle_impl(std::any(v), temps);
}

void Propriete::ajoute_cle(const float v, int temps)
{
    assert(type() == TypePropriete::DECIMAL);
    ajoute_cle_impl(std::any(v), temps);
}

void Propriete::ajoute_cle(const dls::math::vec3f &v, int temps)
{
    assert(type() == TypePropriete::VECTEUR_DECIMAL);
    ajoute_cle_impl(std::any(v), temps);
}

void Propriete::ajoute_cle(const dls::math::vec3i &v, int temps)
{
    assert(type() == TypePropriete::VECTEUR_ENTIER);
    ajoute_cle_impl(std::any(v), temps);
}

void Propriete::ajoute_cle(const dls::phys::couleur32 &v, int temps)
{
    assert(type() == TypePropriete::COULEUR);
    ajoute_cle_impl(std::any(v), temps);
}

void Propriete::supprime_animation()
{
    courbe.efface();
}

bool Propriete::est_animee() const
{
    return !courbe.est_vide();
}

bool Propriete::possede_cle(int temps) const
{
    for (const auto &val : courbe) {
        if (val.first == temps) {
            return true;
        }
    }

    return false;
}

void Propriete::ajoute_cle_impl(const std::any &v, int temps)
{
    bool insere = false;
    long i = 0;

    for (; i < courbe.taille(); ++i) {
        if (courbe[i].first == temps) {
            courbe[i].second = v;
            insere = true;
            break;
        }

        if (courbe[i].first > temps) {
            courbe.insere(courbe.debut() + i, std::make_pair(temps, v));
            insere = true;
            break;
        }
    }

    if (!insere) {
        courbe.ajoute(std::make_pair(temps, v));
    }
}

bool Propriete::trouve_valeurs_temps(int temps, std::any &v1, std::any &v2, int &t1, int &t2) const
{
    bool v1_trouve = false;
    bool v2_trouve = false;

    for (auto i = 0; i < courbe.taille(); ++i) {
        if (courbe[i].first < temps) {
            v1 = courbe[i].second;
            t1 = courbe[i].first;
            v1_trouve = true;
            continue;
        }

        if (courbe[i].first == temps) {
            v1 = courbe[i].second;
            return true;
        }

        if (courbe[i].first > temps) {
            v2 = courbe[i].second;
            t2 = courbe[i].first;
            v2_trouve = true;
            break;
        }
    }

    if (!v1_trouve) {
        v1 = v2;
    }

    if (!v2_trouve) {
        v2 = v1;
    }

    return false;
}

template <typename T, typename S>
static inline T interp_lineaire(T a, S s, T b)
{
    return (1.0f - s) * a + s * b;
}

static inline int interp_lineaire(int a, float s, int b)
{
    return int((1.0f - s) * float(a) + s * float(b));
}

static inline dls::math::vec3i interp_lineaire(dls::math::vec3i a, float s, dls::math::vec3i b)
{
    return dls::math::vec3i(
        interp_lineaire(a.x, s, b.x), interp_lineaire(a.y, s, b.y), interp_lineaire(a.z, s, b.z));
}

template <typename T>
T Propriete::évalue_valeur_impl(int temps) const
{
    std::any v1, v2;
    int t1, t2;

    if (!trouve_valeurs_temps(temps, v1, v2, t1, t2)) {
        return std::any_cast<T>(valeur);
    }

    auto const dt = t2 - t1;
    auto const fac = float(temps - t1) / float(dt);
    auto const i1 = std::any_cast<T>(v1);
    auto const i2 = std::any_cast<T>(v2);

    return interp_lineaire(i1, fac, i2);
}

/** \} */

}  // namespace danjo
