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

#include "erreur.h"

#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/numerique.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "arbre_syntaxique.hh"
#include "compilatrice.hh"
#include "lexemes.hh"
#include "validation_semantique.hh"

namespace erreur {

const char *chaine_erreur(Genre genre)
{
	switch (genre) {
#define ENUMERE_GENRE_ERREUR_EX(genre) case Genre::genre: return #genre;
		ENUMERE_GENRES_ERREUR
#undef ENUMERE_GENRE_ERREUR_EX
	}
	return "Ceci ne devrait pas s'afficher";
}

std::ostream &operator<<(std::ostream &os, Genre genre)
{
	os << chaine_erreur(genre);
	return os;
}

static auto chaine_expression(EspaceDeTravail const &espace, const NoeudExpression *expr)
{
	auto lexeme = expr->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	auto etendue_expr = calcule_etendue_noeud(expr, fichier);
	auto ligne = fichier->tampon()[lexeme->ligne];
	return dls::vue_chaine_compacte(&ligne[etendue_expr.pos_min], etendue_expr.pos_max - etendue_expr.pos_min);
}

void imprime_ligne_avec_message(
		dls::flux_chaine &flux,
		Fichier *fichier,
		Lexeme const *lexeme,
		const char *message)
{
	flux << fichier->chemin() << ':' << lexeme->ligne + 1 << ':' << lexeme->colonne + 1 << " : ";
	flux << message << "\n";

	auto nc = dls::num::nombre_de_chiffres(lexeme->ligne + 1);

	for (auto i = 0; i < 5 - nc; ++i) {
		flux << ' ';
	}

	flux << lexeme->ligne + 1 << " | " << fichier->tampon()[lexeme->ligne];
	flux << "      | ";

	lng::erreur::imprime_caractere_vide(flux, lexeme->colonne, fichier->tampon()[lexeme->ligne]);
	flux << '^';
	lng::erreur::imprime_tilde(flux, lexeme->chaine);
	flux << '\n';
}

void lance_erreur(const dls::chaine &quoi,
		EspaceDeTravail const &espace,
		const NoeudExpression *site,
		Genre type)
{
	rapporte_erreur(&espace, site, quoi, type);
}

void redefinition_fonction(EspaceDeTravail const &espace,
		const NoeudExpression *site_redefinition,
		const NoeudExpression *site_original)
{
	rapporte_erreur(&espace, site_redefinition, "Redéfinition de la fonction !", Genre::FONCTION_REDEFINIE)
			.ajoute_message("La fonction fut déjà définie ici :\n\n")
			.ajoute_site(site_original);
}

void redefinition_symbole(EspaceDeTravail const &espace, const NoeudExpression *site_redefinition, const NoeudExpression *site_original)
{
	rapporte_erreur(&espace, site_redefinition, "Redéfinition du symbole !", Genre::VARIABLE_REDEFINIE)
			.ajoute_message("Le symbole fut déjà défini ici :\n\n")
			.ajoute_site(site_original);
}

[[noreturn]] void lance_erreur_transtypage_impossible(
		const Type *type_cible,
		const Type *type_expression,
		EspaceDeTravail const &espace,
		const NoeudExpression *site_expression,
		const NoeudExpression *site)
{
	rapporte_erreur(&espace, site, "Aucune conversion connue pour transformer vers le type cible", Genre::TYPE_ARGUMENT)
			.ajoute_message("Le type de l'expression '", chaine_expression(espace, site_expression), "' ne peut être transformer vers le type cible !\n")
			.ajoute_message("Type cible           : ", chaine_type(type_cible), "\n")
			.ajoute_message("Type de l'expression : ", chaine_type(type_expression), "\n\n");
}

[[noreturn]] void lance_erreur_assignation_type_differents(
		const Type *type_gauche,
		const Type *type_droite,
		EspaceDeTravail const &espace,
		const NoeudExpression *site)
{
	rapporte_erreur(&espace, site, "Ne peut pas assigner des types différents !", Genre::ASSIGNATION_MAUVAIS_TYPE)
			.ajoute_message("Type à gauche : ", chaine_type(type_gauche), "\n")
			.ajoute_message("Type à droite : ", chaine_type(type_droite), "\n");
}

void lance_erreur_type_operation(
		const Type *type_gauche,
		const Type *type_droite,
		EspaceDeTravail const &espace,
		const NoeudExpression *site)
{
	rapporte_erreur(&espace, site, "Type incompatible pour l'opération !", Genre::TYPE_DIFFERENTS)
			.ajoute_message("Type à gauche : ", chaine_type(type_gauche), "\n")
			.ajoute_message("Type à droite : ", chaine_type(type_droite), "\n");
}

void lance_erreur_fonction_inconnue(
		EspaceDeTravail const &espace,
		NoeudExpression *b,
		dls::tablet<DonneesCandidate, 10> const &candidates)
{
	auto e = rapporte_erreur(&espace, b, "Dans l'expression d'appel :", erreur::Genre::FONCTION_INCONNUE);

	if (candidates.est_vide()) {
		e.ajoute_message("\nFonction inconnue : aucune candidate trouvée\n");
		e.ajoute_message("Vérifiez que la fonction existe bel et bien dans un fichier importé\n");
	}
	else {
		e.ajoute_message("Aucune candidate trouvée pour l'expression « ", chaine_expression(espace, b->comme_appel()->appelee), " » !\n");

		for (auto &dc : candidates) {
			auto decl = dc.noeud_decl;
			e.ajoute_message("\nCandidate :");

			if (decl != nullptr) {
				auto const &lexeme_df = decl->lexeme;
				auto fichier_df = espace.fichier(lexeme_df->fichier);
				auto pos_df = position_lexeme(*lexeme_df);

				e.ajoute_message(' ', decl->ident->nom, " (trouvée à ", fichier_df->chemin(), ':', pos_df.numero_ligne, ")\n");
			}
			else {
				e.ajoute_message('\n');
			}

			if (dc.raison == MECOMPTAGE_ARGS) {
				auto noeud_appel = static_cast<NoeudExpressionAppel *>(b);
				e.ajoute_message("\tLe nombre d'arguments de la fonction est incorrect.\n");

				if (decl && decl->genre == GenreNoeud::DECLARATION_CORPS_FONCTION) {
					auto decl_fonc = decl->comme_entete_fonction();
					e.ajoute_message("\tRequiers ", decl_fonc->params.taille, " arguments\n");
				}
				else if (decl && decl->genre == GenreNoeud::DECLARATION_STRUCTURE) {
					auto type_struct = static_cast<NoeudStruct const *>(decl)->type->comme_structure();
					e.ajoute_message("\tRequiers ", type_struct->membres.taille, " arguments\n");
				}
				else {
					auto type_fonc = dc.type->comme_fonction();
					e.ajoute_message("\tRequiers ", type_fonc->types_entrees.taille - dc.requiers_contexte, " arguments\n");
				}

				e.ajoute_message("\tObtenu ", noeud_appel->params.taille, " arguments\n");
				e.genre_erreur(erreur::Genre::NOMBRE_ARGUMENT);
			}
			else if (dc.raison == MENOMMAGE_ARG) {
				e.ajoute_site(dc.noeud_erreur);
				e.ajoute_message("Argument inconnu");

				if (decl->genre == GenreNoeud::DECLARATION_CORPS_FONCTION) {
					auto decl_fonc = decl->comme_entete_fonction();
					e.ajoute_message("\tLes arguments de la fonction sont : \n");

					for (auto i = 0; i < decl_fonc->params.taille; ++i) {
						auto param = decl_fonc->parametre_entree(i);
						e.ajoute_message("\t\t", param->ident->nom, '\n');
					}

					e.genre_erreur(erreur::Genre::ARGUMENT_INCONNU);
				}
				else if (decl->genre == GenreNoeud::DECLARATION_STRUCTURE) {
					auto decl_struct = decl->comme_structure();

					if (decl_struct->est_polymorphe) {
						e.ajoute_message("\tLes paramètres de la structure sont : \n");

						POUR (decl_struct->params_polymorphiques) {
							e.ajoute_message("\t\t", it->ident->nom, '\n');
						}
					}
					else {
						e.ajoute_message("\tLes membres de la structure sont : \n");

						auto type_struct = decl_struct->type->comme_structure();
						POUR (type_struct->membres) {
							e.ajoute_message("\t\t", it.nom, '\n');
						}
					}

					e.genre_erreur(erreur::Genre::MEMBRE_INCONNU);
				}
			}
			else if (dc.raison == RENOMMAGE_ARG) {
				e.genre_erreur(erreur::Genre::ARGUMENT_REDEFINI);
				e.ajoute_site(dc.noeud_erreur);
				e.ajoute_message("L'argument a déjà été nommé");
			}
			else if (dc.raison == MANQUE_NOM_APRES_VARIADIC) {
				e.genre_erreur(erreur::Genre::ARGUMENT_INCONNU);
				e.ajoute_site(dc.noeud_erreur);
				e.ajoute_message("Nom d'argument manquant, les arguments doivent être nommés s'ils sont précédés d'arguments déjà nommés");
			}
			else if (dc.raison == NOMMAGE_ARG_POINTEUR_FONCTION) {
				e.ajoute_message("\tLes arguments d'un pointeur fonction ne peuvent être nommés\n");
				e.genre_erreur(erreur::Genre::ARGUMENT_INCONNU);
			}
			else if (dc.raison == TYPE_N_EST_PAS_FONCTION) {
				e.ajoute_message("\tAppel d'une variable n'étant pas un pointeur de fonction\n");
				e.genre_erreur(erreur::Genre::FONCTION_INCONNUE);
			}
			else if(dc.raison == TROP_D_EXPRESSION_POUR_UNION) {
				e.ajoute_message("\tOn ne peut initialiser qu'un seul membre d'une union à la fois\n");
				e.genre_erreur(erreur::Genre::NORMAL);
			}
			else if(dc.raison == EXPRESSION_MANQUANTE_POUR_UNION) {
				e.ajoute_message("\tOn doit initialiser au moins un membre de l'union\n");
				e.genre_erreur(erreur::Genre::NORMAL);
			}
			else if (dc.raison == CONTEXTE_MANQUANT) {
				e.ajoute_message("\tNe peut appeler une fonction avec contexte dans un bloc n'ayant pas de contexte\n");
				e.genre_erreur(erreur::Genre::NORMAL);
			}
			else if (dc.raison == EXPANSION_VARIADIQUE_FONCTION_EXTERNE) {
				e.ajoute_message("\tImpossible d'utiliser une expansion variadique dans une fonction variadique externe\n");
				e.genre_erreur(erreur::Genre::NORMAL);
			}
			else if (dc.raison == MULTIPLE_EXPANSIONS_VARIADIQUES) {
				e.ajoute_message("\tPlusieurs expansions variadiques trouvées\n");
				e.genre_erreur(erreur::Genre::NORMAL);
			}
			else if (dc.raison == EXPANSION_VARIADIQUE_APRES_ARGUMENTS_VARIADIQUES) {
				e.ajoute_message("\tTentative d'utiliser une expansion d'arguments variadiques alors que d'autres arguments ont déjà été précisés\n");
				e.genre_erreur(erreur::Genre::NORMAL);
			}
			else if (dc.raison == ARGUMENTS_VARIADIQEUS_APRES_EXPANSION_VARIAQUES) {
				e.ajoute_message("\tTentative d'ajouter des arguments variadiques supplémentaire alors qu'une expansion est également utilisée\n");
				e.genre_erreur(erreur::Genre::NORMAL);
			}
			else if (dc.raison == ARGUMENTS_MANQUANTS) {
				if (dc.arguments_manquants.taille() == 1) {
					e.ajoute_message("\tUn argument est manquant :\n");
				}
				else {
					e.ajoute_message("\tPlusieurs arguments sont manquants :\n");
				}

				for (auto ident : dc.arguments_manquants) {
					e.ajoute_message("\t\t", ident->nom, '\n');
				}
			}
			else if (dc.raison == IMPOSSIBLE_DE_DEFINIR_UN_TYPE_POLYMORPHIQUE) {
				e.ajoute_message("\tImpossible de définir le type polymorphique ", dc.ident_poly_manquant->nom, '\n');
			}
			else if (dc.raison == METYPAGE_ARG) {
				e.ajoute_message("\tLe type de l'argument '", chaine_expression(espace, dc.noeud_erreur), "' ne correspond pas à celui requis !\n");
				e.ajoute_message("\tRequiers : ", chaine_type(dc.type_attendu), '\n');
				e.ajoute_message("\tObtenu   : ", chaine_type(dc.type_obtenu), '\n');
				e.genre_erreur(erreur::Genre::TYPE_ARGUMENT);
			}
		}
	}
}

void lance_erreur_fonction_nulctx(
			EspaceDeTravail const &espace,
			NoeudExpression const *appl_fonc,
			NoeudExpression const *decl_fonc,
			NoeudExpression const *decl_appel)
{
	rapporte_erreur(&espace, appl_fonc, "Ne peut appeler une fonction avec contexte dans une fonction sans contexte !", Genre::APPEL_INVALIDE)
			.ajoute_message("Note : la fonction est appelée dans « ", decl_fonc->ident->nom, " » qui fut déclarée sans contexte via #!nulctx.\n\n")
			.ajoute_message("« ", decl_fonc->ident->nom, " » fut déclarée ici :\n")
			.ajoute_site(decl_fonc)
			.ajoute_message("\n\n")
			.ajoute_message("« ", decl_appel->ident->nom, " » fut déclarée ici :\n")
			.ajoute_site(decl_fonc)
			.ajoute_message("\n\n");
}

void lance_erreur_acces_hors_limites(
			EspaceDeTravail const &espace,
			NoeudExpression *b,
			long taille_tableau,
			Type *type_tableau,
			long index_acces)
{
	rapporte_erreur(&espace, b, "Accès au tableau hors de ses limites !", Genre::NORMAL)
			.ajoute_message("\tLe tableau a une taille de ", taille_tableau, " (de type : ", chaine_type(type_tableau), ").\n")
			.ajoute_message("\tL'accès se fait à l'index ", index_acces, " (index maximal : ", taille_tableau - 1, ").\n");
}

struct CandidatMembre {
	long distance = 0;
	dls::vue_chaine_compacte chaine = "";
};

static auto trouve_candidat(
			dls::ensemble<dls::vue_chaine_compacte> const &membres,
			dls::vue_chaine_compacte const &nom_donne)
{
	auto candidat = CandidatMembre{};
	candidat.distance = 1000;

	for (auto const &nom_membre : membres) {
		auto candidat_possible = CandidatMembre();
		candidat_possible.distance = distance_levenshtein(nom_donne, nom_membre);
		candidat_possible.chaine = nom_membre;

		if (candidat_possible.distance < candidat.distance) {
			candidat = candidat_possible;
		}
	}

	return candidat;
}

void membre_inconnu(
			EspaceDeTravail const &espace,
			NoeudExpression *acces,
			NoeudExpression */*structure*/,
			NoeudExpression *membre,
			TypeCompose *type)
{
	auto membres = dls::ensemble<dls::vue_chaine_compacte>();

	POUR (type->membres) {
		membres.insere(it.nom->nom);
	}

	const char *message;

	if (type->genre == GenreType::ENUM) {
		message = "de l'énumération";
	}
	else if (type->genre == GenreType::UNION) {
		message = "de l'union";
	}
	else if (type->genre == GenreType::ERREUR) {
		message = "de l'erreur";
	}
	else {
		message = "de la structure";
	}

	auto candidat = trouve_candidat(membres, membre->ident->nom);

	auto e = rapporte_erreur(&espace, acces, "Dans l'expression d'accès de membre", Genre::MEMBRE_INCONNU);
	e.ajoute_message("Le membre « ", membre->ident->nom, " » est inconnu !\n\n");

	if (membres.taille() == 0) {
		e.ajoute_message("Aucun membre connu !\n");
	}
	else {
		e.ajoute_message("Les membres ", message, " sont :\n");

		POUR (membres) {
			e.ajoute_message("\t", it, "\n");
		}

		e.ajoute_message("\nCandidat possible : ", candidat.chaine, "\n");
	}
}

void membre_inactif(
			EspaceDeTravail const &espace,
			ContexteValidationCode &contexte,
			NoeudExpression *acces,
			NoeudExpression *structure,
			NoeudExpression *membre)
{
	rapporte_erreur(&espace, acces, "Accès à un membre inactif d'une union", Genre::MEMBRE_INACTIF)
			.ajoute_message("Le membre « ", membre->ident->nom, " » est inactif dans ce contexte !\n")
			.ajoute_message("Le membre actif dans ce contexte est « ", contexte.trouve_membre_actif(structure->ident->nom), " ».\n");
}

void valeur_manquante_discr(
			EspaceDeTravail const &espace,
			NoeudExpression *expression,
			dls::ensemble<dls::vue_chaine_compacte> const &valeurs_manquantes)
{
	auto e = rapporte_erreur(&espace, expression, "Dans l'expression de discrimination", Genre::NORMAL);

	if (valeurs_manquantes.taille() == 1) {
		e.ajoute_message("Une valeur n'est pas prise en compte :\n");
	}
	else {
		e.ajoute_message("Plusieurs valeurs ne sont pas prises en compte :\n");
	}

	POUR (valeurs_manquantes) {
		e.ajoute_message('\t', it, '\n');
	}
}

void fonction_principale_manquante(EspaceDeTravail const &espace)
{
	dls::flux_chaine ss;
	ss << "\n----------------------------------------------------------------\n";
	ss << "Dans l'espace de travail « " << espace.nom << " » :\n";
	ss << "Erreur : impossible de trouver la fonction principale\n";
	ss << "Veuillez vérifier qu'elle soit bien présente dans un module\n";
	ss << "\n----------------------------------------------------------------\n";
	throw erreur::frappe(ss.chn().c_str(), erreur::Genre::MEMBRE_INACTIF);
}

void imprime_site(const EspaceDeTravail &espace, const NoeudExpression *site)
{
	if (site == nullptr) {
		return;
	}

	auto lexeme = site->lexeme;
	auto fichier = espace.fichier(lexeme->fichier);
	std::cerr << fichier->chemin() << ':' << lexeme->ligne + 1 << '\n';

	dls::flux_chaine ss;

	auto etendue = calcule_etendue_noeud(site, fichier);
	auto pos = position_lexeme(*lexeme);
	auto const pos_mot = pos.pos;
	auto ligne = fichier->tampon()[pos.index_ligne];
	ss << ligne;
	lng::erreur::imprime_caractere_vide(ss, etendue.pos_min, ligne);
	lng::erreur::imprime_tilde(ss, ligne, etendue.pos_min, pos_mot);
	ss << '^';
	lng::erreur::imprime_tilde(ss, ligne, pos_mot + 1, etendue.pos_max);
	ss << '\n';

	std::cerr << ss.chn();
}

}

/* ************************************************************************** */

Erreur::Erreur(EspaceDeTravail const *espace_)
	: espace(espace_)
{}

Erreur::~Erreur() noexcept(false)
{
	throw erreur::frappe(message.c_str(), erreur::Genre::NORMAL);
}

Erreur &Erreur::ajoute_message(const dls::chaine &m)
{
	message += m;
	return *this;
}

Erreur &Erreur::ajoute_site(const NoeudExpression *site)
{
	assert(espace);

	auto fichier = espace->fichier(site->lexeme->fichier);
	auto flux = dls::flux_chaine();
	flux << message;

	erreur::imprime_ligne_avec_message(flux, fichier, site->lexeme, "");
	flux << '\n';

	message = flux.chn();
	return *this;
}

Erreur &Erreur::ajoute_conseil(const dls::chaine &c)
{
	auto flux = dls::flux_chaine();
	flux << message;
	flux << "\033[4mConseil\033[00m : " << c;

	message = flux.chn();
	return *this;
}

static dls::chaine chaine_pour_erreur(erreur::Genre genre)
{
	switch (genre) {
		default:
		{
			return "ERREUR";
		}
		case erreur::Genre::LEXAGE:
		{
			return "ERREUR DE LEXAGE";
		}
		case erreur::Genre::SYNTAXAGE:
		{
			return "ERREUR DE LEXAGE";
		}
		case erreur::Genre::TYPE_INCONNU:
		case erreur::Genre::TYPE_DIFFERENTS:
		case erreur::Genre::TYPE_ARGUMENT:
		{
			return "ERREUR DE TYPAGE";
		}
	}

	return "ERREUR";
}

#define COULEUR_NORMALE "\033[0m"
#define COULEUR_CYAN_GRAS "\033[1;36m"

Erreur rapporte_erreur(EspaceDeTravail const *espace, NoeudExpression const *site, const dls::chaine &message, erreur::Genre genre)
{
	auto fichier = espace->fichier(site->lexeme->fichier);

	auto flux = dls::flux_chaine();
	flux << COULEUR_CYAN_GRAS << "-- ";

	auto chaine_erreur = chaine_pour_erreur(genre);
	flux << chaine_erreur << ' ';

	for (auto i = 0; i < 76 - chaine_erreur.taille(); ++i) {
		flux << '-';
	}

	flux << "\n\n" << COULEUR_NORMALE;

	flux << "Dans l'espace de travail \"" << espace->nom << "\" :\n";
	flux << "\nErreur : ";
	erreur::imprime_ligne_avec_message(flux, fichier, site->lexeme, "");
	flux << '\n';
	flux << message;
	flux << '\n';
	flux << '\n';

	auto erreur = Erreur(espace);
	erreur.message = flux.chn();
	return erreur;
}

Erreur rapporte_erreur_sans_site(EspaceDeTravail const *espace, const dls::chaine &message, erreur::Genre genre)
{
	auto flux = dls::flux_chaine();
	flux << COULEUR_CYAN_GRAS << "-- ";

	auto chaine_erreur = chaine_pour_erreur(genre);
	flux << chaine_erreur << ' ';

	for (auto i = 0; i < 76 - chaine_erreur.taille(); ++i) {
		flux << '-';
	}

	flux << "\n\n" << COULEUR_NORMALE;

	flux << "Dans l'espace de travail \"" << espace->nom << "\" :\n";
	flux << "\nErreur : ";
	flux << message;
	flux << '\n';
	flux << '\n';

	auto erreur = Erreur(espace);
	erreur.message = flux.chn();
	erreur.genre_erreur(genre);
	return erreur;
}

Erreur rapporte_erreur(EspaceDeTravail const *espace, kuri::chaine fichier, int ligne, kuri::chaine message)
{
	auto flux = dls::flux_chaine();
	flux << COULEUR_CYAN_GRAS << "-- ";

	auto chaine_erreur = dls::chaine("ERREUR");
	flux << chaine_erreur << ' ';

	for (auto i = 0; i < 76 - chaine_erreur.taille(); ++i) {
		flux << '-';
	}

	flux << "\n\n" << COULEUR_NORMALE;

	const Fichier *f = espace->fichier({ fichier.pointeur, fichier.taille });

	flux << "Dans l'espace de travail \"" << espace->nom << "\" :\n";
	flux << "\nErreur : " << f->chemin() << ":" << ligne << ":\n";
	flux << f->tampon()[ligne - 1];
	flux << '\n';
	flux << message;
	flux << '\n';
	flux << '\n';

	auto erreur = Erreur(espace);
	erreur.message = flux.chn();
	return erreur;
}
