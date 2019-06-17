class Base {
	typedef void (*FP)();
	typedef int (*FPGet)(const Base &);
	typedef void (*FPSet)(Base &, int);
	static FP vtbl[totalClasses][totalMethods];
	uint8_t tag;

public:
	int get() const
	{
		return (static_cast<FPGet>(vtbl[0][tag]))(*this);
	}

	int set(int x)
	{
		(static_cast<FPSet>(vtbl[1][tag]))(*this, x);
	}
};

// initialise la table
Base::vtbl[][] = {
	{ get_x, set_x },
};
