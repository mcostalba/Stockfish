#!/bin/sh

fen=$(cat $1)
echo $fen

$HOME/bin/stockfish << EOF
uci
setoption name Hash value 1024
isready
ucinewgame
setoption name UCI_AnalyseMode value true
position fen $fen
go infinite

EOF
