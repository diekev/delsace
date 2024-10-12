test("properties of the Array constructor", function () {
    vérifie_égalité(Array.length, 1)
});

test("properties of the Array prototype", function () {
    vérifie_égalité(Array.prototype.constructor, Array)
});
