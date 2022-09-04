; Exercise 1.38

(define (cont-frac-iter n d k)
    (define (iter result i)
        (if (= i 0)
            result
            (iter (/ (n i) (+ (d i) result))
                  (- i 1))))
    (iter 0 k))

(define (tan-cf x k)
    (define (n i) (if (= i 1) x (- (square x))))
    (define (d i) (- (* 2 i) 1))
    (cont-frac-iter n d k)
)

(assert-equals '(tan-cf 1 10) (tan 1))
(assert-equals '(tan-cf 0 10) (tan 0))
(assert-equals '(tan-cf -1 10) (tan -1))
