; 1.1.5 The Substitution Model for Procedure Application

(define (sum-of-squares x y)
    (+ (square x) (square y)))

(assert-equals '(sum-of-squares (+ 5 1) (* 5 2)) 136)
(assert-equals '(+ (square 6) (square 10)) 136)
(assert-equals '(+ (* 6 6) (* 10 10)) 136)
(assert-equals '(+ 36 100) 136)

(assert-equals '(+ (square (+ 5 1)) (square (* 5 2)) ) 136)
(assert-equals '(+ (* (+ 5 1) (+ 5 1)) (* (* 5 2) (* 5 2))) 136)
