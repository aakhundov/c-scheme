; Exercise 1.34

(define (f g) (g 2))

(assert-equals '(f square) 4)
(assert-equals '(f (lambda (z) (* z (+ z 1)))) 6)

; error: can't apply 2
; (f f)
