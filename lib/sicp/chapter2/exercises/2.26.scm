; Exercise 2.26

(define x (list 1 2 3))
(define y (list 4 5 6))

(assert-equals '(append x y) '(1 2 3 4 5 6))
(assert-equals '(cons x y) '((1 2 3) 4 5 6))
(assert-equals '(list x y) '((1 2 3) (4 5 6)))
