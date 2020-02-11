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

#include "lexeuse.hh"

#include "biblinternes/langage/nombres.hh"
#include "biblinternes/langage/outils.hh"
#include "biblinternes/langage/unicode.hh"

#include "erreur.h"

/* ************************************************************************** */

/* Point-virgule implicite.
 *
 * Un point-virgule est ajouter quand nous rencontrons une nouvelle ligne si le
 * dernier identifiant correspond à l'un des cas suivants :
 *
 * - une chaine de caractère (nom de variable) ou un type
 * - une littérale (nombre, chaine, faux, vrai)
 * - une des instructions de controle de flux suivantes : retourne, arrête, continue
 * - une parenthèse ou en un crochet fermant
 */
static bool doit_ajouter_point_virgule(GenreLexeme dernier_id)
{
	switch (dernier_id) {
		default:
		{
			return false;
		}
		/* types */
		case GenreLexeme::N8:
		case GenreLexeme::N16:
		case GenreLexeme::N32:
		case GenreLexeme::N64:
		case GenreLexeme::N128:
		case GenreLexeme::R16:
		case GenreLexeme::R32:
		case GenreLexeme::R64:
		case GenreLexeme::R128:
		case GenreLexeme::Z8:
		case GenreLexeme::Z16:
		case GenreLexeme::Z32:
		case GenreLexeme::Z64:
		case GenreLexeme::Z128:
		case GenreLexeme::BOOL:
		case GenreLexeme::RIEN:
		case GenreLexeme::EINI:
		case GenreLexeme::CHAINE:
		case GenreLexeme::OCTET:
		case GenreLexeme::CHAINE_CARACTERE:
		/* littérales */
		case GenreLexeme::CHAINE_LITTERALE:
		case GenreLexeme::NOMBRE_REEL:
		case GenreLexeme::NOMBRE_ENTIER:
		case GenreLexeme::NOMBRE_OCTAL:
		case GenreLexeme::NOMBRE_HEXADECIMAL:
		case GenreLexeme::NOMBRE_BINAIRE:
		case GenreLexeme::CARACTERE:
		case GenreLexeme::VRAI:
		case GenreLexeme::FAUX:
		case GenreLexeme::NUL:
		/* instructions */
		case GenreLexeme::ARRETE:
		case GenreLexeme::CONTINUE:
		case GenreLexeme::RETOURNE:
		/* fermeture */
		case GenreLexeme::PARENTHESE_FERMANTE:
		case GenreLexeme::CROCHET_FERMANT:
		{
			return true;
		}
	}
}

/* ************************************************************************** */

Lexeuse::Lexeuse(Fichier *fichier, int drapeaux)
	: m_fichier(fichier)
	, m_debut_mot(fichier->tampon.debut())
	, m_debut(fichier->tampon.debut())
	, m_fin(fichier->tampon.fin())
	, m_drapeaux(drapeaux)
{
	construit_tables_caractere_speciaux();
}

void Lexeuse::performe_lexage()
{
	m_taille_mot_courant = 0;

	while (!this->fini()) {
		auto nombre_octet = lng::nombre_octets(m_debut);

		switch (nombre_octet) {
			case 1:
			{
				analyse_caractere_simple();
				break;
			}
			case 2:
			case 3:
			case 4:
			{
				if (m_taille_mot_courant == 0) {
					this->enregistre_pos_mot();
				}

				auto c = lng::converti_utf32(m_debut, nombre_octet);

				switch (c) {
					case ESPACE_INSECABLE:
					case ESPACE_D_OGAM:
					case SEPARATEUR_VOYELLES_MONGOL:
					case DEMI_CADRATIN:
					case CADRATIN:
					case ESPACE_DEMI_CADRATIN:
					case ESPACE_CADRATIN:
					case TIERS_DE_CADRATIN:
					case QUART_DE_CADRATIN:
					case SIXIEME_DE_CADRATIN:
					case ESPACE_TABULAIRE:
					case ESPACE_PONCTUATION:
					case ESPACE_FINE:
					case ESPACE_ULTRAFINE:
					case ESPACE_SANS_CHASSE:
					case ESPACE_INSECABLE_ETROITE:
					case ESPACE_MOYENNE_MATHEMATIQUE:
					case ESPACE_IDEOGRAPHIQUE:
					case ESPACE_INSECABLE_SANS_CHASSE:
					{
						if (m_taille_mot_courant != 0) {
							this->pousse_mot(id_chaine(this->mot_courant()));
						}

						if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
							this->enregistre_pos_mot();
							this->pousse_caractere(nombre_octet);
							this->pousse_mot(GenreLexeme::CARACTERE_BLANC);
						}

						this->avance(nombre_octet);

						break;
					}
					case GUILLEMET_OUVRANT:
					{
						if (m_taille_mot_courant != 0) {
							this->pousse_mot(id_chaine(this->mot_courant()));
						}

						/* Saute le premier guillemet si nécessaire. */
						if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
							this->enregistre_pos_mot();
							this->pousse_caractere(nombre_octet);
							this->avance(nombre_octet);
						}
						else {
							this->avance(nombre_octet);
							this->enregistre_pos_mot();
						}

						while (!this->fini()) {
							nombre_octet = lng::nombre_octets(m_debut);
							c = lng::converti_utf32(m_debut, nombre_octet);

							if (c == GUILLEMET_FERMANT) {
								break;
							}

							m_taille_mot_courant += nombre_octet;
							this->avance(nombre_octet);
						}

						/* Saute le dernier guillemet si nécessaire. */
						if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
							this->pousse_caractere(nombre_octet);
						}

						this->avance(nombre_octet);

						this->pousse_mot(GenreLexeme::CHAINE_LITTERALE);
						break;
					}
					default:
					{
						m_taille_mot_courant += nombre_octet;
						this->avance(nombre_octet);
						break;
					}
				}

				break;
			}
			default:
			{
				/* Le caractère (octet) courant est invalide dans le codec unicode. */
				lance_erreur("Le codec Unicode ne peut comprendre le caractère !");
			}
		}
	}

	if (m_taille_mot_courant != 0) {
		lance_erreur("Des caractères en trop se trouvent à la fin du texte !");
	}
}

size_t Lexeuse::memoire_lexemes() const
{
	return static_cast<size_t>(m_fichier->lexemes.taille()) * sizeof(DonneesLexeme);
}

void Lexeuse::imprime_lexemes(std::ostream &os)
{
	for (auto const &lexeme : m_fichier->lexemes) {
		os << chaine_identifiant(lexeme.genre) << '\n';
	}
}

bool Lexeuse::fini() const
{
	return m_debut >= m_fin;
}

void Lexeuse::avance(int n)
{
	for (int i = 0; i < n; ++i) {
		if (this->caractere_courant() == '\n') {
			++m_compte_ligne;
			m_position_ligne = 0;
		}
		else {
			++m_position_ligne;
		}

		++m_debut;
	}
}

char Lexeuse::caractere_courant() const
{
	return *m_debut;
}

char Lexeuse::caractere_voisin(int n) const
{
	return *(m_debut + n);
}

dls::vue_chaine_compacte Lexeuse::mot_courant() const
{
	return dls::vue_chaine_compacte(m_debut_mot, m_taille_mot_courant);
}

void Lexeuse::lance_erreur(const dls::chaine &quoi) const
{
	auto ligne_courante = m_fichier->tampon[m_compte_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : ligne:" << m_compte_ligne + 1 << ":\n";
	ss << ligne_courante;

	/* La position ligne est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (auto i = 0l; i < m_position_ligne;) {
		if (ligne_courante[i] == '\t') {
			ss << '\t';
		}
		else {
			ss << ' ';
		}

		i += lng::decalage_pour_caractere(ligne_courante, i);
	}

	ss << "^~~~\n";
	ss << quoi;

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::DECOUPAGE);
}

// si caractere blanc:
//    ajoute mot
// sinon si caractere speciale:
//    ajoute mot
//    si caractere suivant constitue caractere double
//        ajoute mot caractere double
//    sinon
//        si caractere est '.':
//            decoupe trois point
//        sinon si caractere est '"':
//            decoupe chaine caractere littérale
//        sinon si caractere est '#':
//            decoupe commentaire
//        sinon si caractere est '\'':
//            decoupe caractere
//        sinon:
//        	ajoute mot caractere simple
// sinon si nombre et mot est vide:
//    decoupe nombre
// sinon:
//    ajoute caractere mot courant
void Lexeuse::analyse_caractere_simple()
{
	auto idc = GenreLexeme::INCONNU;

	if (lng::est_espace_blanc(this->caractere_courant())) {
		if (m_taille_mot_courant != 0) {
			this->pousse_mot(id_chaine(this->mot_courant()));
		}

		if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
			this->enregistre_pos_mot();
			this->pousse_caractere();
			this->pousse_mot(GenreLexeme::CARACTERE_BLANC);
		}

		if (this->caractere_courant() == '\n') {
			if (doit_ajouter_point_virgule(m_dernier_id)) {
				this->enregistre_pos_mot();
				this->pousse_caractere();
				this->pousse_mot(GenreLexeme::POINT_VIRGULE);
			}
		}

		this->avance();
	}
	else if (est_caractere_special(this->caractere_courant(), idc)) {
		if (m_taille_mot_courant != 0) {
			this->pousse_mot(id_chaine(this->mot_courant()));
		}

		this->enregistre_pos_mot();

		auto id = id_trigraphe(dls::vue_chaine_compacte(m_debut, 3));

		if (id != GenreLexeme::INCONNU) {
			this->pousse_caractere(3);
			this->pousse_mot(id);
			this->avance(3);
			return;
		}

		id = id_digraphe(dls::vue_chaine_compacte(m_debut, 2));

		if (id != GenreLexeme::INCONNU) {
			if (id == GenreLexeme::DEBUT_LIGNE_COMMENTAIRE) {
				lexe_commentaire();
				return;
			}

			this->pousse_caractere(2);
			this->pousse_mot(id);
			this->avance(2);
			return;
		}

		switch (this->caractere_courant()) {
			case '.':
			{
				this->pousse_caractere();
				this->pousse_mot(GenreLexeme::POINT);
				this->avance();
				break;
			}
			case '"':
			{
				/* Saute le premier guillemet si nécessaire. */
				if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
					this->enregistre_pos_mot();
					this->pousse_caractere();
					this->avance();
				}
				else {
					this->avance();
					this->enregistre_pos_mot();
				}

				while (!this->fini()) {
					if (this->caractere_courant() == '"' && this->caractere_voisin(-1) != '\\') {
						break;
					}

					this->pousse_caractere();
					this->avance();
				}

				/* Saute le dernier guillemet si nécessaire. */
				if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
					this->pousse_caractere();
				}

				this->avance();

				this->pousse_mot(GenreLexeme::CHAINE_LITTERALE);
				break;
			}
			case '\'':
			{
				/* Saute la première apostrophe si nécessaire. */
				if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
					this->enregistre_pos_mot();
					this->pousse_caractere();
					this->avance();
				}
				else {
					this->avance();
					this->enregistre_pos_mot();
				}

				if (this->caractere_courant() == '\\') {
					this->pousse_caractere();
					this->avance();
				}

				this->pousse_caractere();
				this->avance();				

				/* Saute la dernière apostrophe si nécessaire. */
				if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
					this->pousse_caractere();
				}

				if (this->caractere_courant() != '\'') {
					lance_erreur("Plusieurs caractères détectés dans un caractère simple !\n");
				}

				this->avance();
				this->pousse_mot(GenreLexeme::CARACTERE);
				break;
			}
			case '#':
			{
				lexe_commentaire();
				break;
			}
			default:
			{
				this->pousse_caractere();
				this->pousse_mot(idc);
				this->avance();
				break;
			}
		}
	}
	else if (m_taille_mot_courant == 0 && lng::est_nombre_decimal(this->caractere_courant())) {
		this->enregistre_pos_mot();

		using denombreuse = lng::decoupeuse_nombre<GenreLexeme>;
		GenreLexeme id_nombre;

		/* NOTE : on utilise une variable temporaire pour stocker le compte au
		 * lieu d'utiliser m_taille_mot_courant directement, car
		 * m_taille_mot_courant est remis à 0 dans pousse_mot() donc on ne peut
		 * l'utiliser en paramètre de avance() (ce qui causerait une boucle
		 * infinie. */
		auto const compte = denombreuse::extrait_nombre(m_debut, m_fin, id_nombre);
		m_taille_mot_courant = static_cast<long>(compte);

		this->pousse_mot(id_nombre);
		this->avance(static_cast<int>(compte));
	}
	else {
		if (m_taille_mot_courant == 0) {
			this->enregistre_pos_mot();
		}

		this->pousse_caractere();
		this->avance();
	}
}

void Lexeuse::pousse_caractere(int n)
{
	m_taille_mot_courant += n;
}

void Lexeuse::pousse_mot(GenreLexeme identifiant)
{
	m_fichier->lexemes.pousse({ mot_courant(), identifiant, static_cast<int>(m_fichier->id) });
	m_taille_mot_courant = 0;
	m_dernier_id = identifiant;
}

void Lexeuse::enregistre_pos_mot()
{
	m_pos_mot = m_position_ligne;
	m_debut_mot = m_debut;
}

void Lexeuse::lexe_commentaire()
{
	if ((m_drapeaux & INCLUS_COMMENTAIRES) != 0) {
		this->enregistre_pos_mot();
	}

	/* ignore commentaire */
	while (this->caractere_courant() != '\n') {
		this->avance();
		this->pousse_caractere();
	}

	if ((m_drapeaux & INCLUS_COMMENTAIRES) != 0) {
		this->pousse_mot(GenreLexeme::COMMENTAIRE);
	}

	/* Lorsqu'on inclus pas les commentaires, il faut ignorer les
	 * caractères poussées. */
	m_taille_mot_courant = 0;
}
