; Exercise 1.42

(define (compose f g) (lambda (x) (f (g x))))

(assert-equal '((compose square inc) 6) 49)
(assert-equal '((compose inc square) 6) 37)
