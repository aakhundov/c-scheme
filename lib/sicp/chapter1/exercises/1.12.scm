; Exercise 1.11

(define (pascal row col)
    (if (or (= col 1) (= col row))
        1
        (+ (pascal (- row 1) (- col 1))
           (pascal (- row 1) col))))

(assert-equal '(pascal 1 1) 1)

(assert-equal '(pascal 2 1) 1)
(assert-equal '(pascal 2 2) 1)

(assert-equal '(pascal 3 1) 1)
(assert-equal '(pascal 3 2) 2)
(assert-equal '(pascal 3 3) 1)

(assert-equal '(pascal 4 1) 1)
(assert-equal '(pascal 4 2) 3)
(assert-equal '(pascal 4 3) 3)
(assert-equal '(pascal 4 4) 1)

(assert-equal '(pascal 5 1) 1)
(assert-equal '(pascal 5 2) 4)
(assert-equal '(pascal 5 3) 6)
(assert-equal '(pascal 5 4) 4)
(assert-equal '(pascal 5 5) 1)

(assert-equal '(pascal 6 1) 1)
(assert-equal '(pascal 6 2) 5)
(assert-equal '(pascal 6 3) 10)
(assert-equal '(pascal 6 4) 10)
(assert-equal '(pascal 6 5) 5)
(assert-equal '(pascal 6 6) 1)
