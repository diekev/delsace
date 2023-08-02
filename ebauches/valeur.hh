struct Valeur {
	Type *type;

	union {
		bool comme_bool;

		unsigned char comme_octet;

		char  comme_z8;
		short comme_z16;
		int   comme_z32;
		long  comme_z64;

		unsigned char  comme_n8;
		unsigned short comme_n16;
		unsigned int   comme_n32;
		unsigned long  comme_n64;

		r16    comme_r16;
		float  comme_r32;
		double comme_r64;

		kuri::chaine comme_chaine;

		void *comme_pointeur;
	}
}

ValeurChaine {
	kuri::chaine valeur;
}

ValeurStruct {
	kuri::tableau<Valeur> valeurs;
}

ValeurTableau {
	kuri::tableau<Valeur> valeurs;
}
