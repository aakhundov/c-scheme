; Exercise 1.43

(define (compose f g) (lambda (x) (f (g x))))

(define (repeated-rec f n)
    (if (= n 1)
        f
        (let ((g (repeated-rec f (- n 1))))
            (lambda (x) (f (g x))))))

(assert-equal '((repeated-rec square 2) 5) 625)
(assert-equal '((repeated-rec square 3) 2) 256)

(define (repeated-iter f n)
    (lambda (x)
        (define (iter result i)
            (if (= i 0)
                result
                (iter (f result) (- i 1))))
        (iter x n)))

(assert-equal '((repeated-iter square 2) 5) 625)
(assert-equal '((repeated-iter square 3) 2) 256)
