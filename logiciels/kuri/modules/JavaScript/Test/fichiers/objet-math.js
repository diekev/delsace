test("Math.max", function () {
    var x = Math.max(1.0, 5.0, 2.0);
    vérifie_égalité(x, 5.0);

    x = Math.max(-1.0, -5.0, -2.0);
    vérifie_égalité(x, -1.0);
});

test("Math.min", function () {
    var x = Math.min(1.0, 5.0, 2.0);
    vérifie_égalité(x, 1.0);

    x = Math.min(-1.0, -5.0, -2.0);
    vérifie_égalité(x, -5.0);
});

test("Math.sqrt", function () {
    var x = Math.sqrt(4.0);
    vérifie_égalité(x, 2.0);
});

