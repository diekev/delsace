
test("string literal", function () {
    vérifie_égalité("hello friends".length, 13)
});

test("join avec nouvelle ligne", function () {
    const s = [0, 1, 2, 3, 4, 5].join("\n");
    vérifie_égalité(s, "0\n1\n2\n3\n4\n5")
});
