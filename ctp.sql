DROP TABLE IF EXISTS contract;
CREATE TABLE contract(
	id          INTEGER PRIMARY KEY,
	code        CHAR(2) NOT NULL UNIQUE,
	cn_code     CHAR(8) NOT NULL, -- utf8, from 1 to 4
	exchange    CHAR(32) NOT NULL,
	code_format CHAR(4) NOT NULL,
	main_month  CHAR(35) NOT NULL, -- 2 * 12 + 11
	byseason    INT NOT NULL,
	active      INT NOT NULL
);

--CREATE TABLE contract_trade_detail(
--	id INTEGER PRIMARY KEY,
--	contract_id INTEGER NOT NULL,
--	trading_day date NOT NULL,
--	open_price double NOT NULL,
--	close_price double NOT NULL,
--);

DROP TABLE IF EXISTS contract_tick;
CREATE TABLE contract_tick(
	id            INTEGER PRIMARY KEY,
	contract_code CHAR(6) NOT NULL,
	trading_day   CHAR(10) NOT NULL,
	last_time     DATETIME NOT NULL,
	last_price    DOUBLE NOT NULL,
	last_volume   INT NOT NULL,
	sell_price    DOUBLE NOT NULL,
	sell_volume   INT NOT NULL,
	buy_price     DOUBLE NOT NULL,
	buy_volume    INT NOT NULL,
	open_volume   DOUBLE NOT NULL
);

INSERT INTO contract(code, cn_code, exchange, byseason, active, code_format, main_month) VALUES
("au", "沪金", "上海期货交易所", 1, 1, "YYmm", "6 12"),
("ag", "沪银", "上海期货交易所", 1, 1, "YYmm", "6 12"),
("bu", "沥青", "上海期货交易所", 1, 1, "YYmm", "6 12"),
("cu", "沪铜", "上海期货交易所", 0, 1, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("al", "沪铝", "上海期货交易所", 0, 0, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("pb", "沪铅", "上海期货交易所", 0, 0, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("zn", "沪锌", "上海期货交易所", 0, 1, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("ni", "沪镍", "上海期货交易所", 0, 0, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("sn", "沪锡", "上海期货交易所", 0, 0, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("rb", "螺纹", "上海期货交易所", 1, 1, "YYmm", "1 5 10"),
("ru", "橡胶", "上海期货交易所", 1, 1, "YYmm", "1 5 9"),
("i",  "铁矿", "大连商品交易所", 1, 1, "YYmm", "1 5 9"),
("j",  "焦炭", "大连商品交易所", 1, 0, "YYmm", "1 5 9"),
("jm", "焦煤", "大连商品交易所", 1, 0, "YYmm", "1 5 9"),
("m",  "豆粕", "大连商品交易所", 1, 1, "YYmm", "1 5 9"),
("p",  "棕榈", "大连商品交易所", 1, 1, "YYmm", "1 5 9"),
("y",  "豆油", "大连商品交易所", 1, 0, "YYmm", "1 5 9"),
("CF", "棉花", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9"),
("FG", "玻璃", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9"),
("MA", "甲醇", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9"),
("OI", "菜油", "郑州商品交易所", 1, 0, "Ymm",  "1 5 9"),
("RM", "菜粕", "郑州商品交易所", 1, 0, "Ymm",  "1 5 9"),
("SR", "白糖", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9"),
("TA", "PTA",  "郑州商品交易所", 1, 1, "Ymm",  "1 5 9"),
("ZC", "动煤", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9");
