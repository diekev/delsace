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

#include "operatrices_attributs.hh"

#include <mutex>

#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/nombre_decimaux/quatification.h"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/arbre_kd.hh"
#include "biblinternes/structures/dico_fixe.hh"

#include "coeur/base_de_donnees.hh"
#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/nuanceur.hh"
#include "coeur/objet.h"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/iteration_corps.hh"

#include "evaluation/reseau.hh"

#include "normaux.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceCreationAttribut final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Création Attribut";
    static constexpr auto AIDE = "Crée un attribut.";

    OperatriceCreationAttribut(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_3d_creation_attribut.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto const nom_attribut = evalue_chaine("nom_attribut");
        auto const chaine_type = evalue_enum("type_attribut");
        auto const chaine_portee = evalue_enum("portee_attribut");
        auto const chaine_dims = evalue_enum("dimensions_attr");
        auto const chaine_prec = evalue_enum("précision_attr");

        if (nom_attribut == "") {
            ajoute_avertissement("Le nom de l'attribut est vide !");
            return res_exec::ECHOUEE;
        }

        auto dico_precisions = dls::cree_dico(dls::paire{dls::chaine("8"), 8},
                                              dls::paire{dls::chaine("16"), 16},
                                              dls::paire{dls::chaine("32"), 32},
                                              dls::paire{dls::chaine("64"), 64});

        auto dico_dimensions = dls::cree_dico(dls::paire{dls::chaine("1"), 1},
                                              dls::paire{dls::chaine("2"), 2},
                                              dls::paire{dls::chaine("3"), 3},
                                              dls::paire{dls::chaine("4"), 4},
                                              dls::paire{dls::chaine("9"), 9},
                                              dls::paire{dls::chaine("16"), 16});

        auto dico_portee = dls::cree_dico(
            dls::paire{dls::chaine("corps"), portee_attr::CORPS},
            dls::paire{dls::chaine("groupe"), portee_attr::GROUPE},
            dls::paire{dls::chaine("points"), portee_attr::POINT},
            dls::paire{dls::chaine("primitives"), portee_attr::PRIMITIVE},
            dls::paire{dls::chaine("sommets"), portee_attr::VERTEX});

        auto plg_prec = dico_precisions.trouve(chaine_prec);

        if (plg_prec.est_finie()) {
            ajoute_avertissement("Précision d'attribut '", chaine_prec, "' invalide !");
            return res_exec::ECHOUEE;
        }

        auto precision = plg_prec.front().second;

        auto plg_dim = dico_dimensions.trouve(chaine_dims);

        if (plg_dim.est_finie()) {
            ajoute_avertissement("Dimensions d'attribut '", chaine_dims, "' invalide !");
            return res_exec::ECHOUEE;
        }

        auto dimensions = plg_dim.front().second;

        auto plg_portee = dico_portee.trouve(chaine_portee);

        if (plg_portee.est_finie()) {
            ajoute_avertissement("Portée d'attribut '", chaine_portee, "' invalide !");
            return res_exec::ECHOUEE;
        }

        auto portee = plg_portee.front().second;

        type_attribut type;

        if (chaine_type == "naturel") {
            if (precision == 8) {
                type = type_attribut::N8;
            }
            else if (precision == 16) {
                type = type_attribut::N16;
            }
            else if (precision == 32) {
                type = type_attribut::N32;
            }
            else {
                type = type_attribut::N64;
            }
        }
        else if (chaine_type == "relatif") {
            if (precision == 8) {
                type = type_attribut::Z8;
            }
            else if (precision == 16) {
                type = type_attribut::Z16;
            }
            else if (precision == 32) {
                type = type_attribut::Z32;
            }
            else {
                type = type_attribut::Z64;
            }
        }
        else if (chaine_type == "réél") {
            if (precision == 8) {
                ajoute_avertissement("Un nombre réel ne peut avoir une précision de 8-bit !");
                return res_exec::ECHOUEE;
            }
            else if (precision == 16) {
                type = type_attribut::Z16;
            }
            else if (precision == 32) {
                type = type_attribut::Z32;
            }
            else {
                type = type_attribut::Z64;
            }
        }
        else if (chaine_type == "chaine") {
            type = type_attribut::CHAINE;
        }
        else {
            ajoute_avertissement("Type d'attribut '", chaine_type, "' invalide !");
            return res_exec::ECHOUEE;
        }

        m_corps.ajoute_attribut(nom_attribut, type, dimensions, portee);

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

class OperatriceSuppressionAttribut final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Suppression Attribut";
    static constexpr auto AIDE = "Supprime un attribut.";

    OperatriceSuppressionAttribut(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_3d_suppression_attribut.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto const nom_attribut = evalue_chaine("nom_attribut");

        if (nom_attribut == "") {
            ajoute_avertissement("Le nom de l'attribut est vide !");
            return res_exec::ECHOUEE;
        }

        m_corps.supprime_attribut(nom_attribut);

        return res_exec::REUSSIE;
    }

    void obtiens_liste(ContexteEvaluation const &contexte,
                       dls::chaine const &attache,
                       dls::tableau<dls::chaine> &chaines) override
    {
        INUTILISE(contexte);
        if (attache == "nom_attribut") {
            entree(0)->obtiens_liste_attributs(chaines);
        }
    }
};

/* ************************************************************************** */

enum class op_rand_attr {
    REMPLACE,
    AJOUTE,
    MULTIPLIE,
    MINIMUM,
    MAXIMUM,
};

template <typename T>
auto applique_op(op_rand_attr op, T const &a, T const &b)
{
    switch (op) {
        case op_rand_attr::REMPLACE:
        {
            return b;
        }
        case op_rand_attr::AJOUTE:
        {
            /* L'addition de 'char' convertie en 'int'. */
            return static_cast<T>(a + b);
        }
        case op_rand_attr::MULTIPLIE:
        {
            /* La multiplication de 'char' convertie en 'int'. */
            return static_cast<T>(a * b);
        }
        case op_rand_attr::MINIMUM:
        {
            return (a < b) ? a : b;
        }
        case op_rand_attr::MAXIMUM:
        {
            return (a > b) ? a : b;
        }
    }

    return b;
}

struct params_randomisation {
    dls::chaine distribution = "";
    int graine = 0;
    op_rand_attr operation{};
    float constante = 0.0f;
    float moyenne = 0.0f;
    float ecart_type = 0.0f;
    float val_min = 0.0f;
    float val_max = 0.0f;
};

template <typename T>
auto applique_randomisation(Attribut &attr, params_randomisation const &params)
{
    auto gna = GNA(static_cast<unsigned long>(params.graine));

    if (params.distribution == "constante") {
        transforme_attr<T>(attr, [&](T *ptr) {
            for (auto i = 0; i < attr.dimensions; ++i) {
                ptr[i] = applique_op(params.operation, ptr[i], static_cast<T>(params.constante));
            }
        });
    }
    else if (params.distribution == "uniforme") {
        transforme_attr<T>(attr, [&](T *ptr) {
            for (auto i = 0; i < attr.dimensions; ++i) {
                ptr[i] = applique_op(params.operation,
                                     ptr[i],
                                     static_cast<T>(gna.uniforme(params.val_min, params.val_max)));
            }
        });
    }
    else if (params.distribution == "gaussienne") {
        transforme_attr<T>(attr, [&](T *ptr) {
            for (auto i = 0; i < attr.dimensions; ++i) {
                ptr[i] = applique_op(
                    params.operation,
                    ptr[i],
                    static_cast<T>(gna.normale(params.moyenne, params.ecart_type)));
            }
        });
    }
}

class OperatriceRandomisationAttribut final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Randomisation Attribut";
    static constexpr auto AIDE = "Randomise un attribut.";

    OperatriceRandomisationAttribut(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_3d_randomisation_attribut.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto const nom_attribut = evalue_chaine("nom_attribut");
        auto const graine = evalue_entier("graine", contexte.temps_courant);
        auto const distribution = evalue_enum("distribution");
        auto const constante = evalue_decimal("constante");
        auto const val_min = evalue_decimal("valeur_min", contexte.temps_courant);
        auto const val_max = evalue_decimal("valeur_max", contexte.temps_courant);
        auto const moyenne = evalue_decimal("moyenne", contexte.temps_courant);
        auto const ecart_type = evalue_decimal("écart_type", contexte.temps_courant);
        auto const enum_operation = evalue_enum("opération");

        if (nom_attribut == "") {
            ajoute_avertissement("Le nom de l'attribut est vide !");
            return res_exec::ECHOUEE;
        }

        auto attrib = m_corps.attribut(nom_attribut);

        if (attrib == nullptr) {
            ajoute_avertissement("Aucun attribut ne correspond au nom spécifié !");
            return res_exec::ECHOUEE;
        }

        op_rand_attr operation;

        if (enum_operation == "remplace") {
            operation = op_rand_attr::REMPLACE;
        }
        else if (enum_operation == "ajoute") {
            operation = op_rand_attr::AJOUTE;
        }
        else if (enum_operation == "multiplie") {
            operation = op_rand_attr::MULTIPLIE;
        }
        else if (enum_operation == "minimum") {
            operation = op_rand_attr::MINIMUM;
        }
        else if (enum_operation == "maximum") {
            operation = op_rand_attr::MAXIMUM;
        }
        else {
            ajoute_avertissement("Opération '", enum_operation, "' inconnue !");
            return res_exec::ECHOUEE;
        }

        auto params = params_randomisation{};
        params.distribution = distribution;
        params.graine = graine;
        params.moyenne = moyenne;
        params.ecart_type = ecart_type;
        params.constante = constante;
        params.operation = operation;
        params.val_min = val_min;
        params.val_max = val_max;

        switch (attrib->type()) {
            case type_attribut::N8:
            {
                applique_randomisation<unsigned char>(*attrib, params);
                break;
            }
            case type_attribut::N16:
            {
                applique_randomisation<unsigned short>(*attrib, params);
                break;
            }
            case type_attribut::N32:
            {
                applique_randomisation<unsigned int>(*attrib, params);
                break;
            }
            case type_attribut::N64:
            {
                applique_randomisation<unsigned long>(*attrib, params);
                break;
            }
            case type_attribut::Z8:
            {
                applique_randomisation<char>(*attrib, params);
                break;
            }
            case type_attribut::Z16:
            {
                applique_randomisation<short>(*attrib, params);
                break;
            }
            case type_attribut::Z32:
            {
                applique_randomisation<int>(*attrib, params);
                break;
            }
            case type_attribut::Z64:
            {
                applique_randomisation<long>(*attrib, params);
                break;
            }
            case type_attribut::R16:
            {
                applique_randomisation<r16>(*attrib, params);
                break;
            }
            case type_attribut::R32:
            {
                applique_randomisation<float>(*attrib, params);
                break;
            }
            case type_attribut::R64:
            {
                applique_randomisation<double>(*attrib, params);
                break;
            }
            case type_attribut::CHAINE:
            {
                ajoute_avertissement(
                    "La randomisation d'attribut de type chaine n'est pas supportée !");
                break;
            }
            case type_attribut::INVALIDE:
            {
                ajoute_avertissement("Type d'attribut invalide !");
                break;
            }
        }

        return res_exec::REUSSIE;
    }

    bool ajourne_proprietes() override
    {
        auto const distribution = evalue_enum("distribution");

        rend_propriete_visible("constante", distribution == "constante");
        rend_propriete_visible("graine", distribution != "constante");
        rend_propriete_visible("valeur_min", distribution == "uniforme");
        rend_propriete_visible("valeur_max", distribution == "uniforme");
        rend_propriete_visible("moyenne", distribution == "gaussienne");
        rend_propriete_visible("écart_type", distribution == "gaussienne");

        return true;
    }

    void obtiens_liste(ContexteEvaluation const &contexte,
                       dls::chaine const &attache,
                       dls::tableau<dls::chaine> &chaines) override
    {
        INUTILISE(contexte);
        if (attache == "nom_attribut") {
            entree(0)->obtiens_liste_attributs(chaines);
        }
    }
};

/* ************************************************************************** */

class OperatriceAjoutCouleur final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Couleur";
    static constexpr auto AIDE = "Ajoute un attribut de couleur à la géométrie entrante.";

    OperatriceAjoutCouleur(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_attr_couleur.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto const graine = evalue_entier("graine", contexte.temps_courant);
        auto const couleur_ = evalue_couleur("couleur_");
        auto const methode = evalue_enum("méthode");
        auto const chaine_portee = evalue_enum("portée");
        auto const nom_groupe = evalue_chaine("nom_groupe");

        auto attrib = static_cast<Attribut *>(nullptr);
        auto groupe_points = static_cast<GroupePoint *>(nullptr);
        auto groupe_prims = static_cast<GroupePrimitive *>(nullptr);
        portee_attr portee;

        if (chaine_portee == "points") {
            portee = portee_attr::POINT;

            if (nom_groupe != "") {
                groupe_points = m_corps.groupe_point(nom_groupe);

                if (groupe_points == nullptr) {
                    ajoute_avertissement("Groupe '", nom_groupe, "' inconnu !");
                    return res_exec::ECHOUEE;
                }
            }
        }
        else if (chaine_portee == "primitives") {
            portee = portee_attr::PRIMITIVE;

            if (nom_groupe != "") {
                groupe_prims = m_corps.groupe_primitive(nom_groupe);

                if (groupe_prims == nullptr) {
                    ajoute_avertissement("Groupe '", nom_groupe, "' inconnu !");
                    return res_exec::ECHOUEE;
                }
            }
        }
        else if (chaine_portee == "devine_groupe") {
            if (nom_groupe == "") {
                ajoute_avertissement("Le nom du groupe est vide !");
                return res_exec::ECHOUEE;
            }

            groupe_points = m_corps.groupe_point(nom_groupe);

            if (groupe_points == nullptr) {
                groupe_prims = m_corps.groupe_primitive(nom_groupe);

                if (groupe_prims == nullptr) {
                    ajoute_avertissement("Groupe '", nom_groupe, "' inconnu !");
                    return res_exec::ECHOUEE;
                }

                portee = portee_attr::PRIMITIVE;
            }
            else {
                portee = portee_attr::POINT;
            }
        }
        else {
            ajoute_avertissement("Portée '", chaine_portee, "' non-supportée !");
            return res_exec::ECHOUEE;
        }

        attrib = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee);

        iteratrice_index iter;

        if (groupe_points != nullptr) {
            iter = iteratrice_index(groupe_points);
        }
        else if (groupe_prims != nullptr) {
            iter = iteratrice_index(groupe_prims);
        }
        else {
            iter = iteratrice_index(attrib->taille());
        }

        if (methode == "unique") {
            for (auto index : iter) {
                assigne(attrib->r32(index), dls::math::vec3f(couleur_.r, couleur_.v, couleur_.b));
            }
        }
        else if (methode == "aléatoire") {
            auto gna = GNA(static_cast<unsigned long>(graine));

            for (auto index : iter) {
                assigne(attrib->r32(index), gna.uniforme_vec3(0.0f, 1.0f));
            }
        }

        return res_exec::REUSSIE;
    }

    bool ajourne_proprietes() override
    {
        auto const methode = evalue_enum("méthode");
        auto const methode_aleatoire = methode == "aléatoire";

        rend_propriete_visible("graine", methode_aleatoire);
        rend_propriete_visible("couleur_", !methode_aleatoire);

        return true;
    }

    void obtiens_liste(ContexteEvaluation const &contexte,
                       dls::chaine const &attache,
                       dls::tableau<dls::chaine> &chaines) override
    {
        INUTILISE(contexte);
        if (attache == "nom_groupe") {
            entree(0)->obtiens_liste_groupes_points(chaines);
            entree(0)->obtiens_liste_groupes_prims(chaines);
        }
    }
};

/* ************************************************************************** */

class OperatriceCreationNormaux final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Création Normaux";
    static constexpr auto AIDE = "Crée des normaux pour les maillages d'entrée.";

    OperatriceCreationNormaux(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_3d_normaux.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto chaine_location = evalue_chaine("location");
        auto chaine_pesee = evalue_enum("pesée");
        auto inverse_normaux = evalue_bool("inverse_direction");

        if (!valide_corps_entree(*this, &m_corps, true, true)) {
            return res_exec::ECHOUEE;
        }

        auto location = location_normal::POINT;

        if (chaine_location == "primitive") {
            location = location_normal::PRIMITIVE;
        }
        else if (chaine_location == "corps") {
            location = location_normal::CORPS;
        }
        else if (chaine_location != "point") {
            ajoute_avertissement("Méthode de location '", chaine_location, "' inconnue");
            return res_exec::ECHOUEE;
        }

        auto pesee = pesee_normal::AIRE;

        if (chaine_pesee == "angle") {
            pesee = pesee_normal::ANGLE;
        }
        else if (chaine_pesee == "max") {
            pesee = pesee_normal::MAX;
        }
        else if (chaine_pesee == "moyenne") {
            pesee = pesee_normal::MOYENNE;
        }
        else if (chaine_pesee != "aire") {
            ajoute_avertissement("Méthode de pesée '", chaine_location, "' inconnue");
            return res_exec::ECHOUEE;
        }

        calcul_normaux(m_corps, location, pesee, inverse_normaux);

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

// quantifie et déquantifie sur 15-bits (32768 = 2 ^ 15)
inline unsigned short quantifie(float f)
{
    return static_cast<unsigned short>(f * (32768.0f - 1.0f)) & 0x7fff;
}

inline auto dequantifie(unsigned short i)
{
    return static_cast<float>(i) * (1.0f / (32768.0f - 1.0f));
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
// https://johnwhite3d.blogspot.com/2017/10/signed-octahedron-normal-encoding.html
class OperatriceCompressionNormaux final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Compression Normaux";
    static constexpr auto AIDE = "Compresse les normaux dans un nouvel attribut pour les "
                                 "maillages par une projection octahédrale.";

    OperatriceCompressionNormaux(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_3d_compression_normaux.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto chaine_nom_attribut_entree = evalue_chaine("nom_attribut_entrée");
        auto chaine_nom_attribut_sortie = evalue_chaine("nom_attribut_sortie");
        auto visualise = evalue_bool("visualise");

        if (!valide_corps_entree(*this, &m_corps, true, true)) {
            return res_exec::ECHOUEE;
        }

        auto attr_N = m_corps.attribut(chaine_nom_attribut_entree);

        if (attr_N == nullptr) {
            this->ajoute_avertissement("Aucun attribut d'entrée de ce nom !");
            return res_exec::ECHOUEE;
        }

        auto nattr_N = m_corps.ajoute_attribut(
            chaine_nom_attribut_sortie, type_attribut::N32, 1, attr_N->portee);
        nattr_N->redimensionne(attr_N->taille());

        auto encode = [](dls::math::vec3f n) {
            n /= (abs(n.x) + abs(n.y) + abs(n.z));

            auto ny = n.y * 0.5f + 0.5f;
            auto nx = n.x * 0.5f + ny;
            ny = n.x * -0.5f + ny;

            auto nz = dls::math::restreint(n.z * std::numeric_limits<float>::max(), 0.0f, 1.0f);
            return dls::math::vec3f(nx, ny, nz);
        };

        auto decode = [](dls::math::vec3f n) {
            dls::math::vec3f resultat;
            resultat.x = (n.x - n.y);
            resultat.y = (n.x + n.y) - 1.0f;
            resultat.z = n.z * 2.0f - 1.0f;
            resultat.z = resultat.z * (1.0f - abs(resultat.x) - abs(resultat.y));
            return normalise(resultat);
        };

        auto quantifie_vec3 = [](dls::math::vec3f const &v) {
            auto qx = static_cast<unsigned int>(quantifie(v.x));
            auto qy = static_cast<unsigned int>(quantifie(v.y));
            auto qz = (v.z > 0.0f) ? 0u : 1u;

            return (qx << 17 | qy << 2 | qz);
        };

        auto dequantifie_vec3 = [](unsigned int q) {
            auto ix = (q >> 17) & 0x7fff;
            auto iy = (q >> 2) & 0x7fff;
            auto iz = q & 0x3;

            auto x = dequantifie(static_cast<unsigned short>(ix));
            auto y = dequantifie(static_cast<unsigned short>(iy));
            auto z = (iz) ? 0.0f : 1.0f;

            return dls::math::vec3f(x, y, z);
        };

        for (auto i = 0; i < attr_N->taille(); ++i) {
            auto ptr = attr_N->r32(i);
            auto n = dls::math::vec3f(ptr[0], ptr[1], ptr[2]);
            auto r = encode(n);
            auto q = quantifie_vec3(r);
            assigne(nattr_N->n32(i), q);
        }

        if (visualise) {
            auto nattr_C = m_corps.ajoute_attribut("C", type_attribut::R32, 3, attr_N->portee);
            nattr_C->reinitialise();
            nattr_C->redimensionne(attr_N->taille());

            for (auto i = 0; i < nattr_N->taille(); ++i) {
                auto ptr = nattr_N->n32(i);
                auto w = dequantifie_vec3(ptr[0]);
                auto c = decode(w);
                assigne(nattr_C->r32(i), c);
            }
        }

        return res_exec::REUSSIE;
    }
};

/* ************************************************************************** */

class OpTransfereAttributs final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Transfère Attributs";
    static constexpr auto AIDE = "";

    OpTransfereAttributs(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(2);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_transfere_attribut.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto corps_orig = entree(1)->requiers_corps(contexte, donnees_aval);

        if (!valide_corps_entree(*this, corps_orig, true, true, 1)) {
            return res_exec::ECHOUEE;
        }

        auto points = m_corps.points_pour_lecture();
        auto points_orig = corps_orig->points_pour_lecture();

        auto const nom_attribut = evalue_chaine("nom_attribut");
        auto const distance = evalue_decimal("distance", contexte.temps_courant);

        auto attr_orig = corps_orig->attribut(nom_attribut);

        if (attr_orig == nullptr) {
            this->ajoute_avertissement(
                "Le corps d'origine ne possède pas l'attribut '", nom_attribut, "'");
            return res_exec::ECHOUEE;
        }

        if (attr_orig->portee != portee_attr::POINT) {
            this->ajoute_avertissement("L'attribut '", nom_attribut, "' n'est pas sur les points");
            return res_exec::ECHOUEE;
        }

        auto chef = contexte.chef;
        chef->demarre_evaluation("transfère attribut");

        /* À FAIRE : collision attribut. */
        auto attr_dest = m_corps.ajoute_attribut(
            nom_attribut, attr_orig->type(), attr_orig->dimensions, attr_orig->portee);

        auto arbre = arbre_3df();
        arbre.construit_avec_fonction(static_cast<int>(points_orig.taille()),
                                      [&](int idx) { return points_orig.point_local(idx); });

        boucle_parallele(
            tbb::blocked_range<long>(0, points.taille()),
            [&](tbb::blocked_range<long> const &plage) {
                for (auto i = plage.begin(); i < plage.end(); ++i) {
                    auto const point = points.point_monde(i);
                    auto idx_point_plus_pres = -1;

                    arbre.trouve_plus_proche_index(point, distance, idx_point_plus_pres);

                    if (idx_point_plus_pres >= 0) {
                        copie_attribut(attr_orig, idx_point_plus_pres, attr_dest, i);
                    }
                }

                auto delta = static_cast<float>(plage.end() - plage.begin()) * 100.0f;
                chef->indique_progression_parallele(delta / static_cast<float>(points.taille()));
            });

        return res_exec::REUSSIE;
    }

    void obtiens_liste(ContexteEvaluation const &contexte,
                       dls::chaine const &attache,
                       dls::tableau<dls::chaine> &chaines) override
    {
        INUTILISE(contexte);
        if (attache == "nom_attribut") {
            entree(1)->obtiens_liste_attributs(chaines);
        }
    }
};

/* ************************************************************************** */

struct donnees_promotion {
    long idx_dest{};

    /* la promotion se fait en assignant une valeur depuis un index d'origine
     * avec un poids défini selon la portée, l'index est le premier élément de
     * la paire, et le poids le second */
    dls::tableau<std::pair<long, float>> paires_idx_poids{};
};

template <typename T>
static auto copie_attributs(Attribut &attr_dst,
                            Attribut const &attr_src,
                            dls::tableau<donnees_promotion> const &donnees)
{
    auto poids = dls::tableau<float>(attr_dst.taille());

    for (auto const &donnee : donnees) {
        for (auto const &p : donnee.paires_idx_poids) {
            auto ptr_dst = attr_dst.valeur<T>(donnee.idx_dest);
            auto ptr_src = attr_src.valeur<T>(p.first);

            for (auto i = 0; i < attr_dst.dimensions; ++i) {
                /* la syntaxe est étrange car C++ converti les chars en ints et
                 * il faut reconvertir les ints en chars */
                ptr_dst[i] = T(ptr_dst[i] + ptr_src[i] * T(p.second));
            }

            poids[donnee.idx_dest] += p.second;
        }
    }

    for (auto i = 0; i < attr_dst.taille(); ++i) {
        auto pds = poids[i];

        if (pds == 0.0f || pds == 1.0f) {
            continue;
        }

        auto ptr_dst = attr_dst.valeur<T>(i);

        for (auto j = 0; j < attr_dst.dimensions; ++j) {
            /* la syntaxe est étrange car C++ converti les chars en ints et
             * il faut reconvertir les ints en chars */
            ptr_dst[j] = T(ptr_dst[j] / T(pds));
        }
    }
}

static auto promeut_attribut(Corps &corps, Attribut &attr_orig, portee_attr portee_dest)
{
    auto portee_orig = attr_orig.portee;

    auto attr_dest = corps.ajoute_attribut(
        attr_orig.nom() + "_promotion", attr_orig.type(), attr_orig.dimensions, portee_dest);

    dls::tableau<donnees_promotion> donnees;
    donnees.reserve(attr_dest->taille());

    if (portee_orig == portee_attr::POINT) {
        if (portee_dest == portee_attr::PRIMITIVE) {
            /* moyenne des attributs des points autour de la primitive */
            pour_chaque_polygone(corps, [&](Corps const &corps_entree, Polygone *prim) {
                INUTILISE(corps_entree);

                auto donnee = donnees_promotion{};
                donnee.idx_dest = prim->index;

                auto nombre_points = prim->nombre_sommets();

                for (auto i = 0l; i < nombre_points; ++i) {
                    donnee.paires_idx_poids.ajoute(
                        {prim->index_point(i), 1.0f / static_cast<float>(nombre_points)});
                }

                donnees.ajoute(donnee);
            });
        }
        else if (portee_dest == portee_attr::VERTEX) {
            /* attribut du point de ce vertex */
            pour_chaque_polygone(corps, [&](Corps const &corps_entree, Polygone *prim) {
                INUTILISE(corps_entree);

                auto nombre_points = prim->nombre_sommets();

                for (auto i = 0l; i < nombre_points; ++i) {
                    auto donnee = donnees_promotion{};
                    donnee.idx_dest = prim->index_sommet(i);
                    donnee.paires_idx_poids.ajoute({prim->index_point(i), 1.0f});
                    donnees.ajoute(donnee);
                }
            });
        }
        else if (portee_dest == portee_attr::CORPS) {
            /* moyenne de tous les attributs */
            auto donnee = donnees_promotion{};
            donnee.idx_dest = 0;

            for (auto i = 0l; i < attr_orig.taille(); ++i) {
                donnee.paires_idx_poids.ajoute(
                    {i, static_cast<float>(i) / static_cast<float>(attr_orig.taille())});
            }

            donnees.ajoute(donnee);
        }
    }
    else if (portee_orig == portee_attr::PRIMITIVE) {
        if (portee_dest == portee_attr::POINT) {
            /* moyenne des attributs des primitives autour du point */
            pour_chaque_polygone(corps, [&](Corps const &corps_entree, Polygone *prim) {
                INUTILISE(corps_entree);

                auto nombre_points = prim->nombre_sommets();

                for (auto i = 0l; i < nombre_points; ++i) {
                    auto donnee = donnees_promotion{};
                    donnee.idx_dest = prim->index_point(i);
                    donnee.paires_idx_poids.ajoute({prim->index, 1.0f});
                    donnees.ajoute(donnee);
                }
            });
        }
        else if (portee_dest == portee_attr::VERTEX) {
            /* attribut de la primitive contenant le vertex */
            pour_chaque_polygone(corps, [&](Corps const &corps_entree, Polygone *prim) {
                INUTILISE(corps_entree);

                auto nombre_points = prim->nombre_sommets();

                for (auto i = 0l; i < nombre_points; ++i) {
                    auto donnee = donnees_promotion{};
                    donnee.idx_dest = prim->index_sommet(i);
                    donnee.paires_idx_poids.ajoute({prim->index, 1.0f});
                    donnees.ajoute(donnee);
                }
            });
        }
        else if (portee_dest == portee_attr::CORPS) {
            /* moyenne de tous les attributs */
            auto donnee = donnees_promotion{};
            donnee.idx_dest = 0;

            for (auto i = 0l; i < attr_orig.taille(); ++i) {
                donnee.paires_idx_poids.ajoute(
                    {i, static_cast<float>(i) / static_cast<float>(attr_orig.taille())});
            }

            donnees.ajoute(donnee);
        }
    }
    else if (portee_orig == portee_attr::VERTEX) {
        if (portee_dest == portee_attr::POINT) {
            /* moyenne des attributs des vertex autour du point */
            pour_chaque_polygone(corps, [&](Corps const &corps_entree, Polygone *prim) {
                INUTILISE(corps_entree);

                auto nombre_points = prim->nombre_sommets();

                for (auto i = 0l; i < nombre_points; ++i) {
                    auto donnee = donnees_promotion{};
                    donnee.idx_dest = prim->index_point(i);
                    donnee.paires_idx_poids.ajoute({prim->index_sommet(i), 1.0f});
                    donnees.ajoute(donnee);
                }
            });
        }
        else if (portee_dest == portee_attr::PRIMITIVE) {
            /* moyenne des attributs des vertex autour de la primitive */
            pour_chaque_polygone(corps, [&](Corps const &corps_entree, Polygone *prim) {
                INUTILISE(corps_entree);

                auto nombre_points = prim->nombre_sommets();

                for (auto i = 0l; i < nombre_points; ++i) {
                    auto donnee = donnees_promotion{};
                    donnee.idx_dest = prim->index;
                    donnee.paires_idx_poids.ajoute(
                        {prim->index_sommet(i), 1.0f / static_cast<float>(nombre_points)});
                    donnees.ajoute(donnee);
                }
            });
        }
        else if (portee_dest == portee_attr::CORPS) {
            /* moyenne de tous les attributs */
            auto donnee = donnees_promotion{};
            donnee.idx_dest = 0;

            for (auto i = 0l; i < attr_orig.taille(); ++i) {
                donnee.paires_idx_poids.ajoute(
                    {i, static_cast<float>(i) / static_cast<float>(attr_orig.taille())});
            }

            donnees.ajoute(donnee);
        }
    }
    else if (portee_orig == portee_attr::CORPS) {
        /* peut importe la portée de destination, l'attribut est le même */
        for (auto i = 0l; i < attr_dest->taille(); ++i) {
            auto donnee = donnees_promotion{};
            donnee.idx_dest = i;
            donnee.paires_idx_poids.ajoute({0, 1.0f});

            donnees.ajoute(donnee);
        }
    }
    else if (portee_orig == portee_attr::GROUPE) {
        /* À FAIRE : promotion attribut groupe */
    }

    switch (attr_dest->type()) {
        case type_attribut::N8:
        {
            copie_attributs<unsigned char>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::N16:
        {
            copie_attributs<unsigned short>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::N32:
        {
            copie_attributs<unsigned int>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::N64:
        {
            copie_attributs<unsigned long>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::Z8:
        {
            copie_attributs<char>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::Z16:
        {
            copie_attributs<short>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::Z32:
        {
            copie_attributs<int>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::Z64:
        {
            copie_attributs<long>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::R16:
        {
            copie_attributs<r16>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::R32:
        {
            copie_attributs<float>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::R64:
        {
            copie_attributs<double>(*attr_dest, attr_orig, donnees);
            break;
        }
        case type_attribut::CHAINE:
        case type_attribut::INVALIDE:
        {
            return attr_dest;
        }
    }

    auto const &nom = attr_orig.nom();
    corps.supprime_attribut(nom);
    attr_dest->nom(nom);

    return attr_dest;
}

class OpPromeutAttribut final : public OperatriceCorps {
  public:
    static constexpr auto NOM = "Promotion Attribut";
    static constexpr auto AIDE = "";

    OpPromeutAttribut(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
        sorties(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_attribut_promotion.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto const nom_attribut = evalue_chaine("nom_attribut");

        auto attr_src = m_corps.attribut(nom_attribut);

        if (attr_src == nullptr) {
            this->ajoute_avertissement(
                "Le corps d'origine ne possède pas l'attribut '", nom_attribut, "'");
            return res_exec::ECHOUEE;
        }

        if (attr_src->type() == type_attribut::CHAINE) {
            this->ajoute_avertissement("Le transfère de chaine n'est pas encore supporté");
            return res_exec::ECHOUEE;
        }

        if (attr_src->type() == type_attribut::INVALIDE) {
            this->ajoute_avertissement("Le type d'attribut est invalide");
            return res_exec::ECHOUEE;
        }

        auto chaine_portee = evalue_enum("portée_attribut");

        auto dico_portee = dls::cree_dico(
            dls::paire{dls::chaine("corps"), portee_attr::CORPS},
            dls::paire{dls::chaine("groupe"), portee_attr::GROUPE},
            dls::paire{dls::chaine("points"), portee_attr::POINT},
            dls::paire{dls::chaine("primitives"), portee_attr::PRIMITIVE},
            dls::paire{dls::chaine("sommets"), portee_attr::VERTEX});

        auto plg_portee = dico_portee.trouve(chaine_portee);

        if (plg_portee.est_finie()) {
            ajoute_avertissement("Portée d'attribut '", chaine_portee, "' invalide !");
            return res_exec::ECHOUEE;
        }

        auto portee_dst = plg_portee.front().second;

        if (portee_dst != attr_src->portee) {
            auto preserve_lum = evalue_bool("préserve_lum");

            /* préseve la luminosité en appliquant une transformation gamma
             * inverse aux couleurs avant de les interpoler, puis restore le
             * gamma, peut-être inutile
             * voir http://www.iquilezles.org/www/articles/gamma/gamma.htm */
            if (preserve_lum && attr_src->type() == type_attribut::R32) {
                transforme_attr<float>(*attr_src, [&](float *ptr) {
                    for (auto i = 0; i < attr_src->dimensions; ++i) {
                        ptr[i] = std::pow(ptr[i], 2.2f);
                    }
                });
            }

            auto attr_dst = promeut_attribut(m_corps, *attr_src, portee_dst);

            if (preserve_lum && attr_dst->type() == type_attribut::R32) {
                transforme_attr<float>(*attr_dst, [&](float *ptr) {
                    for (auto i = 0; i < attr_dst->dimensions; ++i) {
                        ptr[i] = std::pow(ptr[i], 0.45f);
                    }
                });
            }
        }

        return res_exec::REUSSIE;
    }

    void obtiens_liste(ContexteEvaluation const &contexte,
                       dls::chaine const &attache,
                       dls::tableau<dls::chaine> &chaines) override
    {
        INUTILISE(contexte);
        if (attache == "nom_attribut") {
            entree(0)->obtiens_liste_attributs(chaines);
        }
    }
};

/* ************************************************************************** */

class OpVisibiliteCamera final : public OperatriceCorps {
    dls::chaine m_nom_objet = "";
    Objet *m_objet = nullptr;

  public:
    static constexpr auto NOM = "Visibilité Caméra";
    static constexpr auto AIDE =
        "Ajoute un attribut selon la distance des points depuis une caméra.";

    OpVisibiliteCamera(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
    }

    COPIE_CONSTRUCT(OpVisibiliteCamera);

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_visibilite_camera.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    Objet *trouve_objet(ContexteEvaluation const &contexte)
    {
        auto nom_objet = evalue_chaine("nom_caméra");

        if (nom_objet.est_vide()) {
            return nullptr;
        }

        if (nom_objet != m_nom_objet || m_objet == nullptr) {
            m_nom_objet = nom_objet;
            m_objet = contexte.bdd->objet(nom_objet);
        }

        return m_objet;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        m_objet = trouve_objet(contexte);

        if (m_objet == nullptr) {
            this->ajoute_avertissement("Ne peut pas trouver l'objet caméra !");
            return res_exec::ECHOUEE;
        }

        if (m_objet->type != type_objet::CAMERA) {
            this->ajoute_avertissement("L'objet n'est pas une caméra !");
            return res_exec::ECHOUEE;
        }

        auto camera = static_cast<vision::Camera3D *>(nullptr);

        m_objet->donnees.accede_ecriture(
            [&](DonneesObjet *donnees) { camera = &extrait_camera(donnees); });

        auto nom_attribut = evalue_chaine("nom_attribut");

        if (nom_attribut == "") {
            nom_attribut = "distance";
        }

        auto attr_D = m_corps.ajoute_attribut(
            nom_attribut, type_attribut::R32, 1, portee_attr::POINT);
        auto points = m_corps.points_pour_lecture();

        auto marge_x = evalue_decimal("marge_x");
        auto marge_y = evalue_decimal("marge_y");

        marge_x *= static_cast<float>(camera->largeur());
        marge_y *= static_cast<float>(camera->hauteur());

        auto ajoute_groupe_visible = evalue_bool("ajoute_groupe_visible");
        auto ajoute_groupe_invisible = evalue_bool("ajoute_groupe_invisible");

        auto groupe_visible = static_cast<GroupePoint *>(nullptr);
        auto groupe_invisible = static_cast<GroupePoint *>(nullptr);

        if (ajoute_groupe_visible) {
            auto nom_groupe_visible = evalue_chaine("nom_groupe_visible");

            if (nom_groupe_visible == "") {
                nom_groupe_visible = "visible";
            }

            groupe_visible = m_corps.ajoute_groupe_point(nom_groupe_visible);
        }

        if (ajoute_groupe_invisible) {
            auto nom_groupe_invisible = evalue_chaine("nom_groupe_invisible");

            if (nom_groupe_invisible == "") {
                nom_groupe_invisible = "invisible";
            }

            groupe_invisible = m_corps.ajoute_groupe_point(nom_groupe_invisible);
        }

        auto l_min = constantes<float>::INFINITE;
        auto l_max = -constantes<float>::INFINITE;

        for (auto i = 0; i < points.taille(); ++i) {
            auto p = points.point_monde(i);

            auto pos_ecran = camera->pos_ecran(dls::math::point3f(p));

            if (pos_ecran.x < -marge_x ||
                pos_ecran.x > marge_x + static_cast<float>(camera->largeur())) {
                assigne(attr_D->r32(i), -1.0f);

                if (groupe_invisible) {
                    groupe_invisible->ajoute_index(i);
                }

                continue;
            }

            if (pos_ecran.y < -marge_y ||
                pos_ecran.y > marge_y + static_cast<float>(camera->hauteur())) {
                assigne(attr_D->r32(i), -1.0f);

                if (groupe_invisible) {
                    groupe_invisible->ajoute_index(i);
                }

                continue;
            }

            auto vec = p - camera->pos();

            if (produit_scalaire(vec, camera->dir()) <= 0.0f) {
                assigne(attr_D->r32(i), -1.0f);

                if (groupe_invisible) {
                    groupe_invisible->ajoute_index(i);
                }

                continue;
            }

            auto l = longueur(vec);

            if (l < l_min) {
                l_min = l;
            }

            if (l > l_max) {
                l_max = l;
            }

            if (groupe_visible) {
                groupe_visible->ajoute_index(i);
            }

            assigne(attr_D->r32(i), l);
        }

        auto normalise = evalue_bool("normalise");
        auto visualise = evalue_bool("visualise");
        auto inverse = evalue_bool("inverse");

        if (!visualise && !normalise && !inverse) {
            return res_exec::REUSSIE;
        }

        auto attr_C = static_cast<Attribut *>(nullptr);

        if (visualise) {
            attr_C = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::POINT);
        }

        auto poids_normalise = 1.0f / (l_max - l_min);

        for (auto i = 0; i < points.taille(); ++i) {
            auto ptr_D = attr_D->r32(i);

            if (ptr_D[0] >= 0.0f) {
                if (normalise) {
                    ptr_D[0] = 1.0f - (l_max - ptr_D[0]) * poids_normalise;

                    if (inverse) {
                        ptr_D[0] = 1.0f - ptr_D[0];
                    }
                }
                else if (inverse) {
                    ptr_D[0] = l_max - ptr_D[0];
                }
            }
            else {
                ptr_D[0] = 0.0f;
            }

            if (visualise) {
                auto fac = ptr_D[0];

                if (!normalise) {
                    fac = 1.0f - (l_max - fac) * poids_normalise;
                }

                auto clr = dls::phys::couleur_depuis_poids(fac);
                auto ptr_C = attr_C->r32(i);

                for (int j = 0; j < 3; ++j) {
                    ptr_C[j] = clr[j];
                }
            }
        }

        return res_exec::REUSSIE;
    }

    void renseigne_dependance(ContexteEvaluation const &contexte,
                              CompilatriceReseau &compilatrice,
                              NoeudReseau *noeud_reseau) override
    {
        if (m_objet == nullptr) {
            m_objet = trouve_objet(contexte);

            if (m_objet == nullptr) {
                return;
            }
        }

        compilatrice.ajoute_dependance(noeud_reseau, m_objet->noeud);
    }

    void obtiens_liste(ContexteEvaluation const &contexte,
                       dls::chaine const &raison,
                       dls::tableau<dls::chaine> &liste) override
    {
        if (raison == "nom_caméra") {
            for (auto &objet : contexte.bdd->objets()) {
                liste.ajoute(objet->noeud->nom);
            }
        }
    }

    void performe_versionnage() override
    {
        if (propriete("nom_groupe_invisible") == nullptr) {
            ajoute_propriete(
                "nom_attribut", danjo::TypePropriete::CHAINE_CARACTERE, dls::chaine("distance"));
            ajoute_propriete("inverse", danjo::TypePropriete::BOOL, true);
            ajoute_propriete("visualise", danjo::TypePropriete::BOOL, true);
            ajoute_propriete("normalise", danjo::TypePropriete::BOOL, true);
            ajoute_propriete("nom_groupe_visible",
                             danjo::TypePropriete::CHAINE_CARACTERE,
                             dls::chaine("visible"));
            ajoute_propriete("nom_groupe_invisible",
                             danjo::TypePropriete::CHAINE_CARACTERE,
                             dls::chaine("invisible"));
            ajoute_propriete("ajoute_groupe_visible", danjo::TypePropriete::BOOL, false);
            ajoute_propriete("ajoute_groupe_invisible", danjo::TypePropriete::BOOL, true);
            ajoute_propriete("marge_x", danjo::TypePropriete::DECIMAL, 0.0f);
            ajoute_propriete("marge_y", danjo::TypePropriete::DECIMAL, 0.0f);
        }
    }
};

/* ************************************************************************** */

struct OpAttributNuanceur final : public OperatriceCorps {
    static constexpr auto NOM = "Attribut Nuanceur";
    static constexpr auto AIDE =
        "Ajoute un attribut selon la distance des points depuis une caméra.";

    OpAttributNuanceur(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_attribut_nuanceur.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto nom_nuanceur = evalue_chaine("nom_nuanceur");

        if (nom_nuanceur.est_vide()) {
            this->ajoute_avertissement("Le nom du nuanceur est vide");
            return res_exec::ECHOUEE;
        }

        auto nuanceur = contexte.bdd->nuanceur(nom_nuanceur);

        if (nuanceur == nullptr) {
            this->ajoute_avertissement("Le nuanceur n'existe pas");
            return res_exec::ECHOUEE;
        }

        auto chaine_portee = evalue_enum("portée_attribut");

        auto dico_portee = dls::cree_dico(
            dls::paire{dls::chaine("corps"), portee_attr::CORPS},
            dls::paire{dls::chaine("groupe"), portee_attr::GROUPE},
            dls::paire{dls::chaine("points"), portee_attr::POINT},
            dls::paire{dls::chaine("primitives"), portee_attr::PRIMITIVE},
            dls::paire{dls::chaine("sommets"), portee_attr::VERTEX});

        auto plg_portee = dico_portee.trouve(chaine_portee);

        if (plg_portee.est_finie()) {
            ajoute_avertissement("Portée d'attribut '", chaine_portee, "' invalide !");
            return res_exec::ECHOUEE;
        }

        auto portee = plg_portee.front().second;

        auto attr = m_corps.ajoute_attribut("nuanceur", type_attribut::CHAINE, 1, portee);

        for (auto i = 0; i < attr->taille(); ++i) {
            assigne(attr->chaine(i), nom_nuanceur);
        }

        return res_exec::REUSSIE;
    }

    void obtiens_liste(ContexteEvaluation const &contexte,
                       dls::chaine const &raison,
                       dls::tableau<dls::chaine> &liste) override
    {
        if (raison == "nom_nuanceur") {
            for (auto &nuanceur : contexte.bdd->nuanceurs()) {
                liste.ajoute(nuanceur->noeud.nom);
            }
        }
    }
};

/* ************************************************************************** */

struct OpImprimeAttribut final : public OperatriceCorps {
    static constexpr auto NOM = "Imprime Attribut";
    static constexpr auto AIDE = "Imprime les valeurs d'un attribut dans la sortie standard.";

    OpImprimeAttribut(Graphe &graphe_parent, Noeud &noeud_)
        : OperatriceCorps(graphe_parent, noeud_)
    {
        entrees(1);
    }

    ResultatCheminEntreface chemin_entreface() const override
    {
        return CheminFichier{"entreface/operatrice_attribut_detail.jo"};
    }

    const char *nom_classe() const override
    {
        return NOM;
    }

    const char *texte_aide() const override
    {
        return AIDE;
    }

    res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
    {
        m_corps.reinitialise();
        entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

        auto nom_attribut = evalue_chaine("nom_attribut");

        if (nom_attribut.est_vide()) {
            this->ajoute_avertissement("Le nom du nuanceur est vide");
            return res_exec::ECHOUEE;
        }

        auto attribut = m_corps.attribut(nom_attribut);

        if (attribut == nullptr) {
            this->ajoute_avertissement("L'attribut '", nom_attribut, "' n'existe pas");
            return res_exec::ECHOUEE;
        }

        for (auto i = 0; i < attribut->taille(); ++i) {
            std::cerr << "valeur " << i << " : ";

            switch (attribut->type()) {
                case type_attribut::N8:
                {
                    std::cerr << *attribut->n8(i);
                    break;
                }
                case type_attribut::Z8:
                {
                    std::cerr << *attribut->z8(i);
                    break;
                }
                case type_attribut::N16:
                {
                    std::cerr << *attribut->n16(i);
                    break;
                }
                case type_attribut::Z16:
                {
                    std::cerr << *attribut->z16(i);
                    break;
                }
                case type_attribut::N32:
                {
                    std::cerr << *attribut->n32(i);
                    break;
                }
                case type_attribut::Z32:
                {
                    std::cerr << *attribut->z32(i);
                    break;
                }
                case type_attribut::N64:
                {
                    std::cerr << *attribut->n64(i);
                    break;
                }
                case type_attribut::Z64:
                {
                    std::cerr << *attribut->z64(i);
                    break;
                }
                case type_attribut::R16:
                {
                    break;
                }
                case type_attribut::R32:
                {
                    std::cerr << *attribut->r32(i);
                    break;
                }
                case type_attribut::R64:
                {
                    std::cerr << *attribut->r64(i);
                    break;
                }
                case type_attribut::CHAINE:
                {
                    std::cerr << *attribut->chaine(i);
                    break;
                }
                case type_attribut::INVALIDE:
                {
                    break;
                }
            }

            std::cerr << '\n';
        }

        return res_exec::REUSSIE;
    }

    void obtiens_liste(ContexteEvaluation const &contexte,
                       dls::chaine const &raison,
                       dls::tableau<dls::chaine> &liste) override
    {
        if (raison == "nom_nuanceur") {
            for (auto &nuanceur : contexte.bdd->nuanceurs()) {
                liste.ajoute(nuanceur->noeud.nom);
            }
        }
    }
};

/* ************************************************************************** */

void enregistre_operatrices_attributs(UsineOperatrice &usine)
{
    usine.enregistre_type(cree_desc<OperatriceCreationAttribut>());
    usine.enregistre_type(cree_desc<OperatriceAjoutCouleur>());
    usine.enregistre_type(cree_desc<OperatriceSuppressionAttribut>());
    usine.enregistre_type(cree_desc<OperatriceRandomisationAttribut>());
    usine.enregistre_type(cree_desc<OperatriceCreationNormaux>());
    usine.enregistre_type(cree_desc<OperatriceCompressionNormaux>());
    usine.enregistre_type(cree_desc<OpTransfereAttributs>());
    usine.enregistre_type(cree_desc<OpPromeutAttribut>());
    usine.enregistre_type(cree_desc<OpVisibiliteCamera>());
    usine.enregistre_type(cree_desc<OpAttributNuanceur>());
    usine.enregistre_type(cree_desc<OpImprimeAttribut>());
}

#pragma clang diagnostic pop
