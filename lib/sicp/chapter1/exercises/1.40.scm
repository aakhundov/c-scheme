; Exercise 1.40

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

(define dx 0.00001)

(define (deriv g)
    (lambda (x) (/ (- (g (+ x dx)) (g x)) dx)))

(define (newton-transform g)
    (let ((dg (deriv g)))
        (lambda (x) (- x (/ (g x) (dg x))))))

(define (newtons-method g guess)
    (fixed-point (newton-transform g) guess))

(define (cubic a b c)
    (lambda (x) (+ (cube x) (* a (square x)) (* b x) c)))

(assert-equals '(newtons-method (cubic 1 1 1) 1.0) -1)
(assert-equals '(newtons-method (cubic 1 2 3) 1.0) -1.27568)
(assert-equals '(newtons-method (cubic 3 2 1) 1.0) -2.32472)
