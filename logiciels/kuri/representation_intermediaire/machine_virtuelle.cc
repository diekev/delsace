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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "machine_virtuelle.hh"

#include "biblinternes/chrono/chronometrage.hh"

#include "compilation/arbre_syntaxique.hh"
#include "compilation/identifiant.hh"
#include "compilation/broyage.hh"
#include "compilation/structures.hh"

#include "instructions.hh"

#undef DEBOGUE_INTERPRETEUSE
#undef CHRONOMETRE_INTERPRETATION
#undef DEBOGUE_VALEURS_ENTREE_SORTIE
#undef DEBOGUE_LOCALES

#define LIS_OCTET() \
	(*frame->pointeur++)

#define LIS_4_OCTETS() \
	*reinterpret_cast<int *>(frame->pointeur); (frame->pointeur += 4)

#define LIS_8_OCTETS() \
	*reinterpret_cast<long *>(frame->pointeur); (frame->pointeur += 8)

#define LIS_POINTEUR(type) \
	*reinterpret_cast<type **>(frame->pointeur); (frame->pointeur += 8)

#define OP_UNAIRE_POUR_TYPE(op, type) \
	if (taille == static_cast<int>(sizeof(type))) { \
		auto a = depile<type>(); \
		/* std::cerr <<  #op << a << '\n'; */ \
		empile(op a); \
	}

#define OP_UNAIRE(op) \
	auto taille = LIS_4_OCTETS(); \
	OP_UNAIRE_POUR_TYPE(op, char) \
	else OP_UNAIRE_POUR_TYPE(op, short) \
	else OP_UNAIRE_POUR_TYPE(op, int) \
	else OP_UNAIRE_POUR_TYPE(op, long)

#define OP_UNAIRE_REEL(op) \
	auto taille = LIS_4_OCTETS(); \
	OP_UNAIRE_POUR_TYPE(op, float) \
	else OP_UNAIRE_POUR_TYPE(op, double)


#define OP_BINAIRE_POUR_TYPE(op, type) \
	if (taille == static_cast<int>(sizeof(type))) { \
		auto b = depile<type>(); \
		auto a = depile<type>(); \
		/* std::cerr << a << #op << b << '\n'; */ \
		empile(a op b); \
	}

#define OP_BINAIRE(op) \
	auto taille = LIS_4_OCTETS(); \
	OP_BINAIRE_POUR_TYPE(op, char) \
	else OP_BINAIRE_POUR_TYPE(op, short) \
	else OP_BINAIRE_POUR_TYPE(op, int) \
	else OP_BINAIRE_POUR_TYPE(op, long)

#define OP_BINAIRE_NATUREL(op) \
	auto taille = LIS_4_OCTETS(); \
	OP_BINAIRE_POUR_TYPE(op, unsigned char) \
	else OP_BINAIRE_POUR_TYPE(op, unsigned short) \
	else OP_BINAIRE_POUR_TYPE(op, unsigned int) \
	else OP_BINAIRE_POUR_TYPE(op, unsigned long)

#define OP_BINAIRE_REEL(op) \
	auto taille = LIS_4_OCTETS(); \
	OP_BINAIRE_POUR_TYPE(op, float) \
	else OP_BINAIRE_POUR_TYPE(op, double)

#define FAIS_TRANSTYPE(type_de, type_vers) \
	if (taille_vers == static_cast<int>(sizeof(type_vers))) { \
		auto v = depile<type_de>(); \
		empile(static_cast<type_vers>(v)); \
	}

#define FAIS_TRANSTYPE_AUGMENTE(type1, type2, type3, type4) \
	if (taille_de == 1) { \
		FAIS_TRANSTYPE(type1, type2) \
		else FAIS_TRANSTYPE(type1, type3) \
		else FAIS_TRANSTYPE(type1, type4) \
	} \
	else if (taille_de == 2) { \
		FAIS_TRANSTYPE(type2, type3) \
		else FAIS_TRANSTYPE(type2, type4) \
	} \
	else if (taille_de == 4) { \
		FAIS_TRANSTYPE(type3, type4) \
	}

#define FAIS_TRANSTYPE_DIMINUE(type1, type2, type3, type4) \
	if (taille_de == 8) { \
		FAIS_TRANSTYPE(type4, type3) \
		else FAIS_TRANSTYPE(type4, type2) \
		else FAIS_TRANSTYPE(type4, type1) \
	} \
	else if (taille_de == 4) { \
		FAIS_TRANSTYPE(type3, type2) \
		else FAIS_TRANSTYPE(type3, type1) \
	} \
	else if (taille_de == 2) { \
		FAIS_TRANSTYPE(type2, type1) \
	}

/* ************************************************************************** */

static auto imprime_tab(std::ostream &os, int n)
{
	for (auto i = 0; i < n - 1; ++i) {
		os << ' ';
	}
}

#if defined(DEBOGUE_VALEURS_ENTREE_SORTIE) || defined (DEBOGUE_LOCALES)
static void lis_valeur(octet_t *pointeur, Type *type, std::ostream &os)
{
	switch (type->genre) {
		default:
		{
			os << "valeur non prise en charge";
			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				os << *reinterpret_cast<char *>(pointeur);
			}
			else if (type->taille_octet == 2) {
				os << *reinterpret_cast<short *>(pointeur);
			}
			else if (type->taille_octet == 4) {
				os << *reinterpret_cast<int *>(pointeur);
			}
			else if (type->taille_octet == 8) {
				os << *reinterpret_cast<long *>(pointeur);
			}

			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		case GenreType::ENTIER_NATUREL:
		{
			if (type->taille_octet == 1) {
				os << *pointeur;
			}
			else if (type->taille_octet == 2) {
				os << *reinterpret_cast<unsigned short *>(pointeur);
			}
			else if (type->taille_octet == 4) {
				os << *reinterpret_cast<unsigned int *>(pointeur);
			}
			else if (type->taille_octet == 8) {
				os << *reinterpret_cast<unsigned long *>(pointeur);
			}

			break;
		}
		case GenreType::BOOL:
		{
			os << (*reinterpret_cast<bool *>(pointeur) ? "vrai" : "faux");
			break;
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 4) {
				os << *reinterpret_cast<float *>(pointeur);
			}
			else if (type->taille_octet == 8) {
				os << *reinterpret_cast<double *>(pointeur);
			}

			break;
		}
		case GenreType::FONCTION:
		case GenreType::POINTEUR:
		{
			os << *reinterpret_cast<void **>(pointeur);
			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_structure = type->comme_structure();

			auto virgule = "{ ";

			POUR (type_structure->membres) {
				os << virgule;
				os << it.nom << " = ";

				pointeur += it.decalage;
				lis_valeur(pointeur, it.type, os);

				virgule = ", ";
			}

			os << " }";

			break;
		}
		case GenreType::CHAINE:
		{
			auto valeur_pointeur = pointeur;
			auto valeur_chaine = *reinterpret_cast<long *>(pointeur + 8);

			kuri::chaine chaine;
			chaine.pointeur = reinterpret_cast<char *>(valeur_pointeur);
			chaine.taille = valeur_chaine;

			os << '"' << chaine << '"';

			break;
		}
	}
}
#endif

#ifdef DEBOGUE_VALEURS_ENTREE_SORTIE
static auto imprime_valeurs_entrees(octet_t *pointeur_debut_entree, TypeFonction *type_fonction, dls::chaine const &nom, int profondeur_appel)
{
	imprime_tab(std::cerr, profondeur_appel);

	std::cerr << "Appel de " << nom << '\n';

	auto index_sortie = 0;
	auto pointeur_lecture_retour = pointeur_debut_entree;
	POUR (type_fonction->types_entrees) {
		imprime_tab(std::cerr, profondeur_appel);
		std::cerr << "-- paramètre " << index_sortie << " : ";
		lis_valeur(pointeur_lecture_retour, it, std::cerr);
		std::cerr << '\n';

		pointeur_lecture_retour += it->taille_octet;
		index_sortie += 1;
	}
}

static auto imprime_valeurs_sorties(octet_t *pointeur_debut_retour, TypeFonction *type_fonction, dls::chaine const &nom, int profondeur_appel)
{
	imprime_tab(std::cerr, profondeur_appel);

	std::cerr << "Retour de " << nom << '\n';

	auto index_entree = 0;
	auto pointeur_lecture_retour = pointeur_debut_retour;
	POUR (type_fonction->types_sorties) {
		if (it->genre == GenreType::RIEN) {
			continue;
		}

		imprime_tab(std::cerr, profondeur_appel);
		std::cerr << "-- résultat " << index_entree << " : ";
		lis_valeur(pointeur_lecture_retour, it, std::cerr);
		std::cerr << '\n';

		pointeur_lecture_retour += it->taille_octet;
		index_entree += 1;
	}
}
#endif

#ifdef DEBOGUE_LOCALES
static auto imprime_valeurs_locales(FrameAppel *frame, int profondeur_appel, std::ostream &os)
{
	imprime_tab(os, profondeur_appel);
	os << frame->fonction->nom << " :\n";

	POUR (frame->fonction->chunk.locales) {
		auto pointeur_locale = &frame->pointeur_pile[it.adresse];
		imprime_tab(std::cerr, profondeur_appel);
		os << "Locale : ";

		if (it.ident) {
			os << it.ident->nom;
		}
		else {
			os << "temporaire";
		}

		os << " = ";
		lis_valeur(pointeur_locale, it.type->comme_pointeur()->type_pointe, std::cerr);
		os << '\n';
	}
}
#endif

/* ************************************************************************** */

/* Redéfini certaines fonction afin de pouvoir controler leurs comportements.
 * Par exemple, pour les fonctions d'allocations nous voudrions pouvoir libérer
 * la mémoire de notre coté, ou encore vérifier qu'il n'y ait pas de fuite de
 * mémoire dans les métaprogrammes.
 */
static void *notre_malloc(size_t n)
{
	return malloc(n);
}

static void *notre_realloc(void *ptr, size_t taille)
{
	return realloc(ptr, taille);
}

static void notre_free(void *ptr)
{
	free(ptr);
}

/* ************************************************************************** */

void GestionnaireBibliotheques::ajoute_bibliotheque(dls::chaine const &chemin)
{
	auto objet = dls::systeme_fichier::shared_library(chemin.c_str());
	bibliotheques.pousse({ std::move(objet), chemin });
}

void GestionnaireBibliotheques::ajoute_fonction_pour_symbole(IdentifiantCode *symbole, GestionnaireBibliotheques::type_fonction fonction)
{
	symboles_et_fonctions.insere({ symbole, fonction });
}

GestionnaireBibliotheques::type_fonction GestionnaireBibliotheques::fonction_pour_symbole(IdentifiantCode *symbole)
{
	auto iter = symboles_et_fonctions.trouve(symbole);

	if (iter != symboles_et_fonctions.fin()) {
		return iter->second;
	}

	POUR (bibliotheques) {
		try {
			auto ptr_symbole = it.bib(symbole->nom);
			auto fonction = reinterpret_cast<MachineVirtuelle::fonction_symbole>(ptr_symbole.ptr());
			ajoute_fonction_pour_symbole(symbole, fonction);
			return fonction;
		}
		catch (...) {
			continue;
		}
	}

	//std::cerr << "Impossible de trouver le symbole : " << symbole << '\n';
	return nullptr;
}

long GestionnaireBibliotheques::memoire_utilisee() const
{
	return bibliotheques.taille() * taille_de(BibliothequePartagee);
}

MachineVirtuelle::MachineVirtuelle()
{
	gestionnaire_bibliotheques.ajoute_bibliotheque("/lib/x86_64-linux-gnu/libc.so.6");
	gestionnaire_bibliotheques.ajoute_bibliotheque("/tmp/r16_tables_x64.so");

	gestionnaire_bibliotheques.ajoute_fonction_pour_symbole(ID::malloc_, reinterpret_cast<GestionnaireBibliotheques::type_fonction>(notre_malloc));
	gestionnaire_bibliotheques.ajoute_fonction_pour_symbole(ID::realloc_, reinterpret_cast<GestionnaireBibliotheques::type_fonction>(notre_realloc));
	gestionnaire_bibliotheques.ajoute_fonction_pour_symbole(ID::free_, reinterpret_cast<GestionnaireBibliotheques::type_fonction>(notre_free));
}

void MachineVirtuelle::reinitialise_pile()
{
	this->pointeur_pile = pile;
}

long MachineVirtuelle::depile()
{
	pointeur_pile -= 8;
	return *reinterpret_cast<long *>(pointeur_pile);
}

void MachineVirtuelle::depile(long n)
{
	pointeur_pile -= n;
}

template <typename T>
void MachineVirtuelle::empile(T valeur)
{
	*reinterpret_cast<T *>(this->pointeur_pile) = valeur;
	this->pointeur_pile += static_cast<long>(sizeof(T));
}

template <typename T>
T MachineVirtuelle::depile()
{
	this->pointeur_pile -= static_cast<long>(sizeof(T));
	return *reinterpret_cast<T *>(this->pointeur_pile);
}

bool MachineVirtuelle::appel(AtomeFonction *fonction, int /*taille_argument*/)
{
	auto frame = &frames[profondeur_appel++];
	frame->fonction = fonction;
	frame->pointeur = fonction->chunk.code;
	frame->pointeur_pile = pointeur_pile;
	return true;
}

MachineVirtuelle::ResultatInterpretation MachineVirtuelle::interprete(AtomeFonction *fonction)
{
	POUR (patchs_donnees_constantes) {
		void *adresse_ou = nullptr;
		void *adresse_quoi = nullptr;

		if (it.quoi == ADRESSE_CONSTANTE) {
			adresse_quoi = donnees_constantes.donnees() + it.decalage_quoi;
		}
		else {
			adresse_quoi = donnees_globales.donnees() + it.decalage_quoi;
		}

		if (it.ou == DONNEES_CONSTANTES) {
			adresse_ou = donnees_constantes.donnees() + it.decalage_ou;
		}
		else {
			adresse_ou = donnees_globales.donnees() + it.decalage_ou;
		}

		*reinterpret_cast<void **>(adresse_ou) = adresse_quoi;
		//std::cerr << "Écris adresse : " << adresse_quoi << ", à " << adresse_ou << '\n';
	}

	profondeur_appel = 0;
	nombre_de_metaprogrammes_executes += 1;
	reinitialise_pile();
	appel(fonction, 0);
	return lance();
}

MachineVirtuelle::ResultatInterpretation MachineVirtuelle::lance()
{
	auto frame = &frames[profondeur_appel - 1];

#ifdef DEBOGUE_INTERPRETEUSE
	std::cerr << "== exécution " << frame->fonction->nom << " ==\n";
#endif

	auto debut_execution = dls::chrono::compte_seconde();

	auto appel_fonction_interne = [this, &frame](AtomeFonction *ptr_fonction, int taille_argument)
	{
		// puisque les arguments utilisent des instructions d'allocations retire la taille des arguments du pointeur
		// de la pile pour ne pas que les allocations ne l'augmente
		pointeur_pile -= taille_argument;

#ifdef DEBOGUE_VALEURS_ENTREE_SORTIE
		imprime_valeurs_entrees(pointeur_pile, ptr_fonction->type->comme_fonction(), ptr_fonction->nom, profondeur_appel);
#endif

		if (!appel(ptr_fonction, taille_argument)) {
			return false;
		}

		frame = &frames[profondeur_appel - 1];
		return true;
	};

	auto appel_fonction_externe = [this](AtomeFonction *ptr_fonction, int taille_argument, InstructionAppel *inst_appel)
	{
		auto type_fonction = ptr_fonction->decl->type->comme_fonction();
		auto &donnees_externe = ptr_fonction->donnees_externe;

		auto pointeur_arguments = pointeur_pile - taille_argument;

#ifdef DEBOGUE_VALEURS_ENTREE_SORTIE
		imprime_valeurs_entrees(pointeur_arguments, type_fonction, ptr_fonction->nom, profondeur_appel);
#endif

		auto pointeurs_arguments = dls::tablet<void *, 12>();
		auto decalage_argument = 0u;

		if (ptr_fonction->decl->est_variadique) {
			auto nombre_arguments_fixes = static_cast<unsigned>(type_fonction->types_entrees.taille - 1);
			auto nombre_arguments_totaux = static_cast<unsigned>(inst_appel->args.taille);

			donnees_externe.types_entrees.efface();
			donnees_externe.types_entrees.reserve(nombre_arguments_totaux);

			POUR (inst_appel->args) {
				auto type = converti_type_ffi(it->type);
				donnees_externe.types_entrees.pousse(type);

				auto ptr = &pointeur_arguments[decalage_argument];
				pointeurs_arguments.pousse(ptr);

				if (it->type->genre == GenreType::ENTIER_CONSTANT) {
					decalage_argument += 4;
				}
				else {
					decalage_argument += it->type->taille_octet;
				}
			}

			donnees_externe.types_entrees.pousse(nullptr);

			auto type_ffi_sortie = converti_type_ffi(type_fonction->types_sorties[0]);
			auto ptr_types_entrees = donnees_externe.types_entrees.donnees();

			auto status = ffi_prep_cif_var(&donnees_externe.cif, FFI_DEFAULT_ABI, nombre_arguments_fixes, nombre_arguments_totaux, type_ffi_sortie, ptr_types_entrees);

			if (status != FFI_OK) {
				std::cerr << "Impossible de préparer la fonction variadique externe !\n";
				return;
			}
		}
		else {
			POUR (type_fonction->types_entrees) {
				auto ptr = &pointeur_arguments[decalage_argument];
				pointeurs_arguments.pousse(ptr);
				decalage_argument += it->taille_octet;
			}
		}

		ffi_call(&donnees_externe.cif, ptr_fonction->donnees_externe.ptr_fonction, pointeur_pile, pointeurs_arguments.donnees());

		auto taille_type_retour = type_fonction->types_sorties[0]->taille_octet;

		if (taille_type_retour != 0) {
			memcpy(pointeur_arguments, pointeur_pile, taille_type_retour);
		}

		// écrase la liste d'arguments
		pointeur_pile = pointeur_arguments + taille_type_retour;
	};

	for (;;) {
#ifdef DEBOGUE_INTERPRETEUSE
		auto &sortie = std::cerr;
		imprime_tab(sortie, profondeur_appel);
		desassemble_instruction(frame->fonction->chunk, (frame->pointeur - frame->fonction->chunk.code), sortie);
#endif
		auto instruction = LIS_OCTET();

		switch (instruction) {
			case OP_LABEL:
			{
				// saute le label
				frame->pointeur += 4;
				break;
			}
			case OP_BRANCHE:
			{
				auto decalage = LIS_4_OCTETS();
				frame->pointeur = frame->fonction->chunk.code + decalage;
				break;
			}
			case OP_BRANCHE_CONDITION:
			{
				auto decalage_si_vrai = LIS_4_OCTETS();
				auto decalage_si_faux = LIS_4_OCTETS();
				auto condition = depile<bool>();

				if (condition) {
					frame->pointeur = frame->fonction->chunk.code + decalage_si_vrai;
				}
				else {
					frame->pointeur = frame->fonction->chunk.code + decalage_si_faux;
				}

				break;
			}
			case OP_CONSTANTE:
			{
				empile_constante(frame);
				break;
			}
			case OP_CHAINE_CONSTANTE:
			{
				auto pointeur_chaine = LIS_8_OCTETS();
				auto taille_chaine = LIS_8_OCTETS();
				empile(pointeur_chaine);
				empile(taille_chaine);
				break;
			}
			case OP_COMPLEMENT_ENTIER:
			{
				OP_UNAIRE(-);
				break;
			}
			case OP_COMPLEMENT_REEL:
			{
				OP_UNAIRE_REEL(-);
				break;
			}
			case OP_NON_BINAIRE:
			{
				OP_UNAIRE(~);
				break;
			}
			case OP_AJOUTE:
			{
				OP_BINAIRE(+);
				break;
			}
			case OP_SOUSTRAIT:
			{
				OP_BINAIRE(-);
				break;
			}
			case OP_MULTIPLIE:
			{
				OP_BINAIRE(*);
				break;
			}
			case OP_DIVISE:
			{
				OP_BINAIRE_NATUREL(/);
				break;
			}
			case OP_DIVISE_RELATIF:
			{
				OP_BINAIRE(/);
				break;
			}
			case OP_AJOUTE_REEL:
			{
				OP_BINAIRE_REEL(+);
				break;
			}
			case OP_SOUSTRAIT_REEL:
			{
				OP_BINAIRE_REEL(-);
				break;
			}
			case OP_MULTIPLIE_REEL:
			{
				OP_BINAIRE_REEL(*);
				break;
			}
			case OP_DIVISE_REEL:
			{
				OP_BINAIRE_REEL(/);
				break;
			}
			case OP_RESTE_NATUREL:
			{
				OP_BINAIRE_NATUREL(%);
				break;
			}
			case OP_RESTE_RELATIF:
			{
				OP_BINAIRE(%);
				break;
			}
			case OP_COMP_EGAL:
			{
				OP_BINAIRE(==);
				break;
			}
			case OP_COMP_INEGAL:
			{
				OP_BINAIRE(!=);
				break;
			}
			case OP_COMP_INF:
			{
				OP_BINAIRE(<);
				break;
			}
			case OP_COMP_INF_EGAL:
			{
				OP_BINAIRE(<=);
				break;
			}
			case OP_COMP_SUP:
			{
				OP_BINAIRE(>);
				break;
			}
			case OP_COMP_SUP_EGAL:
			{
				OP_BINAIRE(>=);
				break;
			}
			case OP_COMP_INF_NATUREL:
			{
				OP_BINAIRE_NATUREL(<);
				break;
			}
			case OP_COMP_INF_EGAL_NATUREL:
			{
				OP_BINAIRE_NATUREL(<=);
				break;
			}
			case OP_COMP_SUP_NATUREL:
			{
				OP_BINAIRE_NATUREL(>);
				break;
			}
			case OP_COMP_SUP_EGAL_NATUREL:
			{
				OP_BINAIRE_NATUREL(>=);
				break;
			}
			case OP_COMP_EGAL_REEL:
			{
				OP_BINAIRE_REEL(==);
				break;
			}
			case OP_COMP_INEGAL_REEL:
			{
				OP_BINAIRE_REEL(!=);
				break;
			}
			case OP_COMP_INF_REEL:
			{
				OP_BINAIRE_REEL(<);
				break;
			}
			case OP_COMP_INF_EGAL_REEL:
			{
				OP_BINAIRE_REEL(<=);
				break;
			}
			case OP_COMP_SUP_REEL:
			{
				OP_BINAIRE_REEL(>);
				break;
			}
			case OP_COMP_SUP_EGAL_REEL:
			{
				OP_BINAIRE_REEL(>=);
				break;
			}
			case OP_ET_LOGIQUE:
			{
				OP_BINAIRE(&&);
				break;
			}
			case OP_OU_LOGIQUE:
			{
				OP_BINAIRE(||);
				break;
			}
			case OP_ET_BINAIRE:
			{
				OP_BINAIRE(&);
				break;
			}
			case OP_OU_BINAIRE:
			{
				OP_BINAIRE(|);
				break;
			}
			case OP_OU_EXCLUSIF:
			{
				OP_BINAIRE(^);
				break;
			}
			case OP_DEC_GAUCHE:
			{
				OP_BINAIRE(<<);
				break;
			}
			case OP_DEC_DROITE_ARITHM:
			{
				OP_BINAIRE(>>);
				break;
			}
			case OP_DEC_DROITE_LOGIQUE:
			{
				OP_BINAIRE_NATUREL(>>);
				break;
			}
			case OP_AUGMENTE_NATUREL:
			{
				auto taille_de = LIS_4_OCTETS();
				auto taille_vers = LIS_4_OCTETS();
				FAIS_TRANSTYPE_AUGMENTE(unsigned char, unsigned short, unsigned int, unsigned long);
				break;
			}
			case OP_DIMINUE_NATUREL:
			{
				auto taille_de = LIS_4_OCTETS();
				auto taille_vers = LIS_4_OCTETS();
				FAIS_TRANSTYPE_DIMINUE(unsigned char, unsigned short, unsigned int, unsigned long);
				break;
			}
			case OP_AUGMENTE_RELATIF:
			{
				auto taille_de = LIS_4_OCTETS();
				auto taille_vers = LIS_4_OCTETS();
				FAIS_TRANSTYPE_AUGMENTE(char, short, int, long);
				break;
			}
			case OP_DIMINUE_RELATIF:
			{
				auto taille_de = LIS_4_OCTETS();
				auto taille_vers = LIS_4_OCTETS();
				FAIS_TRANSTYPE_DIMINUE(unsigned char, unsigned short, unsigned int, unsigned long);
				break;
			}
			case OP_AUGMENTE_REEL:
			{
				auto taille_de = LIS_4_OCTETS();
				auto taille_vers = LIS_4_OCTETS();

				if (taille_de == 2 && taille_vers == 4) {
					auto v = depile<float>();
					empile(static_cast<double>(v));
				}

				break;
			}
			case OP_DIMINUE_REEL:
			{
				auto taille_de = LIS_4_OCTETS();
				auto taille_vers = LIS_4_OCTETS();

				if (taille_de == 4 && taille_vers == 2) {
					auto v = depile<double>();
					empile(static_cast<float>(v));
				}

				break;
			}
			case OP_RETOURNE:
			{
				auto type_fonction = frame->fonction->type->comme_fonction();
				auto taille_retour = 0;

				POUR (type_fonction->types_sorties) {
					taille_retour += static_cast<int>(it->taille_octet);
				}

				auto pointeur_debut_retour = pointeur_pile - taille_retour;

#ifdef DEBOGUE_LOCALES
				imprime_valeurs_locales(frame, profondeur_appel, std::cerr);
#endif

#ifdef DEBOGUE_VALEURS_ENTREE_SORTIE
				imprime_valeurs_sorties(pointeur_debut_retour, type_fonction, frame->fonction->nom, profondeur_appel);
#endif

				profondeur_appel--;

				if (profondeur_appel == 0) {
					if (pointeur_pile != pile) {
						pointeur_pile = pointeur_debut_retour;
						if (type_fonction->types_sorties[0]->genre == GenreType::ENTIER_RELATIF && type_fonction->types_sorties[0]->taille_octet == 4) {
							auto resultat = *reinterpret_cast<int *>(pointeur_debut_retour);
							std::cerr << "Résultat : " << resultat << '\n';
						}
					}

					auto temps = debut_execution.temps();
					temps_execution_metaprogammes += temps;

#if defined(DEBOGUE_INTERPRETEUSE) || defined(CHRONOMETRE_INTERPRETATION)
					std::cerr << "Temps exécution : " << temps * 1000.0 << " ms\n";
#endif
					return ResultatInterpretation::OK;
				}

				pointeur_pile = frame->pointeur_pile;

				if (taille_retour != 0 && pointeur_pile != pointeur_debut_retour) {
					memcpy(pointeur_pile, pointeur_debut_retour, static_cast<unsigned>(taille_retour));
				}

				pointeur_pile += taille_retour;

				frame = &frames[profondeur_appel - 1];
				break;
			}
			case OP_APPEL:
			{
				auto valeur_ptr = LIS_8_OCTETS();
				auto taille_argument = LIS_4_OCTETS();
				// saute l'instruction d'appel
				frame->pointeur += 8;
				auto ptr_fonction = reinterpret_cast<AtomeFonction *>(valeur_ptr);

#ifdef DEBOGUE_INTERPRETEUSE
				std::cerr << "-- appel : " << ptr_fonction->nom << '\n';
#endif

				if (!appel_fonction_interne(ptr_fonction, taille_argument)) {
					return ResultatInterpretation::ERREUR;
				}

				break;
			}
			case OP_APPEL_EXTERNE:
			{
				auto valeur_ptr = LIS_8_OCTETS();
				auto taille_argument = LIS_4_OCTETS();
				auto valeur_inst = LIS_8_OCTETS();
				auto ptr_fonction = reinterpret_cast<AtomeFonction *>(valeur_ptr);
				auto ptr_inst_appel = reinterpret_cast<InstructionAppel *>(valeur_inst);
				appel_fonction_externe(ptr_fonction, taille_argument, ptr_inst_appel);
				break;
			}
			case OP_APPEL_POINTEUR:
			{
				auto taille_argument = LIS_4_OCTETS();
				auto valeur_inst = LIS_8_OCTETS();
				auto adresse = depile<void *>();
				auto ptr_fonction = reinterpret_cast<AtomeFonction *>(adresse);
				auto ptr_inst_appel = reinterpret_cast<InstructionAppel *>(valeur_inst);

				if (ptr_fonction->est_externe) {
					appel_fonction_externe(ptr_fonction, taille_argument, ptr_inst_appel);
				}
				else {
					if (!appel_fonction_interne(ptr_fonction, taille_argument)) {
						return ResultatInterpretation::ERREUR;
					}
				}

				break;
			}
			case OP_ASSIGNE:
			{
				auto taille = LIS_4_OCTETS();
				auto adresse_ou = depile<void *>();
				auto adresse_de = static_cast<void *>(this->pointeur_pile - taille);
//				std::cerr << "adresse_ou : " << adresse_ou << '\n';
//				std::cerr << "adresse_de : " << adresse_de << '\n';
				memcpy(adresse_ou, adresse_de, static_cast<size_t>(taille));
				depile(taille);
				break;
			}
			case OP_ALLOUE:
			{
				auto type = LIS_POINTEUR(Type);
				// saute l'identifiant
				frame->pointeur += 8;
				this->pointeur_pile += type->taille_octet;
				break;
			}
			case OP_CHARGE:
			{
				auto taille = LIS_4_OCTETS();
				auto adresse_de = depile<void *>();
				auto adresse_ou = static_cast<void *>(this->pointeur_pile);
				memcpy(adresse_ou, adresse_de, static_cast<size_t>(taille));
				this->pointeur_pile += taille;
				break;
			}
			case OP_REFERENCE_VARIABLE:
			{
				auto index = LIS_4_OCTETS();
				auto const &locale = frame->fonction->chunk.locales[index];
				empile(&frame->pointeur_pile[locale.adresse]);
				break;
			}
			case OP_REFERENCE_GLOBALE:
			{
				auto index = LIS_4_OCTETS();
				auto const &globale = this->globales[index];
				empile(&donnees_globales[globale.adresse]);
				break;
			}
			case OP_REFERENCE_MEMBRE:
			{
				auto decalage = LIS_4_OCTETS();
				auto adresse_de = depile<char *>();
				empile(adresse_de + decalage);
				//std::cerr << "adresse_de : " << static_cast<void *>(adresse_de) << '\n';
				break;
			}
			case OP_ACCEDE_INDEX:
			{
				auto taille_donnees = LIS_4_OCTETS();
				auto adresse = depile<char *>();
				auto index = depile<long>();
				auto nouvelle_adresse = adresse + index * taille_donnees;
				empile(nouvelle_adresse);
//				std::cerr << "nouvelle_adresse : " << static_cast<void *>(nouvelle_adresse) << '\n';
//				std::cerr << "index            : " << index << '\n';
//				std::cerr << "taille_donnees   : " << taille_donnees << '\n';
				break;
			}
			default:
			{
				std::cerr << "Opération inconnue dans la MV\n";
				return ResultatInterpretation::ERREUR;
			}
		}
	}
}

void MachineVirtuelle::empile_constante(FrameAppel *frame)
{
	auto drapeaux = LIS_OCTET();

#define EMPILE_CONSTANTE(type) \
	type v = *(reinterpret_cast<type *>(frame->pointeur)); \
	empile(v); \
	frame->pointeur += (drapeaux >> 3); \
	break;

	switch (drapeaux) {
		case CONSTANTE_ENTIER_RELATIF | BITS_8:
		{
			EMPILE_CONSTANTE(char);
		}
		case CONSTANTE_ENTIER_RELATIF | BITS_16:
		{
			EMPILE_CONSTANTE(short);
		}
		case CONSTANTE_ENTIER_RELATIF | BITS_32:
		{
			EMPILE_CONSTANTE(int);
		}
		case CONSTANTE_ENTIER_RELATIF | BITS_64:
		{
			EMPILE_CONSTANTE(long);
		}
		case CONSTANTE_ENTIER_NATUREL | BITS_8:
		{
			// erreur de compilation pour transtype inutile avec drapeaux stricts
			empile(LIS_OCTET());
			break;
		}
		case CONSTANTE_ENTIER_NATUREL | BITS_16:
		{
			EMPILE_CONSTANTE(unsigned short);
		}
		case CONSTANTE_ENTIER_NATUREL | BITS_32:
		{
			EMPILE_CONSTANTE(unsigned int);
		}
		case CONSTANTE_ENTIER_NATUREL | BITS_64:
		{
			EMPILE_CONSTANTE(unsigned long);
		}
		case CONSTANTE_NOMBRE_REEL | BITS_32:
		{
			EMPILE_CONSTANTE(float);
		}
		case CONSTANTE_NOMBRE_REEL | BITS_64:
		{
			EMPILE_CONSTANTE(double);
		}
	}

#undef EMPILE_CONSTANTE
}

MachineVirtuelle::fonction_symbole MachineVirtuelle::trouve_symbole(IdentifiantCode *symbole)
{
	return gestionnaire_bibliotheques.fonction_pour_symbole(symbole);
}

int MachineVirtuelle::ajoute_globale(Type *type, IdentifiantCode *ident)
{
	auto globale = Globale{};
	globale.type = type;
	globale.ident = ident;
	globale.adresse = static_cast<int>(donnees_globales.taille());

	donnees_globales.redimensionne(donnees_globales.taille() + static_cast<long>(type->taille_octet));

	auto ptr = static_cast<int>(globales.taille());

	globales.pousse(globale);

	return ptr;
}

std::ostream &operator<<(std::ostream &os, PatchDonneesConstantes const &patch)
{
	os << "Patch données constantes :\n";

	os << "-- où           : ";
	if (patch.ou == DONNEES_CONSTANTES) {
		os << "données constantes\n";
	}
	else {
		os << "données globales\n";
	}

	os << "-- quoi         : ";
	if (patch.quoi == ADRESSE_CONSTANTE) {
		os << "adresse constante\n";
	}
	else {
		os << "adresse globale\n";
	}

	os << "-- adresse quoi : " << patch.decalage_quoi << '\n';
	os << "-- adresse où   : " << patch.decalage_ou << '\n';

	return os;
}
