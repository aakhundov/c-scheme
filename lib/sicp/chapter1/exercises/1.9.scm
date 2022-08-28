; Exercise 1.9

(define (inc x) (+ x 1))
(define (dec x) (- x 1))

(define (plus1 a b)
    (if (= a 0) b (inc (+ (dec a) b))))

(define (plus2 a b)
    (if (= a 0) b (+ (dec a) (inc b))))

(assert-equal '(plus1 2 3) 5)
(assert-equal '(plus2 2 3) 5)
