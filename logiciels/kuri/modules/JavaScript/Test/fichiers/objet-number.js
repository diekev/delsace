test("properties of the Number constructor", function () {
    vérifie_égalité(Number.name, "Number")
});

test("properties of the Number prototype", function () {
    vérifie_égalité(Number.prototype.constructor, Number)
});
