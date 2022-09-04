; 1.1.2 Naming and the Environment

(define size 2)

(assert-equals 'size 2)
(assert-equals '(* 5 size) 10)

(define pi 3.14159)
(define radius 10)

(assert-equals '(* pi (* radius radius)) 314.159)

(define circumference (* 2 pi radius))

(assert-equals 'circumference 62.8318)
