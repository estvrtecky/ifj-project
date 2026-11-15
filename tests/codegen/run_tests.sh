#!/bin/sh
#Tento kod bol 15.11. vygenerovany Claude AI verziou Sonnet 5.5, na ziadost aby vygeneroval script na otestovanie codegen

cd "$(dirname "$0")" || exit 1
INT=${INT:-./ic26int-linux-x86_64}
mkdir -p out

gcc -Wall -Wextra -std=gnu11 -I. gen_runner.c codegen.c -o gen_runner || exit 1

pass=0; fail=0

# run <meno> <vstup> <ocakavany vystup>
run() {
  name=$1; input=$2; expected=$3
  printf '%b' "$expected" > "out/$name.exp"
  if ! ./gen_runner "$name" > "out/$name.ifjcode"; then
    echo "FAIL  $name (generator zlyhal)"; fail=$((fail+1)); return
  fi
  printf '%b' "$input" | $INT "out/$name.ifjcode" > "out/$name.act" 2> "out/$name.err"
  rc=$?
  if [ $rc -eq 0 ] && diff -q "out/$name.exp" "out/$name.act" > /dev/null; then
    echo "PASS  $name"; pass=$((pass+1))
  else
    echo "FAIL  $name (kod interpretu $rc)"
    cat "out/$name.err"
    diff "out/$name.exp" "out/$name.act" | head -5
    fail=$((fail+1))
  fi
}

# run_err <meno> <vstup> <ocakavany navratovy kod>
run_err() {
  name=$1; input=$2; expected_rc=$3
  if ! ./gen_runner "$name" > "out/$name.ifjcode"; then
    echo "FAIL  $name (generator zlyhal)"; fail=$((fail+1)); return
  fi
  printf '%b' "$input" | $INT "out/$name.ifjcode" > "out/$name.act" 2> "out/$name.err"
  rc=$?
  if [ $rc -eq $expected_rc ]; then
    echo "PASS  $name"; pass=$((pass+1))
  else
    echo "FAIL  $name (ocakavany kod $expected_rc, skutocny $rc)"
    cat "out/$name.err"
    fail=$((fail+1))
  fi
}

echo "== zaklad =="
run print_int   ''        '42'
run arith       ''        '7'
run print_str   ''        'a b#c\nd'
run neg         ''        '-5'
run cmp         ''        'truetruefalse'
run var         ''        '5'
run if_else     ''        '1'
run if_else2    ''        '2'
run while       ''        '012'
run while_def   ''        '024'
run readint     '4\n'     '5'
run readint_bad 'abc\n'   '0'
run strlen      ''        '3'
run ord         ''        '65 -1'
run call        ''        '7'
run chr         ''        'A'
run unit        ''        '()'
run tostr       ''        '42 2.50 ()'
run double      ''        '0x1.4p+1 3'
run strcat      ''        'IFJ26 true'
run readline    'ahoj\n'  'ahoj\n'
run readdouble  '3.5\n'   '3.50'
run shadow      ''        '20 10'
run params      ''        '7'
run fact        ''        '120'

echo "== subStr =="
run     substr     '' 'bcd '
run_err substr_neg '' 40
run_err substr_ord '' 40
run_err substr_oob '' 40

echo "== BOOLTHEN =="
run b_not        '' 'false'
run b_and        '' 'false'
run b_or         '' 'true'
run b_and_short  '' 'false'
run b_or_short   '' 'true'
run b_var        '' 'false'
run if_noelse    '' '12'
run if_noelse2   '' '2'
run else_if      '' '2'
run b_return     '' 'true'

echo "== FUNEXP =="
run nested_call  '' '9'
run nested_arg   '' '12'
run mutual_rec   '' 'true'
run fe_sum       '' '2'
run fe_two       '' '10'
run fe_expr      '' '9'
run fe_deep      '' '3'

echo "== prešlo $pass, zlyhalo $fail =="
[ $fail -eq 0 ]
