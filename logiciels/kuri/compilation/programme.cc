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
            std::cerr << "-- typage non terminé pour type " << chaine_type(it) << ", " << it << '\n';
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

static void rassemble_globales_supplementaire(ProgrammeRepreInter &repr_inter)
{
    dls::ensemble<Atome *> globales_utilisess;

    std::cerr << __func__ << ", " << repr_inter.globales.taille() << " globales en entrée\n";

    POUR (repr_inter.globales) {
        globales_utilisess.insere(it);
    }

    POUR (repr_inter.fonctions) {
        for (auto instruction : it->instructions) {
            visite_atome(instruction, [&](Atome *atome) {
                if (atome->genre_atome == Atome::Genre::GLOBALE) {
                    if (globales_utilisess.possede(atome)) {
                        return;
                    }

                    repr_inter.globales.ajoute(static_cast<AtomeGlobale *>(atome));
                    globales_utilisess.insere(atome);
                }
            });
        }
    }

    std::cerr << __func__ << ", " << repr_inter.globales.taille() << " globales en sortie\n";
}

ProgrammeRepreInter representation_intermediaire_programme(Programme const &programme, EspaceDeTravail &espace)
{
    auto resultat = ProgrammeRepreInter{};

    resultat.fonctions.reserve(programme.fonctions().taille() + programme.types().taille());
    resultat.globales.reserve(programme.globales().taille());

    /* Nous pouvons directement copiés les types. */
    resultat.types = programme.types();

    /* Extrait les atomes pour les fonctions. */
    POUR (programme.fonctions()) {
        assert_rappel(it->possede_drapeau(RI_FUT_GENEREE), [&](){
            std::cerr << "La RI ne fut pas généré pour:\n";
            erreur::imprime_site(espace, it);
        });
        assert_rappel(it->atome, [&](){
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
        assert_rappel(it->possede_drapeau(RI_FUT_GENEREE), [&](){
            std::cerr << "La RI ne fut pas généré pour:\n";
            erreur::imprime_site(espace, it);
        });
        assert_rappel(it->atome, [&](){
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

    return resultat;
}
