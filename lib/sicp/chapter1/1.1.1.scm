; 1.1.1 Expressions

(assert-equal '486 486)

(assert-equal '(+ 137 349) 486)
(assert-equal '(- 1000 334) 666)
(assert-equal '(* 5 99) 495)
(assert-equal '(/ 10 5) 2)
(assert-equal '(+ 2.7 10) 12.7)

(assert-equal '(+ 21 35 12 7) 75)
(assert-equal '(* 25 4 12) 1200)

(assert-equal '(+ (* 3 5) (- 10 6)) 19)
(assert-equal '(+ (* 3 (+ (* 2 4) (+ 3 5))) (+ (- 10 7) 6)) 57)
