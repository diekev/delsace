test("les arguments des fonctions peuvent être passés via '...'", function () {
    function ma_fonction(x, y, z) {
        vérifie_égalité(x, 0);
        vérifie_égalité(y, 1);
        vérifie_égalité(z, 2);
    }

    const args = [0, 1, 2];
    ma_fonction(...args);
});

test("les arguments des fonctions peuvent avoir un mélange de '...' et d'arguments normaux", function () {
    function ma_fonction(v, w, x, y, z) {
        vérifie_égalité(v, -1);
        vérifie_égalité(w, 0);
        vérifie_égalité(x, 1);
        vérifie_égalité(y, 2);
        vérifie_égalité(z, 3);
    }

    const args = [0, 1];
    ma_fonction(-1, ...args, 2, ...[3]);;
});
