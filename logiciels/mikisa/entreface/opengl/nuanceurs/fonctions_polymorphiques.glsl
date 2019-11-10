void plus_grand_que(in TYPE_POLY valeur1,in TYPE_POLY valeur2,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = greaterThan(valeur1, valeur2);
}

void inverse(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = 1.0 / valeur;
}

void arrondis(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
	valeur_sortie = round(valeur);
}

void hermite1(in TYPE_POLY valeur,in TYPE_POLY min,in TYPE_POLY max,in TYPE_POLY neuf_min,in TYPE_POLY neuf_max,out TYPE_POLY valeur_sortie)
{
    TYPE_POLY x = clamp(valeur - min / max - min, 0.0, 1.0);
    x = (x * x * (3.0 - 2.0 * x));
    valeur_sortie = neuf_min + x * (neuf_max - neuf_min);
}

void hermite2(in TYPE_POLY valeur,in TYPE_POLY min,in TYPE_POLY max,in TYPE_POLY neuf_min,in TYPE_POLY neuf_max,out TYPE_POLY valeur_sortie)
{
    TYPE_POLY x = clamp(valeur - min / max - min, 0.0, 1.0);
    x = (x * x * x * (x * (x * 6.0 - 15.0) + 10));
    valeur_sortie = neuf_min + x * (neuf_max - neuf_min);
}

void hermite3(in TYPE_POLY valeur,in TYPE_POLY min,in TYPE_POLY max,in TYPE_POLY neuf_min,in TYPE_POLY neuf_max,out TYPE_POLY valeur_sortie)
{
    TYPE_POLY x = clamp(valeur - min / max - min, 0.0, 1.0);
    x = (x * x * x * x * (x * (x * (x * -20.0 + 70.0) - 84.0) + 35.0));
    valeur_sortie = neuf_min + x * (neuf_max - neuf_min);
}

void hermite4(in TYPE_POLY valeur,in TYPE_POLY min,in TYPE_POLY max,in TYPE_POLY neuf_min,in TYPE_POLY neuf_max,out TYPE_POLY valeur_sortie)
{
    TYPE_POLY x = clamp(valeur - min / max - min, 0.0, 1.0);
    x = (x * x * x * x * x * (x * (x * (x * (x * 70.0 - 315.0) + 540.0) - 420.0) + 126.0));
    valeur_sortie = neuf_min + x * (neuf_max - neuf_min);
}

void hermite5(in TYPE_POLY valeur,in TYPE_POLY min,in TYPE_POLY max,in TYPE_POLY neuf_min,in TYPE_POLY neuf_max,out TYPE_POLY valeur_sortie)
{
    TYPE_POLY x = clamp(valeur - min / max - min, 0.0, 1.0);
    x = (x * x * x * x * x * x * (x * (x * (x * (x * (x * -252.0 + 1386.0) - 3080.0) + 3465.0) - 1980.0) + 462.0));
    valeur_sortie = neuf_min + x * (neuf_max - neuf_min);
}

void hermite6(in TYPE_POLY valeur,in TYPE_POLY min,in TYPE_POLY max,in TYPE_POLY neuf_min,in TYPE_POLY neuf_max,out TYPE_POLY valeur_sortie)
{
    TYPE_POLY x = clamp(valeur - min / max - min, 0.0, 1.0);
    x = (x * x * x * x * x * x * x * (x * (x * (x * (x * (x * (x * 924.0 - 6006.0) + 16380.0) - 24024.0) + 20020.0) - 9009.0) + 1716.0));
    valeur_sortie = neuf_min + x * (neuf_max - neuf_min);
}

void frac(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = frac(valeur);
}

void log(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = log(valeur);
}

void divise(in TYPE_POLY valeur1,in TYPE_POLY valeur2,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = valeur1 / valeur2;
}

void abs(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = abs(valeur);
}

void soustrait(in TYPE_POLY valeur1,in TYPE_POLY valeur2,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = valeur1 - valeur2;
}

void sol(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = floor(valeur);
}

void sin(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = sin(valeur);
}

void cos(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = cos(valeur);
}

void tan(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = tan(valeur);
}

void min(in TYPE_POLY valeur1,in TYPE_POLY valeur2,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = min(valeur1, valeur2);
}

void modulo(in TYPE_POLY valeur1,in TYPE_POLY valeur2,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = mod(valeur1, valeur2);
}

void enligne(in TYPE_POLY valeur1,in TYPE_POLY valeur2,in TYPE_POLY facteur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = mix(valeu1, valeur2, facteur);
}

void atan(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = atan(valeur);
}

void exp(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = exp(valeur);
}

void ajoute(in TYPE_POLY valeur1,in TYPE_POLY valeur2,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = valeur1 + valeur2;
}

void acos(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = acos(valeur);
}

void complement(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = 1.0 - valeur;
}

void max(in TYPE_POLY valeur1,in TYPE_POLY valeur2,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = max(valeur1, valeur2);
}

void racine_carree(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = sqrt(valeur);
}

void asin(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = asin(valeur);
}

void plus_petit_que(in TYPE_POLY valeur1,in TYPE_POLY valeur2,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = lessThan(valeur1, valeur2);
}

void puissance(in TYPE_POLY valeur,in TYPE_POLY exposant,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = pow(valeur, exposant);
}

void restreint(in TYPE_POLY valeur,in TYPE_POLY min,in TYPE_POLY max,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = clamp(valeur, min, max);
}

void traduit(in TYPE_POLY valeur,in TYPE_POLY min,in TYPE_POLY max,in TYPE_POLY neuf_min,in TYPE_POLY neuf_max,out TYPE_POLY valeur_sortie)
{
    TYPE_POLY tmp;

	if (any(greaterThan(vieux_min, vieux_max))) {
		tmp = vieux_min - clamp(valeur, vieux_max, vieux_min);
	}
	else {
		tmp = clamp(valeur, vieux_min, vieux_max);
	}

	tmp = (tmp - vieux_min) / (vieux_max - vieux_min);
	valeur_sortie = neuf_min + tmp * (neuf_max - neuf_min);
}

void plafond(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = ceil(valeur);
}

void nie(in TYPE_POLY valeur,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = -valeur;
}

void multiplie(in TYPE_POLY valeur1,in TYPE_POLY valeur2,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = valeur1 * valeur2;
}

void atan2(in TYPE_POLY valeur1,in TYPE_POLY valeur2,out TYPE_POLY valeur_sortie)
{
    valeur_sortie = atan2(valeur1, valeur2);
}
