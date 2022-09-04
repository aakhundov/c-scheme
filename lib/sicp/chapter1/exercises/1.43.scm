; Exercise 1.43

(define (compose f g) (lambda (x) (f (g x))))

(define (repeated-rec f n)
    (if (= n 1)
        f
        (let ((g (repeated-rec f (- n 1))))
            (lambda (x) (f (g x))))))

(assert-equals '((repeated-rec square 2) 5) 625)
(assert-equals '((repeated-rec square 3) 2) 256)

(define (repeated-iter f n)
    (lambda (x)
        (define (iter result i)
            (if (= i 0)
                result
                (iter (f result) (- i 1))))
        (iter x n)))

(assert-equals '((repeated-iter square 2) 5) 625)
(assert-equals '((repeated-iter square 3) 2) 256)
