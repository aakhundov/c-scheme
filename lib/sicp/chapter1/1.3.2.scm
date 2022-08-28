; 1.3.2 Constructing Procedures Using lambda

(define (sum term a next b)
    (define (iter a result)
        (if (> a b)
            result
            (iter (next a) (+ result (term a)))))
    (iter a 0))

(define (pi-sum a b)
    (sum (lambda (x) (/ 1.0 (* x (+ x 2))))
          a
          (lambda (x) (+ x 4))
          b))

(assert-equal '(* 8 (pi-sum 1 1000)) 3.139592655589783)

(define (integral f a b dx)
    (* (sum f
            (+ a (/ dx 2.0))
            (lambda (x) (+ x dx))
            b)
       dx))

(assert-equal '(integral cube 0 1 0.01) .24998750000000042)
(assert-equal '(integral cube 0 1 0.001) .249999875000001)

(assert-equal '
    ((lambda (x y z) (+ x y (square z)))
     1 2 3)
    12)

(define (f x y)
    (let ((a (+ 1 (* x y)))
          (b (- 1 y)))
        (+ (* x (square a))
           (* y b)
           (* a b))))

(assert-equal '(f 2 3) 78)
(assert-equal '(f 3 2) 138)

(define (f2 x)
    (+ (let ((x 3))
           (+ x (* x 10)))
     x)
)

(assert-equal '(f2 5) 38)

(define (f3 x)
    (let ((x 3)
          (y (+ x 2)))
        (* x y))
)

(assert-equal '(f3 2) 12)
