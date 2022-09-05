; Exercise 2.30

(define (square-tree1 tree)
    (cond ((null? tree) nil)
          ((not (pair? tree)) (square tree))
          (else (cons (square-tree1 (car tree))
                      (square-tree1 (cdr tree))))))

(define (square-tree2 tree)
    (map (lambda (sub-tree)
             (if (pair? sub-tree)
                 (square-tree2 sub-tree)
                 (square sub-tree)))
         tree))

(assert-equals '
    (square-tree1
        (list 1
            (list 2 (list 3 4) 5)
            (list 6 7)))
    '(1 (4 (9 16) 25) (36 49))
)

(assert-equals '
    (square-tree2
        (list 1
            (list 2 (list 3 4) 5)
            (list 6 7)))
    '(1 (4 (9 16) 25) (36 49))
)
