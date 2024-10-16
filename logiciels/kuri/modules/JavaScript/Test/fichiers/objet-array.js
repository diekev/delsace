test("properties of the Array constructor", function () {
    vérifie_égalité(Array.length, 1)
    vérifie_égalité(Array.name, "Array")
});

test("properties of the Array prototype", function () {
    vérifie_égalité(Array.prototype.constructor, Array)
});
