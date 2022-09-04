; 1.2.6 Example: Testing for Primality

(define (smallest-divisor n) (find-divisor n 2))

(define (find-divisor n test-divisor)
    (cond ((> (square test-divisor) n) n)
          ((divides? test-divisor n) test-divisor)
          (else (find-divisor n (+ test-divisor 1)))))

(define (divides? a b) (= (remainder b a) 0))

(define (prime? n)
    (= n (smallest-divisor n)))

(assert-equals '(prime? 2) true)
(assert-equals '(prime? 3) true)
(assert-equals '(prime? 4) false)
(assert-equals '(prime? 5) true)
(assert-equals '(prime? 6) false)
(assert-equals '(prime? 7) true)
(assert-equals '(prime? 8) false)
(assert-equals '(prime? 9) false)
(assert-equals '(prime? 10) false)
(assert-equals '(prime? 11) true)
(assert-equals '(prime? 13) true)
(assert-equals '(prime? 17) true)
(assert-equals '(prime? 19) true)
(assert-equals '(prime? 561) false)
(assert-equals '(prime? 1007) false)
(assert-equals '(prime? 1009) true)
(assert-equals '(prime? 10007) true)

; Carmichael numbers
(assert-equals '(prime? 561) false)
(assert-equals '(prime? 1105) false)
(assert-equals '(prime? 1729) false)

(define (expmod base exp m)
    (cond ((= exp 0) 1)
          ((even? exp)
           (remainder
            (square (expmod base (/ exp 2) m))
            m))
          (else
           (remainder
            (* base (expmod base (- exp 1) m))
            m))))

(define (fermat-test n)
    (define (try-it a)
        (= (expmod a n n) a))
    (try-it (+ 1 (random (- n 1)))))

(define (fast-prime? n times)
    (cond ((= times 0) true)
          ((fermat-test n) (fast-prime? n (- times 1)))
          (else false)))

(assert-equals '(fast-prime? 2 10) true)
(assert-equals '(fast-prime? 3 10) true)
(assert-equals '(fast-prime? 4 10) false)
(assert-equals '(fast-prime? 5 10) true)
(assert-equals '(fast-prime? 6 10) false)
(assert-equals '(fast-prime? 7 10) true)
(assert-equals '(fast-prime? 8 10) false)
(assert-equals '(fast-prime? 9 10) false)
(assert-equals '(fast-prime? 10 10) false)
(assert-equals '(fast-prime? 11 10) true)
(assert-equals '(fast-prime? 13 10) true)
(assert-equals '(fast-prime? 17 10) true)
(assert-equals '(fast-prime? 19 10) true)
(assert-equals '(fast-prime? 1007 10) false)
(assert-equals '(fast-prime? 1009 10) true)
(assert-equals '(fast-prime? 10007 10) true)

; Carmichael numbers fool the Fermat test
(assert-equals '(fast-prime? 561 100) true)
(assert-equals '(fast-prime? 1105 100) true)
(assert-equals '(fast-prime? 1729 100) true)
