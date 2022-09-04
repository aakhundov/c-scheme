; Exercise 1.41

(define (double f)
    (lambda (x) (f (f x))))

(assert-equals '((double inc) 5) 7)
(assert-equals '((double (double inc)) 5) 9)
(assert-equals '(((double double) inc) 5) 9)
(assert-equals '((double (double (double inc))) 5) 13)
(assert-equals '(((double (double double)) inc) 5) 21)
(assert-equals '((((double double) double) inc) 5) 21)
