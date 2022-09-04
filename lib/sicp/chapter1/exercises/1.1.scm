; Exercise 1.1

(assert-equals '10 10)
(assert-equals '(+ 5 3 4) 12)
(assert-equals '(- 9 1) 8)
(assert-equals '(/ 6 2) 3)
(assert-equals '(+ (* 2 4) (- 4 6)) 6)

(define a 3)
(define b (+ a 1))

(assert-equals '(+ a b (* a b)) 19)
(assert-equals '(= a b) false)

(assert-equals '
    (if (and (> b a) (< b (* a b)))
        b
        a)
    4)

(assert-equals '
    (cond ((= a 4) 6)
          ((= b 4) (+ 6 7 a))
          (else 25))
    16)

(assert-equals '
    (+ 2 (if (> b a) b a))
    6)

(assert-equals '
    (* (cond ((> a b) a)
           ((< a b) b)
           (else -1))
       (+ a 1))
    16)
