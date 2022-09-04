; 2.1.3 What Is Meant by Data?

(define (my-cons x y)
    (define (dispatch m)
        (cond ((= m 0) x)
              ((= m 1) y)
              (else (error "Argument not 0 or 1: CONS %s" m))))
    dispatch)

(define (my-car z) (z 0))
(define (my-cdr z) (z 1))

(assert-equal '(my-car (my-cons 1 2)) 1)
(assert-equal '(my-cdr (my-cons 1 2)) 2)
