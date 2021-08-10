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

#include "coulisse.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "typage.hh"

Programme *Programme::cree(EspaceDeTravail *espace)
{
    Programme *resultat = memoire::loge<Programme>("Programme");
    resultat->m_espace = espace;
    return resultat;
}

Programme *Programme::cree_pour_espace(EspaceDeTravail *espace)
{
    auto resultat = Programme::cree(espace);
    resultat->m_coulisse = Coulisse::cree_pour_options(espace->options);
    return resultat;
}

Programme *Programme::cree_pour_metaprogramme(EspaceDeTravail *espace,
                                              MetaProgramme *metaprogramme)
{
    Programme *resultat = Programme::cree(espace);
    resultat->m_pour_metaprogramme = metaprogramme;
    resultat->m_coulisse = Coulisse::cree_pour_metaprogramme();
    return resultat;
}

void Programme::ajoute_fonction(NoeudDeclarationEnteteFonction *fonction)
{
    if (possede(fonction)) {
        return;
    }
    m_fonctions.ajoute(fonction);
    m_fonctions_utilisees.insere(fonction);
    ajoute_fichier(m_espace->fichier(fonction->lexeme->fichier));
}

void Programme::ajoute_globale(NoeudDeclarationVariable *globale)
{
    if (possede(globale)) {
        return;
    }
    m_globales.ajoute(globale);
    m_globales_utilisees.insere(globale);
    ajoute_fichier(m_espace->fichier(globale->lexeme->fichier));
}

void Programme::ajoute_type(Type *type)
{
    if (possede(type)) {
        return;
    }
    m_types.ajoute(type);
    m_types_utilises.insere(type);
}

bool Programme::typages_termines(DiagnostiqueEtatCompilation &diagnostique) const
{
    POUR (m_fonctions) {
        if (!it->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            diagnostique.declaration_a_valider = it;
            return false;
        }

        if (!it->est_externe && !it->corps->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            diagnostique.declaration_a_valider = it;
            return false;
        }
    }

    POUR (m_globales) {
        if (!it->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            diagnostique.declaration_a_valider = it;
            return false;
        }
    }

    POUR (m_types) {
        if ((it->drapeaux & TYPE_FUT_VALIDE) == 0) {
            diagnostique.type_a_valider = it;
            return false;
        }
    }

    diagnostique.toutes_les_declarations_a_typer_le_sont = true;
    return true;
}

bool Programme::ri_generees(DiagnostiqueEtatCompilation &diagnostique) const
{
    if (!typages_termines(diagnostique)) {
        return false;
    }

    POUR (m_fonctions) {
        if (!it->possede_drapeau(RI_FUT_GENEREE)) {
            assert_rappel(it->unite, [&]() {
                std::cerr << "Aucune unité pour de compilation pour :\n";
                erreur::imprime_site(*m_espace, it);
            });
            diagnostique.ri_declaration_a_generer = it;
            return false;
        }
    }

    POUR (m_globales) {
        if (!it->possede_drapeau(RI_FUT_GENEREE)) {
            diagnostique.ri_declaration_a_generer = it;
            return false;
        }
    }

    POUR (m_types) {
        if ((it->drapeaux & RI_TYPE_FUT_GENEREE) == 0) {
            diagnostique.ri_type_a_generer = it;
            return false;
        }
    }

    diagnostique.toutes_les_ri_sont_generees = true;
    return true;
}

bool Programme::ri_generees() const
{
    auto diagnostic = diagnositique_compilation();
    return diagnostic.toutes_les_ri_sont_generees;
}

DiagnostiqueEtatCompilation Programme::diagnositique_compilation() const
{
    DiagnostiqueEtatCompilation diagnositique{};
    verifie_etat_compilation_fichier(diagnositique);
    ri_generees(diagnositique);
    return diagnositique;
}

EtatCompilation Programme::ajourne_etat_compilation()
{
    auto diagnostic = diagnositique_compilation();

    // À FAIRE(gestion) : ceci n'est que pour les métaprogrammes
    m_etat_compilation.essaie_d_aller_a(PhaseCompilation::PARSAGE_EN_COURS);
    m_etat_compilation.essaie_d_aller_a(PhaseCompilation::PARSAGE_TERMINE);

    if (diagnostic.toutes_les_declarations_a_typer_le_sont) {
        m_etat_compilation.essaie_d_aller_a(PhaseCompilation::TYPAGE_TERMINE);
    }

    if (diagnostic.toutes_les_ri_sont_generees) {
        m_etat_compilation.essaie_d_aller_a(PhaseCompilation::GENERATION_CODE_TERMINEE);
    }

    return m_etat_compilation;
}

void Programme::change_de_phase(PhaseCompilation phase)
{
    m_etat_compilation.essaie_d_aller_a(phase);
}

void Programme::verifie_etat_compilation_fichier(DiagnostiqueEtatCompilation &diagnostique) const
{
    diagnostique.tous_les_fichiers_sont_charges = true;
    diagnostique.tous_les_fichiers_sont_lexes = true;
    diagnostique.tous_les_fichiers_sont_parses = true;

    POUR (m_fichiers) {
        if (!it->donnees_constantes->fut_charge) {
            diagnostique.tous_les_fichiers_sont_charges = false;
        }

        if (!it->donnees_constantes->fut_lexe) {
            diagnostique.tous_les_fichiers_sont_lexes = false;
        }

        if (!it->fut_parse) {
            diagnostique.tous_les_fichiers_sont_parses = false;
        }
    }
}

void Programme::ajoute_fichier(Fichier *fichier)
{
    if (m_fichiers_utilises.possede(fichier)) {
        return;
    }

    m_fichiers.ajoute(fichier);
    m_fichiers_utilises.insere(fichier);
}

void Programme::ajoute_racine(NoeudDeclarationEnteteFonction *racine)
{
    if (pour_metaprogramme()) {
        /* Pour les métaprogrammes, nous n'ajoutons que les racines pour la création de
         * l'exécutable. */
        if (racine->ident == ID::cree_contexte) {
            ajoute_fonction(racine);
        }
    }
    else {
        if (racine->ident != ID::cree_contexte) {
            ajoute_fonction(racine);
        }
    }
}

void imprime_contenu_programme(const Programme &programme, unsigned int quoi, std::ostream &os)
{
    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_TYPES) != 0) {
        os << "Types dans le programme...\n";
        POUR (programme.types()) {
            os << "-- " << chaine_type(it) << '\n';
        }
    }

    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_FONCTIONS) != 0) {
        os << "Fonctions dans le programme...\n";
        POUR (programme.fonctions()) {
            os << "-- " << it->ident->nom << '\n';
        }
    }

    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_GLOBALES) != 0) {
        os << "Globales dans le programme...\n";
        POUR (programme.globales()) {
            os << "-- " << it->ident->nom << '\n';
        }
    }
}

// --------------------------------------------------------
// RepresentationIntermediaireProgramme

void imprime_contenu_programme(const ProgrammeRepreInter &programme,
                               unsigned int quoi,
                               std::ostream &os)
{
    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_TYPES) != 0) {
        os << "Types dans le programme...\n";
        POUR (programme.types) {
            os << "-- " << chaine_type(it) << '\n';
        }
    }

    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_FONCTIONS) != 0) {
        os << "Fonctions dans le programme...\n";
        POUR (programme.fonctions) {
            if (it->decl && it->decl->ident) {
                os << "-- " << it->decl->ident->nom << ' ' << chaine_type(it->type) << '\n';
            }
            else {
                os << "-- anonyme de type " << chaine_type(it->type) << '\n';
            }
        }
    }

    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_GLOBALES) != 0) {
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
}

template <typename T>
static kuri::ensemble<T> cree_ensemble(const kuri::tableau<T> &tableau)
{
    kuri::ensemble<T> resultat;

    POUR (tableau) {
        resultat.insere(it);
    }

    return resultat;
}

/* La seule raison d'existence pour cette fonction est de rassembler les globales pour les chaines
 * et InfoType. */
static void rassemble_globales_supplementaires(ProgrammeRepreInter &repr_inter,
                                               AtomeFonction *fonction,
                                               VisiteuseAtome &visiteuse,
                                               kuri::ensemble<AtomeGlobale *> &globales_utilisees)
{
    POUR (fonction->instructions) {
        visiteuse.visite_atome(it, [&](Atome *atome) {
            if (atome->genre_atome == Atome::Genre::GLOBALE) {
                if (globales_utilisees.possede(static_cast<AtomeGlobale *>(atome))) {
                    return;
                }

                repr_inter.globales.ajoute(static_cast<AtomeGlobale *>(atome));
                globales_utilisees.insere(static_cast<AtomeGlobale *>(atome));
            }
        });
    }
}

static void rassemble_globales_supplementaires(ProgrammeRepreInter &repr_inter)
{
    auto globales_utilisees = cree_ensemble(repr_inter.globales);
    VisiteuseAtome visiteuse{};

    POUR (repr_inter.fonctions) {
        visiteuse.reinitialise();
        rassemble_globales_supplementaires(repr_inter, it, visiteuse, globales_utilisees);
    }
}

struct VisiteuseType {
    kuri::ensemble<Type *> visites{};

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
                auto reference = type->comme_reference();
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
                auto type_union = type->comme_union();
                POUR (type_union->membres) {
                    visite_type(it.type, rappel);
                }
                break;
            }
            case GenreType::STRUCTURE:
            {
                auto type_structure = type->comme_structure();
                POUR (type_structure->membres) {
                    visite_type(it.type, rappel);
                }
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
                auto type_enum = static_cast<TypeEnum *>(type);
                visite_type(type_enum->type_donnees, rappel);
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

ProgrammeRepreInter representation_intermediaire_programme(Programme const &programme)
{
    auto resultat = ProgrammeRepreInter{};

    resultat.fonctions.reserve(programme.fonctions().taille() + programme.types().taille());
    resultat.globales.reserve(programme.globales().taille());

    /* Nous pouvons directement copier les types. */
    resultat.types = programme.types();

    /* Extrait les atomes pour les fonctions. */
    POUR (programme.fonctions()) {
        assert_rappel(it->possede_drapeau(RI_FUT_GENEREE), [&]() {
            std::cerr << "La RI ne fut pas généré pour:\n";
            erreur::imprime_site(*programme.espace(), it);
        });
        assert_rappel(it->atome, [&]() {
            std::cerr << "Aucun atome pour:\n";
            erreur::imprime_site(*programme.espace(), it);
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
            erreur::imprime_site(*programme.espace(), it);
        });
        assert_rappel(it->atome, [&]() {
            std::cerr << "Aucun atome pour:\n";
            erreur::imprime_site(*programme.espace(), it);
            std::cerr << "Taille données decl  : " << it->donnees_decl.taille() << '\n';
            std::cerr << "Possède substitution : " << (it->substitution != nullptr) << '\n';
        });
        resultat.globales.ajoute(static_cast<AtomeGlobale *>(it->atome));
    }

    /* Traverse les instructions des fonctions, et rassemble les globales pour les chaines, les
     * tableaux, et les infos-types. */
    rassemble_globales_supplementaires(resultat);

    {
        /* Ajoute les types de toutes les globales et toutes les fonctions, dans le cas où nous en
         * aurions ajoutées (qui ne sont pas dans le programme initiale). */
        auto type_utilises = cree_ensemble(resultat.types);

        VisiteuseType visiteuse{};
        auto ajoute_type_si_necessaire = [&](Type *type_racine) {
            visiteuse.visite_type(type_racine, [&](Type *type) {
                if (type_utilises.possede(type)) {
                    return;
                }

                type_utilises.insere(type);
                resultat.types.ajoute(type);
            });
        };

        POUR (resultat.fonctions) {
            ajoute_type_si_necessaire(it->type);
            for (auto &inst : it->instructions) {
                ajoute_type_si_necessaire(inst->type);
            }
        }

        POUR (resultat.globales) {
            ajoute_type_si_necessaire(it->type);
        }
    }

    return resultat;
}

void imprime_diagnostique(const DiagnostiqueEtatCompilation &diagnositic)
{
    if (!diagnositic.toutes_les_declarations_a_typer_le_sont) {
        if (diagnositic.type_a_valider) {
            std::cerr << "-- validation non performée pour le type : "
                      << chaine_type(diagnositic.type_a_valider) << '\n';
        }
        if (diagnositic.declaration_a_valider) {
            std::cerr << "-- validation non performée pour déclaration "
                      << diagnositic.declaration_a_valider->lexeme->chaine << '\n';
        }
        return;
    }

    if (diagnositic.ri_type_a_generer) {
        std::cerr << "-- RI non générée pour le type : "
                  << chaine_type(diagnositic.ri_type_a_generer) << '\n';
    }
    if (diagnositic.ri_declaration_a_generer) {
        std::cerr << "-- RI non générée pour déclaration "
                  << diagnositic.ri_declaration_a_generer->lexeme->chaine << '\n';
    }
}

/* Cette fonction n'existe que parce que la principale peut ajouter des globales pour les
 * constructeurs globaux... */
void ProgrammeRepreInter::ajoute_fonction(AtomeFonction *fonction)
{
    auto globales_utilisees = cree_ensemble(this->globales);
    VisiteuseAtome visiteuse{};
    rassemble_globales_supplementaires(*this, fonction, visiteuse, globales_utilisees);
    fonctions.ajoute(fonction);
}
