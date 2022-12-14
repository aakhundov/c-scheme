; 1.1.4 Compound Procedures

(define (square x) (* x x))

(assert-equals '(square 21) 441)
(assert-equals '(square (+ 2 5)) 49)
(assert-equals '(square (square 3)) 81)

(define (sum-of-squares x y)
    (+ (square x) (square y)))

(assert-equals '(sum-of-squares 3 4) 25)

(define (f a)
    (sum-of-squares (+ a 1) (* a 2)))

(assert-equals '(f 5) 136)
