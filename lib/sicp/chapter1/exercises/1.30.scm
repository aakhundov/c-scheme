; Exercise 1.30

(define (sum-iter term a next b)
    (define (iter a result)
        (if (> a b)
            result
            (iter (next a) (+ result (term a)))))
    (iter a 0))

(define (sum-cubes a b)
    (sum-iter cube a inc b))

(assert-equal '(sum-cubes 1 10) 3025)

(define (sum-integers a b)
    (sum-iter identity a inc b))

(assert-equal '(sum-integers 1 10) 55)

(define (pi-sum a b)
    (define (pi-term x)
        (/ 1.0 (* x (+ x 2))))
    (define (pi-next x)
        (+ x 4))
    (sum-iter pi-term a pi-next b))

(assert-equal '(* 8 (pi-sum 1 1000)) 3.139592655589783)
