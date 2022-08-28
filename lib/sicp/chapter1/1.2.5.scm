; 1.2.5 Greatest Common Divisors

(define (gcd a b)
    (if (= b 0)
        a
        (gcd b (remainder a b))))

(assert-equal '(gcd 1 0) 1)
(assert-equal '(gcd 0 1) 1)
(assert-equal '(gcd 2 3) 1)
(assert-equal '(gcd 3 2) 1)
(assert-equal '(gcd 5 10) 5)
(assert-equal '(gcd 10 5) 5)
(assert-equal '(gcd 60 42) 6)

(assert-equal '(gcd 206 40) 2)
