; Exercise 2.20

(define (same-parity first . rest)
    (define (same-parity-rec items)
        (cond ((null? items) '())
              ((or (and (odd? first) (odd? (car items)))
                   (and (even? first) (even? (car items))))
               (cons (car items) (same-parity-rec (cdr items))))
              (else (same-parity-rec (cdr items)))))
    (append (list first) (same-parity-rec rest)))

(assert-equal '(same-parity 1 2 3 4 5 6 7) '(1 3 5 7))
(assert-equal '(same-parity 2 3 4 5 6 7) '(2 4 6))

(assert-equal '(same-parity 1) '(1))
(assert-equal '(same-parity 1 2) '(1))
(assert-equal '(same-parity 2 1) '(2))
(assert-equal '(same-parity 1 3) '(1 3))
(assert-equal '(same-parity 2 4) '(2 4))
(assert-equal '(same-parity 1 3 4) '(1 3))
(assert-equal '(same-parity 2 3 4) '(2 4))
