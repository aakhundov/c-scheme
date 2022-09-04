; 1.1.7 Example: Square Roots by Newton’s Method

(define (sqrt-iter guess x)
    (if (good-enough? guess x)
        guess
        (sqrt-iter (improve guess x) x)))

(define (improve guess x)
    (average guess (/ x guess)))

(define (average x y)
    (/ (+ x y) 2))

(define (good-enough? guess x)
    (< (abs (- (square guess) x)) 1e-4))

(define (sqrt x)
    (sqrt-iter 1.0 x))

(assert-equals '(sqrt 9) 3)
(assert-equals '(sqrt (+ 100 37)) 11.7047)
(assert-equals '(sqrt (+ (sqrt 2) (sqrt 3))) 1.77377)
(assert-equals '(square (sqrt 1000)) 1000)
