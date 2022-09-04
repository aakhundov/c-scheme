; Exercise 2.21

(define (square-list1 items)
    (if (null? items)
        nil
        (cons (square (car items)) (square-list1 (cdr items)))))

(assert-equal '(square-list1 '(1 2 3 4 5)) '(1 4 9 16 25))

(define (square-list2 items)
    (map square items))

(assert-equal '(square-list2 '(1 2 3 4 5)) '(1 4 9 16 25))
