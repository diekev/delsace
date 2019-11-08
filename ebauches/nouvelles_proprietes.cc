
// vers un nouveau système de propriété avec un type de base pour changer la valeur selon une constante, une animation, une expression, un graphe, une texture

//auto ptr_nom = evalue_entier("nom", temps, position);
#if 0
evalue()
{
	si constante
	sinon si animation
	sinon si expression
	sinon si texture // uv
	sinon si graphe ??
	sinon si mappage_courbe ??
}
#endif

// un système flexible permettant de mieux controler les données
// crée un graphe pour les paramètres ? le graphe peut contenir les images, courbes, et autres

template <typename T, int N>
struct paire_anim {
	int temps;
	T valeur[N];
};

struct Expression {
	int temps_compile;
	int temps_modifie;
};

struct propriete {
	Noeud *noeud;
	EvaluatriceExpression *evaluatrice;
	Expression *expression;
};

template <typename T, int N>
struct propriete_typee : public propriete {
	T ptr[N];
	dls::tableau<paire_anim> m_anim;
	Texture *m_texture;

public:
	bool possede_animation() const
	{
		return !m_anim.est_vide();
	}

	T *evalue_anim(int temps)
	{
		return m_anim[0].valeur;
	}

	bool possede_texture() const
	{
		return m_texture != nullptr;
	}

	T *evalue_texture(dls::math::vec3f const &pos)
	{
		m_texture.evalue(ptr, pos);
		return ptr;
	}

	bool possede_expression() const
	{
		return m_expression != nullptr;
	}

	T *evalue_expression()
	{
		// il nous un faut un contexte pour accéder à d'autres variables, d'autres objets
		return evaluatrice->evalue(expression, noeud, ptr);
	}
};

using prop_entier = propriete_typee<int, 1>;
using prop_decimal = propriete_typee<float, 1>;

using prop_vec2i = propriete_typee<int, 2>;
using prop_vec2f = propriete_typee<float, 2>;

using prop_vec3i = propriete_typee<int, 3>;
using prop_vec3f = propriete_typee<float, 3>;

using prop_vec4i = propriete_typee<int, 4>;
using prop_vec4f = propriete_typee<float, 4>;

template <typename T>
T *evalue_propriete(propriete *prop, int temps, dls::math::vec3f cosnt &pos)
{
	if (prop->possede_animation()) {
		return prop->evalue(temps);
	}

	if (prop->possede_texture()) {
		return prop->evalue_texture(temps, pos);
	}

	if (prop->possede_expression()) {
		return prop->evalue_expression();
	}

	return prop->evalue_value();
}

int *evalue_entier(dls::vue_chaine const &nom, int dims = 1, int temps = 0, dls::math::vec3f pos = dls::math::vec3f())
{
	auto prop = cherche_propriete(nom);

	if (prop == nullptr) {
		// erreur
		return ptr_entier_nul;
	}

	return evalue_propriete<int>(prop);
}

// avec
struct propriete_entier  : public propriete;
struct propriete_decimal : public propriete;
struct propriete_couleur : public propriete;
// sans
struct propriete_chaine  : public propriete;
struct propriete_fichier : public propriete;
struct propriete_enum    : public propriete;
struct propriete_courbe  : public propriete;
struct propriete_rampe   : public propriete;
