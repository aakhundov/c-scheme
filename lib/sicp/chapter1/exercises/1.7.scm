; Exercise 1.7

(define (sqrt-iter guess x)
    (let ((improved (improve guess x)))
        (if (good-enough? guess improved)
             guess
             (sqrt-iter improved x))))

(define (improve guess x)
    (average guess (/ x guess)))

(define (average x y)
    (/ (+ x y) 2))

(define (good-enough? guess improved)
    (< (abs (- guess improved)) 1e-8))

(define (sqrt x)
    (sqrt-iter 1.0 x))

(assert-equals '(sqrt 1e9) 31622.7766)
(assert-equals '(sqrt 1e8) 10000.0000)
(assert-equals '(sqrt 1e7) 3162.27776)
(assert-equals '(sqrt 1e6) 1000.00000)
(assert-equals '(sqrt 1e5) 316.227776)
(assert-equals '(sqrt 1e4) 100.000000)
(assert-equals '(sqrt 1e3) 31.6227776)
(assert-equals '(sqrt 1e2) 10.0000000)
(assert-equals '(sqrt 1e1) 3.16227776)

(assert-equals '(sqrt 1e0) 1)

(assert-equals '(sqrt 1e-1) 0.3162277)
(assert-equals '(sqrt 1e-2) 0.1000000)
(assert-equals '(sqrt 1e-3) 0.0316227)
(assert-equals '(sqrt 1e-4) 0.0100000)
(assert-equals '(sqrt 1e-5) 0.0031623)
(assert-equals '(sqrt 1e-6) 0.0010000)
(assert-equals '(sqrt 1e-7) 0.0003162)
(assert-equals '(sqrt 1e-8) 0.0001000)
