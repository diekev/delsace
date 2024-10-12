
test("postfix", function () {
    var x = 0;
    var y = x++;
    vérifie_égalité(x, 1);
    vérifie_égalité(y, 0);

    var w = 1;
    var z = w--;
    vérifie_égalité(w, 0);
    vérifie_égalité(z, 1);
});

test("pretfix", function () {
    var x = 0;
    var y = ++x;
    vérifie_égalité(x, 1);
    vérifie_égalité(y, 1);

    var w = 1;
    var z = --w;
    vérifie_égalité(w, 0);
    vérifie_égalité(z, 0);
});
