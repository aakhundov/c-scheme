; 1.2.2 Tree Recursion

(define (fib1 n)
    (cond ((= n 0) 0)
          ((= n 1) 1)
          (else (+ (fib1 (- n 1))
                   (fib1 (- n 2))))))

(assert-equal '(fib1 0) 0)
(assert-equal '(fib1 1) 1)
(assert-equal '(fib1 2) 1)
(assert-equal '(fib1 3) 2)
(assert-equal '(fib1 4) 3)
(assert-equal '(fib1 5) 5)
(assert-equal '(fib1 6) 8)
(assert-equal '(fib1 7) 13)
(assert-equal '(fib1 8) 21)

(define (fib2 n)
    (fib-iter 1 0 n))
(define (fib-iter a b count)
    (if (= count 0)
        b
        (fib-iter (+ a b) a (- count 1))))

(assert-equal '(fib2 0) 0)
(assert-equal '(fib2 1) 1)
(assert-equal '(fib2 2) 1)
(assert-equal '(fib2 3) 2)
(assert-equal '(fib2 4) 3)
(assert-equal '(fib2 5) 5)
(assert-equal '(fib2 6) 8)
(assert-equal '(fib2 7) 13)
(assert-equal '(fib2 8) 21)

(define (count-change amount) (cc amount 5))
(define (cc amount kinds-of-coins)
    (cond ((= amount 0) 1)
          ((or (< amount 0) (= kinds-of-coins 0)) 0)
          (else (+ (cc amount
                       (- kinds-of-coins 1))
                   (cc (- amount
                          (first-denomination
                           kinds-of-coins))
                       kinds-of-coins)))))
(define (first-denomination kinds-of-coins)
    (cond ((= kinds-of-coins 1) 1)
          ((= kinds-of-coins 2) 5)
          ((= kinds-of-coins 3) 10)
          ((= kinds-of-coins 4) 25)
          ((= kinds-of-coins 5) 50)))

(assert-equal '(count-change 100) 292)
