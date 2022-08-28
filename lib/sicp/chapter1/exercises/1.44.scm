; Exercise 1.44

(define (smooth f dx)
    (lambda (x) (/ (+ (f (- x dx))
                      (f x)
                      (f (+ x dx)))
                   3)))

(assert-equal '((smooth square 0.001) 2) 4)
(assert-equal '((smooth cos 0.001) PI) -1)

(define (repeated f n)
    (lambda (x)
        (define (iter result i)
            (if (= i 0)
                result
                (iter (f result) (- i 1))))
        (iter x n)))

(define (repeated-smooth n f dx)
    ((repeated (lambda (f) (smooth f dx)) n) f))

(assert-equal '((repeated-smooth 3 square 0.001) 2) 4)
(assert-equal '((repeated-smooth 3 cos 0.001) PI) -1)
