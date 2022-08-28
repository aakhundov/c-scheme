; Exercise 1.11

(define (f-rec n)
    (if (< n 3)
        n
        (+ (f-rec (- n 1))
           (* 2 (f-rec (- n 2)))
           (* 3 (f-rec (- n 3))))))

(assert-equal '(f-rec 0) 0)
(assert-equal '(f-rec 1) 1)
(assert-equal '(f-rec 2) 2)
(assert-equal '(f-rec 3) 4)
(assert-equal '(f-rec 4) 11)
(assert-equal '(f-rec 5) 25)
(assert-equal '(f-rec 6) 59)
(assert-equal '(f-rec 7) 142)
(assert-equal '(f-rec 8) 335)
(assert-equal '(f-rec 9) 796)
(assert-equal '(f-rec 10) 1892)

(define (f-iter n)
    (define (iter a b c count)
        (if (= count 0)
            a
            (iter b
                  c
                  (+ c (* 2 b) (* 3 a))
                  (- count 1))))
    (iter 0 1 2 n))

(assert-equal '(f-iter 0) 0)
(assert-equal '(f-iter 1) 1)
(assert-equal '(f-iter 2) 2)
(assert-equal '(f-iter 3) 4)
(assert-equal '(f-iter 4) 11)
(assert-equal '(f-iter 5) 25)
(assert-equal '(f-iter 6) 59)
(assert-equal '(f-iter 7) 142)
(assert-equal '(f-iter 8) 335)
(assert-equal '(f-iter 9) 796)
(assert-equal '(f-iter 10) 1892)
