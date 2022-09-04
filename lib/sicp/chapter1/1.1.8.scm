; 1.1.8 Procedures as Black-Box Abstractions

(define (square x) (* x x))
(define (square2 x) (exp (double (log x))))
(define (double x) (+ x x))

(assert-equals '(square 3.14) 9.8596)
(assert-equals '(square2 3.14) 9.8596)

(define (sqrt x)
    (define (good-enough? guess)
        (< (abs (- (square guess) x)) 1e-4))
    (define (improve guess)
        (average guess (/ x guess)))
    (define (sqrt-iter guess)
        (if (good-enough? guess)
            guess
            (sqrt-iter (improve guess))))
    (sqrt-iter 1.0))

(assert-equals '(sqrt 9) 3)
(assert-equals '(sqrt (+ 100 37)) 11.7047)
(assert-equals '(sqrt (+ (sqrt 2) (sqrt 3))) 1.77377)
(assert-equals '(square (sqrt 1000)) 1000)
