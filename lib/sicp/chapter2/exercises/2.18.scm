; Exercise 2.18

(define (reverse-rec items)
    (if (null? items)
        '()
        (append (reverse-rec (cdr items))
                (list (car items)))))

(assert-equals '(reverse-rec '()) '())
(assert-equals '(reverse-rec '(1)) '(1))
(assert-equals '(reverse-rec '(1 2 1)) '(1 2 1))
(assert-equals '(reverse-rec '(1 1 1)) '(1 1 1))
(assert-equals '(reverse-rec '(1 2 3 4 5)) '(5 4 3 2 1))
