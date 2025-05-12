# Lammm

Lammm (pronounced like "Lamb") is a little language based on the 𝜆𝜇𝜇~ calculus (hence the name).  
It's currently pretty much just a clone of the Core language from [this paper](https://arxiv.org/abs/2406.14719) with a lispy syntax.


## Installation

Requires CMake of version `>= 3.10.0`, C++ compiler with `C++20` support, and Doxygen for doc generation.  
No external libraries needed.

To install: use CMake

To run: `build/lammm`, then input your program (terminated by EOF) or `cat` one of the programs in `examples`  
Your program will then be parsed, typechecked, and run, with errors being reported and execution steps being displayed.

To run tests: `build/test`


## Writing Lammm programs

Each Lammm program consists of a sequence of *definitions* and *statements*, which may refer to any of the preceding *definitions*. The *statements* are executed (more on that later) in sequence, *definitions* are more or less like named functions.

To describe the grammar semi-formally:
```
<Program> : a whitespace-separated sequence of <Command>s 
<Command> := <Definition> | <Statement>
<Definition> := (def <Definition name> (<Parameters>) (<Coparameters>) <Statement>)
<Definition name> : more or less any sequence of letters (though the parser permits more)
<Parameters> : a whitespace-separated sequence of <Variable>s
<Coparameters> : a whitespace-separated sequence of <Covariable>s
<Variable> and <Covariable> : like <Definition name> 
<Statement> := <Arithmetic statement> | <If-zero statement> | <Cut statement> | <Call statement>
<Arithmetic statement> := (<Operator> <Producer> <Producer> <Consumer>)
<Operator> := + | - | * | / | %
<If-zero statement> := (ifz <Producer> <Statement> <Statement>)
<Cut statement> := [<Producer> <Consumer>]
<Call statement> := (<Definition name> (<Arguments>) (<Coarguments>))
<Arguments> : a whitespace-separated sequence of <Producer>s
<Coarguments> : a whitespace-separated sequence of <Consumer>s
<Producer> := <Variable producer> | <Value producer> | <Mu producer> | <Constructor producer> | <Cocase producer>
<Consumer> := <Covariable consumer> | <Mu' consumer> | <Destructor consumer> | <End consumer>
<Variable producer> := <Variable>
<Covariable consumer> := <Covariable>
<Mu producer> := (<Mu> <Covariable> <Statement>)
<Mu' consumer> := (<Mu'> <Variable> <Statement>)
<Constructor producer> : (Nil) | (Cons (<Producer> <Producer>)) | (Pair (<Producer> <Producer>))
<Destructor producer> : (Head (<Consumer>)) | (Tail (<Consumer>)) | (Fst (<Consumer>)) | (Snd (<Consumer>)) | (Ap (<Producer>) (<Consumer>))
<Cocase producer> := (<Destructor clauses>)
<Case producer> := (<Constructor clauses>)
<(Con/De)structor clauses> : a whitespace-separated sequence of clauses, which are like (con/de)structors, but with (co)variables instead of (co)arguments, followed by a body - a <Statement> containing uses of said (co)variables
<Value producer> : an integer literal 
<End consumer> : "<END>" (without the quotes)
```

To describe the different syntax elements briefly: 

Arithmetic statements perform arithmetic operations on integers.  
If-zero statements are for conditional branching (you can link the branches via covariables).  
Cut statements combine constructors with case expressions, cocase expressions with destructors, mu abstractions with consumers, values\* with mu' abstractions or the `<END>` expression. (more on *values* later)  
Call statements are for making use of definitions.

Mu producers and mu' consumers are simple abstractions, necessary to herd (co)values around.  
Constructors construct (algebraic) data, case expressions take them apart.  
Cocase expressions construct codata (a bit like objects with methods), destructors consume them (like destructively running a method).  
Value producers are integer literals.  
The `<END>` expression denotes end of computation.


### A note on "values"

Lammm follows Core from the paper in implementing call-by-value semantics, which necessitates that arguments (but not coarguments) of constructors, destructors and definition calls be *values* for computation to be possible. A *value* in this context is either the literal integer value producer, or a constructor with only value arguments.  
To avoid getting stuck due to computation being needed inside one of the parts of a cut statement, there are *focusing rules*
which twist the program a bit to allow computation to continue.


## Running Lammm programs

It follows closely the semantics of Core from the paper, but with focusing being done as-needed, in order to make the process less confusing.  

It is possible to write programs that loop infinitely however, so beware!


## Lammm types

Lammm contains a typechecker that should catch most semantically invalid programs. Types of definitions (and of course of (con/de)structors) are generalized, behaving as if all free variables were instead universally quantified. There's no way to add manual type annotation, but the type checker is able to infer the types of most things.

The only primitive type is `Integer`; the data types are `List` (with constructors `Nil` and `Cons`) and `Pair` (with constructor `Pair`); the codata types are (infinite) `Stream` (with destructors `Head` and `Tail`), `LazyPair` (with destructors `Fst` and `Snd`) and `Lambda` (with constructor `Ap`).


## Lammm internals

TBA. For now, just look at the code. (If you know something about implementing programming languages, you'll notice most things are named terribly. Sorry!)