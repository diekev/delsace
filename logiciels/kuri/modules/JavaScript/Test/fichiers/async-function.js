test("nous pouvons définir des méthodes async", function () {
    class AsyncMethod {
        async fonction_attendu() { }
        async fonction() {
            const a = await this.fonction_attendu();
        }
    }
});
