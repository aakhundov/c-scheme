start
    (test (op =) (reg b) (const 0))
    (branch (label end))
    (assign t (op rem) (reg a) (reg b))
    (assign a (reg b))
    (assign b (reg t))
    (goto (label start))
end
