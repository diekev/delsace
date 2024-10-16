test("modulo", function () {
    vérifie_égalité(0 % 3, 0);
    vérifie_égalité(1 % 3, 1);
    vérifie_égalité(2 % 3, 2);
    vérifie_égalité(3 % 3, 0);
    vérifie_égalité(4 % 3, 1);
});

test("xor", function () {
    vérifie_égalité(0 ^ 0, 0);
    vérifie_égalité(0 ^ 1, 1);
    vérifie_égalité(1 ^ 0, 1);
    vérifie_égalité(1 ^ 1, 0);

    vérifie_égalité(3 ^ 5, 6);
});
