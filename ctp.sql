DROP TABLE IF EXISTS futures_tick;
CREATE TABLE futures_tick(
	id            INTEGER PRIMARY KEY,
	contract_code CHAR(6) NOT NULL,
	last_iso_time CHAR(19) NOT NULL,

	last_time     BIGINT NOT NULL,
	last_volume   BIGINT NOT NULL,
	last_price    DOUBLE NOT NULL,
	sell_volume   BIGINT NOT NULL,
	sell_price    DOUBLE NOT NULL,
	buy_volume    BIGINT NOT NULL,
	buy_price     DOUBLE NOT NULL,

	trading_day   CHAR(10) NOT NULL,
	day_volume    BIGINT NOT NULL,
	open_interest BIGINT NOT NULL
);

DROP TABLE IF EXISTS futures_bar_min;
CREATE TABLE futures_bar_min(
	id            INTEGER PRIMARY KEY,
	contract_code CHAR(6) NOT NULL,
	begin_iso_time CHAR(19) NOT NULL,

	begin_time    BIGINT NOT NULL,
	end_time      BIGINT NOT NULL,
	volume        BIGINT NOT NULL,
	open          DOUBLE NOT NULL,
	close         DOUBLE NOT NULL,
	high          DOUBLE NOT NULL,
	low           DOUBLE NOT NULL,
	avg           DOUBLE NOT NULL,
	wavg          DOUBLE NOT NULL
);

DROP TABLE IF EXISTS futures_contract_base;
CREATE TABLE futures_contract_base(
	id          INTEGER PRIMARY KEY,
	code        CHAR(2) NOT NULL UNIQUE,
	cn_code     CHAR(8) NOT NULL, -- utf8, from 1 to 4
	exchange    CHAR(32) NOT NULL,
	code_format CHAR(4) NOT NULL,
	main_month  CHAR(35) NOT NULL, -- 2 * 12 + 11
	byseason    INT NOT NULL,
	active      INT NOT NULL
);

INSERT INTO futures_contract_base(code, cn_code, exchange, byseason, active, code_format, main_month) VALUES
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

DROP VIEW IF EXISTS v_futures_bar_15;
CREATE VIEW v_futures_bar_15 AS
	SELECT datetime(a.begin_time, 'unixepoch', 'localtime') btime,
		datetime(a.end_time, 'unixepoch', 'localtime') etime,
		a.contract_code, a.volume, b.open, c.close, a.high, a.low, a.avg, a.wavg
	FROM
	(SELECT contract_code, min(begin_time) begin_time, max(end_time) end_time,
	sum(volume) volume, max(high) high, min(low) low, round(avg(avg), 2) avg, round(sum(volume*wavg)/sum(volume), 2) wavg
	FROM futures_bar_min
	GROUP BY contract_code, begin_time / 900) as a
	LEFT JOIN futures_bar_min b
	ON a.contract_code = b.contract_code AND a.begin_time = b.begin_time
	LEFT JOIN futures_bar_min c
	ON a.contract_code = c.contract_code AND a.end_time = c.end_time;

--SELECT datetime(a.begin_time, 'unixepoch', 'localtime') btime,
--	datetime(a.end_time, 'unixepoch', 'localtime') etime,
--	a.contract_code, a.volume, b.last_price open, c.last_price close, a.high, a.low, a.avg, a.wavg
--FROM
--(SELECT contract_code, min(last_time) begin_time, max(last_time) end_time,
--sum(last_volume) volume, max(last_price) high, min(last_price) low, round(avg(last_price), 2) avg, round(sum(last_volume*last_price)/sum(last_volume), 2) wavg
--FROM futures_tick
--GROUP BY contract_code, last_time / 60) as a
--LEFT JOIN futures_tick b
--ON a.contract_code = b.contract_code AND a.begin_time = b.last_time
--LEFT JOIN futures_tick c
--ON a.contract_code = c.contract_code AND a.end_time = c.last_time;

--SELECT datetime(last_time, 'unixepoch', 'localtime') ltime, *
--FROM futures_tick;
