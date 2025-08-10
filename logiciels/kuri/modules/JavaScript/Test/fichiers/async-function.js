test("nous pouvons définir des méthodes async", function () {
    class AsyncMethod {
        async fonction_attendu() { }
        async fonction() {
            const a = await this.fonction_attendu();
        }
    }
});

test("nous pouvons déclarer des fonction async dans littérales d'objets", function () {
    var o = {
        async fonction_attendu() { },
        async fonction() {
            const a = await this.fonction_attendu();
        }
    }
});

test("nous pouvons déclarer des fonction arrow asyn", function () {
    var e = async t => {
    }
});
