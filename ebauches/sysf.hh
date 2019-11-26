

#include <sys/stat.h>

namespace sysf {

struct chemin {
private:
	dls::chaine m_chaine{};

public:
	chemin() = default;

	chemin(dls::chaine const &chn)
		: m_chaine(chn)
	{}

	chemin(char const *chn)
		: m_chaine(chn)
	{}

	dls::chaine chaine() const
	{
		return m_chaine;
	}

	chemin extension() const
	{
		auto pos = m_chaine.trouve_dernier_de('.');
		return chemin(m_chaine.sous_chaine(pos));
	}

	char const *c_str() const
	{
		return m_chaine.c_str();
	}
};

inline bool operator==(chemin const &chm1, chemin const &chm2)
{
	return chm1.chaine() == chm2.chaine();
}

inline bool operator==(chemin const &chm1, dls::chaine const &chm2)
{
	return chm1.chaine() == chm2;
}

inline bool operator==(dls::chaine const &chm1, chemin const &chm2)
{
	return chm1 == chm2.chaine();
}

inline bool operator==(chemin const &chm1, char const *chm2)
{
	return chm1.chaine() == chm2;
}

inline bool operator==(char const *chm1, chemin const &chm2)
{
	return chm1 == chm2.chaine();
}

inline bool operator!=(chemin const &chm1, chemin const &chm2)
{
	return !(chm1 == chm2);
}

inline bool operator!=(chemin const &chm1, dls::chaine const &chm2)
{
	return !(chm1 == chm2);
}

inline bool operator!=(dls::chaine const &chm1, chemin const &chm2)
{
	return !(chm1 == chm2);
}

inline bool operator!=(chemin const &chm1, char const *chm2)
{
	return !(chm1 == chm2);
}

inline bool operator!=(char const *chm1, chemin const &chm2)
{
	return !(chm1 == chm2);
}

bool est_dossier(chemin const &chm)
{
	struct stat st;
	if (stat(chm.c_str(), &st) != 0) {
		return false;
	}

	return S_ISDIR(st.st_mode);
}

bool est_fichier(chemin const &chm)
{
	struct stat st;
	if (stat(chm.c_str(), &st) != 0) {
		return false;
	}

	return S_ISREG(st.st_mode);
}

}
