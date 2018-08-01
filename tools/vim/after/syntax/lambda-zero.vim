" vim: set fenc=utf-8:
" if one operator is a substring of another, make sure the shortest comes
" first

syntax match zeroNiceOperator "!" conceal cchar=¬
syntax match zeroNiceOperator "\*\*" conceal cchar=×
syntax match zeroNiceOperator ">:<" conceal cchar=⨰
syntax match zeroNiceOperator "><" conceal cchar=⪤
syntax match zeroNiceOperator ":" conceal cchar=∈
syntax match zeroNiceOperator "!:" conceal cchar=∉
syntax match zeroNiceOperator "::" conceal cchar=∷
syntax match zeroNiceOperator "++" conceal cchar=⧺
syntax match zeroNiceOperator "\.\." conceal cchar=‥
syntax match zeroNiceOperator "-:" conceal cchar=∸
syntax match zeroNiceOperator "##" conceal cchar=⩨
syntax match zeroNiceOperator "&" conceal cchar=∩
syntax match zeroNiceOperator "\\\./" conceal cchar=⋃
syntax match zeroNiceOperator "\~\~" conceal cchar=≀
syntax match zeroNiceOperator "+-" conceal cchar=±
syntax match zeroNiceOperator "-+" conceal cchar=∓

" comparison operators
syntax match zeroNiceOperator "\~:" conceal cchar=⩪
syntax match zeroNiceOperator "!\~" conceal cchar=≁
syntax match zeroNiceOperator "<=" conceal cchar=≤
syntax match zeroNiceOperator ">=" conceal cchar=≥
syntax match zeroNiceOperator "=<" conceal cchar=≼
syntax match zeroNiceOperator "\~<" conceal cchar=≾
syntax match zeroNiceOperator "<:" conceal cchar=⊆
syntax match zeroNiceOperator ":>" conceal cchar=⊇
syntax match zeroNiceOperator "!<:" conceal cchar=⊈
syntax match zeroNiceOperator "!:>" conceal cchar=⊉
syntax match zeroNiceOperator "!=" conceal cchar=≠
syntax match zeroNiceOperator "==" conceal cchar=≡
syntax match zeroNiceOperator "!==" conceal cchar=≢
syntax match zeroNiceOperator ":=" conceal cchar=≔
syntax match zeroNiceOperator "=:" conceal cchar=≕
syntax match zeroNiceOperator "::=" conceal cchar=⩴
syntax match zeroNiceOperator "=:=" conceal cchar=≐
syntax match zeroNiceOperator "=\~=" conceal cchar=≅
syntax match zeroNiceOperator "!=\~=" conceal cchar=≇
syntax match zeroNiceOperator "=\*=" conceal cchar=≛
syntax match zeroNiceOperator "=<>=" conceal cchar=≗
syntax match zeroNiceOperator "=?=" conceal cchar=≟


" arrow operators
syntax match zeroNiceOperator "<-" conceal cchar=↤
syntax match zeroNiceOperator "->" conceal cchar=↦
syntax match zeroNiceOperator "<=>" conceal cchar=⇔
syntax match zeroNiceOperator "=>" conceal cchar=⇒
syntax match zeroNiceOperator "-->" conceal cchar=→
syntax match zeroNiceOperator "<--" conceal cchar=←
syntax match zeroNiceOperator "<-->" conceal cchar=↔
syntax match zeroNiceOperator "\~>" conceal cchar=↝
syntax match zeroNiceOperator "<\~" conceal cchar=↜
syntax match zeroNiceOperator ">->" conceal cchar=↣
syntax match zeroNiceOperator "<-<" conceal cchar=↢
syntax match zeroNiceOperator "<<" conceal cchar=⟨
syntax match zeroNiceOperator ">>" conceal cchar=⟩
syntax match zeroNiceOperator ">>=" conceal cchar=≫
syntax match zeroNiceOperator "=<<" conceal cchar=≪
syntax match zeroNiceOperator "->>" conceal cchar=↠
syntax match zeroNiceOperator "<<-" conceal cchar=↞
syntax match zeroNiceOperator "<<<" conceal cchar=⋘
syntax match zeroNiceOperator ">>>" conceal cchar=⋙
syntax match zeroNiceOperator "=>>" conceal cchar=⪢
syntax match zeroNiceOperator "<<=" conceal cchar=⪡

" wedge and triangle operators
syntax match zeroNiceOperator "\\-/" conceal cchar=∀
syntax match zeroNiceOperator "/\\" conceal cchar=∧
syntax match zeroNiceOperator "\\/" conceal cchar=∨
syntax match zeroNiceOperator "/|" conceal cchar=⩘
syntax match zeroNiceOperator "|/" conceal cchar=⩗
syntax match zeroNiceOperator "|\\" conceal cchar=◺
syntax match zeroNiceOperator "\\|" conceal cchar=◹
syntax match zeroNiceOperator "\\|/" conceal cchar=⩛
syntax match zeroNiceOperator "/|\\" conceal cchar=⩚
syntax match zeroNiceOperator "/:\\" conceal cchar=⟑
syntax match zeroNiceOperator "\\:/" conceal cchar=⟇
syntax match zeroNiceOperator "<:<" conceal cchar=⋖
syntax match zeroNiceOperator ">:>" conceal cchar=⋗
syntax match zeroNiceOperator "/^\\" conceal cchar=⩓
syntax match zeroNiceOperator "\\^/" conceal cchar=⩔
syntax match zeroNiceOperator "/+\\" conceal cchar=⨹
syntax match zeroNiceOperator "/-\\" conceal cchar=⨺
syntax match zeroNiceOperator "/\*\\" conceal cchar=⨻
syntax match zeroNiceOperator "<|" conceal cchar=⊲
syntax match zeroNiceOperator "|>" conceal cchar=⊳
syntax match zeroNiceOperator "=<|" conceal cchar=⊴
syntax match zeroNiceOperator "|>=" conceal cchar=⊵
syntax match zeroNiceOperator "|><" conceal cchar=⋉
syntax match zeroNiceOperator "><|" conceal cchar=⋊
syntax match zeroNiceOperator "|><|" conceal cchar=⋈

" circle and diamond operators
syntax match zeroNiceOperator "<>" conceal cchar=○
syntax match zeroNiceOperator "<+>" conceal cchar=⊕
syntax match zeroNiceOperator "<->" conceal cchar=⊖
syntax match zeroNiceOperator "<\*>" conceal cchar=⊛
syntax match zeroNiceOperator "<\*\*>" conceal cchar=⊗
syntax match zeroNiceOperator "<:>" conceal cchar=⊙
syntax match zeroNiceOperator "</>" conceal cchar=⊘
syntax match zeroNiceOperator "<\\>" conceal cchar=⦸
syntax match zeroNiceOperator "<%>" conceal cchar=⦼
syntax match zeroNiceOperator "<|>" conceal cchar=⦶
syntax match zeroNiceOperator "<||>" conceal cchar=⦷
syntax match zeroNiceOperator "<==>" conceal cchar=⊜  " conflict with arrow
syntax match zeroNiceOperator "<<>" conceal cchar=⧀
syntax match zeroNiceOperator "<>>" conceal cchar=⧁
syntax match zeroNiceOperator "<<>>" conceal cchar=⦾
syntax match zeroNiceOperator "<@>" conceal cchar=⦿
syntax match zeroNiceOperator "<\\/>" conceal cchar=⎊
syntax match zeroNiceOperator "<^>" conceal cchar=⬦
syntax match zeroNiceOperator "<\.>" conceal cchar=⟐

" bar and turnstile operators (must come before boxed operators)
syntax match zeroNiceOperator "||" conceal cchar=‖
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
syntax match zeroNiceOperator "|&|" conceal cchar=⊓
syntax match zeroNiceOperator "|\.\.|" conceal cchar=⊔
syntax match zeroNiceOperator "|||" conceal cchar=⧠
syntax match zeroNiceOperator "|+|" conceal cchar=⊞
syntax match zeroNiceOperator "|-|" conceal cchar=⊟
syntax match zeroNiceOperator "|\*|" conceal cchar=⧆
syntax match zeroNiceOperator "|\*\*|" conceal cchar=⊠
syntax match zeroNiceOperator "|\.|" conceal cchar=⊡
syntax match zeroNiceOperator "|/|" conceal cchar=⍁
syntax match zeroNiceOperator "|\\|" conceal cchar=⍂
syntax match zeroNiceOperator "|<>|" conceal cchar=⧇
syntax match zeroNiceOperator "||||" conceal cchar=⧈
syntax match zeroNiceOperator "|<|" conceal cchar=⍃
syntax match zeroNiceOperator "|>|" conceal cchar=⍄
syntax match zeroNiceOperator "|=|" conceal cchar=⌸
syntax match zeroNiceOperator "|!=|" conceal cchar=⍯
syntax match zeroNiceOperator "|:|" conceal cchar=⍠
syntax match zeroNiceOperator "|/\\|" conceal cchar=⍓
syntax match zeroNiceOperator "|\\/|" conceal cchar=⍌
syntax match zeroNiceOperator "|<-|" conceal cchar=⍇
syntax match zeroNiceOperator "|->|" conceal cchar=⍈
syntax match zeroNiceOperator "|?|" conceal cchar=⍰
syntax match zeroNiceOperator "|^|" conceal cchar=⌺

" superscript operators
syntax match zeroNiceOperator "\^\^(n)" conceal cchar=ⁿ
syntax match zeroNiceOperator "\^\^(2)" conceal cchar=²
syntax match zeroNiceOperator "\^\^(3)" conceal cchar=³

" named operators
syntax match zeroNiceOperator "\<int\>" conceal cchar=∫
syntax match zeroNiceOperator "\<infinity\>" conceal cchar=∞
syntax match zeroNiceOperator "\<powerSet\>" conceal cchar=℘
syntax match zeroNiceOperator "\<sum\>" conceal cchar=∑
syntax match zeroNiceOperator "\<product\>" conceal cchar=∏
syntax match zeroNiceOperator "\<sqrt\>" conceal cchar=√
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

syntax match zeroNiceOperand "\<A\>" conceal cchar=𝔸
syntax match zeroNiceOperand "\<B\>" conceal cchar=𝔹
syntax match zeroNiceOperand "\<C\>" conceal cchar=ℂ
syntax match zeroNiceOperand "\<D\>" conceal cchar=𝔻
syntax match zeroNiceOperand "\<E\>" conceal cchar=𝔼
syntax match zeroNiceOperand "\<F\>" conceal cchar=𝔽
syntax match zeroNiceOperand "\<G\>" conceal cchar=𝔾
syntax match zeroNiceOperand "\<H\>" conceal cchar=ℍ
syntax match zeroNiceOperand "\<I\>" conceal cchar=𝕀
syntax match zeroNiceOperand "\<J\>" conceal cchar=𝕁
syntax match zeroNiceOperand "\<K\>" conceal cchar=𝕂
syntax match zeroNiceOperand "\<L\>" conceal cchar=𝕃
syntax match zeroNiceOperand "\<M\>" conceal cchar=𝕄
syntax match zeroNiceOperand "\<N\>" conceal cchar=ℕ
syntax match zeroNiceOperand "\<O\>" conceal cchar=𝕆
syntax match zeroNiceOperand "\<P\>" conceal cchar=ℙ
syntax match zeroNiceOperand "\<Q\>" conceal cchar=ℚ
syntax match zeroNiceOperand "\<R\>" conceal cchar=ℝ
syntax match zeroNiceOperand "\<S\>" conceal cchar=𝕊
syntax match zeroNiceOperand "\<T\>" conceal cchar=𝕋
syntax match zeroNiceOperand "\<U\>" conceal cchar=𝕌
syntax match zeroNiceOperand "\<V\>" conceal cchar=𝕍
syntax match zeroNiceOperand "\<W\>" conceal cchar=𝕎
syntax match zeroNiceOperand "\<X\>" conceal cchar=𝕏
syntax match zeroNiceOperand "\<Y\>" conceal cchar=𝕐
syntax match zeroNiceOperand "\<Z\>" conceal cchar=ℤ
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

syntax match lambdaZeroPrime "''''\ze\([^']\|$\)" conceal contained cchar=⁗
syntax match lambdaZeroPrime "'''\ze\([^']\|$\)" conceal contained cchar=‴
syntax match lambdaZeroPrime "''\ze\([^']\|$\)" conceal contained cchar=″
syntax match lambdaZeroPrime "'\ze\([^']\|$\)" conceal contained cchar=′
syntax match lambdaZeroSubscript "_i" conceal contained cchar=ᵢ
syntax match lambdaZeroSubscript "_j" conceal contained cchar=ⱼ
syntax match lambdaZeroSubscript "_x" conceal contained cchar=ₓ
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


highlight link zeroNiceOperator Operator
highlight! link Conceal Operator
setlocal conceallevel=2
