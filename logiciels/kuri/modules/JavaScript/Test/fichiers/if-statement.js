test("simple if", function () {
    var x = 5;

    if (x == 5) {
        x = 7;
    }
    else {
        x = 0.0;
    }

    vérifie_égalité(x, 7);
});

test("simple if sans bloc", function () {
    var x = 5;

    if (x == 5)
        x = 7;
    else
        x = 0.0;

    vérifie_égalité(x, 7);
});

test("conversion de l'expression vers bool", function () {
    var o = { x: 123 };
    var ok = false;
    if (o.x) {
        ok = true;
    }
    vérifie_égalité(ok, true);
});
