; 1.1.2 Naming and the Environment

(define size 2)

(assert-equal 'size 2)
(assert-equal '(* 5 size) 10)

(define pi 3.14159)
(define radius 10)

(assert-equal '(* pi (* radius radius)) 314.159)

(define circumference (* 2 pi radius))

(assert-equal 'circumference 62.8318)
