test("nous pouvons définir des objets avec 'class'", function () {
    class Rectangle {
        constructor(height, width) {
            this.name = "Rectangle";
            this.height = height;
            this.width = width;
        }
    }

    const rectangle = new Rectangle(1000, 2000);
    vérifie_égalité(rectangle.name, "Rectangle");
    vérifie_égalité(rectangle.height, 1000);
    vérifie_égalité(rectangle.width, 2000);
});

test("nous pouvons hériter d'une 'class'", function () {
    class Rectangle {
        constructor(height, width) {
            this.name = "Rectangle";
            this.height = height;
            this.width = width;
        }
    }

    class FilledRectangle extends Rectangle {
        constructor(height, width, color) {
            super(height, width);
            this.name = "Filled rectangle";
            this.color = color;
        }
    }

    const rectangle = new FilledRectangle(1000, 2000, 'red');
    vérifie_égalité(rectangle.name, "Filled rectangle");
    vérifie_égalité(rectangle.height, 1000);
    vérifie_égalité(rectangle.width, 2000);
    vérifie_égalité(rectangle.color, 'red');
});

test("nous pouvons définir une classe sans constructeurr", function () {
    class SansConstructeur {
    }
});

test("nous pouvons avoir un bloc statique vide", function () {
    class Rectangle {
        constructor() { }
        static { }
    }
});

test("un bloc statique possède son propre contexte", function () {
    var y = "Outer y";

    class A {
        static field = "Inner y";
        static {
            // y n'est pas hissé hors de la classe
            var y = this.field;
        }
    }

    vérifie_égalité(y, "Outer y")
});

test("nous pouvons déclarer des rubriques privées", function () {
    class ClassWithPrivate {
        constructor() { }

        #privateField;
        #privateFieldWithInitializer = 42;

        #privateMethod() {
            // …
        }

        static #privateStaticField;
        static #privateStaticFieldWithInitializer = 42;

        static #privateStaticMethod() {
            // …
        }
    }
});

test("nous pouvons accéder à des rubriques privées via une fonction public", function () {
    class ClassWithPrivateStaticField {
        static #privateStaticField = 42;

        static publicStaticMethod() {
            return ClassWithPrivateStaticField.#privateStaticField;
        }
    }

    vérifie_égalité(ClassWithPrivateStaticField.publicStaticMethod(), 42);
});

test("nous pouvons assigner des rubriques privées", function () {
    class ClassWithPrivateField {
        #privateField;

        constructor() {
            this.#privateField = 42;
        }

        getPrivateField() {
            return this.#privateField;
        }
    }

    var c = new ClassWithPrivateField();
    vérifie_égalité(c.getPrivateField(), 42);
});

test("nous pouvons accéder à une méthode privée via une méthode publique", function () {
    class ClassWithPrivateMethod {
        #privateMethod() {
            return 42;
        }

        publicMethod() {
            return this.#privateMethod();
        }
    }

    const instance = new ClassWithPrivateMethod();
    vérifie_égalité(instance.publicMethod(), 42);
});

test("nous pouvons déclarer des getters et setters", function () {
    class ClassWithPrivateAccessor {
        #message;

        get #decoratedMessage() {
            return `🎬${this.#message}🛑`;
        }
        set #decoratedMessage(msg) {
            this.#message = msg;
        }

        constructor() {
            this.#decoratedMessage = "hello world";
        }

        getMessage() {
            return this.#message;
        }

        getDecoratedMessage() {
            return this.#decoratedMessage;
        }
    }

    const instance = new ClassWithPrivateAccessor();
    vérifie_égalité(instance.getMessage(), "hello world");
    vérifie_égalité(instance.getDecoratedMessage(), "🎬hello world🛑");
});

test("le prototype n'a pas accès aux méthodes privées des instance", function () {
    class C {
        #method() { }

        static getMethod(x) {
            return x.#method;
        }
    }

    try {
        C.getMethod(C.prototype);
    }
    catch (e) {
        vérifie(e instanceof TypeError);
        // À FAIRE : exclus la trace d'appel vérifie_égalité(e.message, "TypeError: private member #method not found in Object");
    }
});

test("une méthode statique publique peut accéder à une méthode statique privée", function () {
    class ClassWithPrivateStaticMethod {
        static #privateStaticMethod() {
            return 42;
        }

        static publicStaticMethod() {
            return ClassWithPrivateStaticMethod.#privateStaticMethod();
        }
    }

    vérifie_égalité(ClassWithPrivateStaticMethod.publicStaticMethod(), 42);
});

test("une sous-classe ne peut pas accéder à une méthode privée de la base même via une méthode publique", function () {
    class ClassWithPrivateStaticMethod {
        static #privateStaticMethod() {
            return 42;
        }

        static publicStaticMethod() {
            return this.#privateStaticMethod();
        }
    }

    class Subclass extends ClassWithPrivateStaticMethod { }

    try {
        Subclass.publicStaticMethod()
    }
    catch (e) {
        vérifie(e instanceof TypeError);
        // À FAIRE : vérifie le message d'erreur
    }
});

test("simulation d'un constructeur privée", function () {
    class PrivateConstructor {
        static #isInternalConstructing = false;

        constructor() {
            if (!PrivateConstructor.#isInternalConstructing) {
                throw new TypeError("PrivateConstructor is not constructable");
            }
            PrivateConstructor.#isInternalConstructing = false;
            // More initialization logic
        }

        static create() {
            PrivateConstructor.#isInternalConstructing = true;
            const instance = new PrivateConstructor();
            return instance;
        }
    }

    try {
        new PrivateConstructor(); // TypeError: PrivateConstructor is not constructable
    }
    catch (e) {
        vérifie(e instanceof TypeError);
        vérifie_égalité(e.message, "PrivateConstructor is not constructable");
    }

    PrivateConstructor.create(); // PrivateConstructor {}
});
