; Exercise 1.34

(define (f g) (g 2))

(assert-equal '(f square) 4)
(assert-equal '(f (lambda (z) (* z (+ z 1)))) 6)

(f f)  ; error: "can't apply 2"
