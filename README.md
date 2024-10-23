# MLLanguageCoversion

# syntax for the ML language

program:
           ( program-item )*

program-item:
           statement
        |  function identifier ( identifier )*
           ←–tab–→ statement1
           ←–tab–→ statement2
           ....

statement:
           identifier "<-" expression
        |  print  expression
        |  return expression
        |  functioncall

expression:
          term   [ ("+" | "-")  expression ]

term:
          factor [ ("*" | "/")  term ]

factor:
          realconstant
        | identifier
        | functioncall
        | "(" expression ")"

functioncall:
          identifier "(" [ expression ( "," expression )* ] ")"
