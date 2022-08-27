; Exercise 1.4

(define (a-plus-abs-b a b)
  ((if (> b 0) + -) a b))

(assert-equal '(a-plus-abs-b 1 -1) 2)
(assert-equal '(a-plus-abs-b 1 1) 2)
(assert-equal '(a-plus-abs-b 1 0) 1)
(assert-equal '(a-plus-abs-b -1 -1) 0)
(assert-equal '(a-plus-abs-b -1 1) 0)
(assert-equal '(a-plus-abs-b -1 0) -1)
