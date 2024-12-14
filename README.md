# itch-parser
Nasdaq TotalView-ITCH 5.0 in various languages and abstraction levels, from Python to Verilog

## Python parser

Run with:

`python itch-parser.py 2`

where you can specify how many messages you want to parse from the start of file - 2 in the example.

Example output:

`Message Details:

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
`