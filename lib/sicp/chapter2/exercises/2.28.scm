; Exercise 2.28

(define (fringe items)
    (cond ((null? items) '())
          ((not (list? (car items)))
           (cons (car items)
                 (fringe (cdr items))))
          (else (append (fringe (car items))
                        (fringe (cdr items))))))

(define x (list (list 1 2) (list 3 4)))

(assert-equals '(fringe x) '(1 2 3 4))
(assert-equals '(fringe (list x x)) '(1 2 3 4 1 2 3 4))

(assert-equals '(fringe '()) '())
(assert-equals '(fringe '(1)) '(1))
(assert-equals '(fringe '(1 2 3 4 5)) '(1 2 3 4 5))
(assert-equals '(fringe '((1 2) 3 (4 5))) '(1 2 3 4 5))
(assert-equals '(fringe '((1 (2)) (((3) 4) 5 (6 7 8)))) '(1 2 3 4 5 6 7 8))
