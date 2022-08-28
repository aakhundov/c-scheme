; Exercise 1.41

(define (double f)
    (lambda (x) (f (f x))))

(assert-equal '((double inc) 5) 7)
(assert-equal '((double (double inc)) 5) 9)
(assert-equal '(((double double) inc) 5) 9)
(assert-equal '((double (double (double inc))) 5) 13)
(assert-equal '(((double (double double)) inc) 5) 21)
(assert-equal '((((double double) double) inc) 5) 21)
