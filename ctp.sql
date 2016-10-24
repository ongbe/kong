DROP TABLE IF EXISTS contract;
CREATE TABLE contract(
	id INTEGER PRIMARY KEY,
	code CHAR(6) NOT NULL
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
