(define (assert-equal exp result)
  (if (equal? (eval exp) result)
      (info "ok")
      (error "%s != %s" exp result)))
