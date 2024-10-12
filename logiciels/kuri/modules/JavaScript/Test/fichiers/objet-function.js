test("properties of the Function constructor", function () {
    vérifie_égalité(Function.length, 1)
});

test("properties of the Function prototype", function () {
    vérifie_égalité(Function.prototype.constructor, Function)
});
