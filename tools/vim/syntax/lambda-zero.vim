syntax match lambdaZeroDelimiter "(\|)\|\[\|\]\|{\|}\|,\|;"
syntax match lambdaZeroName "\<[a-zA-Z_][a-zA-Z0-9_']*" contains=lambdaZeroSubscript,lambdaZeroPrime,lambdaZeroGreek

syntax match lambdaZeroOperator "[-`~!@#$%^&\*=\+\\|:<.>/\?]\+"
syntax match lambdaZeroInteger "\<\d\+"
syntax region lambdaZeroString start=/\(^\|[^"]\)"/ skip=/\v(\\[\\"]){-1}/ end=/"/ oneline
syntax region lambdaZeroCharacter start=/\(^\|\W\)'/ skip=/\v(\\[\\']){-1}/ end=/'/ oneline
syntax match lambdaZeroLineComment "\(^\|\s\)//.*$"

highlight link lambdaZeroDelimiter Operator
highlight link lambdaZeroName Normal
highlight link lambdaZeroOperator Operator
highlight link lambdaZeroInteger Constant
highlight link lambdaZeroString String
highlight link lambdaZeroCharacter Character
highlight link lambdaZeroLineComment Comment
