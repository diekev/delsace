// Littérale
var a = [1, 2, 3];

console.log(a.toString());

a[1] = 5;

var push_result = a.push(7);

for (var i = 0; i < a.length; i++) {
    console.log(a[i]);
}

console.log("résultat push : " + push_result);

// Constructeur
var b = Array(16);

console.log(b.toString());
