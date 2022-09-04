; Exercise 1.32

(define (accumulate-rec combiner null-value term a next b)
    (if (> a b)
        null-value
        (combiner (term a)
                  (accumulate-rec combiner null-value term (next a) next b))))

(define (accumulate-iter combiner null-value term a next b)
    (define (iter a result)
        (if (> a b)
            result
            (iter (next a) (combiner result (term a)))))
    (iter a null-value))

(define (sum-rec term a next b)
    (accumulate-rec + 0 term a next b))

(define (sum-iter term a next b)
    (accumulate-iter + 0 term a next b))

(define (product-rec term a next b)
    (accumulate-rec * 1 term a next b))

(define (product-iter term a next b)
    (accumulate-iter * 1 term a next b))

(define (sum-integers acc a b) (acc identity a inc b))

(assert-equals '(sum-integers sum-rec 1 10) 55)
(assert-equals '(sum-integers sum-iter 1 10) 55)

(define (fact acc n) (acc identity 1 inc n))

(assert-equals '(fact product-rec 1) 1)
(assert-equals '(fact product-rec 5) 120)
(assert-equals '(fact product-rec 10) 3628800)
(assert-equals '(fact product-iter 1) 1)
(assert-equals '(fact product-iter 5) 120)
(assert-equals '(fact product-iter 10) 3628800)
