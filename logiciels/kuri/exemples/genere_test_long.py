# -*- coding:utf8 -*-

fonctions = u"""
structure Vecteur3D{0}{1} {{
	donnée_x : {1};
	donnée_y : {1};
	donnée_z : {1};
}}

# ligne de commentaire
# ligne de commentaire
fonction {0}addition_{1}(a : {1}, b : {1}) : {1}
{{
	soit x = a + b;
	soit y = x + x;
	soit z = (a + x + b) + y;
	soit w = a + y + x + (a + z + b) + z;
	retourne a + b + (x + y) + z + w;
}}

structure Vecteur3D{1}{0} {{
	donnée_x : Vecteur3D{0}{1};
	donnée_y : Vecteur3D{0}{1};
	donnée_z : Vecteur3D{0}{1};
}}

# ligne de commentaire
# ligne de commentaire
# ligne de commentaire
# ligne de commentaire
fonction {0}multiplication_{1}(a : {1}, b : {1}) : {1}
{{
	soit x = a * b;
	soit y = {0}addition_{1}(b=a, a=b) * {0}addition_{1}(a=b, b=a);
	soit z = a * x * b * y;
	soit w = a * y * (x * a * z * b * z);
	retourne a * b * x * y * z * w;
}}

# ligne de commentaire
fonction {0}division_{1}(a : {1}, b : {1}) : {1}
{{
	# ligne de commentaire
	soit x = a / b;
	soit y = {0}multiplication_{1}(b=a, a=b) / {0}multiplication_{1}(a=b, b=a);
	soit z = ((a / x) / b / y);
	soit w = (a / y / (x / a / z / b)) / z;
	retourne a / b / x / (y / z / w);
}}

# ligne de commentaire
# ligne de commentaire
# ligne de commentaire
# ligne de commentaire
# ligne de commentaire
# ligne de commentaire
fonction {0}soustraction_{1}(a : {1}, b : {1}) : {1}
{{
	soit x = a - b;
	soit y = {0}division_{1}(b=a, a=b) / {0}division_{1}(a=b, b=a);
	soit z = a - (x - b - y);
	soit w = a - y - x - a - z - b - z;
	retourne (((((a - b) - x) - y) - z) - w);
}}
"""

fonction_principale = u"""
fonction principale(c : z32, a : **n8) : z32
{
	retourne 0;
}
"""

types = [u'n8', u'n16', u'n32', u'n64', u'r16', u'r32', u'r64']
lettres = u'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'

tampon = u""
"""
for l0 in lettres:
	for l in lettres:
		for t in types:
			tampon += fonctions.format(l0 + l, t)

tampon += fonction_principale

with open(u'/tmp/test_long.kuri', 'w') as fichier:
	fichier.write(tampon)"""

chaines = [
	u"fonc principale(args : []*z8) : z32\n",
	u"{\n",
	u"\tdyn a = 1\n",
	u"\tdyn b = 2\n",
	u"\tdyn c = 3\n",
	u"\tdyn d = 4\n",
	u"\ta = b + c * d\n",
	u"\tretourne 0\n",
	u"}"
]

for i in range(0, 6):
	tampon += chaines[i]

for i in range(1, 20000):
	tampon += chaines[6]

tampon += chaines[7]
tampon += chaines[8]

with open(u'/tmp/test_long.kuri', 'w') as fichier:
	fichier.write(tampon)