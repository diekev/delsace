test("boucle for avec bloc", function () {
    var résultat = 0;

    for (var i = 0; i < 10; i += 1) {
        résultat += i;
    }

    vérifie_égalité(résultat, 45);
});

test("boucle for sans bloc", function () {
    var résultat = 0;

    for (var i = 0; i < 10; i += 1)
        résultat += i;


    vérifie_égalité(résultat, 45);
});

test("boucles for imbriquées avec bloc", function () {
    var résultat = 0;

    for (var i = 0; i < 10; i += 1) {
        for (var j = 0; j < 10; j += 1) {
            résultat += i;
        }
    }

    vérifie_égalité(résultat, 450);
});

test("boucles for imbriquées sans bloc", function () {
    var résultat = 0;

    for (var i = 0; i < 10; i += 1)
        for (var j = 0; j < 10; j += 1)
            résultat += i;


    vérifie_égalité(résultat, 450);
});
