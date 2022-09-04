; Exercise 1.31

(define (product-rec term a next b)
    (if (> a b)
        1
        (* (term a)
           (product-rec term (next a) next b))))

(define (product-iter term a next b)
    (define (iter a result)
        (if (> a b)
            result
            (iter (next a) (* result (term a)))))
    (iter a 1))

(define (fact acc n) (acc identity 1 inc n))

(assert-equals '(fact product-rec 1) 1)
(assert-equals '(fact product-rec 5) 120)
(assert-equals '(fact product-rec 10) 3628800)
(assert-equals '(fact product-iter 1) 1)
(assert-equals '(fact product-iter 5) 120)
(assert-equals '(fact product-iter 10) 3628800)

(define (pi-product acc n)
    (define (pi-term x)
        (/ (* (- x 1) (+ x 1)) (* x x)))
    (define (pi-next x)
        (+ x 2))
    (* 4 (acc pi-term 3 pi-next n)))

(assert-equals '(pi-product product-iter 1000) 3.14316)
(assert-equals '(pi-product product-rec 1000) 3.14316)
