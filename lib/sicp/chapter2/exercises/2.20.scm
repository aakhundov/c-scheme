; Exercise 2.20

(define (same-parity first . rest)
    (define (same-parity-rec items)
        (cond ((null? items) '())
              ((or (and (odd? first) (odd? (car items)))
                   (and (even? first) (even? (car items))))
               (cons (car items) (same-parity-rec (cdr items))))
              (else (same-parity-rec (cdr items)))))
    (append (list first) (same-parity-rec rest)))

(assert-equals '(same-parity 1 2 3 4 5 6 7) '(1 3 5 7))
(assert-equals '(same-parity 2 3 4 5 6 7) '(2 4 6))

(assert-equals '(same-parity 1) '(1))
(assert-equals '(same-parity 1 2) '(1))
(assert-equals '(same-parity 2 1) '(2))
(assert-equals '(same-parity 1 3) '(1 3))
(assert-equals '(same-parity 2 4) '(2 4))
(assert-equals '(same-parity 1 3 4) '(1 3))
(assert-equals '(same-parity 2 3 4) '(2 4))
