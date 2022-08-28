; 1.2.1 Linear Recursion and Iteration

(define (factorial1 n)
    (if (= n 1)
        1
        (* n (factorial1 (- n 1)))))

(assert-equal '(factorial1 1) 1)
(assert-equal '(factorial1 2) 2)
(assert-equal '(factorial1 3) 6)
(assert-equal '(factorial1 5) 120)
(assert-equal '(factorial1 10) 3628800)

(define (factorial2 n)
    (fact-iter 1 1 n))
(define (fact-iter product counter max-count)
    (if (> counter max-count)
        product
        (fact-iter (* counter product)
                   (+ counter 1)
                   max-count)))

(assert-equal '(factorial2 1) 1)
(assert-equal '(factorial2 2) 2)
(assert-equal '(factorial2 3) 6)
(assert-equal '(factorial2 5) 120)
(assert-equal '(factorial2 10) 3628800)
