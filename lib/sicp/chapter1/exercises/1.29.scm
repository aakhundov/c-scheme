; Exercise 1.29

(define (sum term a next b)
    (if (> a b)
        0
        (+ (term a)
           (sum term (next a) next b))))

(define (simpson f a b n)
    (let ((h (/ (- b a) n)))
        (define (summand k)
            (let ((y (f (+ a (* k h)))))
                (cond ((or (= k 0) (= k n)) y)
                        ((even? k) (* 2 y))
                        (else (* 4 y)))))
        (* (/ h 3) (sum summand 0 inc n))))

(assert-equals '(simpson cube 0 1 100) 0.25)
(assert-equals '(simpson cube 0 1 1000) 0.25)
