syntax match lambdaZeroDelimiter "(\|)\|\[\|\]\|{\|}\|,\|;"
syntax match lambdaZeroName "\<[a-zA-Z_][a-zA-Z0-9_']*\>"
syntax match lambdaZeroOperator "[-`~!@#$%^&\*=\+\\|:<.>/\?]\+"
syntax match lambdaZeroInteger "\<\d\+"
syntax region lambdaZeroString start=/\v"/ skip=/\v(\\[\\"]){-1}/ end=/\v"/
syntax region lambdaZeroCharacter start=/[^\w"']\v'/ skip=/\v(\\[\\']){-1}/ end=/\v'/
syntax match lambdaZeroLineComment "\(^\|\s\)''.*$"

highlight link lambdaZeroDelimiter Operator
highlight link lambdaZeroName Normal
highlight link lambdaZeroOperator Operator
highlight link lambdaZeroInteger Constant
highlight link lambdaZeroString String
highlight link lambdaZeroCharacter Character
highlight link lambdaZeroLineComment Comment
