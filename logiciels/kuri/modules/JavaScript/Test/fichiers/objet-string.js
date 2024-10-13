test("properties of the String constructor", function () {
    vérifie_égalité(String.length, 1)
    vérifie_égalité(String.name, "String")
});

test("properties of the String prototype", function () {
    vérifie_égalité(String.prototype.constructor, String)
});
