test("Math.abs", function () {
    var x = Math.abs(1.5);
    vérifie_égalité(x, 1.5);

    x = Math.abs(-1.5);
    vérifie_égalité(x, 1.5);
});

test("Math.ceil", function () {
    var x = Math.ceil(1.5);
    vérifie_égalité(x, 2.0);

    x = Math.ceil(-1.5);
    vérifie_égalité(x, -1.0);

    vérifie_égalité(Math.ceil(1.5), -Math.floor(-1.5));
});

test("Math.floor", function () {
    var x = Math.floor(1.5);
    vérifie_égalité(x, 1.0);

    x = Math.floor(-1.5);
    vérifie_égalité(x, -2.0);

    vérifie_égalité(Math.floor(1.5), -Math.ceil(-1.5));
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

test("Math.round", function () {
    var x = Math.round(4.0);
    vérifie_égalité(x, 4.0);

    x = Math.round(4.5);
    vérifie_égalité(x, 5.0);

    x = Math.round(3.5);
    vérifie_égalité(x, 4.0);

    x = Math.round(-0.0);
    vérifie_égalité(x, -0.0);

    x = Math.round(-0.2);
    vérifie_égalité(x, -0.0);
});

test("Math.sqrt", function () {
    var x = Math.sqrt(4.0);
    vérifie_égalité(x, 2.0);
});

