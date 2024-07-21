'5' - 3.0 // 2.0

'5' + 3.0 // '53'

'5' - '4' // 1

'5' + + '5' // '55'

'b' + 'a' + + 'a' + 'a' // baNaNa

'<scrip' + !0 // 'x3Cscriptrue'

[] + []; // ""

[] + {}; // [object Object]

{ } +[]; // 0

{ } + {}; // NaN

Array(16) // ",,,,,,,,,,,,,,,,"

Array(16).join("wat") // "watwatwatwatwatwatwatwatwatwatwatwatwatwatwatwat"

Array(16).join("wat" + 1) // "wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1"

Array(16).join("wat" - 1) + " Batman!" // "NaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaN Batman!"

// https://blog.kevinchisholm.com/javascript/javascript-wat/