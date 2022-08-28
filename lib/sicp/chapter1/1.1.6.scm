; 1.1.6 Conditional Expressions and Predicates

(define (abs1 x)
    (cond ((> x 0) x)
          ((= x 0) 0)
          ((< x 0) (- x))))

(assert-equal '(abs1 1) 1)
(assert-equal '(abs1 0) 0)
(assert-equal '(abs1 -1) 1)

(define (abs2 x)
    (cond ((< x 0) (- x))
          (else x)))

(assert-equal '(abs2 1) 1)
(assert-equal '(abs2 0) 0)
(assert-equal '(abs2 -1) 1)

(define (abs3 x)
    (if (< x 0)
        (- x)
        x))

(assert-equal '(abs3 1) 1)
(assert-equal '(abs3 0) 0)
(assert-equal '(abs3 -1) 1)

(define (>=1 x y) (or (> x y) (= x y)))

(assert-equal '(>=1 1 2) false)
(assert-equal '(>=1 1 1) true)
(assert-equal '(>=1 2 1) true)

(define (>=2 x y) (not (< x y)))

(assert-equal '(>=2 1 2) false)
(assert-equal '(>=2 1 1) true)
(assert-equal '(>=2 2 1) true)
