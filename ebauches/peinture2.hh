struct Calque {
	float *tampon;
	std::string nom;
};

struct Composite {
	std::vector<Calque *> calques;
	int res_x;
	int res_y;
};

struct Maillage {
	Composite *composite;
	std::string nom;
};

struct Object {
	std::vector<Maillage *> versions;
	std::string nom;
};

struct Scene {
	std::vector<Object *> objects;
};
