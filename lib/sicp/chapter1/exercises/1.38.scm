; Exercise 1.38

(define (cont-frac-iter n d k)
    (define (iter result i)
        (if (= i 0)
            result
            (iter (/ (n i) (+ (d i) result))
                  (- i 1))))
    (iter 0 k))

(define (n i) 1)

(define (d i)
    (if (= (remainder (+ i 1) 3) 0)
        (* (/ (+ i 1) 3) 2)
        1))

(assert-equal '(+ (cont-frac-iter n d 10) 2) 2.71828)
