test("nous pouvons parser les destructuration d'objets", function () {
    const user = {
        id: 42,
        isVerified: true,
    };

    const { id, isVerified } = user;
    vérifie_égalité(id, 42);
    vérifie_égalité(isVerified, true);
});

test("nous pouvons utiliser des identifiants invalide dans les destructuration", function () {
    const foo = { "fizz-buzz": true };
    const { "fizz-buzz": fizzBuzz } = foo;

    vérifie_égalité(fizzBuzz, true);
})

test("nous pouvons parser les destructuration d'arrays", function () {
    const foo = ["one", "two", "three"];

    const [red, yellow, green] = foo;
    vérifie_égalité(red, "one");
    vérifie_égalité(yellow, "two");
    vérifie_égalité(green, "three");
});
