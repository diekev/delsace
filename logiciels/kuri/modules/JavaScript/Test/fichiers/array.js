var a = [1, 2, 3];

// console.log(a);

a[1] = 5;

var push_result = a.push(7);

for (var i = 0; i < a.length; i++) {
    console.log(a[i]);
}

console.log("résultat push : " + push_result);
