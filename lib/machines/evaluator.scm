start
    (assign continue (label end))
    (branch (label eval-dispatch) (op dispatch-table-ready?) (reg dispatch))

init-dispatch
    (assign dispatch (op make-dispatch-table))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "self") (label ev-self-eval))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "var") (label ev-variable))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "quote") (label ev-quoted))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "set!") (label ev-assignment))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "define") (label ev-definition))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "if") (label ev-if))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "lambda") (label ev-lambda))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "let") (label ev-let))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "begin") (label ev-begin))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "cond") (label ev-cond))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "and") (label ev-and))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "or") (label ev-or))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "eval") (label ev-eval))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "apply") (label ev-apply))
    (assign dispatch (op add-dispatch-record) (reg dispatch) (const "default") (label ev-application))

eval-dispatch
    (assign type (op dispatch-on-type) (reg exp) (reg dispatch))
    (goto (reg type))

ev-self-eval
    (assign val (reg exp))
    (goto (reg continue))

ev-variable
    (assign val (op lookup-variable-value) (reg exp) (reg env))
    (goto (reg continue))

ev-quoted
    (perform (op check-quoted) (reg exp))
    (assign val (op text-of-quotation) (reg exp))
    (goto (reg continue))

ev-assignment
    (perform (op check-assignment) (reg exp))
    (assign unev (op assignment-variable) (reg exp))
    (save unev)
    (assign exp (op assignment-value) (reg exp))
    (save env)
    (save continue)
    (assign continue (label ev-assignment-did-value))
    (goto (label eval-dispatch))
ev-assignment-did-value
    (restore continue)
    (restore env)
    (restore unev)
    (assign val (op set-variable-value!) (reg unev) (reg val) (reg env))
    (goto (reg continue))

ev-definition
    (perform (op check-definition) (reg exp))
    (assign unev (op definition-variable) (reg exp))
    (save unev)
    (assign exp (op definition-value) (reg exp))
    (save env)
    (save continue)
    (assign continue (label ev-definition-did-value))
    (goto (label eval-dispatch))
ev-definition-did-value
    (restore continue)
    (restore env)
    (restore unev)
    (assign val (op define-variable!) (reg unev) (reg val) (reg env))
    (goto (reg continue))

ev-if
    (perform (op check-if) (reg exp))
    (save exp)
    (save env)
    (save continue)
    (assign continue (label ev-if-decide))
    (assign exp (op if-predicate) (reg exp))
    (goto (label eval-dispatch))
ev-if-decide
    (restore continue)
    (restore env)
    (restore exp)
    (branch (label ev-if-consequent) (op true?) (reg val))
ev-if-alternative
    (assign exp (op if-alternative) (reg exp))
    (goto (label eval-dispatch))
ev-if-consequent
    (assign exp (op if-consequent) (reg exp))
    (goto (label eval-dispatch))

ev-lambda
    (perform (op check-lambda) (reg exp))
    (assign unev (op lambda-parameters) (reg exp))
    (assign exp (op lambda-body) (reg exp))
    (assign val (op make-compound-procedure) (reg unev) (reg exp) (reg env))
    (goto (reg continue))

ev-let
    (perform (op check-let) (reg exp))
    (assign exp (op transform-let) (reg exp))
    (goto (label eval-dispatch))

ev-begin
    (perform (op check-begin) (reg exp))
    (assign unev (op begin-actions) (reg exp))
    (save continue)
    (goto (label ev-sequence))

ev-cond
    (perform (op check-cond) (reg exp))
    (assign exp (op transform-cond) (reg exp))
    (goto (label eval-dispatch))

ev-and
    (perform (op check-and) (reg exp))
    (assign unev (op and-expressions) (reg exp))
    (branch (label ev-and-return-true) (op no-exps?) (reg unev))
    (save continue)
ev-and-before-eval
    (assign exp (op first-exp) (reg unev))
    (branch (label ev-and-last-exp) (op last-exp?) (reg unev))
    (save unev)
    (save env)
    (assign continue (label ev-and-after-eval))
    (goto (label eval-dispatch))
ev-and-after-eval
    (restore env)
    (restore unev)
    (branch (label ev-and-return-val) (op false?) (reg val))
    (assign unev (op rest-exps) (reg unev))
    (goto (label ev-and-before-eval))
ev-and-last-exp
    (restore continue)
    (goto (label eval-dispatch))
ev-and-return-val
    (restore continue)
    (goto (reg continue))
ev-and-return-true
    (assign val (op make-true))
    (goto (reg continue))

ev-or
    (perform (op check-or) (reg exp))
    (assign unev (op or-expressions) (reg exp))
    (branch (label ev-or-return-false) (op no-exps?) (reg unev))
    (save continue)
ev-or-before-eval
    (assign exp (op first-exp) (reg unev))
    (branch (label ev-or-last-exp) (op last-exp?) (reg unev))
    (save unev)
    (save env)
    (assign continue (label ev-or-after-eval))
    (goto (label eval-dispatch))
ev-or-after-eval
    (restore env)
    (restore unev)
    (branch (label ev-or-return-val) (op true?) (reg val))
    (assign unev (op rest-exps) (reg unev))
    (goto (label ev-or-before-eval))
ev-or-last-exp
    (restore continue)
    (goto (label eval-dispatch))
ev-or-return-val
    (restore continue)
    (goto (reg continue))
ev-or-return-false
    (assign val (op make-false))
    (goto (reg continue))

ev-eval
    (perform (op check-eval) (reg exp))
    (save continue)
    (assign exp (op eval-expression) (reg exp))
    (assign continue (label ev-eval-did-once))
    (goto (label eval-dispatch))
ev-eval-did-once
    (restore continue)
    (assign exp (reg val))
    (goto (label eval-dispatch))

ev-apply
    (perform (op check-apply) (reg exp))
    (save continue)
    (save env)
    (assign unev (op apply-arguments) (reg exp))
    (save unev)
    (assign exp (op apply-operator) (reg exp))
    (assign continue (label ev-apply-did-operator))
    (goto (label eval-dispatch))
ev-apply-did-operator
    (restore unev)
    (restore env)
    (assign proc (reg val))
    (save proc)
    (assign exp (reg unev))
    (assign continue (label ev-apply-did-arguments))
    (goto (label eval-dispatch))
ev-apply-did-arguments
    (assign argl (reg val))
    (restore proc)
    (perform (op check-apply-args) (reg argl))
    (goto (label apply-dispatch))

ev-application
    (perform (op check-application) (reg exp))
    (save continue)
    (save env)
    (assign unev (op operands) (reg exp))
    (save unev)
    (assign exp (op operator) (reg exp))
    (assign continue (label ev-application-did-operator))
    (goto (label eval-dispatch))
ev-application-did-operator
    (restore unev)
    (restore env)
    (assign argl (op make-empty-arglist))
    (assign proc (reg val))
    (branch (label apply-dispatch) (op no-operands?) (reg unev))
    (save proc)
ev-application-operand-loop
    (save argl)
    (assign exp (op first-operand) (reg unev))
    (branch (label ev-application-last-arg) (op last-operand?) (reg unev))
    (save env)
    (save unev)
    (assign continue (label ev-application-accumulate-arg))
    (goto (label eval-dispatch))
ev-application-accumulate-arg
    (restore unev)
    (restore env)
    (restore argl)
    (assign argl (op adjoin-arg) (reg val) (reg argl))
    (assign unev (op rest-operands) (reg unev))
    (goto (label ev-application-operand-loop))
ev-application-last-arg
    (assign continue (label ev-application-accumulate-last-arg))
    (goto (label eval-dispatch))
ev-application-accumulate-last-arg
    (restore argl)
    (assign argl (op adjoin-arg) (reg val) (reg argl))
    (restore proc)
    (goto (label apply-dispatch))

apply-dispatch
    (branch (label primitive-apply) (op primitive-procedure?) (reg proc))
    (branch (label compound-apply) (op compound-procedure?) (reg proc))
    (perform (op signal-error) (const "can't apply %s") (reg proc))
primitive-apply
    (assign val (op apply-primitive-procedure) (reg proc) (reg argl))
    (restore continue)
    (goto (reg continue))
compound-apply
    (assign unev (op procedure-parameters) (reg proc))
    (assign env (op procedure-environment) (reg proc))
    (assign env (op extend-environment) (reg unev) (reg argl) (reg env))
    (assign unev (op procedure-body) (reg proc))
    (goto (label ev-sequence))

ev-sequence
    (assign exp (op first-exp) (reg unev))
    (branch (label ev-sequence-last-exp) (op last-exp?) (reg unev))
    (save unev)
    (save env)
    (assign continue (label ev-sequence-continue))
    (goto (label eval-dispatch))
ev-sequence-continue
    (restore env)
    (restore unev)
    (assign unev (op rest-exps) (reg unev))
    (goto (label ev-sequence))
ev-sequence-last-exp
    (restore continue)
    (goto (label eval-dispatch))

end
