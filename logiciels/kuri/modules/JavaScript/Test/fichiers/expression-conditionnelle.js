test("une expression conditionnelle permet de choisir entre deux valeurs", function () {
    var x = 5;
    var y = x == 5 ? 6 : 7;
    vérifie_égalité(y, 6);
});

test("une expression conditionnelle d'assigner une variable dans ses branches", function () {
    var x = 5;
    var y = 0;
    x == 5 ? y = 6 : y = 7;
    vérifie_égalité(y, 6);
});

test("une expression conditionnelle court-circuite l'exécution", function () {
    var x = 5;
    var y = 0;
    x == 5 ? y = 6 : y = variable_inconnue;
    vérifie_égalité(y, 6);
});

test("une expression conditionnelle peut-être dans une expression conditionnelle", function () {
    var x = 5;
    var y = 0;
    x > 4 ? x > 6 ? y = variable_inconnue : y = 6 : y = variable_inconnue;
    vérifie_égalité(y, 6);
});
