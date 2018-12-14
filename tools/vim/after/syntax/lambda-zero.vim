" vim: set fenc=utf-8:
" if one operator is a substring of another, make sure the shortest comes
" first

syntax match zeroNiceOperator "\*" conceal cchar=⋅
syntax match zeroNiceOperator "\*\*" conceal cchar=×
syntax match zeroNiceOperator "><" conceal cchar=⪤
syntax match zeroNiceOperator "\*:\*" conceal cchar=⨰
syntax match zeroNiceOperator ":" conceal cchar=∈
syntax match zeroNiceOperator "/:" conceal cchar=∉
syntax match zeroNiceOperator "::" conceal cchar=∷
syntax match zeroNiceOperator "++" conceal cchar=⧺
syntax match zeroNiceOperator "+=" conceal cchar=⧧
syntax match zeroNiceOperator "+++" conceal cchar=⧻
syntax match zeroNiceOperator "\.\." conceal cchar=‥
syntax match zeroNiceOperator "\.\.\." conceal cchar=…
syntax match zeroNiceOperator "\*\*\*" conceal cchar=⨳
syntax match zeroNiceOperator "&" conceal cchar=∩
syntax match zeroNiceOperator "\<U\>" conceal cchar=⋃   " union
syntax match zeroNiceOperator "\~\~" conceal cchar=≀
syntax match zeroNiceOperator "--" conceal cchar=¬
syntax match zeroNiceOperator "+-" conceal cchar=±
syntax match zeroNiceOperator "-+" conceal cchar=∓
syntax match zeroNiceOperator "-+-" conceal cchar=⩱
syntax match zeroNiceOperator "|:" conceal cchar=¦
syntax match zeroNiceOperator "!!" conceal cchar=‼
syntax match zeroNiceOperator "?!" conceal cchar=⁈
syntax match zeroNiceOperator "??" conceal cchar=⁇
syntax match zeroNiceOperator "//" conceal cchar=⫽
syntax match zeroNiceOperator ":-" conceal cchar=∸
syntax match zeroNiceOperator "-:" conceal cchar=∹
syntax match zeroNiceOperator "-:-" conceal cchar=⫬      " bitwise not
syntax match zeroNiceOperator "||" conceal cchar=‖

" comparison operators
syntax match zeroNiceOperator "<:" conceal cchar=⊆
syntax match zeroNiceOperator ":>" conceal cchar=⊇
syntax match zeroNiceOperator "</:" conceal cchar=⊈
syntax match zeroNiceOperator ":/>" conceal cchar=⊉
syntax match zeroNiceOperator ":<" conceal cchar=⊂
syntax match zeroNiceOperator ">:" conceal cchar=⊃
syntax match zeroNiceOperator ":/<" conceal cchar=⊄
syntax match zeroNiceOperator ">/:" conceal cchar=⊅
syntax match zeroNiceOperator "<:<" conceal cchar=⪡     " bitwise shift left
syntax match zeroNiceOperator ">:>" conceal cchar=⪢     " bitwise shift right
syntax match zeroNiceOperator ">:<" conceal cchar=⩙     " bitwise xor

syntax match zeroNiceOperator "/<" conceal cchar=≮
syntax match zeroNiceOperator "/>" conceal cchar=≯
syntax match zeroNiceOperator "<=" conceal cchar=≤
syntax match zeroNiceOperator ">=" conceal cchar=≥
syntax match zeroNiceOperator "=<" conceal cchar=≼
"syntax match zeroNiceOperator "" conceal cchar=≽
syntax match zeroNiceOperator "-<" conceal cchar=⋜
syntax match zeroNiceOperator ">-" conceal cchar=⋝
syntax match zeroNiceOperator "\~<" conceal cchar=≾
syntax match zeroNiceOperator ">\~" conceal cchar=≿
syntax match zeroNiceOperator "<\*=" conceal cchar=⩿   " string compare
syntax match zeroNiceOperator ">\*=" conceal cchar=⪀   " string compare

syntax match zeroNiceOperator "<|" conceal cchar=⊲
syntax match zeroNiceOperator "|>" conceal cchar=⊳
syntax match zeroNiceOperator "<||" conceal cchar=⧏
syntax match zeroNiceOperator "||>" conceal cchar=⧐
syntax match zeroNiceOperator "=<|" conceal cchar=⊴
syntax match zeroNiceOperator "|>=" conceal cchar=⊵
syntax match zeroNiceOperator "=/<|" conceal cchar=⋬
syntax match zeroNiceOperator "|>/=" conceal cchar=⋭

syntax match zeroNiceOperator "\~:" conceal cchar=∻     " or ⩪ ?
syntax match zeroNiceOperator "/\~" conceal cchar=≁

syntax match zeroNiceOperator ":=" conceal cchar=≔
syntax match zeroNiceOperator "=:" conceal cchar=≕
syntax match zeroNiceOperator "::=" conceal cchar=⩴
syntax match zeroNiceOperator "=/=" conceal cchar=≠
syntax match zeroNiceOperator "=:=" conceal cchar=≐
syntax match zeroNiceOperator "=::=" conceal cchar=⩷
syntax match zeroNiceOperator "=\*=" conceal cchar=≛    " string equality
syntax match zeroNiceOperator "=<>=" conceal cchar=≗
syntax match zeroNiceOperator "=?=" conceal cchar=≟
syntax match zeroNiceOperator "\~=" conceal cchar=≃
syntax match zeroNiceOperator "/\~=" conceal cchar=≄
syntax match zeroNiceOperator "=\~=" conceal cchar=≅
syntax match zeroNiceOperator "/=\~=" conceal cchar=≇
syntax match zeroNiceOperator "==" conceal cchar=≡
syntax match zeroNiceOperator "/==" conceal cchar=≢
syntax match zeroNiceOperator "===" conceal cchar=≣
syntax match zeroNiceOperator "\~=\~" conceal cchar=≍

" arrow operators
"syntax match zeroNiceOperator "->" conceal cchar=→
syntax match zeroNiceOperator "<-" conceal cchar=←
"syntax match zeroNiceOperator "<->" conceal cchar=↔    " circled minus
syntax match zeroNiceOperator "->" conceal cchar=↦
"syntax match zeroNiceOperator "<-" conceal cchar=↤
syntax match zeroNiceOperator ":->" conceal cchar=⧴
syntax match zeroNiceOperator "=>" conceal cchar=⇒
"syntax match zeroNiceOperator "<=" conceal cchar=⇐     " less than or equal
syntax match zeroNiceOperator "\~>" conceal cchar=↝
syntax match zeroNiceOperator "<\~" conceal cchar=↜
syntax match zeroNiceOperator ">->" conceal cchar=↣
syntax match zeroNiceOperator "<-<" conceal cchar=↢
syntax match zeroNiceOperator ">>" conceal cchar=≫
syntax match zeroNiceOperator "<<" conceal cchar=≪
syntax match zeroNiceOperator "->>" conceal cchar=↠
syntax match zeroNiceOperator "<<-" conceal cchar=↞
syntax match zeroNiceOperator "<<<" conceal cchar=⋘
syntax match zeroNiceOperator ">>>" conceal cchar=⋙
syntax match zeroNiceOperator "=>>" conceal cchar=⇉
syntax match zeroNiceOperator "<<=" conceal cchar=⇇
syntax match zeroNiceOperator "==>" conceal cchar=⇛
syntax match zeroNiceOperator "<==" conceal cchar=⇚
syntax match zeroNiceOperator "<==>" conceal cchar=⇔

" wedge and triangle operators
syntax match zeroNiceOperator "/\\" conceal cchar=∧
syntax match zeroNiceOperator "\\/" conceal cchar=∨
syntax match zeroNiceOperator "/|" conceal cchar=⩘
syntax match zeroNiceOperator "|/" conceal cchar=⩗
syntax match zeroNiceOperator "|\\" conceal cchar=◺
syntax match zeroNiceOperator "\\|" conceal cchar=◹
syntax match zeroNiceOperator "\\|/" conceal cchar=⩛        " logical xor
syntax match zeroNiceOperator "/|\\" conceal cchar=⩚
syntax match zeroNiceOperator "/:\\" conceal cchar=⩓        " bitwise and
syntax match zeroNiceOperator "\\:/" conceal cchar=⩔        " bitwise or
syntax match zeroNiceOperator "/^\\" conceal cchar=⟁
syntax match zeroNiceOperator "/+\\" conceal cchar=⨹
syntax match zeroNiceOperator "/-\\" conceal cchar=⨺
syntax match zeroNiceOperator "/\*\\" conceal cchar=⨻
syntax match zeroNiceOperator "|><" conceal cchar=⋉
syntax match zeroNiceOperator "><|" conceal cchar=⋊
syntax match zeroNiceOperator "|><|" conceal cchar=⋈

" circle and diamond operators
syntax match zeroNiceOperator "<>" conceal cchar=∘
syntax match zeroNiceOperator "<:>" conceal cchar=⬫
syntax match zeroNiceOperator "<+>" conceal cchar=⊕
syntax match zeroNiceOperator "<->" conceal cchar=⊖
syntax match zeroNiceOperator "<\*>" conceal cchar=⊙
syntax match zeroNiceOperator "<\*\*>" conceal cchar=⊗
"syntax match zeroNiceOperator "< >" conceal cchar=⊛
syntax match zeroNiceOperator "</>" conceal cchar=⊘
syntax match zeroNiceOperator "<\\>" conceal cchar=⦸
syntax match zeroNiceOperator "<%>" conceal cchar=⦼
syntax match zeroNiceOperator "<|>" conceal cchar=⦶
syntax match zeroNiceOperator "<||>" conceal cchar=⦷
syntax match zeroNiceOperator "<=>" conceal cchar=⊜
syntax match zeroNiceOperator "<<>" conceal cchar=⧀
syntax match zeroNiceOperator "<>>" conceal cchar=⧁
syntax match zeroNiceOperator "<<>>" conceal cchar=⦾
syntax match zeroNiceOperator "<\\/>" conceal cchar=⎊
"syntax match zeroNiceOperator "" conceal cchar=⦿       " looks like circled dot
syntax match zeroNiceOperator "<^>" conceal cchar=⬦

" bar and turnstile operators (must come before boxed operators)
syntax match zeroNiceOperand "\<T\>" conceal cchar=⊤    " top
syntax match zeroNiceOperator "_|_" conceal cchar=⊥
syntax match zeroNiceOperator "|-" conceal cchar=⊢
syntax match zeroNiceOperator "-|" conceal cchar=⊣
syntax match zeroNiceOperator "||-" conceal cchar=⊩
syntax match zeroNiceOperator "-||" conceal cchar=⫣
syntax match zeroNiceOperator "|=" conceal cchar=⊨
syntax match zeroNiceOperator "=|" conceal cchar=⫤
syntax match zeroNiceOperator "||=" conceal cchar=⊫
syntax match zeroNiceOperator "=||" conceal cchar=⫥
syntax match zeroNiceOperator "|==" conceal cchar=⫢
syntax match zeroNiceOperator "-||-" conceal cchar=⟛
syntax match zeroNiceOperator "=||=" conceal cchar=⟚
syntax match zeroNiceOperator "|==|" conceal cchar=⧦

" boxed operators
syntax match zeroNiceOperator "&&" conceal cchar=⊓
syntax match zeroNiceOperator "|_|" conceal cchar=⊔
syntax match zeroNiceOperator "|\~|" conceal cchar=⧠
syntax match zeroNiceOperator "|+|" conceal cchar=⊞
syntax match zeroNiceOperator "|-|" conceal cchar=⊟
syntax match zeroNiceOperator "|\*|" conceal cchar=⊡
syntax match zeroNiceOperator "|\*\*|" conceal cchar=⊠
"syntax match zeroNiceOperator "| |" conceal cchar=⧆
syntax match zeroNiceOperator "|||" conceal cchar=◫
syntax match zeroNiceOperator "|/|" conceal cchar=⍁
syntax match zeroNiceOperator "|\\|" conceal cchar=⍂
syntax match zeroNiceOperator "|<>|" conceal cchar=⧇
syntax match zeroNiceOperator "||||" conceal cchar=⧈
syntax match zeroNiceOperator "|<|" conceal cchar=⍃
syntax match zeroNiceOperator "|>|" conceal cchar=⍄
syntax match zeroNiceOperator "|=|" conceal cchar=⌸
syntax match zeroNiceOperator "|=/=|" conceal cchar=⍯
syntax match zeroNiceOperator "|/\\|" conceal cchar=⍓
syntax match zeroNiceOperator "|\\/|" conceal cchar=⍌
syntax match zeroNiceOperator "|<-|" conceal cchar=⍇
syntax match zeroNiceOperator "|->|" conceal cchar=⍈
syntax match zeroNiceOperator "|?|" conceal cchar=⍰
syntax match zeroNiceOperator "|^|" conceal cchar=⌺
syntax match zeroNiceOperator "|:|" conceal cchar=⍠

" named operators
syntax match zeroNiceOperator "\<contradiction\>" conceal cchar=※
syntax match zeroNiceOperator "\<forall\>" conceal cchar=∀
syntax match zeroNiceOperator "\<exists\>" conceal cchar=∃
syntax match zeroNiceOperator "\<power\>" conceal cchar=℘
syntax match zeroNiceOperator "\<up\>" conceal cchar=↑
syntax match zeroNiceOperator "\<down\>" conceal cchar=↓
syntax match zeroNiceOperator "\<integral\>" conceal cchar=∫
syntax match zeroNiceOperator "\<infinity\>" conceal cchar=∞
syntax match zeroNiceOperator "\<sum\>" conceal cchar=∑
syntax match zeroNiceOperator "\<product\>" conceal cchar=∏
syntax match zeroNiceOperator "\<partial\>" conceal cchar=∂
syntax match zeroNiceOperator "\<grad\>" conceal cchar=𝛁
syn match DivHead contained "di" conceal cchar=𝛁
syn match DivTail contained "v" conceal cchar=∙
syn match zeroNiceOperator "\<div\>" contains=DivHead,DivTail
syn match DivHead contained "cu" conceal cchar=𝛁
syn match DivTail contained "rl" conceal cchar=×
syn match zeroNiceOperator "\<curl\>" contains=DivHead,DivTail
syn match LapHead contained "lapla" conceal cchar=𝛁
syn match LapTail contained "cian" conceal cchar=²
syn match zeroNiceOperator "\<laplacian\>" contains=LapHead,LapTail

syntax match zeroNiceOperand "\<AA\>" conceal cchar=𝔸
syntax match zeroNiceOperand "\<BB\>" conceal cchar=𝔹    " booleans
syntax match zeroNiceOperand "\<CC\>" conceal cchar=ℂ    " complex
syntax match zeroNiceOperand "\<DD\>" conceal cchar=𝔻    " decidable
syntax match zeroNiceOperand "\<EE\>" conceal cchar=𝔼    " euclidean space
syntax match zeroNiceOperand "\<FF\>" conceal cchar=𝔽    " field
syntax match zeroNiceOperand "\<GG\>" conceal cchar=𝔾    " group
syntax match zeroNiceOperand "\<HH\>" conceal cchar=ℍ    " quaternions
syntax match zeroNiceOperand "\<II\>" conceal cchar=𝕀
syntax match zeroNiceOperand "\<JJ\>" conceal cchar=𝕁
syntax match zeroNiceOperand "\<KK\>" conceal cchar=𝕂
syntax match zeroNiceOperand "\<LL\>" conceal cchar=𝕃
syntax match zeroNiceOperand "\<MM\>" conceal cchar=𝕄    " minkowski space
syntax match zeroNiceOperand "\<NN\>" conceal cchar=ℕ    " naturals
syntax match zeroNiceOperand "\<OO\>" conceal cchar=𝕆    " octonions
syntax match zeroNiceOperand "\<PP\>" conceal cchar=ℙ    " propositions
syntax match zeroNiceOperand "\<QQ\>" conceal cchar=ℚ    " rationals (quotient)
syntax match zeroNiceOperand "\<RR\>" conceal cchar=ℝ    " real numbers
syntax match zeroNiceOperand "\<SS\>" conceal cchar=𝕊    " class of all sets
syntax match zeroNiceOperand "\<TT\>" conceal cchar=𝕋    " alias for Top
syntax match zeroNiceOperand "\<UU\>" conceal cchar=𝕌    " alias for Top
syntax match zeroNiceOperand "\<VV\>" conceal cchar=𝕍    " vector space
syntax match zeroNiceOperand "\<WW\>" conceal cchar=𝕎
syntax match zeroNiceOperand "\<XX\>" conceal cchar=𝕏
syntax match zeroNiceOperand "\<YY\>" conceal cchar=𝕐
syntax match zeroNiceOperand "\<ZZ\>" conceal cchar=ℤ    " integers
syntax match zeroNiceOperand "\<i\>" conceal cchar=ℹ

" greek letters
syntax match lambdaZeroGreek "\<alpha\ze\(\A\|$\)" conceal contained cchar=𝛂
syntax match lambdaZeroGreek "\<beta\ze\(\A\|$\)" conceal contained cchar=𝛃
syntax match lambdaZeroGreek "\<gamma\ze\(\A\|$\)" conceal contained cchar=𝛄
syntax match lambdaZeroGreek "\<delta\ze\(\A\|$\)" conceal contained cchar=𝛅
syntax match lambdaZeroGreek "\<epsilon\ze\(\A\|$\)" conceal contained cchar=𝛆
syntax match lambdaZeroGreek "\<zeta\ze\(\A\|$\)" conceal contained cchar=𝛇
syntax match lambdaZeroGreek "\<eta\ze\(\A\|$\)" conceal contained cchar=𝛈
syntax match lambdaZeroGreek "\<theta\ze\(\A\|$\)" conceal contained cchar=𝛉
syntax match lambdaZeroGreek "\<iota\ze\(\A\|$\)" conceal contained cchar=𝛊
syntax match lambdaZeroGreek "\<kappa\ze\(\A\|$\)" conceal contained cchar=𝛋
syntax match lambdaZeroGreek "\<lambda\ze\(\A\|$\)" conceal contained cchar=𝛌
syntax match lambdaZeroGreek "\<mu\ze\(\A\|$\)" conceal contained cchar=𝛍
syntax match lambdaZeroGreek "\<nu\ze\(\A\|$\)" conceal contained cchar=𝛎
syntax match lambdaZeroGreek "\<xi\ze\(\A\|$\)" conceal contained cchar=𝛏
syntax match lambdaZeroGreek "\<omicron\ze\(\A\|$\)" conceal contained cchar=𝛐
syntax match lambdaZeroGreek "\<pi\ze\(\A\|$\)" conceal contained cchar=𝛑
syntax match lambdaZeroGreek "\<rho\ze\(\A\|$\)" conceal contained cchar=𝛒
syntax match lambdaZeroGreek "\<sigma\ze\(\A\|$\)" conceal contained cchar=𝛔
syntax match lambdaZeroGreek "\<tau\ze\(\A\|$\)" conceal contained cchar=𝛕
syntax match lambdaZeroGreek "\<upsilon\ze\(\A\|$\)" conceal contained cchar=𝛖
syntax match lambdaZeroGreek "\<phi\ze\(\A\|$\)" conceal contained cchar=𝛗
syntax match lambdaZeroGreek "\<chi\ze\(\A\|$\)" conceal contained cchar=𝛘
syntax match lambdaZeroGreek "\<psi\ze\(\A\|$\)" conceal contained cchar=𝛙
syntax match lambdaZeroGreek "\<omega\ze\(\A\|$\)" conceal contained cchar=𝛚
syntax match lambdaZeroGreek "\<Gamma\ze\(\A\|$\)" conceal contained cchar=𝚪
syntax match lambdaZeroGreek "\<Delta\ze\(\A\|$\)" conceal contained cchar=𝚫
syntax match lambdaZeroGreek "\<Theta\ze\(\A\|$\)" conceal contained cchar=𝚯
syntax match lambdaZeroGreek "\<Lambda\ze\(\A\|$\)" conceal contained cchar=𝚲
syntax match lambdaZeroGreek "\<Xi\ze\(\A\|$\)" conceal contained cchar=𝚵
syntax match lambdaZeroGreek "\<Pi\ze\(\A\|$\)" conceal contained cchar=𝚷
syntax match lambdaZeroGreek "\<Sigma\ze\(\A\|$\)" conceal contained cchar=𝚺
syntax match lambdaZeroGreek "\<Upsilon\ze\(\A\|$\)" conceal contained cchar=𝚼
syntax match lambdaZeroGreek "\<Phi\ze\(\A\|$\)" conceal contained cchar=𝚽
syntax match lambdaZeroGreek "\<Psi\ze\(\A\|$\)" conceal contained cchar=𝚿
syntax match lambdaZeroGreek "\<Omega\ze\(\A\|$\)" conceal contained cchar=𝛀

" primes and subscripts (appearing inside a name)
syntax match lambdaZeroPrime "''''\ze\([^']\|$\)" conceal contained cchar=⁗
syntax match lambdaZeroPrime "'''\ze\([^']\|$\)" conceal contained cchar=‴
syntax match lambdaZeroPrime "''\ze\([^']\|$\)" conceal contained cchar=″
syntax match lambdaZeroPrime "'\ze\([^']\|$\)" conceal contained cchar=′
"syntax match lambdaZeroSubscript "_i" conceal contained cchar=ᵢ
"syntax match lambdaZeroSubscript "_j" conceal contained cchar=ⱼ
"syntax match lambdaZeroSubscript "_x" conceal contained cchar=ₓ
syntax match lambdaZeroSubscript "_0" conceal contained cchar=₀
syntax match lambdaZeroSubscript "_1" conceal contained cchar=₁
syntax match lambdaZeroSubscript "_2" conceal contained cchar=₂
syntax match lambdaZeroSubscript "_3" conceal contained cchar=₃
syntax match lambdaZeroSubscript "_4" conceal contained cchar=₄
syntax match lambdaZeroSubscript "_5" conceal contained cchar=₅
syntax match lambdaZeroSubscript "_6" conceal contained cchar=₆
syntax match lambdaZeroSubscript "_7" conceal contained cchar=₇
syntax match lambdaZeroSubscript "_8" conceal contained cchar=₈
syntax match lambdaZeroSubscript "_9" conceal contained cchar=₉

" superscript operators
" ᵃ ᵇ ᶜ ᵈ ᵉ ᶠ ᵍ ʰ ⁱ ʲ ᵏ ˡ ᵐ ⁿ ᵒ ᵖ ʳ ˢ ᵗ ᵘ ᵛ ʷ ˣ ʸ ᶻ
syntax match zeroNiceOperator "\^<>" conceal cchar=°
syntax match zeroNiceOperator "\^+" conceal cchar=⁺
syntax match zeroNiceOperator "\^-" conceal cchar=⁻
syntax match zeroNiceOperator "\^=" conceal cchar=⁼
syntax match zeroNiceOperator "\^0" conceal cchar=⁰
syntax match zeroNiceOperator "\^1" conceal cchar=¹
syntax match zeroNiceOperator "\^2" conceal cchar=²
syntax match zeroNiceOperator "\^3" conceal cchar=³
syntax match zeroNiceOperator "\^4" conceal cchar=⁴
syntax match zeroNiceOperator "\^5" conceal cchar=⁵
syntax match zeroNiceOperator "\^6" conceal cchar=⁶
syntax match zeroNiceOperator "\^7" conceal cchar=⁷
syntax match zeroNiceOperator "\^8" conceal cchar=⁸
syntax match zeroNiceOperator "\^9" conceal cchar=⁹
syntax match zeroNiceOperator "\^n" conceal cchar=ⁿ
syntax match zeroNiceOperator "\^i" conceal cchar=ⁱ
syntax match zeroNiceOperator "\^j" conceal cchar=ʲ
syntax match zeroNiceOperator "\^k" conceal cchar=ᵏ

" combining characters (appearing inside a name)
syntax match zeroNiceOperator "\^_" conceal cchar=̅
syntax match zeroNiceOperator "\^\^" conceal cchar=̂
syntax match zeroNiceOperator "\^__" conceal cchar=̿
syntax match zeroNiceOperator "\^\~" conceal cchar=̃
syntax match zeroNiceOperator "\^\*" conceal cchar=̇
syntax match zeroNiceOperator "\^\*\*" conceal cchar=̈
syntax match zeroNiceOperator "\^->" conceal cchar=⃗

highlight link zeroNiceOperator Operator
highlight! link Conceal Operator
setlocal conceallevel=2
setlocal concealcursor=nc
