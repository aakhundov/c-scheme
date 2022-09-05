; Exercise 2.25

(assert-equals '(cadaddr '(1 3 (5 7) 9)) 7)
(assert-equals '(caar '((7))) 7)
(assert-equals '(cadadadr (cadadadr '(1 (2 (3 (4 (5 (6 7)))))))) 7)
