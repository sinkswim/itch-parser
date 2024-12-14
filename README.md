# itch-parser
Nasdaq TotalView-ITCH 5.0 in various languages and abstraction levels, from Python to Verilog

## Python parser

Run with:

`python itch-parser.py 2`

<<<<<<< HEAD
where you can specify how many messages you want to parse from the start of file (2 in the above example).
=======
where you can specify how many messages you want to parse from the start of file (2 in the example).
>>>>>>> d1f611d (Added C parser)

Example output:

```
Message Details:

Message 1:
Type: S
Time: 03:03:59.687760
stock_locate: 0
tracking_number: 0
timestamp: 11039687760787
vent_code: b'O'

Message 2:
Type: R
Time: 03:08:34.234545
stock_locate: 1
tracking_number: 0
timestamp: 11314234545561
stock: A
market_category: b'N'
financial_status_indicator: b' '
round_lot_size: 100
round_lots_only: b'N'
issue_classification: b'C'
issue_sub_type: Z
authenticity: b'P'
short_sale_threshold_indicator: b'N'
ipo_flag: b' '
luld_reference_price_tier: b'1'
tp_flag: b'N'
tp_leverage_factor: 0
inverse_indicator: b'N'

---------------------

Parsing Performance:
Messages processed: 2
Total time: 47.000µs
Average time per message: 23.500µs
<<<<<<< HEAD
```
=======
`

## C parser

Tested on MacOS, to run on other OS modify the big endian to host representation functions. On Linux, you can simply call `be16toh`, `be32toh`, `be64toh` in `endian.h`.

This solution, which is still quite far from optimal, uses lib2xml. On MacOS, you can install it with `brew install libxml2`.

Compiling:

`gcc -o itch_parser itch_parser.c -I/opt/homebrew/opt/libxml2/include/libxml2 -L/opt/homebrew/opt/libxml2/lib -lxml2`

Running:

`./itch_parser 2`

where you can specify how many messages you want to parse from the start of file (2 in the example).

Output is similar to Python parser.
>>>>>>> d1f611d (Added C parser)
