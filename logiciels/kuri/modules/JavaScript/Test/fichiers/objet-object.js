test("properties of the Object constructor", function () {
    vérifie_égalité(Object.length, 1)
});

test("properties of the Object prototype", function () {
    vérifie_égalité(Object.prototype.constructor, Object)
});

test("Object.getPrototypeOf", function () {
    var o = {};
    vérifie_égalité(Object.getPrototypeOf(o), Object.prototype)
});
