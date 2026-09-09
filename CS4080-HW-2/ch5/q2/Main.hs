module Main where

-- A record containing one function for each operation.
-- The functions act as an interface for the expression.
data Expr = Expr
    { evaluate  :: () -> Double
    , printExpr :: () -> String
    }

-- Type 1: Number Literal
-- The closures capture the 'value' state, encapsulating it entirely.
number :: Double -> Expr
number value = Expr
    { evaluate  = \() -> value
    , printExpr = \() -> show value
    }

-- Type 2: Addition Expression
-- The closures capture 'left' and 'right', acting as the internal state.
addition :: Expr -> Expr -> Expr
addition left right = Expr
    { evaluate  = \() -> evaluate left () + evaluate right ()
    , printExpr = \() -> "(" ++ printExpr left () ++ " + " ++ printExpr right () ++ ")"
    }

-- Type 3: Negation Expression (Added without modifying existing code)
negation :: Expr -> Expr
negation expr = Expr
    { evaluate  = \() -> -(evaluate expr ())
    , printExpr = \() -> "(-" ++ printExpr expr () ++ ")"
    }

main :: IO ()
main = do
    let expr = negation (addition (number 3.0) (number 4.5))
    putStrLn $ "Expression: " ++ printExpr expr ()
    putStrLn $ "Result: "     ++ show (evaluate expr ())