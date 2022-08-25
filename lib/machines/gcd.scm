start
    (branch (label end) (op =) (reg b) (const 0))
    (assign t (op rem) (reg a) (reg b))
    (assign a (reg b))
    (assign b (reg t))
    (goto (label start))
end
