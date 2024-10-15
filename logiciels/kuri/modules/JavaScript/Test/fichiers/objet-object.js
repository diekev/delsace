test("properties of the Object constructor", function () {
    vérifie_égalité(Object.length, 1)
    vérifie_égalité(Object.name, "Object")
});

test("properties of the Object prototype", function () {
    vérifie_égalité(Object.prototype.constructor, Object)
});

test("Object.prototype.__proto__", function () {
    var o = {};
    vérifie_égalité(o.__proto__, Object.getPrototypeOf(o))
});

test("Object.getPrototypeOf", function () {
    var o = {};
    vérifie_égalité(Object.getPrototypeOf(o), Object.prototype)
});

test("Object.setPrototypeOf", function () {
    var obj = {};
    var proto = { ok: 5 };

    vérifie_égalité(Object.getPrototypeOf(obj), Object.prototype);
    vérifie_égalité(obj.hasOwnProperty("ok"), false);
    vérifie_égalité(proto.hasOwnProperty("ok"), true);

    Object.setPrototypeOf(obj, proto);
    vérifie_égalité(Object.getPrototypeOf(obj), proto);
    vérifie_égalité(obj.hasOwnProperty("ok"), false);
    vérifie_égalité(obj.ok, 5);
});
