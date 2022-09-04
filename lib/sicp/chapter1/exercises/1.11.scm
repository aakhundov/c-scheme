; Exercise 1.11

(define (f-rec n)
    (if (< n 3)
        n
        (+ (f-rec (- n 1))
           (* 2 (f-rec (- n 2)))
           (* 3 (f-rec (- n 3))))))

(assert-equals '(f-rec 0) 0)
(assert-equals '(f-rec 1) 1)
(assert-equals '(f-rec 2) 2)
(assert-equals '(f-rec 3) 4)
(assert-equals '(f-rec 4) 11)
(assert-equals '(f-rec 5) 25)
(assert-equals '(f-rec 6) 59)
(assert-equals '(f-rec 7) 142)
(assert-equals '(f-rec 8) 335)
(assert-equals '(f-rec 9) 796)
(assert-equals '(f-rec 10) 1892)

(define (f-iter n)
    (define (iter a b c count)
        (if (= count 0)
            a
            (iter b
                  c
                  (+ c (* 2 b) (* 3 a))
                  (- count 1))))
    (iter 0 1 2 n))

(assert-equals '(f-iter 0) 0)
(assert-equals '(f-iter 1) 1)
(assert-equals '(f-iter 2) 2)
(assert-equals '(f-iter 3) 4)
(assert-equals '(f-iter 4) 11)
(assert-equals '(f-iter 5) 25)
(assert-equals '(f-iter 6) 59)
(assert-equals '(f-iter 7) 142)
(assert-equals '(f-iter 8) 335)
(assert-equals '(f-iter 9) 796)
(assert-equals '(f-iter 10) 1892)
