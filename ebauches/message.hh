struct requete {
	int version;   // 10 ou 11
	std::string methode; // "GET", "POST"
	std::string target; // "/index.html"
	dls::dico<std::string, std::string> champs; // paires nom/valeurs
	std::string corps; // taille variable
};

struct reponse {
	int version; // 10 ou 11
	int status; // 200 veut dire OK
	std::string raison; // humainement lisible
	dls::dico<std::string, std::string> champs; // paires nom/valeurs
	std::string corps; // taille variable
};

// ne connaissent pas les allocateurs
// le corps est codé en dur en std::string
// les champs sont codé en dur en dls::dico
// les noms ne sont pas sensible à la casse
// les types requete, reponse sont distint, unrelated, mais nous ne voulons qu'un seul type pour dédupliquer le code :

/* sérialise un message HTTP */
template <class Message>
void write (std::ostream &os, Message const &);

--------------------

template <bool est_requete, class Corps>
struct message;

using requete = message<true>;
using reponse = message<false>;

struct corps_chaine {
	using value_type = std::string;

	static void lis(std::istream &, value_type &corps)
	{
		is >> corps;
	}

	/**
	 * Nous permets d'abstraire la logique d'écriture de message avant d'optimiser
	 * les cas où le corps est vide.
	 */
	static void ecrit(std::ostream &os, const value_type &corps)
	{
		os << corps;
	}
};

struct corps_vide {
	struct value_type {};

	static void lis(std::istream &, value_type &)
	{
	}

	static void ecrit(std::ostream &, const value_type &)
	{
	}
};

struct corps_fichier {
	using value_type = std::string;

	static void ecrit(std::ostream &os, const value_type &chemin)
	{
		size_t n;
		char tampon[4096];
		FILE *f = fopen(path.c_str(), "rb");

		while (n = fread(buf, 1, sizeof(buf), f)) {
			os.write(buf, n);
		}

		fclose(f);
	}
};

template <
	class Corps,
	class Champs
>
struct message<true, Corps, Champs> {
	int version;   // 10 ou 11
	std::string methode; // "GET", "POST"
	std::string target; // "/index.html"
	dls::dico<std::string, std::string> champs; // paires nom/valeurs

	/* Arugments forwardés au constructeur du corps, au cas où le corps aurait besoin d'un allocateur. */
	template <class... Args>
	explicit message(Args&&... args)
		: Corps::value_type(std::forward<Args>(args)...)
	{}

	typename Corps::value_type corps; // taille variable
};

/**
 * Hérite de Corps::value_type pour optimiser le cas où le corps est vide,
 * c'est-à-dire que si le corps est vide, il n'ajoute pas à la taille du message
 * (sizeof).
 * Empty base optimisation.
 */
template <
	class Corps,
	class Champs
>
struct message<false, Corps> : private Corps::value_type, private Champs {
	int version; // 10 ou 11
	int status; // 200 veut dire OK
	std::string raison; // humainement lisible
	dls::dico<std::string, std::string> champs; // paires nom/valeurs

	// taille variable
	typename Corps::value_type &corps()
	{
		return *this;
	}

	typename Corps::value_type const &corps() const
	{
		return *this;
	}
};

template <bool est_requete, class Corps>
void write(std::ostream &os, message<est_requete, Corps> &msg)
{
	lis_entete(os, msg);
	Corps::lis(os, msg.body());
}

template <bool est_requete, class Corps>
void write(std::ostream &os, message<est_requete, Corps> const &msg)
{
	write_header(os, msg);
	Corps::ecrit(os, msg.body());
}
