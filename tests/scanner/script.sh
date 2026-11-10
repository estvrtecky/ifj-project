#!/bin/bash
# (skupina "anotacia" a "ciarka" predpokladaju tokeny AT a COMMA - pozri driver skener.c)
# Testovacia sada pre lexikalny analyzator (Scala podmnozina, odsadenie, vnorene komentare).
# Tento cely subor bol vygenerovany pomocou AI (Claude verzia Sonnet 5.5) na ziadost o vygenerovanie testovacej sady pre scanner
#
# Pouzitie: bash test_scanner.sh [skupina]
#   bez argumentu bezia vsetky skupiny, inak iba zadana:
#   komentare zatvorky odsadenie div mlkod koment indent import ciarka anotacia znaky
#   operatory cisla retazce klucove program biele pamat riadky vlastnictvo zadanie
#   (skupiny "riadky" a "vlastnictvo" vyzaduju token.line a driver s argumentom --lines)
#
# Ocakavania:
#   - t  : porovnava IBA stdout driveru (skener.c). Ak ocakavany vystup konci na ERROR, driver musi
#          skoncit s kodom 1, inak s kodom 0.
#   - te : lexikalna chyba (kod 1) a stderr obsahuje dany text
#   - kod 77 = nahlasila chybu pamate sanitizer (ASan/UBSan) - double free, pretecenie pola, ...

ONLY="$1"
CFLAGS="-Wall -Wextra -std=c11 -g"
# riadkove testy bezia iba ak scanner.h obsahuje token_t.line
LINES_OK=0
if grep -Eq '^[[:space:]]*int[[:space:]]+line;' scanner.h 2>/dev/null; then
    LINES_OK=1
    CFLAGS="$CFLAGS -DHAVE_TOKEN_LINE"
fi
SAN="-fsanitize=address,undefined -fno-omit-frame-pointer"

if gcc $CFLAGS $SAN -o scanner_test skener.c scanner.c 2>build.log; then
    [ -s build.log ] && cat build.log
    export ASAN_OPTIONS=detect_leaks=0:exitcode=77
    export UBSAN_OPTIONS=halt_on_error=1:exitcode=77
else
    echo "(sanitizery nie su dostupne, kompilujem bez nich - chyby pamate sa nemusia prejavit)"
    if ! gcc $CFLAGS -o scanner_test skener.c scanner.c; then
        echo "KOMPILACIA ZLYHALA - oprav chyby vyssie, testy sa nespustili"
        exit 1
    fi
fi
rm -f build.log

# macOS nema timeout (je v coreutils ako gtimeout)
if command -v timeout >/dev/null 2>&1; then TO="timeout 2"
elif command -v gtimeout >/dev/null 2>&1; then TO="gtimeout 2"
else TO=""; fi

pass=0; fail=0; SKIP=0

section() {  # section skupina "popis"
    if [ -n "$ONLY" ] && [ "$ONLY" != "$1" ]; then
        SKIP=1
    elif [ "$LINES_OK" -eq 0 ] && { [ "$1" = riadky ] || [ "$1" = vlastnictvo ]; }; then
        SKIP=1
        echo
        echo "=== $2 === PRESKOCENE (scanner.h nema token_t.line)"
    else
        SKIP=0
        echo
        echo "=== $2 ==="
    fi
}

t() {  # t "nazov" 'vstup (printf %b)' 'ocakavany stdout'
    [ "$SKIP" -eq 1 ] && return
    local got rc exp_rc=0
    [[ "$3" == *ERROR ]] && exp_rc=1
    got=$(printf '%b' "$2" | $TO ./scanner_test 2>/dev/null)
    rc=$?
    if [ "$got" = "$3" ] && [ "$rc" -eq "$exp_rc" ]; then
        pass=$((pass+1)); echo "PASS  $1"
    else
        fail=$((fail+1)); echo "FAIL  $1"
        local shown_exp="$3" shown_got="$got"
        [ "${#shown_exp}" -gt 300 ] && shown_exp="${shown_exp:0:300} ...(skratene)"
        [ "${#shown_got}" -gt 300 ] && shown_got="${shown_got:0:300} ...(skratene)"
        echo "   ocakavane: $shown_exp   (kod $exp_rc)"
        echo "   dostal:    $shown_got   (kod $rc)"
        [ "$rc" -eq 124 ] && echo "   (timeout: scanner sa zacyklil)"
        if [ "$rc" -eq 77 ]; then
            echo "   (chyba pamate / UB:)"
            printf '%b' "$2" | $TO ./scanner_test 2>&1 >/dev/null | grep -m3 -E 'ERROR: |runtime error|SUMMARY' | sed 's/^/     /'
        elif [ "$rc" -ne 0 ] && [ "$rc" -ne 1 ] && [ "$rc" -ne 124 ]; then
            echo "   (program skoncil s kodom $rc)"
        fi
    fi
}

te() {  # te "nazov" 'vstup' 'text, ktory musi byt na stderr'
    [ "$SKIP" -eq 1 ] && return
    local err rc
    err=$(printf '%b' "$2" | $TO ./scanner_test 2>&1 >/dev/null)
    rc=$?
    if [ "$rc" -eq 1 ] && [[ "$err" == *"$3"* ]]; then
        pass=$((pass+1)); echo "PASS  $1"
    else
        fail=$((fail+1)); echo "FAIL  $1"
        echo "   ocakavane: kod 1 a stderr obsahuje: $3"
        echo "   dostal:    kod $rc, stderr: $err"
    fi
}

tl() {  # tl "nazov" 'vstup' 'ocakavany stdout s @riadok (driver --lines)'
    [ "$SKIP" -eq 1 ] && return
    local got rc
    got=$(printf '%b' "$2" | $TO ./scanner_test --lines 2>/dev/null)
    rc=$?
    if [ "$got" = "$3" ] && [ "$rc" -eq 0 ]; then
        pass=$((pass+1)); echo "PASS  $1"
    else
        fail=$((fail+1)); echo "FAIL  $1"
        echo "   ocakavane: $3"
        echo "   dostal:    $got   (kod $rc)"
        [ "$rc" -eq 77 ] && echo "   (chyba pamate: double free / UB)"
    fi
}

tleak() {  # tleak "nazov" 'vstup' - bez uniku pamate a bez chyby
    [ "$SKIP" -eq 1 ] && return
    local rc
    printf '%b' "$2" | ASAN_OPTIONS=detect_leaks=1:exitcode=77 $TO ./scanner_test --lines >/dev/null 2>&1
    rc=$?
    if [ "$rc" -eq 0 ]; then pass=$((pass+1)); echo "PASS  $1"
    else fail=$((fail+1)); echo "FAIL  $1  (kod $rc; 77 = unik/double free)"; fi
}

H='def f(): Unit =\n'
HX='DEF ID(f) L_PAREN R_PAREN COLON TYPE_UNIT ASSIGN EOL'
V='VAL ID(x) ASSIGN INT(1) EOL EOF'

# ---------------------------------------------------------------------------
section komentare "viacriadkove (vnorene) komentare"
t "vnoreny komentar"            '/* a /* b */ c */\n'                      'EOF'
t "komentar medzi kodom"        'val a = 1 /* x /* y */ z */ + 2\n'        'VAL ID(a) ASSIGN INT(1) ADD INT(2) EOL EOF'
t "doc komentar /** **/"        '/** doc **/\nval x = 1\n'                 "$V"
t "/**/ prazdny komentar"       '/**/\nval x = 1\n'                        "$V"
t "/***/"                       '/***/\nval x = 1\n'                       "$V"
t "/*/ nezatvara komentar"      '/*/ x */\nval x = 1\n'                    "$V"
t "viacriadkovy vnoreny"        '/* a\n/* b\n*/\nc */\nval x = 1\n'        "$V"
t "// obsahuje /*"              '// /* nie je blokovy\nval x = 1\n'        "$V"
t "// na konci riadka"          'val x = 1 // koment\n'                    "$V"
t "// na konci bez noveho riadka" 'val x = 1 // koment'                    "$V"
t "iba // koment"               '// c\n'                                   'EOF'
t "iba // koment bez \\n"       '// c'                                     'EOF'
t "iba /* */ koment"            '/* c */'                                  'EOF'
t "prazdny vstup"               ''                                         'EOF'
t "iba prazdne riadky"          '\n\n\n'                                   'EOF'
t "iba medzery"                 '   '                                      'EOF'
t "/* v retazci"                'val s = "a /* b"\n'                       'VAL ID(s) ASSIGN STRING("a /* b") EOL EOF'
t "// v retazci"                'val s = "a // b"\n'                       'VAL ID(s) ASSIGN STRING("a // b") EOL EOF'
t "delenie + komentar"          '10 / 2 /* c */\n'                         'INT(10) DIV INT(2) EOL EOF'
t "dva komentare za sebou"      'val x = 1 /* a */ /* b */\n'              "$V"
t "** / v komentari"            'val x = /* a **/ 1\n'                     "$V"
t "// vo vnutri /* */"          'val x = /* a // b */ 1\n'                 "$V"
t "uvodzovka v komentari"       'val x /* " */ = 1\n'                      "$V"
t "uvodzovka v // komentari"    'val x = 1 // "nezatvoreny\n'              "$V"
t "ciarka a @ v komentari"      'val x = 1 // a, b @ c\n'                  "$V"
t "ciarka a @ v /* */"          'val x = /* a, @ # $ */ 1\n'               'VAL ID(x) ASSIGN INT(1) EOL EOF'
t "stray */ je MUL DIV"         '*/\n'                                     'MUL DIV EOL EOF'
t "/*\\n*/ potom kod"           '/*\n*/\nval x = 1\n'                      "$V"
t "nezatvoreny vnoreny"         '/* a /* b */ c\n'                         'ERROR'
t "vonkajsi po vnorenom"        '/* /* *//\n'                              'ERROR'
t "iba /*"                      '/*'                                       'ERROR'
t "/*/ bez konca"               '/*/\n'                                    'ERROR'
t "nezatvoreny po kode"         'val x = 1 /* a\n'                         'VAL ID(x) ASSIGN INT(1) ERROR'

# ---------------------------------------------------------------------------
section zatvorky "zatvorky, medzery a nove riadky"
t "novy riadok v ( )"           'foo(\n    1 +\n\n    2\n)\nval x = 3\n'   'ID(foo) L_PAREN INT(1) ADD INT(2) R_PAREN EOL VAL ID(x) ASSIGN INT(3) EOL EOF'
t "vnorene ( )"                 'f((\n1\n)\n+\n2)\n'                       'ID(f) L_PAREN L_PAREN INT(1) R_PAREN ADD INT(2) R_PAREN EOL EOF'
t "jednoriadkovy komentar v ( )" 'f(1 /* a */ + // x\n 2)\n'               'ID(f) L_PAREN INT(1) ADD INT(2) R_PAREN EOL EOF'
t "bez ( ) je EOL"              'a +\nb\n'                                 'ID(a) ADD EOL ID(b) EOL EOF'
t "odsadenie v ( ) ignorovane"  "${H}    foo(\n  1\n)\n    val x = 1\n"   "$HX INDENT ID(foo) L_PAREN INT(1) R_PAREN EOL VAL ID(x) ASSIGN INT(1) EOL DEDENT EOF"
t "nezatvorena ( ) na EOF"      'f(1\n'                                    'ID(f) L_PAREN INT(1) EOL EOF'
t "vyraz v zatvorkach"          'val x = (1 + 2) * 3\n'                    'VAL ID(x) ASSIGN L_PAREN INT(1) ADD INT(2) R_PAREN MUL INT(3) EOL EOF'
t "prazdne ( )"                 'f()\n'                                    'ID(f) L_PAREN R_PAREN EOL EOF'
t "retazec s ) v ( )"           'f(")")\n'                                 'ID(f) L_PAREN STRING(")") R_PAREN EOL EOF'
t "nadbytocna ) nerozbije EOL"  'a)\nb\n'                                  'ID(a) R_PAREN EOL ID(b) EOL EOF'
t "( ) po nadbytocnej )"        'a)\nf(\n1)\n'                             'ID(a) R_PAREN EOL ID(f) L_PAREN INT(1) R_PAREN EOL EOF'

# ---------------------------------------------------------------------------
section odsadenie "komentare a odsadenie"
t "koment v stlpci 0 v bloku"   "${H}    val x = 1\n// c\n    val y = 2\n" "$HX INDENT VAL ID(x) ASSIGN INT(1) EOL VAL ID(y) ASSIGN INT(2) EOL DEDENT EOF"
t "odsadeny viacr. komentar"    "${H}    val x = 1\n        /* a\n        b */\n    val y = 2\n" "$HX INDENT VAL ID(x) ASSIGN INT(1) EOL VAL ID(y) ASSIGN INT(2) EOL DEDENT EOF"
t "komentar pred kodom"         "${H}    // c\n    val x = 1\n"            "$HX INDENT VAL ID(x) ASSIGN INT(1) EOL DEDENT EOF"
t "// po kode, dalsi riadok odsadeny" 'a // c\n  b\n'                     'ID(a) EOL INDENT ID(b) EOL DEDENT EOF'

# ---------------------------------------------------------------------------
section div "/abc na zaciatku riadka"
t "/abc bez odsadenia"          '/abc\n'                                   'DIV ID(abc) EOL EOF'
t "/abc dedent"                 "${H}    val x = 1\n/abc\n"                "$HX INDENT VAL ID(x) ASSIGN INT(1) EOL DEDENT DIV ID(abc) EOL EOF"
t "/abc rovnake odsadenie"      "${H}    val x = 1\n    /abc\n"            "$HX INDENT VAL ID(x) ASSIGN INT(1) EOL DIV ID(abc) EOL DEDENT EOF"
t "/abc indent"                 'a\n  /b\n'                               'ID(a) EOL INDENT DIV ID(b) EOL DEDENT EOF'
t "iba /"                       '/'                                        'DIV EOL EOF'
t "a/b bez medzier"             'a/b\n'                                    'ID(a) DIV ID(b) EOL EOF'

# ---------------------------------------------------------------------------
section koment "komentare za kodom v bloku"
t "komentar za kodom"           "${H}    val x = 1 // komentar tu\n    val y = 2\n" "$HX INDENT VAL ID(x) ASSIGN INT(1) EOL VAL ID(y) ASSIGN INT(2) EOL DEDENT EOF"
t "blokovy koment za kodom"     "${H}    val x = 1 /* blokovy */\n    val y = 2\n" "$HX INDENT VAL ID(x) ASSIGN INT(1) EOL VAL ID(y) ASSIGN INT(2) EOL DEDENT EOF"
t "kod pokracuje za komentom"   "${H}    val x = /* skryte */ 1\n    val y = 2\n" "$HX INDENT VAL ID(x) ASSIGN INT(1) EOL VAL ID(y) ASSIGN INT(2) EOL DEDENT EOF"
t "komentar na zaciatku s medzerami"  "    // komentar na riadku 1\nval x = 1\n" "VAL ID(x) ASSIGN INT(1) EOL EOF"
t "viacriadkovy koment na zaciatku"  "        /* prvy riadok\n        druhy riadok */\nval x = 1\n" "VAL ID(x) ASSIGN INT(1) EOL EOF"

# ---------------------------------------------------------------------------
section indent "odsadenie: INDENT / DEDENT / chyby"
t "jeden indent"                'a\n  b\n'                                'ID(a) EOL INDENT ID(b) EOL DEDENT EOF'
t "viac urovni nahor"           'a\n  b\n    c\n      d\n'                'ID(a) EOL INDENT ID(b) EOL INDENT ID(c) EOL INDENT ID(d) EOL DEDENT DEDENT DEDENT EOF'
t "rovnaka uroven bez tokenu"   'a\n  b\n  c\n  d\n'                      'ID(a) EOL INDENT ID(b) EOL ID(c) EOL ID(d) EOL DEDENT EOF'
t "dedent o jednu uroven"       'a\n    b\n        c\n    d\n'            'ID(a) EOL INDENT ID(b) EOL INDENT ID(c) EOL DEDENT ID(d) EOL DEDENT EOF'
t "dedent na strednu uroven"    'a\n  b\n    c\n  d\n'                    'ID(a) EOL INDENT ID(b) EOL INDENT ID(c) EOL DEDENT ID(d) EOL DEDENT EOF'
t "dedent o 3 urovne naraz"     'a\n  b\n    c\n      d\ne\n'            'ID(a) EOL INDENT ID(b) EOL INDENT ID(c) EOL INDENT ID(d) EOL DEDENT DEDENT DEDENT ID(e) EOL EOF'
t "dedent a znova indent"       'a\n  b\nc\n  d\n'                       'ID(a) EOL INDENT ID(b) EOL DEDENT ID(c) EOL INDENT ID(d) EOL DEDENT EOF'
t "indent po dedente"           'a\n  b\n    c\n  d\n    e\n'            'ID(a) EOL INDENT ID(b) EOL INDENT ID(c) EOL DEDENT ID(d) EOL INDENT ID(e) EOL DEDENT DEDENT EOF'
t "indent 3 nad uroven 2"       'a\n  b\n   c\n'                         'ID(a) EOL INDENT ID(b) EOL INDENT ID(c) EOL DEDENT DEDENT EOF'
t "dedenty na konci suboru"     'a\n  b'                                 'ID(a) EOL INDENT ID(b) EOL DEDENT EOF'
t "2 dedenty na konci bez \\n"  'a\n  b\n    c'                          'ID(a) EOL INDENT ID(b) EOL INDENT ID(c) EOL DEDENT DEDENT EOF'
t "prazdny riadok"              'a\n  b\n\n  c\n'                       'ID(a) EOL INDENT ID(b) EOL ID(c) EOL DEDENT EOF'
t "viac prazdnych riadkov"      'a\n  b\n\n\n\n  c\n'                   'ID(a) EOL INDENT ID(b) EOL ID(c) EOL DEDENT EOF'
t "riadok iba z medzier"        'a\n  b\n        \n  c\n'                'ID(a) EOL INDENT ID(b) EOL ID(c) EOL DEDENT EOF'
t "prazdny riadok pred dedentom" 'a\n  b\n\nc\n'                         'ID(a) EOL INDENT ID(b) EOL DEDENT ID(c) EOL EOF'
t "komentar hlbsie odsadeny"    'a\n  b\n      // c\n  d\n'               'ID(a) EOL INDENT ID(b) EOL ID(d) EOL DEDENT EOF'
t "komentar plytsie odsadeny"   'a\n    b\n  // c\n    d\n'               'ID(a) EOL INDENT ID(b) EOL ID(d) EOL DEDENT EOF'
t "viacr. komentar inak odsadeny" 'a\n    b\n  /* x\n   y */\n    c\n'    'ID(a) EOL INDENT ID(b) EOL ID(c) EOL DEDENT EOF'
t "trailing medzery za kodom"   'a   \n  b   \n'                          'ID(a) EOL INDENT ID(b) EOL DEDENT EOF'

te "chyba: 3 pri stacku 0,2,4"  'a\n  b\n    c\n   d\n'                   'Inconsistent indentation'
te "chyba: 6 pri stacku 0,4,8"  'a\n    b\n        c\n      d\n'          'Inconsistent indentation'
te "chyba: 2 pri stacku 0,4"    'a\n    b\n  c\n'                         'Inconsistent indentation'
te "chyba: 1 pri stacku 0,2"    'a\n  b\n c\n'                            'Inconsistent indentation'
t  "chyba odsadenia vypise ERROR" 'a\n  b\n c\n'                          'ID(a) EOL INDENT ID(b) EOL ERROR'
t  "chyba odsadenia pred /"     'a\n    b\n  /c\n'                       'ID(a) EOL INDENT ID(b) EOL ERROR'

# ---------------------------------------------------------------------------
section mlkod "kod za viacriadkovym komentarom (chyba, ak kod nasleduje za */ na tom istom riadku)"
t "kod za */ na dalsom riadku"  'val x = 1 /* a\nb */\nval y = 2\n'       'VAL ID(x) ASSIGN INT(1) EOL VAL ID(y) ASSIGN INT(2) EOL EOF'
t "medzery za */ a novy riadok" 'val x = 1 /* a\nb */   \nval y = 2\n'    'VAL ID(x) ASSIGN INT(1) EOL VAL ID(y) ASSIGN INT(2) EOL EOF'
t "*/ tesne pred EOF"           'val x = 1 /* a\nb */'                     'VAL ID(x) ASSIGN INT(1) EOL EOF'
t "v bloku: */ a dalsi riadok"  "${H}    val x = 1 /* a\n b */\n    val y = 2\n" "$HX INDENT VAL ID(x) ASSIGN INT(1) EOL VAL ID(y) ASSIGN INT(2) EOL DEDENT EOF"

t "kod za */ cez 2 riadky"      'val x = /* a\nb */ 1\n'                  'VAL ID(x) ASSIGN ERROR'
t "kod za */ po kode"           'val x = 1 /* a\nb */ val y = 2\n'         'VAL ID(x) ASSIGN INT(1) ERROR'
t "kod za */ na zaciatku"       '/* a\nb */ val x = 1\n'                   'ERROR'
t "kod za */ hned po /*\\n"     '/*\n*/ x\n'                               'ERROR'
t "vnoreny cez riadky + kod"    '/* a /* b\nc */ d */ x\n'                 'ERROR'
t "v bloku: kod za */"          "${H}    val x = /* a\n b */ 1\n"          "$HX INDENT VAL ID(x) ASSIGN ERROR"
t "viacriadkovy koment v kode v bloku" "${H}    val x = /* riadok1\n    riadok2 */ 1\n    val y = 2\n" "$HX INDENT VAL ID(x) ASSIGN ERROR"
t "viacr. koment v ( ) + kod"   'f(1 /* a\nb */ + // x\n 2)\n'             'ID(f) L_PAREN INT(1) ERROR'

t "jednoriadkovy = medzera"     'val x = /* a */ 1\n'                      'VAL ID(x) ASSIGN INT(1) EOL EOF'
t "jednoriadkovy vnoreny"       'val x = /* a /* b */ c */ 1\n'            'VAL ID(x) ASSIGN INT(1) EOL EOF'
t "viac komentarov v riadku"    'val /* a */ x /* b */ = /* c */ 1\n'      'VAL ID(x) ASSIGN INT(1) EOL EOF'
t "koment oddeluje tokeny"      'val/* a */x = 1\n'                        'VAL ID(x) ASSIGN INT(1) EOL EOF'
t "* novy riadok / nie je koniec" '/* a *\n/ b */\nval x = 1\n'           'VAL ID(x) ASSIGN INT(1) EOL EOF'

# ---------------------------------------------------------------------------
section import "prolog: import ifj26.*"
t "import ifj26.*"              'import ifj26.*\n'                         'IMPORT ID(ifj26) DOT MUL EOL EOF'
t "import ifj26.* bez \\n"      'import ifj26.*'                           'IMPORT ID(ifj26) DOT MUL EOL EOF'
t "import s medzerami"          'import  ifj26 . *\n'                      'IMPORT ID(ifj26) DOT MUL EOL EOF'
t "import + // koment"          'import ifj26.* // prolog\n'               'IMPORT ID(ifj26) DOT MUL EOL EOF'
t "import + /* */ koment"       'import ifj26.* /* prolog */\n'            'IMPORT ID(ifj26) DOT MUL EOL EOF'
t "import ifj26.write"          'import ifj26.write\n'                     'IMPORT ID(ifj26) DOT ID(write) EOL EOF'
t "import + prazdny riadok + def" 'import ifj26.*\n\ndef main(): Unit =\n  val x = 1\n' 'IMPORT ID(ifj26) DOT MUL EOL DEF ID(main) L_PAREN R_PAREN COLON TYPE_UNIT ASSIGN EOL INDENT VAL ID(x) ASSIGN INT(1) EOL DEDENT EOF'
t "volanie ifj26.write(..)"     'ifj26.write("a")\n'                       'ID(ifj26) DOT ID(write) L_PAREN STRING("a") R_PAREN EOL EOF'
t "ifj26.readInt()"             'val x = ifj26.readInt()\n'                'VAL ID(x) ASSIGN ID(ifj26) DOT ID(readInt) L_PAREN R_PAREN EOL EOF'
t "ifj26.* nasobenie nie je import" 'val x = a.b * 2\n'                    'VAL ID(x) ASSIGN ID(a) DOT ID(b) MUL INT(2) EOL EOF'
t "ID s cislom: ifj26"          'ifj26\n'                                  'ID(ifj26) EOL EOF'
t "import bez bodky (ID)"       'import ifj26\n'                           'IMPORT ID(ifj26) EOL EOF'
t "viac pristupov cez bodku"    'a.b.c.d\n'                                'ID(a) DOT ID(b) DOT ID(c) DOT ID(d) EOL EOF'

# ---------------------------------------------------------------------------
section ciarka "ciarka ',' (parametre funkcii, List(...))"
t "volanie f(a, b)"             'f(a, b)\n'                                'ID(f) L_PAREN ID(a) COMMA ID(b) R_PAREN EOL EOF'
t "f(a,b) bez medzier"          'f(a,b)\n'                                 'ID(f) L_PAREN ID(a) COMMA ID(b) R_PAREN EOL EOF'
t "viac argumentov"             'f(1, 2.5, "s", x)\n'                      'ID(f) L_PAREN INT(1) COMMA DOUBLE(2.5) COMMA STRING("s") COMMA ID(x) R_PAREN EOL EOF'
t "definicia funkcie s param."  'def add(a: Int, b: Int): Int =\n  return a + b\n' 'DEF ID(add) L_PAREN ID(a) COLON TYPE_INT COMMA ID(b) COLON TYPE_INT R_PAREN COLON TYPE_INT ASSIGN EOL INDENT RETURN ID(a) ADD ID(b) EOL DEDENT EOF'
t "List(1, 2, 3)"               'val l = List(1, 2, 3)\n'                  'VAL ID(l) ASSIGN LIST L_PAREN INT(1) COMMA INT(2) COMMA INT(3) R_PAREN EOL EOF'
t "ciarka a novy riadok v ( )"  'f(a,\n  b,\n  c)\n'                      'ID(f) L_PAREN ID(a) COMMA ID(b) COMMA ID(c) R_PAREN EOL EOF'
t "ciarka bez medzier medzi cislami" '1,5\n'                              'INT(1) COMMA INT(5) EOL EOF'
t "iba ciarka"                  ',\n'                                      'COMMA EOL EOF'
t "ciarka na konci vstupu"      'a,'                                       'ID(a) COMMA EOL EOF'
t "ciarka v retazci"            'val s = "a, b"\n'                         'VAL ID(s) ASSIGN STRING("a, b") EOL EOF'
t "ciarka za retazcom"          '"a","b"\n'                                'STRING("a") COMMA STRING("b") EOL EOF'
t "ciarka na zaciatku riadka"   'a\n  , b\n'                               'ID(a) EOL INDENT COMMA ID(b) EOL DEDENT EOF'

# ---------------------------------------------------------------------------
section anotacia "anotacia @main (token AT, potom ID(main))"
t "@main"                       '@main\n'                                  'AT ID(main) EOL EOF'
t "@ main s medzerou"           '@ main\n'                                 'AT ID(main) EOL EOF'
t "@main + komentar"            '@main // vstupny bod\n'                   'AT ID(main) EOL EOF'
t "@main pred def"              '@main\ndef main(): Unit =\n  val x = 1\n' 'AT ID(main) EOL DEF ID(main) L_PAREN R_PAREN COLON TYPE_UNIT ASSIGN EOL INDENT VAL ID(x) ASSIGN INT(1) EOL DEDENT EOF'
t "iba @"                       '@\n'                                      'AT EOL EOF'
t "@ za identifikatorom"        'a@b\n'                                    'ID(a) AT ID(b) EOL EOF'

# ---------------------------------------------------------------------------
section znaky "nezname znaky musia dat chybu a nesmu sa ticho zahodit"
t "@ v retazci je OK"           'val s = "a@b"\n'                          'VAL ID(s) ASSIGN STRING("a@b") EOL EOF'
t "@ v retazci s ciarkou a unicode" 'val s = "a, b @ c č"\n'               'VAL ID(s) ASSIGN STRING("a, b @ c č") EOL EOF'
t "#"                           'a # b\n'                                  'ID(a) ERROR'
t "\$"                          'a $ b\n'                                  'ID(a) ERROR'
t "?"                           'a ? b\n'                                  'ID(a) ERROR'
t "~"                           'a ~ b\n'                                  'ID(a) ERROR'
t "backtick"                    'a ` b\n'                                  'ID(a) ERROR'
t "spatne lomitko mimo retazca" 'a \\\\ b\n'                               'ID(a) ERROR'
t "non-ASCII mimo retazca"      'val é = 1\n'                              'VAL ERROR'
t "znak na zaciatku suboru"     '#\n'                                      'ERROR'
t "znak na zaciatku riadka v bloku" 'a\n  b\n  #\n'                        'ID(a) EOL INDENT ID(b) EOL ERROR'

# ---------------------------------------------------------------------------
section operatory "operatory a oddelovace"
t "aritmetika"                  '1+2*3-4/5\n'                              'INT(1) ADD INT(2) MUL INT(3) SUB INT(4) DIV INT(5) EOL EOF'
t "== a !="                     'a == b != c\n'                            'ID(a) EQ ID(b) NE ID(c) EOL EOF'
t "< <= > >="                   'a <= b >= c < d > e\n'                    'ID(a) LE ID(b) GE ID(c) LT ID(d) GT ID(e) EOL EOF'
t "&& || !"                     'a && b || !c\n'                           'ID(a) AND ID(b) OR NOT ID(c) EOL EOF'
t "! ="                         'a ! = b\n'                                'ID(a) NOT ASSIGN ID(b) EOL EOF'
t "= bez medzier"               'a=b\n'                                    'ID(a) ASSIGN ID(b) EOL EOF'
t "==="                         'a===b\n'                                  'ID(a) EQ ASSIGN ID(b) EOL EOF'
t "<-"                          'a<-b\n'                                   'ID(a) LT SUB ID(b) EOL EOF'
t "x-1 (bez medzier)"           'x-1\n'                                    'ID(x) SUB INT(1) EOL EOF'
t "dvojity minus"               'a - -1\n'                                 'ID(a) SUB SUB INT(1) EOL EOF'
t "a+-b"                        'a+-b\n'                                   'ID(a) ADD SUB ID(b) EOL EOF'
t "!x"                          '!x\n'                                     'NOT ID(x) EOL EOF'
t "!!x"                         '!!x\n'                                    'NOT NOT ID(x) EOL EOF'
t "= na konci vstupu"           'a ='                                      'ID(a) ASSIGN EOL EOF'
t "== na konci vstupu"          'a =='                                     'ID(a) EQ EOL EOF'
t "< na konci vstupu"           'a <'                                      'ID(a) LT EOL EOF'
t "bodkociarka"                 'a; b\n'                                   'ID(a) SEMICOLON ID(b) EOL EOF'
t "dvojbodka"                   'x: Int\n'                                 'ID(x) COLON TYPE_INT EOL EOF'
t "dvojbodka bez medzery"       'x:Int=1\n'                                'ID(x) COLON TYPE_INT ASSIGN INT(1) EOL EOF'
t "samotne &"                   'a & b\n'                                  'ID(a) ERROR'
t "samotne |"                   'a | b\n'                                  'ID(a) ERROR'
t "& na konci vstupu"           'a &'                                      'ID(a) ERROR'
t "| na konci vstupu"           'a |'                                      'ID(a) ERROR'
t "&&& "                        'a &&& b\n'                                'ID(a) AND ERROR'

# ---------------------------------------------------------------------------
section cisla "celociselne a desatinne literaly"
t "0"                           '0\n'                                      'INT(0) EOL EOF'
t "42"                          '42\n'                                     'INT(42) EOL EOF'
t "INT_MAX"                     '2147483647\n'                             'INT(2147483647) EOL EOF'
t "1_000"                       '1_000\n'                                  'INT(1000) EOL EOF'
t "1_000_000"                   '1_000_000\n'                              'INT(1000000) EOL EOF'
t "1__0 (viac podciarkovnikov)" '1__0\n'                                   'INT(10) EOL EOF'
t "1_ (podciarkovnik na konci)" '1_\n'                                     'ERROR'
t "3.14"                        '3.14\n'                                   'DOUBLE(3.14) EOL EOF'
t "0.5"                         '0.5\n'                                    'DOUBLE(0.5) EOL EOF'
t "0.0"                         '0.0\n'                                    'DOUBLE(0) EOL EOF'
t "1e3"                         '1e3\n'                                    'DOUBLE(1000) EOL EOF'
t "1E3"                         '1E3\n'                                    'DOUBLE(1000) EOL EOF'
t "1e+3"                        '1e+3\n'                                   'DOUBLE(1000) EOL EOF'
t "1e-3"                        '1e-3\n'                                   'DOUBLE(0.001) EOL EOF'
t "2.5e2"                       '2.5e2\n'                                  'DOUBLE(250) EOL EOF'
t "0e0"                         '0e0\n'                                    'DOUBLE(0) EOL EOF'
t "0.5e1"                       '0.5e1\n'                                  'DOUBLE(5) EOL EOF'
t "1.5d (suffix)"               '1.5d\n'                                   'DOUBLE(1.5) EOL EOF'
t "1.5_5"                       '1.5_5\n'                                  'DOUBLE(1.55) EOL EOF'
t "1.5e1_0"                     '1.5e1_0\n'                                'DOUBLE(1.5e+10) EOL EOF'
t "vedúca nula: 007 (zadanie: nesmie zacinat nulou)" '007\n'              'ERROR'
t "vedúca nula: 01"             '01\n'                                     'ERROR'
t "1. (bez cislice po bodke)"   '1.\n'                                     'ERROR'
t "1e (bez exponentu)"          '1e\n'                                     'ERROR'
t "1e+ (bez exponentu)"         '1e+\n'                                    'ERROR'
t "1.5e (bez exponentu)"        '1.5e\n'                                   'ERROR'
t "cislo+cislo"                 '1+2\n'                                    'INT(1) ADD INT(2) EOL EOF'
t "cislo na konci vstupu"       '42'                                       'INT(42) EOL EOF'
t "desatinne cislo na konci"    '4.5'                                      'DOUBLE(4.5) EOL EOF'
t "zaporne cislo = SUB INT"     '-5\n'                                     'SUB INT(5) EOL EOF'
t "cislo v zatvorkach"          '(7)\n'                                    'L_PAREN INT(7) R_PAREN EOL EOF'
t "cislo oddelene ciarkou"      '1.5,2\n'                                  'DOUBLE(1.5) COMMA INT(2) EOL EOF'

# ---------------------------------------------------------------------------
section retazce "retazcove literaly"
t "jednoduchy"                  '"abc"\n'                                  'STRING("abc") EOL EOF'
t "prazdny"                     '""\n'                                     'STRING("") EOL EOF'
t "prazdny v priradeni"         'val s = ""\n'                             'VAL ID(s) ASSIGN STRING("") EOL EOF'
t "prazdny na konci vstupu"     '""'                                       'STRING("") EOL EOF'
t "medzery vnutri"              '"a b  c"\n'                               'STRING("a b  c") EOL EOF'
t "escape \\\""                 '"a\\"b"\n'                                'STRING("a\"b") EOL EOF'
t "escape \\n"                  '"a\\nb"\n'                                'STRING("a\nb") EOL EOF'
t "escape \\t \\r"              '"a\\tb\\rc"\n'                            'STRING("a\tb\rc") EOL EOF'
t "escape \\\\"                 '"a\\\\b"\n'                               'STRING("a\\b") EOL EOF'
t "escape na konci"             '"a\\\\"\n'                                'STRING("a\\") EOL EOF'
t "dva retazce bez medzery"     '"a""b"\n'                                 'STRING("a") STRING("b") EOL EOF'
t "retazec a ID bez medzery"    '"a"b\n'                                   'STRING("a") ID(b) EOL EOF'
t "retazec + retazec"           '"a" + "b"\n'                              'STRING("a") ADD STRING("b") EOL EOF'
t "zatvorky v retazci"          '"(a)"\n'                                  'STRING("(a)") EOL EOF'
t "triple quote"                '"""abc"""\n'                              'STRING("""abc""") EOL EOF'
t "triple quote s \" vnutri"    '"""a"b"""\n'                              'STRING("""a"b""") EOL EOF'
t "triple quote s dvoma \""     '"""a""b"""\n'                             'STRING("""a""b""") EOL EOF'
t "prazdny triple quote"        '""""""\n'                                 'STRING("""""") EOL EOF'
t "viacriadkovy retazec"        '"""a\nb"""\n'                             "STRING(\"\"\"a"$'\n'"b\"\"\") EOL EOF"
t "viacriadkovy: escape sa nevyhodnocuje" '"""a\\nb"""\n'                  'STRING("""a\nb""") EOL EOF'
t "neukonceny retazec"          '"abc\n'                                   'ERROR'
t "neukonceny retazec na EOF"   '"abc'                                    'ERROR'
t "samotna uvodzovka"           '"'                                        'ERROR'
t "neplatny escape \\q"         '"a\\qb"\n'                                'ERROR'
t "retazec konci backslashom"   '"abc\\\\\n'                               'ERROR'
t "novy riadok v retazci"       '"a\nb"\n'                                 'ERROR'
t "neukonceny triple quote"     '"""abc'                                   'ERROR'
t "neukonceny triple quote 2"   '"""abc""'                                 'ERROR'

# ---------------------------------------------------------------------------
section klucove "klucove slova a identifikatory"
t "klucove slova"               'def do else if import return then val var while case class extension for match type\n' 'DEF DO ELSE IF IMPORT RETURN THEN VAL VAR WHILE CASE CLASS EXTENSION FOR MATCH TYPE EOL EOF'
t "typy"                        'Int Double String Unit Boolean List\n'    'TYPE_INT TYPE_DOUBLE TYPE_STRING TYPE_UNIT TYPE_BOOLEAN LIST EOL EOF'
t "klucove slovo ako prefix ID" 'define iffy Integer valx vars\n'          'ID(define) ID(iffy) ID(Integer) ID(valx) ID(vars) EOL EOF'
t "klucove slovo ako suffix ID" 'xdef ifx\n'                               'ID(xdef) ID(ifx) EOL EOF'
t "rozlisuje velkost pismen"    'DEF Def int string unit list\n'           'ID(DEF) ID(Def) ID(int) ID(string) ID(unit) ID(list) EOL EOF'
t "_x"                          '_x\n'                                     'ID(_x) EOL EOF'
t "x_1"                         'x_1\n'                                    'ID(x_1) EOL EOF'
t "a1b2"                        'a1b2\n'                                   'ID(a1b2) EOL EOF'
t "__x"                         '__x\n'                                    'ID(__x) EOL EOF'
t "klucove slovo na konci vstupu" 'val'                                    'VAL EOL EOF'
t "ID na konci vstupu"          'abc'                                      'ID(abc) EOL EOF'
t "ID nasledovane ("            'foo(1)\n'                                 'ID(foo) L_PAREN INT(1) R_PAREN EOL EOF'
t "ID nasledovane :"            'x:\n'                                     'ID(x) COLON EOL EOF'
t "dlhy identifikator (200 znakov)" "$(printf 'a%.0s' $(seq 1 200))\n"    "ID($(printf 'a%.0s' $(seq 1 200))) EOL EOF"
t "dlhy retazec (300 znakov)"   "\"$(printf 'b%.0s' $(seq 1 300))\"\n"    "STRING(\"$(printf 'b%.0s' $(seq 1 300))\") EOL EOF"
t "ID zacina cislom -> INT ID"  '1abc\n'                                   'INT(1) ID(abc) EOL EOF'

# ---------------------------------------------------------------------------
section program "realisticke programy"
P_IN='import ifj26.*\n\ndef main(): Unit =\n  val a: Int = 5\n  var b: Double = 2.5e1\n  if a > 3 then\n    ifj26.write("big, ok\\n")\n  else\n    b = b * 2.0\n  while a >= 0 do\n    a = a - 1\n'
P_EX='IMPORT ID(ifj26) DOT MUL EOL DEF ID(main) L_PAREN R_PAREN COLON TYPE_UNIT ASSIGN EOL INDENT VAL ID(a) COLON TYPE_INT ASSIGN INT(5) EOL VAR ID(b) COLON TYPE_DOUBLE ASSIGN DOUBLE(25) EOL IF ID(a) GT INT(3) THEN EOL INDENT ID(ifj26) DOT ID(write) L_PAREN STRING("big, ok\n") R_PAREN EOL DEDENT ELSE EOL INDENT ID(b) ASSIGN ID(b) MUL DOUBLE(2) EOL DEDENT WHILE ID(a) GE INT(0) DO EOL INDENT ID(a) ASSIGN ID(a) SUB INT(1) EOL DEDENT DEDENT EOF'
t "mini program s if/while"     "$P_IN"                                    "$P_EX"
t "funkcia s vysledkom"         'def sq(x: Int): Int =\n  return x * x\n\ndef main(): Unit =\n  ifj26.write(sq(3))\n' 'DEF ID(sq) L_PAREN ID(x) COLON TYPE_INT R_PAREN COLON TYPE_INT ASSIGN EOL INDENT RETURN ID(x) MUL ID(x) EOL DEDENT DEF ID(main) L_PAREN R_PAREN COLON TYPE_UNIT ASSIGN EOL INDENT ID(ifj26) DOT ID(write) L_PAREN ID(sq) L_PAREN INT(3) R_PAREN R_PAREN EOL DEDENT EOF'
t "viacriadkove volanie + koment" 'def main(): Unit =\n  f(1, // prvy\n    2, /* druhy */\n    3)\n  val y = 0\n' 'DEF ID(main) L_PAREN R_PAREN COLON TYPE_UNIT ASSIGN EOL INDENT ID(f) L_PAREN INT(1) COMMA INT(2) COMMA INT(3) R_PAREN EOL VAL ID(y) ASSIGN INT(0) EOL DEDENT EOF'
t "viac prikazov v riadku (;)"  'val a = 1; val b = 2\n'                    'VAL ID(a) ASSIGN INT(1) SEMICOLON VAL ID(b) ASSIGN INT(2) EOL EOF'
t "podmienka s && a ||"         'if a < 1 && !b || c != 2 then\n  x = 1\n' 'IF ID(a) LT INT(1) AND NOT ID(b) OR ID(c) NE INT(2) THEN EOL INDENT ID(x) ASSIGN INT(1) EOL DEDENT EOF'

# ---------------------------------------------------------------------------
section biele "biele znaky: CRLF, tabulatory"
t "CRLF konce riadkov"          'val x = 1\r\n'                            "$V"
t "CRLF + blok"                 'a\r\n  b\r\n'                            'ID(a) EOL INDENT ID(b) EOL DEDENT EOF'
t "CRLF prazdny riadok v bloku" 'a\r\n  b\r\n\r\n  c\r\n'                  'ID(a) EOL INDENT ID(b) EOL ID(c) EOL DEDENT EOF'
t "CRLF + komentar"             'val x = 1 // c\r\n'                       "$V"
t "tabulator medzi tokenmi"     'val\tx\t=\t1\n'                           "$V"
t "tabulator na konci riadka"   'val x = 1\t\n'                            "$V"
t "tabulator na zaciatku riadku = chyba" 'a\n\tb\n'                        'ID(a) EOL ERROR'
t "tabulator na zaciatku 1. riadku" '\tval x = 1\n'                        'ERROR'
t "medzery a tabulator na zaciatku" 'a\n  \tb\n'                           'ID(a) EOL ERROR'

# ---------------------------------------------------------------------------
section pamat "zatazove testy / pamat (ASan)"
deep() {  # deep N -> nastavi DEEP_IN a DEEP_EX pre N urovni odsadenia
    local n=$1 i
    DEEP_IN=""; DEEP_EX="ID(x) EOL"
    for i in $(seq 0 $((n-1))); do
        DEEP_IN+="$(printf '%*s' "$i" '')x\\n"
        [ "$i" -gt 0 ] && DEEP_EX+=" INDENT ID(x) EOL"
    done
    for i in $(seq 1 $((n-1))); do DEEP_EX+=" DEDENT"; done
    DEEP_EX+=" EOF"
}
deep 250
t "250 urovni odsadenia" "$DEEP_IN" "$DEEP_EX"
MANY_IN=""; MANY_EX=""
for i in $(seq 1 3000); do MANY_IN+="a "; MANY_EX+="ID(a) "; done
t "3000 tokenov"                "$MANY_IN" "${MANY_EX}EOL EOF"

# ---------------------------------------------------------------------------
section riadky "cisla riadkov v tokenoch (token.line)"
tl "jeden riadok"               'val x = 1\n'                'VAL@1 ID(x)@1 ASSIGN@1 INT(1)@1 EOL@1 EOF@2'
tl "tri riadky"                 'a\nb\nc\n'                  'ID(a)@1 EOL@1 ID(b)@2 EOL@2 ID(c)@3 EOL@3 EOF@4'
tl "bez \\n na konci"           'a'                          'ID(a)@1 EOL@1 EOF@1'
tl "bez \\n na 2. riadku"       'a\nb'                       'ID(a)@1 EOL@1 ID(b)@2 EOL@2 EOF@2'
tl "prazdne riadky"             'a\n\n\nb\n'                 'ID(a)@1 EOL@1 ID(b)@4 EOL@4 EOF@5'
tl "iba medzery na riadku"      'a\n   \nb\n'                'ID(a)@1 EOL@1 ID(b)@3 EOL@3 EOF@4'
tl "// komentar riadok"         'a\n// c\nb\n'               'ID(a)@1 EOL@1 ID(b)@3 EOL@3 EOF@4'
tl "// na konci riadka"         'a // c\nb\n'                'ID(a)@1 EOL@1 ID(b)@2 EOL@2 EOF@3'
tl "viacr. komentar"            'a\n/* x\ny */\nb\n'         'ID(a)@1 EOL@1 ID(b)@4 EOL@4 EOF@5'
tl "vnoreny viacr. komentar"    'a\n/* x\n/* y\n*/\nz */\nb\n' 'ID(a)@1 EOL@1 ID(b)@6 EOL@6 EOF@7'
tl "komentar za kodom na rovnakom riadku" 'a /* x */ b\nc\n' 'ID(a)@1 ID(b)@1 EOL@1 ID(c)@2 EOL@2 EOF@3'
tl "novy riadok v ( )"          'f(\n1\n)\nx\n'              'ID(f)@1 L_PAREN@1 INT(1)@2 R_PAREN@3 EOL@3 ID(x)@4 EOL@4 EOF@5'
tl "CRLF"                       'a\r\nb\r\n'                 'ID(a)@1 EOL@1 ID(b)@2 EOL@2 EOF@3'
tl "INDENT/DEDENT riadok"       'a\n  b\nc\n'                'ID(a)@1 EOL@1 INDENT@2 ID(b)@2 EOL@2 DEDENT@3 ID(c)@3 EOL@3 EOF@4'
tl "DEDENT na konci suboru"     'a\n  b\n'                   'ID(a)@1 EOL@1 INDENT@2 ID(b)@2 EOL@2 DEDENT@3 EOF@3'
tl "DEDENT na EOF bez \\n"      'a\n  b'                     'ID(a)@1 EOL@1 INDENT@2 ID(b)@2 EOL@2 DEDENT@2 EOF@2'
tl "viac DEDENT naraz"          'a\n  b\n    c\nd\n'         'ID(a)@1 EOL@1 INDENT@2 ID(b)@2 EOL@2 INDENT@3 ID(c)@3 EOL@3 DEDENT@4 DEDENT@4 ID(d)@4 EOL@4 EOF@5'
tl "/abc po INDENT"             'a\n  /b\n'                   'ID(a)@1 EOL@1 INDENT@2 DIV@2 ID(b)@2 EOL@2 DEDENT@3 EOF@3'
tl "retazec na riadku 2"        'a\n"s"\n'                   'ID(a)@1 EOL@1 STRING("s")@2 EOL@2 EOF@3'
tl "cislo na riadku 3"          'a\nb\n3.5\n'                'ID(a)@1 EOL@1 ID(b)@2 EOL@2 DOUBLE(3.5)@3 EOL@3 EOF@4'
tl "prazdny vstup"              ''                           'EOF@1'
tl "iba prazdne riadky"         '\n\n\n'                     'EOF@4'


# ---------------------------------------------------------------------------
section vlastnictvo "token_free / vlastnictvo attribute.s"
tl "ID a STRING (ASan: double free)" 'val s = "abc"\nval t = s\n' 'VAL@1 ID(s)@1 ASSIGN@1 STRING("abc")@1 EOL@1 VAL@2 ID(t)@2 ASSIGN@2 ID(s)@2 EOL@2 EOF@3'
tl "prazdny retazec"            '""\n'                       'STRING("")@1 EOL@1 EOF@2'
tleak "bez uniku: ID, STRING, klucove slova" 'def f(a: Int): String =\n  val s = "x"\n  return s\n'
tleak "bez uniku: cisla a operatory"        'val x = 1.5e3 + 2 * (3 - 1)\n'
tleak "bez uniku: komentare a odsadenie"    'a\n  /* c\n  */\n  b // x\nc\n'
tleak "bez uniku: dlhy ID (realloc)"        "$(printf 'a%.0s' $(seq 1 200))\n"

# ---------------------------------------------------------------------------
section zadanie "(zavisi od zadania - skontroluj a pripadne zmaz/uprav)"
t "// za viacr. */ (komentar nie je kod)" 'val x = 1 /* a\nb */ // c\nval y = 2\n' 'VAL ID(x) ASSIGN INT(1) EOL VAL ID(y) ASSIGN INT(2) EOL EOF'
t "pretecenie INT: 2147483648"  '2147483648\n'                             'ERROR'
t "velmi velky INT"             '99999999999999999999\n'                   'ERROR'
t "modulo %"                    'a % b\n'                                  'ID(a) ERROR'
t "{ }"                         'a { b }\n'                                'ID(a) ERROR'
t "[ ]"                         'a[1]\n'                                   'ID(a) ERROR'
t ".5 bez uvodnej nuly"         '.5\n'                                     'ERROR'
t "samotne _"                   '_\n'                                      'ERROR'

deep 300
t "300 urovni odsadenia (zasobnik ma 256: nesmie zhavarovat; spravne je rast zasobnika alebo ERROR)" "$DEEP_IN" "$DEEP_EX"

echo
echo "OK: $pass, FAIL: $fail"
[ "$fail" -eq 0 ]
