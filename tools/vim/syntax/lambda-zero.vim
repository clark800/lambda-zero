autocmd BufEnter * :syntax sync fromstart " improves reliability of highlighting

syntax match lambdaZeroName "\<[a-zA-Z_][a-zA-Z0-9_']*"
\ contains=lambdaZeroSubscript,lambdaZeroPrime,lambdaZeroGreek
syntax match lambdaZeroDelimiter "[()\[\]{},;.`@$]"
syntax match lambdaZeroOperator "[^a-zA-Z0-9_'" ()\[\]{},;.`@$]\+"
syntax match lambdaZeroInteger "\<\d\+"
syntax region lambdaZeroString start=/"/ skip=/\\./ end=/"/ oneline
syntax region lambdaZeroCharacter start=/[ ()\[\]{},;.`@$]'/lc=1 skip=/\\./ end=/'/ oneline
syntax region lambdaZeroCharacter start=/^'/ skip=/\\./ end=/'/ oneline
syntax match lambdaZeroLineComment "#.*$"
syntax keyword lambdaZeroConstant true false
syntax keyword lambdaZeroKeyword not and or in if then else define match case return throw void try catch

highlight link lambdaZeroName Normal
highlight link lambdaZeroOperator Operator
highlight link lambdaZeroDelimiter Operator
highlight link lambdaZeroInteger Constant
highlight link lambdaZeroConstant Constant
highlight link lambdaZeroString String
highlight link lambdaZeroCharacter Character
highlight link lambdaZeroLineComment Comment
highlight link lambdaZeroKeyword Keyword
