

template <typename T>
struct ConceptDecoupeuse {
	static bool est_caractere_blanc(char);
	static bool est_caractere_special(char, int&);
	static bool est_caractere_double(char, char, int&);
	static bool est_caractere_litteral(char);
	static bool est_commentaire(char);
	static bool est_chaine_litterale(char);
	static bool id_chaine(char);
	static bool est_nombre_decimal(char);
	static size_t extrait_nombre(const char *, const char *, int&);

	void pousse_mot(std::string_view, size_t, size_t, size_t);
};

template <typename T>
struct decoupeuse {
	const char *m_debut;
	const char *m_debut_mot;
	const char *m_fin;
	size_t m_taille_mot_courant;
	size_t m_compte_ligne;
	size_t m_position_ligne;
	size_t m_pos_mot;

	void pousse_mot(int id)
	{
		this->pousse_mot(this->mot_courant(), m_compte_ligne, m_position_ligne, id);
	}

	void genere_morceaux()
	{
		while (!this->fini()) {
			const auto nombre_octet = nombre_octets(m_debut);

			if (nombre_octet == 1) {
				this->analyse_caractere_simple();
			}
			else if (nombre_octet >= 2 && nombre_octet <= 4) {
				/* Les caractères spéciaux ne peuvent être des caractères unicode
				 * pour le moment, donc on les copie directement dans le tampon du
				 * mot_courant. */
				if (m_taille_mot_courant == 0) {
					this->enregistre_pos_mot();
				}

				//m_taille_mot_courant += static_cast<size_t>(nombre_octet);
				this->avance(nombre_octet);
			}
			else {
				/* Le caractère (octet) courant est invalide dans le codec unicode. */
				this->lance_erreur("Le codec Unicode ne peut comprendre le caractère !");
			}
		}
	}

	void analyse_caractere_simple()
	{
		int idc;

		if (T::est_caractere_blanc(this->caractere_courant())) {
			if (m_taille_mot_courant != 0) {
				this->pousse_mot(T::id_chaine(this->mot_courant()));
			}

			this->avance();
		}
		else if (T::est_caractere_special(this->caractere_courant(), idc)) {
			if (m_taille_mot_courant != 0) {
				this->pousse_mot(T::id_chaine(this->mot_courant()));
			}

			if (T::est_caractere_double(this->caractere_courant(), this->caractere_voisin(), idc)) {
				this->enregistre_pos_mot();
				this->pousse_caractere();
				this->pousse_caractere();
				this->pousse_mot(idc);
				this->avance(2);
			}
			else if (T::est_caractere_litteral(this->caractere_courant(), idc)) {
				/* Saute la première apostrophe. */
				this->avance();

				this->enregistre_pos_mot();

				if (this->caractere_courant() == '\\') {
					this->pousse_caractere();
					this->avance();
				}

				this->pousse_caractere();
				this->pousse_mot(idc);

				this->avance();

				/* Saute la dernière apostrophe. */
				if (T::est_caractere_litteral(this->caractere_courant(), idc)) {
					this->lance_erreur("Plusieurs caractères détectés dans un caractère simple !\n");
				}

				this->avance();
			}
			else if (T::est_commentaire(this->caractere_courant())) {
				/* ignore commentaire */
				while (this->caractere_courant() != '\n') {
					this->avance();
				}
			}
			else if (T::est_chaine_litterale(this->caractere_courant(), idc)) {
				// Saute le premier guillemet.
				this->avance();
				this->enregistre_pos_mot();

				while (!this->fini()) {
					if (this->caractere_courant() == '"' && this->caractere_voisin(-1) != '\\') {
						break;
					}

					this->pousse_caractere();
					this->avance();
				}

				// Saute le dernier guillemet.
				this->avance();

				this->pousse_mot(idc);
			}
			else {
				this->enregistre_pos_mot();
				this->pousse_caractere();
				this->pousse_mot(idc);
				this->avance();
			}
		}
		else if (T::est_nombre_decimal(this->caractere_courant()) && m_taille_mot_courant == 0) {
			this->enregistre_pos_mot();

			int id_nombre;
			const auto compte = T::extrait_nombre(m_debut, m_fin, id_nombre);

			m_taille_mot_courant = static_cast<size_t>(compte);

			this->pousse_mot(id_nombre);
			this->avance(compte);
		}
		else {
			if (m_taille_mot_courant == 0) {
				this->enregistre_pos_mot();
			}

			this->pousse_caractere();
			this->avance();
		}
	}

	void enregistre_pos_mot()
	{
		m_pos_mot = m_position_ligne;
		m_debut_mot = m_debut;
	}

	void pousse_caractere()
	{
		m_taille_mot_courant += 1;
	}

	char caractere_courant() const
	{
		return *m_debut;
	}

	void avance(int n = 1)
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
};
