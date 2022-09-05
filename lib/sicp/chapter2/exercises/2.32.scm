; Exercise 2.32

(define (subsets s)
    (if (null? s)
        (list nil)
        (let ((rest (subsets (cdr s))))
            (append rest (map (lambda (x) (cons (car s) x))
                              rest)))))

(assert-equals '(subsets '()) '(()))
(assert-equals '(subsets '(1)) '(() (1)))
(assert-equals '(subsets '(1 2)) '(() (2) (1) (1 2)))
(assert-equals '(subsets '(1 2 3)) '(() (3) (2) (2 3) (1) (1 3) (1 2) (1 2 3)))
