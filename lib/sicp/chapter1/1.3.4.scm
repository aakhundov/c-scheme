; 1.3.4 Procedures as Returned Values

(define tolerance 0.00001)
(define (fixed-point f first-guess)
    (define (close-enough? v1 v2)
        (< (abs (- v1 v2))
           tolerance))
    (define (try guess)
        (let ((next (f guess)))
            (if (close-enough? guess next)
                next
                (try next))))
    (try first-guess))

(define (average-damp f)
    (lambda (x) (average x (f x))))

(assert-equals '((average-damp square) 10) 55)

(define (sqrt1 x)
    (fixed-point (average-damp (lambda (y) (/ x y)))
                 1.0))

(assert-equals '(sqrt1 2) 1.41421)
(assert-equals '(sqrt1 3) 1.73205)

(define (cube-root1 x)
    (fixed-point (average-damp (lambda (y) (/ x (square y))))
                 1.0))

(assert-equals '(cube-root1 2) 1.25992)
(assert-equals '(cube-root1 3) 1.44225)

(define (deriv g)
    (lambda (x) (/ (- (g (+ x dx)) (g x)) dx)))

(define dx 0.00001)

(assert-equals '((deriv cube) 5) 75.00014999664018)

(define (newton-transform g)
    (let ((dg (deriv g)))
        (lambda (x) (- x (/ (g x) (dg x))))))

(define (newtons-method g guess)
    (fixed-point (newton-transform g) guess))

(define (sqrt2 x)
    (newtons-method (lambda (y) (- (square y) x))
                    1.0))

(assert-equals '(sqrt2 2) 1.41421)
(assert-equals '(sqrt2 3) 1.73205)

(define (cube-root2 x)
    (newtons-method (lambda (y) (- (cube y) x))
                    1.0))

(assert-equals '(cube-root2 2) 1.25992)
(assert-equals '(cube-root2 3) 1.44225)

(define (fixed-point-of-transform g transform guess)
    (fixed-point (transform g) guess))

(define (sqrt3 x)
    (fixed-point-of-transform (lambda (y) (/ x y))
                              average-damp
                              1.0))

(assert-equals '(sqrt3 2) 1.41421)
(assert-equals '(sqrt3 3) 1.73205)

(define (sqrt4 x)
    (fixed-point-of-transform (lambda (y) (- (square y) x))
                              newton-transform
                              1.0))

(assert-equals '(sqrt4 2) 1.41421)
(assert-equals '(sqrt4 3) 1.73205)
