; Exercise 2.4

(define (my-cons x y)
    (lambda (m) (m x y)))
(define (my-car z)
    (z (lambda (p q) p)))
(define (my-cdr z)
    (z (lambda (p q) q)))

(assert-equals '(my-car (my-cons 1 2)) 1)
(assert-equals '(my-cdr (my-cons 1 2)) 2)
