; 1.2.5 Greatest Common Divisors

(define (gcd a b)
    (if (= b 0)
        a
        (gcd b (remainder a b))))

(assert-equals '(gcd 1 0) 1)
(assert-equals '(gcd 0 1) 1)
(assert-equals '(gcd 2 3) 1)
(assert-equals '(gcd 3 2) 1)
(assert-equals '(gcd 5 10) 5)
(assert-equals '(gcd 10 5) 5)
(assert-equals '(gcd 60 42) 6)

(assert-equals '(gcd 206 40) 2)
