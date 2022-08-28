; 1.1.8 Procedures as Black-Box Abstractions

(define (square x) (* x x))
(define (square2 x) (exp (double (log x))))
(define (double x) (+ x x))

(assert-equal '(square 3.14) 9.8596)
(assert-equal '(square2 3.14) 9.8596)

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

(assert-equal '(sqrt 9) 3)
(assert-equal '(sqrt (+ 100 37)) 11.7047)
(assert-equal '(sqrt (+ (sqrt 2) (sqrt 3))) 1.77377)
(assert-equal '(square (sqrt 1000)) 1000)
