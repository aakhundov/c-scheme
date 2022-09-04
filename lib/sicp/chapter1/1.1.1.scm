; 1.1.1 Expressions

(assert-equals '486 486)

(assert-equals '(+ 137 349) 486)
(assert-equals '(- 1000 334) 666)
(assert-equals '(* 5 99) 495)
(assert-equals '(/ 10 5) 2)
(assert-equals '(+ 2.7 10) 12.7)

(assert-equals '(+ 21 35 12 7) 75)
(assert-equals '(* 25 4 12) 1200)

(assert-equals '(+ (* 3 5) (- 10 6)) 19)
(assert-equals '(+ (* 3 (+ (* 2 4) (+ 3 5))) (+ (- 10 7) 6)) 57)
