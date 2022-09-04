; Exercise 1.37

(define (cont-frac-iter n d k)
    (define (iter result i)
        (if (= i 0)
            result
            (iter (/ (n i) (+ (d i) result))
                  (- i 1))))
    (iter 0 k))

(assert-equals '
    (cont-frac-iter (lambda (i) 1.0)
                    (lambda (i) 1.0)
                    10)
    0.61803)

(define (cont-frac-rec n d k)
    (define (rec i)
        (if (= i k)
            0
            (/ (n i) (+ (d i) (rec (+ i 1))))))
    (rec 0))

(assert-equals '
    (cont-frac-rec (lambda (i) 1.0)
                   (lambda (i) 1.0)
                   10)
    0.61803)
