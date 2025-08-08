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

test("nous pouvons avoir un bloc statique vide", function () {
    class Rectangle {
        constructor() { }
        static { }
    }
});
