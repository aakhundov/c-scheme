; 1.1.5 The Substitution Model for Procedure Application

(define (sum-of-squares x y)
    (+ (square x) (square y)))

(assert-equal '(sum-of-squares (+ 5 1) (* 5 2)) 136)
(assert-equal '(+ (square 6) (square 10)) 136)
(assert-equal '(+ (* 6 6) (* 10 10)) 136)
(assert-equal '(+ 36 100) 136)

(assert-equal '(+ (square (+ 5 1)) (square (* 5 2)) ) 136)
(assert-equal '(+ (* (+ 5 1) (+ 5 1)) (* (* 5 2) (* 5 2))) 136)
