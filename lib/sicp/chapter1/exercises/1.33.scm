; Exercise 1.33

(define (filtered-accumulate filter combiner null-value term a next b)
    (define (iter a result)
        (cond ((> a b) result)
              ((filter a) (iter (next a) (combiner result (term a))))
              (else (iter (next a) result))))
    (iter a null-value))


(define (smallest-divisor n) (find-divisor n 2))

(define (find-divisor n test-divisor)
    (cond ((> (square test-divisor) n) n)
          ((divides? test-divisor n) test-divisor)
          (else (find-divisor n (+ test-divisor 1)))))

(define (divides? a b) (= (remainder b a) 0))

(define (prime? n)
    (= n (smallest-divisor n)))

(define (sum-of-squared-primes a b)
    (filtered-accumulate prime? + 0 square a inc b))

(assert-equal '(sum-of-squared-primes 2 10) 87)
(assert-equal '(sum-of-squared-primes 10 50) 10379)
(assert-equal '(sum-of-squared-primes 2 100) 65796)


(define (sum-of-relatively-prime n)
    (define (rel-prime? x)
        (= (gcd x n) 1))
    (filtered-accumulate rel-prime? + 0 identity 1 inc (- n 1)))

(assert-equal '(sum-of-relatively-prime 10) 20)
(assert-equal '(sum-of-relatively-prime 97) 4656)
(assert-equal '(sum-of-relatively-prime 99) 2970)
(assert-equal '(sum-of-relatively-prime 100) 2000)
