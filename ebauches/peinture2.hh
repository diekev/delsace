struct Calque {
	float *tampon;
	dls::chaine nom;
};

struct Composite {
	dls::tableau<Calque *> calques;
	int res_x;
	int res_y;
};

struct Maillage {
	Composite *composite;
	dls::chaine nom;
};

struct Object {
	dls::tableau<Maillage *> versions;
	dls::chaine nom;
};

struct Scene {
	dls::tableau<Object *> objects;
};
