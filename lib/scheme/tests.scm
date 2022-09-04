; unit tests

(assert-equals '(caar '((1 2) 3 4)) 1)
(assert-equals '(cadr '((1 2) 3 4)) 3)
(assert-equals '(cdar '((1 2) 3 4)) '(2))
(assert-equals '(cddr '((1 2) 3 4)) '(4))

(assert-equals '(caaar '(((1 2) 3 4) (5 6) 7)) 1)
(assert-equals '(caadr '(((1 2) 3 4) (5 6) 7)) 5)
(assert-equals '(cadar '(((1 2) 3 4) (5 6) 7)) 3)
(assert-equals '(caddr '(((1 2) 3 4) (5 6) 7)) 7)
(assert-equals '(cdaar '(((1 2) 3 4) (5 6) 7)) '(2))
(assert-equals '(cdadr '(((1 2) 3 4) (5 6) 7)) '(6))
(assert-equals '(cddar '(((1 2) 3 4) (5 6) 7)) '(4))
(assert-equals '(cdddr '(((1 2) 3 4) (5 6) 7)) '())

(assert-equals '(first '(1 2 3 4 5 6 7 8 9 10)) 1)
(assert-equals '(second '(1 2 3 4 5 6 7 8 9 10)) 2)
(assert-equals '(third '(1 2 3 4 5 6 7 8 9 10)) 3)
(assert-equals '(fourth '(1 2 3 4 5 6 7 8 9 10)) 4)
(assert-equals '(fifth '(1 2 3 4 5 6 7 8 9 10)) 5)
(assert-equals '(sixth '(1 2 3 4 5 6 7 8 9 10)) 6)
(assert-equals '(seventh '(1 2 3 4 5 6 7 8 9 10)) 7)
(assert-equals '(eighth '(1 2 3 4 5 6 7 8 9 10)) 8)
(assert-equals '(ninth '(1 2 3 4 5 6 7 8 9 10)) 9)
(assert-equals '(tenth '(1 2 3 4 5 6 7 8 9 10)) 10)

(assert-equals '(square 0) 0)
(assert-equals '(square 1) 1)
(assert-equals '(square -1) 1)
(assert-equals '(square 2) 4)
(assert-equals '(square -2) 4)

(assert-equals '(sqrt 0) 0)
(assert-equals '(sqrt 1) 1)
(assert-equals '(sqrt 2) 1.41421)
(assert-equals '(sqrt 3) 1.73205)
(assert-equals '(sqrt 4) 2)

(assert-equals '(cube 0) 0)
(assert-equals '(cube 1) 1)
(assert-equals '(cube -1) -1)
(assert-equals '(cube 2) 8)
(assert-equals '(cube -2) -8)

(assert-equals '(inc 0) 1)
(assert-equals '(inc 1) 2)
(assert-equals '(inc -1) 0)
(assert-equals '(inc 2) 3)
(assert-equals '(inc -2) -1)

(assert-equals '(identity 0) 0)
(assert-equals '(identity 1) 1)
(assert-equals '(identity -1) -1)
(assert-equals '(identity 2) 2)
(assert-equals '(identity -2) -2)

(assert-equals '(average 1 1) 1)
(assert-equals '(average 1 2) 1.5)
(assert-equals '(average -1 1) 0)
(assert-equals '(average 2.71 3.14) 2.925)
(assert-equals '(average 3.14 2.71) 2.925)

(assert-equals '(positive? 1) true)
(assert-equals '(positive? 0) false)
(assert-equals '(positive? -1) false)

(assert-equals '(negative? 1) false)
(assert-equals '(negative? 0) false)
(assert-equals '(negative? -1) true)

(assert-equals '(gcd 1 0) 1)
(assert-equals '(gcd 0 1) 1)
(assert-equals '(gcd 2 3) 1)
(assert-equals '(gcd 3 2) 1)
(assert-equals '(gcd 5 10) 5)
(assert-equals '(gcd 10 5) 5)
(assert-equals '(gcd 63 78) 3)
(assert-equals '(gcd 111 222) 111)
(assert-equals '(gcd 1234 4321) 1)

(assert-equals '(list-ref '(1 2 3 4 5) 0) 1)
(assert-equals '(list-ref '(1 2 3 4 5) 4) 5)
(assert-equals '(list-ref '(1 2 3 4 5) 5) '())

(assert-equals '(map - '(1 2 3)) '(-1 -2 -3))
(assert-equals '(map car '((1 1) (2 2) (3 3))) '(1 2 3))
(assert-equals '(map cdr '((1 1) (2 2) (3 3))) '((1) (2) (3)))
(assert-equals '(map + '()) '())

(assert-equals '(length '()) 0)
(assert-equals '(length '(1)) 1)
(assert-equals '(length '(1 2 3)) 3)
(assert-equals '(length '(1 2 3 4 5 6 7 8 9 10)) 10)

(assert-equals '(append '(1) '(2)) '(1 2))
(assert-equals '(append '(1) '(2 3 4)) '(1 2 3 4))
(assert-equals '(append '(1 2 3) '(4)) '(1 2 3 4))
(assert-equals '(append '(1 2 3) '(4 5)) '(1 2 3 4 5))
(assert-equals '(append '(1 2) '(3 4 5)) '(1 2 3 4 5))
(assert-equals '(append '() '()) '())
(assert-equals '(append '() '(1)) '(1))
(assert-equals '(append '(1) '()) '(1))
(assert-equals '(append '() '(1 2 3)) '(1 2 3))
(assert-equals '(append '(1 2 3) '()) '(1 2 3))

(assert-equals '(reverse '()) '())
(assert-equals '(reverse '(1)) '(1))
(assert-equals '(reverse '(1 2 1)) '(1 2 1))
(assert-equals '(reverse '(1 1 1)) '(1 1 1))
(assert-equals '(reverse '(1 2 3 4 5)) '(5 4 3 2 1))
