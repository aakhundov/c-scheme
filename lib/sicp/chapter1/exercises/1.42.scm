; Exercise 1.42

(define (compose f g) (lambda (x) (f (g x))))

(assert-equals '((compose square inc) 6) 49)
(assert-equals '((compose inc square) 6) 37)
