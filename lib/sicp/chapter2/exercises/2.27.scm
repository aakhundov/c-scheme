; Exercise 2.27

(define (deep-reverse items)
    (define (iter rest head)
        (if (null? rest)
            head
            (let ((new-head (list (deep-reverse (car rest)))))
                (set-cdr! new-head head)
                (iter (cdr rest) new-head))))
    (cond ((or (null? items) (not (list? items))) items)
          ((null? (cdr items)) (list (deep-reverse (car items))))
          (else (iter items '()))))

(define x (list (list 1 2) (list 3 4)))

(assert-equals 'x '((1 2) (3 4)))
(assert-equals '(reverse x) '((3 4) (1 2)))
(assert-equals '(deep-reverse x) '((4 3) (2 1)))

(assert-equals '(deep-reverse '()) '())
(assert-equals '(deep-reverse '(1)) '(1))
(assert-equals '(deep-reverse '((1))) '((1)))
(assert-equals '(deep-reverse '((1) (2))) '((2) (1)))
(assert-equals '(deep-reverse '((1 2) 3 (4 (5 6) 7))) '((7 (6 5) 4) 3 (2 1)))
