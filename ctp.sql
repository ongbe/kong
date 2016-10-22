DROP TABLE IF EXISTS contract;
CREATE TABLE contract(
	id INTEGER PRIMARY KEY,
	code VARCHAR(31) NOT NULL
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
	id INTEGER PRIMARY KEY,
	contract_code VARCHAR(32) NOT NULL,
	trading_day VARCHAR(32) NOT NULL,
	time datetime NOT NULL,
	last_price double NOT NULL,
	last_volume int NOT NULL,
	sell_price double NOT NULL,
	sell_volume int NOT NULL,
	buy_price double NOT NULL,
	buy_volume int NOT NULL,
	open_volume double NOT NULL
);
