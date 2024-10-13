test("Math.ceil", function () {
    var x = Math.ceil(1.5);
    vérifie_égalité(x, 2.0);

    x = Math.ceil(-1.5);
    vérifie_égalité(x, -1.0);

    var fx = Math.floor(-1.5);
    vérifie_égalité(Math.ceil(1.5), -fx);
});

test("Math.floor", function () {
    var x = Math.floor(1.5);
    vérifie_égalité(x, 1.0);

    x = Math.floor(-1.5);
    vérifie_égalité(x, -2.0);

    /* À FAIRE : -Math.ceil(x) fait crasher. */
    var cx = Math.ceil(-1.5);
    vérifie_égalité(Math.floor(1.5), -cx);
});

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

