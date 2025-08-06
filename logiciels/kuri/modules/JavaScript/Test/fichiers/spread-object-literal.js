test("nous pouvons utiliser '...' pour créer des propriétés sur des objets", function () {
    const obj1 = { foo: "bar", x: 42 };
    const obj2 = { bar: "baz", y: 13 };

    const obj3 = { ...obj1, ...obj2 };

    vérifie("foo" in obj3);
    vérifie("x" in obj3);
    vérifie("bar" in obj3);
    vérifie("y" in obj3);

    vérifie_égalité(obj3.foo, "bar");
    vérifie_égalité(obj3.x, 42);
    vérifie_égalité(obj3.bar, "baz");
    vérifie_égalité(obj3.y, 13);
});

test("'...' ne crée pas de membre en double, mais remplace leurs valeurs", function () {
    const obj1 = { foo: "bar", x: 42 };
    const obj2 = { foo: "baz", y: 13 };

    const obj3 = { x: 41, ...obj1, ...obj2, y: 9 };

    vérifie("x" in obj3);
    vérifie("foo" in obj3);
    vérifie("y" in obj3);

    vérifie_égalité(obj3.x, 42);
    vérifie_égalité(obj3.foo, "baz");
    vérifie_égalité(obj3.y, 9);
});
