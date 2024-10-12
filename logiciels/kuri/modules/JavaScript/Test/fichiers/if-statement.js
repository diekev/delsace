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
