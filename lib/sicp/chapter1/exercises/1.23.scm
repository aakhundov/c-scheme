; Exercise 1.23

(define (smallest-divisor n) (find-divisor n 2))

(define (find-divisor n test-divisor)
    (cond ((> (square test-divisor) n) n)
          ((divides? test-divisor n) test-divisor)
          (else (find-divisor n (next test-divisor)))))

(define (divides? a b) (= (remainder b a) 0))

(define (next d)
    (if (= d 2)
        3
        (+ d 2)))

(assert-equal '(smallest-divisor 199) 199)
(assert-equal '(smallest-divisor 1999) 1999)
(assert-equal '(smallest-divisor 19999) 7)
