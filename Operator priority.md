Operators priority
==================

| Priority | Operators                                                                                                                                |
|----------|------------------------------------------------------------------------------------------------------------------------------------------|
| 0        | a[\<expr\>], a(\<args\>), a.b                                                                                                            |
| 1        | !x, ~x, +x, -x, typeof x, @x, \+\+x, --x, x\+\+, x--                                                                                     |
| 2        | a / b, a * b, a # b, a \\\\ b, a % b, a => b, a => b, a as b, a istypeof b, key in a                                                     |
| 3        | a + b, a - b, a \\ b, a -> b                                                                                                             |
| 4        | a >> b, a << b, a >>> b, a <<< b                                                                                                         |
| 5        | a > b, a < b, a >= b, a <= b                                                                                                             |
| 6        | a == b, a != b, a === b, a !== b                                                                                                         |
| 7        | a & b                                                                                                                                    |
| 8        | a ^ b                                                                                                                                    |
| 9        | a \| b                                                                                                                                   |
| 10       | a && b                                                                                                                                   |
| 11       | a \|\| b                                                                                                                                 |
| 12       | a ? b : c                                                                                                                                |
| 13       | a = b, a += b, a -= b, a *= b, a /= b, a <<<= b, a >>>= b, a >>= b, a <<= b, a #= b, a \\= b, a \\\\= b, a \|= b, a &= b, a %= b, a ^= b |