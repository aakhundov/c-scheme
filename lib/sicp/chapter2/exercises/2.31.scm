; Exercise 2.31

(define (tree-map f tree)
    (map (lambda (sub-tree)
             (if (pair? sub-tree)
                 (tree-map f sub-tree)
                 (f sub-tree)))
         tree))

(define (square-tree tree) (tree-map square tree))

(assert-equals '
    (square-tree (list 1 (list 2 (list 3 4) 5) (list 6 7)))
    '(1 (4 (9 16) 25) (36 49)))

(define (scale-tree tree factor)
    (tree-map (lambda (x) (* x factor)) tree))

(assert-equals '
    (scale-tree (list 1 (list 2 (list 3 4) 5) (list 6 7)) 10)
    '(10 (20 (30 40) 50) (60 70)))
