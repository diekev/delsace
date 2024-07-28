/* eval doit pouvoir déclarer dans l'environnemnt. */
eval("x = 3");
console.log(x)

/* eval doit pouvoir accéder à l'environnement. */
function foo(y) {
    var a = 3;
    eval("a += y;");
    return a
}
console.log(foo(2))

/* eval doit retourner la valeur évaluée. */
console.log(eval("1 + 2;"))
