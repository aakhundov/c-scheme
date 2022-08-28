; 1.3.1 Procedures as Arguments

(define (sum term a next b)
    (if (> a b)
        0
        (+ (term a)
           (sum term (next a) next b))))

(define (sum-cubes a b)
    (sum cube a inc b))

(assert-equal '(sum-cubes 1 10) 3025)

(define (sum-integers a b)
    (sum identity a inc b))

(assert-equal '(sum-integers 1 10) 55)

(define (pi-sum a b)
    (define (pi-term x)
        (/ 1.0 (* x (+ x 2))))
    (define (pi-next x)
        (+ x 4))
    (sum pi-term a pi-next b))

(assert-equal '(* 8 (pi-sum 1 1000)) 3.139592655589783)

(define (integral f a b dx)
    (define (add-dx x)
        (+ x dx))
    (* (sum f (+ a (/ dx 2.0)) add-dx b)
       dx))

(assert-equal '(integral cube 0 1 0.01) .24998750000000042)
(assert-equal '(integral cube 0 1 0.001) .249999875000001)
