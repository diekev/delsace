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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "programme.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "representation_intermediaire/instructions.hh"

#include "erreur.h"
#include "typage.hh"

Programme *Programme::cree(EspaceDeTravail *espace)
{
    Programme *resultat = memoire::loge<Programme>("Programme");
    resultat->m_espace = espace;
    return resultat;
}

Programme *Programme::cree_pour_espace(EspaceDeTravail *espace)
{
    return Programme::cree(espace);
}

Programme *Programme::cree_pour_metaprogramme(EspaceDeTravail *espace, MetaProgramme *metaprogramme)
{
    Programme *resultat = Programme::cree(espace);
    resultat->m_pour_metaprogramme = metaprogramme;
    return resultat;
}

void Programme::ajoute_fonction(NoeudDeclarationEnteteFonction *fonction)
{
    if (possede(fonction)) {
        return;
    }
    m_fonctions.ajoute(fonction);
    m_fonctions_utilisees.insere(fonction);
}

void Programme::ajoute_globale(NoeudDeclarationVariable *globale)
{
    if (possede(globale)) {
        return;
    }
    m_globales.ajoute(globale);
    m_globales_utilisees.insere(globale);
}

void Programme::ajoute_type(Type *type)
{
    if (possede(type)) {
        return;
    }
    m_types.ajoute(type);
    m_types_utilises.insere(type);
}

#undef DEBOGUE_VERIFICATIONS

bool Programme::typages_termines() const
{
    POUR (m_fonctions) {
        if (!it->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
#ifdef DEBOGUE_VERIFICATIONS
            std::cerr << "-- typage non terminé pour entête " << it->lexeme->chaine << '\n';
#endif
            return false;
        }

        if (!it->est_externe && !it->corps->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
#ifdef DEBOGUE_VERIFICATIONS
            std::cerr << "-- typage non terminé pour corps " << it->lexeme->chaine << '\n';
#endif
            return false;
        }
    }

    POUR (m_globales) {
        if (!it->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
#ifdef DEBOGUE_VERIFICATIONS
            std::cerr << "-- typage non terminé pour globale " << it->lexeme->chaine << '\n';
#endif
            return false;
        }
    }

    POUR (m_types) {
        if ((it->drapeaux & TYPE_FUT_VALIDE) == 0) {
#ifdef DEBOGUE_VERIFICATIONS
            std::cerr << "-- typage non terminé pour type " << chaine_type(it) << ", " << it
                      << '\n';
#endif
            return false;
        }
    }

    return true;
}

bool Programme::ri_generees() const
{
#ifdef DEBOGUE_VERIFICATIONS
    std::cerr << __func__ << '\n';
#endif
    if (!typages_termines()) {
#ifdef DEBOGUE_VERIFICATIONS
        std::cerr << "-- typages non terminés !\n";
#endif
        return false;
    }

    POUR (m_fonctions) {
        if (!it->possede_drapeau(RI_FUT_GENEREE)) {
            assert(it->unite);
#ifdef DEBOGUE_VERIFICATIONS
            std::cerr << "-- ri non générée pour fonction " << it->lexeme->chaine << '\n';
#endif
            return false;
        }
    }
#ifdef DEBOGUE_VERIFICATIONS
    std::cerr << "-- ri fonctions générées !\n";
#endif

    POUR (m_globales) {
        if (!it->possede_drapeau(RI_FUT_GENEREE)) {
            std::cerr << "-- ri non générée pour globale " << it->ident->nom << '\n';
            return false;
        }
    }
#ifdef DEBOGUE_VERIFICATIONS
    std::cerr << "-- ri globales générées !\n";
#endif

    POUR (m_types) {
        if ((it->drapeaux & RI_TYPE_FUT_GENEREE) == 0) {
            std::cerr << "-- ri non générée pour type " << chaine_type(it) << '\n';
            return false;
        }
    }
#ifdef DEBOGUE_VERIFICATIONS
    std::cerr << "-- ri types générées !\n";
#endif

    return true;
}

void Programme::ajoute_racine(NoeudDeclarationEnteteFonction *racine)
{
    if (pour_metaprogramme()) {
        /* Pour les métaprogrammes, nous n'ajoutons que les racines pour la création de l'exécutable. */
        if (racine->ident == ID::creation_contexte) {
            ajoute_fonction(racine);
        }
    }
    else {
        ajoute_fonction(racine);
    }
}

void imprime_contenu_programme(const Programme &programme, std::ostream &os)
{
    os << "Types dans le programme...\n";
    POUR (programme.types()) {
        os << "-- " << chaine_type(it) << '\n';
    }
    os << "Fonctions dans le programme...\n";
    POUR (programme.fonctions()) {
        os << "-- " << it->ident->nom << '\n';
    }
    os << "Globales dans le programme...\n";
    POUR (programme.globales()) {
        os << "-- " << it->ident->nom << '\n';
    }
}

// --------------------------------------------------------
// RepresentationIntermediaireProgramme

void imprime_contenu_programme(const ProgrammeRepreInter &programme, std::ostream &os)
{
    os << "Types dans le programme...\n";
    POUR (programme.types) {
        os << "-- " << chaine_type(it) << '\n';
    }
    os << "Fonctions dans le programme...\n";
    POUR (programme.fonctions) {
        if (it->ident) {
            os << "-- " << it->ident->nom << '\n';
        }
        else {
            os << "-- anonyme de type " << chaine_type(it->type) << '\n';
        }
    }
    os << "Globales dans le programme...\n";
    POUR (programme.globales) {
        if (it->ident) {
            os << "-- " << it->ident->nom << '\n';
        }
        else {
            os << "-- anonyme de type " << chaine_type(it->type) << '\n';
        }
    }
}

template <typename T>
static dls::ensemble<T> cree_ensemble(const kuri::tableau<T> &tableau)
{
    dls::ensemble<T> resultat;

    POUR (tableau) {
        resultat.insere(it);
    }

    return resultat;
}

/* La seule raison d'existence pour cette fonction est de rassembler les globales pour les chaines
 * et InfoType. */
static void rassemble_globales_supplementaire(ProgrammeRepreInter &repr_inter)
{
    auto globales_utilisess = cree_ensemble(repr_inter.globales);

    POUR (repr_inter.fonctions) {
        for (auto instruction : it->instructions) {
            visite_atome(instruction, [&](Atome *atome) {
                if (atome->genre_atome == Atome::Genre::GLOBALE) {
                    if (globales_utilisess.possede(static_cast<AtomeGlobale *>(atome))) {
                        return;
                    }

                    repr_inter.globales.ajoute(static_cast<AtomeGlobale *>(atome));
                    globales_utilisess.insere(static_cast<AtomeGlobale *>(atome));
                }
            });
        }
    }
}

struct VisiteuseType {
    dls::ensemble<Type *> visites{};

    void visite_type(Type *type, std::function<void(Type *)> rappel)
    {
        if (!type) {
            return;
        }

        if (visites.possede(type)) {
            return;
        }

        visites.insere(type);
        rappel(type);

        if (type->fonction_init) {
            visite_type(type->fonction_init->type, rappel);
        }

        switch (type->genre) {
            case GenreType::EINI:
            {
                break;
            }
            case GenreType::CHAINE:
            {
                break;
            }
            case GenreType::RIEN:
            case GenreType::BOOL:
            case GenreType::OCTET:
            case GenreType::ENTIER_CONSTANT:
            case GenreType::ENTIER_NATUREL:
            case GenreType::ENTIER_RELATIF:
            case GenreType::REEL:
            {
                break;
            }
            case GenreType::REFERENCE:
            {
                auto reference = type->comme_pointeur();
                visite_type(reference->type_pointe, rappel);
                break;
            }
            case GenreType::POINTEUR:
            {
                auto pointeur = type->comme_pointeur();
                visite_type(pointeur->type_pointe, rappel);
                break;
            }
            case GenreType::UNION:
            {
                break;
            }
            case GenreType::STRUCTURE:
            {
                break;
            }
            case GenreType::TABLEAU_DYNAMIQUE:
            {
                auto tableau = type->comme_tableau_dynamique();
                visite_type(tableau->type_pointe, rappel);
                break;
            }
            case GenreType::TABLEAU_FIXE:
            {
                auto tableau = type->comme_tableau_fixe();
                visite_type(tableau->type_pointe, rappel);
                break;
            }
            case GenreType::VARIADIQUE:
            {
                auto variadique = type->comme_variadique();
                visite_type(variadique->type_pointe, rappel);
                break;
            }
            case GenreType::FONCTION:
            {
                auto fonction = type->comme_fonction();
                POUR (fonction->types_entrees) {
                    visite_type(it, rappel);
                }
                visite_type(fonction->type_sortie, rappel);
                break;
            }
            case GenreType::ENUM:
            case GenreType::ERREUR:
            {
                break;
            }
            case GenreType::TYPE_DE_DONNEES:
            {
                break;
            }
            case GenreType::POLYMORPHIQUE:
            {
                break;
            }
            case GenreType::OPAQUE:
            {
                break;
            }
            case GenreType::TUPLE:
            {
                break;
            }
        }
    }
};

static void visite_type(Type *type, std::function<void(Type *)> rappel)
{
    VisiteuseType visiteuse{};
    visiteuse.visite_type(type, rappel);
}

ProgrammeRepreInter representation_intermediaire_programme(Programme const &programme,
                                                           EspaceDeTravail &espace)
{
    auto resultat = ProgrammeRepreInter{};

    resultat.fonctions.reserve(programme.fonctions().taille() + programme.types().taille());
    resultat.globales.reserve(programme.globales().taille());

    /* Nous pouvons directement copiés les types. */
    resultat.types = programme.types();

    /* Extrait les atomes pour les fonctions. */
    POUR (programme.fonctions()) {
        assert_rappel(it->possede_drapeau(RI_FUT_GENEREE), [&]() {
            std::cerr << "La RI ne fut pas généré pour:\n";
            erreur::imprime_site(espace, it);
        });
        assert_rappel(it->atome, [&]() {
            std::cerr << "Aucun atome pour:\n";
            erreur::imprime_site(espace, it);
        });
        resultat.fonctions.ajoute(static_cast<AtomeFonction *>(it->atome));
    }

    /* Extrait les atomes pour les fonctions d'initalisation des types. */
    POUR (programme.types()) {
        if (it->fonction_init) {
            resultat.fonctions.ajoute(it->fonction_init);
        }
    }

    /* Extrait les atomes pour les globales. */
    POUR (programme.globales()) {
        assert_rappel(it->possede_drapeau(RI_FUT_GENEREE), [&]() {
            std::cerr << "La RI ne fut pas généré pour:\n";
            erreur::imprime_site(espace, it);
        });
        assert_rappel(it->atome, [&]() {
            std::cerr << "Aucun atome pour:\n";
            erreur::imprime_site(espace, it);
            std::cerr << "Taille données decl  : " << it->donnees_decl.taille() << '\n';
            std::cerr << "Possède substitution : " << (it->substitution != nullptr) << '\n';
        });
        resultat.globales.ajoute(static_cast<AtomeGlobale *>(it->atome));
    }

    /* Traverse les instructions des fonctions, et rassemble les globales pour les chaines, les
     * tableaux, et les infos-types. */
    rassemble_globales_supplementaire(resultat);

    {
        /* Ajoute les types de toutes les globales et toutes les fonctions, dans le cas où nous en
         * aurions ajoutées (qui ne sont pas dans le programme initiale). */
        auto type_utilises = cree_ensemble(resultat.types);

        POUR (resultat.fonctions) {
            visite_type(it->type, [&](Type *type) {
                if (type_utilises.possede(type)) {
                    return;
                }

                type_utilises.insere(type);
                resultat.types.ajoute(type);
            });
        }

        POUR (resultat.globales) {
            visite_type(it->type, [&](Type *type) {
                if (type_utilises.possede(type)) {
                    return;
                }

                type_utilises.insere(type);
                resultat.types.ajoute(type);
            });
        }
    }

    return resultat;
}
