" vim: set fenc=utf-8:
syntax match zeroNiceOperator ":=" conceal cchar=≔
syntax match zeroNiceOperator "||" conceal cchar=⋮
syntax match zeroNiceOperator "<-" conceal cchar=↤
syntax match zeroNiceOperator "->" conceal cchar=↦
syntax match zeroNiceOperator "<>" conceal cchar=∘

syntax match zeroNiceOperator "/\\" conceal cchar=∧
syntax match zeroNiceOperator "\\/" conceal cchar=∨
syntax match zeroNiceOperator "<=>" conceal cchar=⇔
syntax match zeroNiceOperator "=>" conceal cchar=⇒
syntax match zeroNiceOperator "\~" conceal cchar=¬

syntax match zeroNiceOperator "\*\*" conceal cchar=×
syntax match zeroNiceOperator "\~=" conceal cchar=≠
syntax match zeroNiceOperator "<=\ze[^<>]" conceal cchar=≤
syntax match zeroNiceOperator ">=\ze[^<>]" conceal cchar=≥

syntax match zeroNiceOperator ":\ze[^:=]" conceal cchar=∈
syntax match zeroNiceOperator "::\ze[^?]" conceal cchar=∷
syntax match zeroNiceOperator "++" conceal cchar=⧺
syntax match zeroNiceOperator "==" conceal cchar=≌
syntax match zeroNiceOperator "<:" conceal cchar=⊆
syntax match zeroNiceOperator "\.\." conceal cchar=‥

syntax match zeroNiceOperator "\<sum\>" conceal cchar=∑
syntax match zeroNiceOperator "\<product\>" conceal cchar=∏
syntax match zeroNiceOperator "\<pi\>" conceal cchar=π
syntax match zeroNiceOperator "\<sqrt\>" conceal cchar=√

highlight link zeroNiceOperator Operator
highlight! link Conceal Operator
setlocal conceallevel=2
