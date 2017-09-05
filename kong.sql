-- ctp.sql

CREATE TABLE IF NOT EXISTS candlestick(
	id            INTEGER PRIMARY KEY,
	symbol        CHAR(16) NOT NULL,
	begin_time    BIGINT NOT NULL,
	end_time      BIGINT NOT NULL,
	open          DOUBLE NOT NULL,
	close         DOUBLE NOT NULL,
	high          DOUBLE NOT NULL,
	low           DOUBLE NOT NULL,
	avg           DOUBLE NOT NULL,
	volume        BIGINT NOT NULL,
	open_interest BIGINT NOT NULL
);

CREATE TABLE IF NOT EXISTS contract(
	id         INTEGER PRIMARY KEY,
	name       CHAR(16) NOT NULL UNIQUE,
	symbol     CHAR(16) NOT NULL, -- utf8, from 1 to 4
	exchange   CHAR(32) NOT NULL,
	symbol_fmt CHAR(8) NOT NULL,
	main_month CHAR(32) NOT NULL, -- 2 * 12 + 11
	byseason   INT NOT NULL,
	active     INT NOT NULL
);

INSERT INTO contract(name, symbol, exchange, byseason, active, symbol_fmt, main_month) VALUES
("沪金", "au", "上海期货交易所", 1, 1, "YYmm", "6 12"),
("沪银", "ag", "上海期货交易所", 1, 1, "YYmm", "6 12"),
("沥青", "bu", "上海期货交易所", 1, 1, "YYmm", "6 12"),
("沪铜", "cu", "上海期货交易所", 0, 1, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("沪铝", "al", "上海期货交易所", 0, 0, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("沪铅", "pb", "上海期货交易所", 0, 0, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("沪锌", "zn", "上海期货交易所", 0, 1, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("沪镍", "ni", "上海期货交易所", 0, 0, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("沪锡", "sn", "上海期货交易所", 0, 0, "YYmm", "1 2 3 4 5 6 7 8 9 10 11 12"),
("螺纹", "rb", "上海期货交易所", 1, 1, "YYmm", "1 5 10"),
("橡胶", "ru", "上海期货交易所", 1, 1, "YYmm", "1 5 9"),
("铁矿", "i",  "大连商品交易所", 1, 1, "YYmm", "1 5 9"),
("焦炭", "j",  "大连商品交易所", 1, 0, "YYmm", "1 5 9"),
("焦煤", "jm", "大连商品交易所", 1, 0, "YYmm", "1 5 9"),
("豆粕", "m",  "大连商品交易所", 1, 1, "YYmm", "1 5 9"),
("棕榈", "p",  "大连商品交易所", 1, 1, "YYmm", "1 5 9"),
("豆油", "y",  "大连商品交易所", 1, 0, "YYmm", "1 5 9"),
("棉花", "CF", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9"),
("玻璃", "FG", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9"),
("甲醇", "MA", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9"),
("菜油", "OI", "郑州商品交易所", 1, 0, "Ymm",  "1 5 9"),
("菜粕", "RM", "郑州商品交易所", 1, 0, "Ymm",  "1 5 9"),
("白糖", "SR", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9"),
("PTA",  "TA", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9"),
("动煤", "ZC", "郑州商品交易所", 1, 1, "Ymm",  "1 5 9");

DROP VIEW IF EXISTS v_candlestick_min;
CREATE VIEW v_candlestick_min AS
	SELECT symbol,
		datetime(begin_time, 'unixepoch', 'localtime') btime,
		datetime(end_time, 'unixepoch', 'localtime') etime,
		open, close, high, low, avg, volume, open_interest
	FROM candlestick;

DROP VIEW IF EXISTS v_candlestick_quarter;
CREATE VIEW v_candlestick_quarter AS
	SELECT a.symbol,
		datetime(a.begin_time, 'unixepoch', 'localtime') btime,
		datetime(a.end_time, 'unixepoch', 'localtime') etime,
		b.open, c.close, a.high, a.low, a.avg, a.volume, c.open_interest
	FROM
	(SELECT symbol, min(begin_time) begin_time, max(end_time) end_time,
	max(high) high, min(low) low, round(sum(volume*avg)/sum(volume), 2) avg,
	sum(volume) volume
	FROM candlestick
	GROUP BY symbol, begin_time / 900) as a
	LEFT JOIN candlestick b
	ON a.symbol = b.symbol AND a.begin_time = b.begin_time
	LEFT JOIN candlestick c
	ON a.symbol = c.symbol AND a.end_time = c.end_time;
