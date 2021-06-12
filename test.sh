#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"
    
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 'return 0;'
assert 42 'return 42;'
assert 21 'return 5+20-4;'
assert 41 'return  12 + 34 - 5 ;'
assert 47 'return 5+6*7;'
assert 15 'return 5*(9-6);'
assert 4 'return (3+5)/2;'
assert 10 'return -10+20;'
assert 10 'return - -10;'
assert 10 'return - - +10;'

assert 0 'return 0==1;'
assert 1 'return 42==42;'
assert 1 'return 0!=1;'
assert 0 'return 42!=42;'

assert 1 'return 0<1;'
assert 0 'return 1<1;'
assert 0 'return 2<1;'
assert 1 'return 0<=1;'
assert 1 'return 1<=1;'
assert 0 'return 2<=1;'

assert 1 'return 1>0;'
assert 0 'return 1>1;'
assert 0 'return 1>2;'
assert 1 'return 1>=0;'
assert 1 'return 1>=1;'
assert 0 'return 1>=2;'

assert 3 'a=3; return a;'
assert 8 'a=3; z=5; return a+z;'

assert 6 'a=b=3; return a+b;'
assert 3 'foo=3; return foo;'
assert 8 'foo123=3; bar=5; return foo123+bar;'

assert 1 'return 1; 2; 3;'
assert 2 '1; return 2; 3;'
assert 3 '1; 2; return 3;'

# test of if
assert 3 'a = 3; if (a == 3) return a; return 5;'
assert 5 'a = 3; if (a != 3) return a; return 5;'

# test of if else
assert 5 'a = 3; if (a != 3) return a; else return 5; return 2;'

# test of while
assert 5 'i = 0; while (i < 5) i = i + 1; return i;'

# test of for
assert 10 'sum = 0; for (i = 1; i < 5; i = i + 1) sum = sum + i; return sum;'
assert 20 'sum = 0; for (; sum < 20;) sum = sum + 1; return sum;'
assert 5 'sum = 0; for (;;sum = sum + 1) if (sum == 5) return sum;'

assert 3 'a = 3; if (a == 1) return 1; if (a == 2) return 2; if (a == 3) return 3;'

assert 3 '{1; {2;} return 3;}'
assert 55 'i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j;'

echo -e "\e[32mOK\e[m"