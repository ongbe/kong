#ifndef _MARKET_IF_H
#define _MARKET_IF_H

#include "ThostFtdcMdApi.h"
#include "contract_tick.h"
#include <glog/logging.h>
#include <cstdlib>
#include <cctype>
#include <string>
#include <iostream>
#include <thread>
using namespace std;
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace ctp {

/*
 * interface with ftdc market
 * has a api & is a spi, like multi inherit and api is the core
 */
class market_if : public CThostFtdcMdSpi {
	market_if(const market_if&);
	const market_if& operator=(const market_if&);

private:
	// market api, heap resources
	CThostFtdcMdApi *api;
	thread *api_thread;
	int api_reqid;

private:
	// rc
	char market_addr[64];
	char broker_id[64];
	char username[64];
	char password[64];
	char contract_codes[256]; // separated by ' '

public:
	// signals
	void *udata;
	void (*tick_event)(contract_tick &tick, void *udata);

public:
	market_if(char const *market_addr, char const *broker_id,
			char const *username, char const *password, char const *contract_codes)
		: api(NULL)
		, api_thread(NULL)
		, api_reqid(0)
		, udata(NULL)
		, tick_event(NULL)
	{
		strncpy(this->market_addr, market_addr, sizeof(this->market_addr));
		strncpy(this->broker_id, broker_id, sizeof(this->broker_id));
		strncpy(this->username, username, sizeof(this->username));
		strncpy(this->password, password, sizeof(this->password));
		strncpy(this->contract_codes, contract_codes, sizeof(this->contract_codes));
		api_init();
	}

	virtual ~market_if()
	{
		if (api)
			api->Release();

		if (api_thread)
			delete api_thread;
	}

private:
	void api_init()
	{
		api = CThostFtdcMdApi::CreateFtdcMdApi("/tmp/");

		market_if *that = this;
		api_thread = new thread([that] () -> void {
			try {
				LOG(INFO) << "initializing market spi ...";
				that->api->RegisterSpi(that);
				that->api->RegisterFront(that->market_addr);
				that->api->Init();
				that->api->Join();

				LOG(INFO) << "uninitializing market spi ...";
				that->api->Release();
			} catch (...) {
				LOG(FATAL) << "catched exception in market_if thread!";
			}
		});
	}

	int login()
	{
		CThostFtdcReqUserLoginField req;
		memset(&req, 0, sizeof(req));
		strncpy(req.BrokerID, broker_id, sizeof(req.BrokerID));
		strncpy(req.UserID, username, sizeof(req.UserID));
		strncpy(req.Password, password, sizeof(req.Password));
		return api->ReqUserLogin(&req, api_reqid++);
	}

	int subscribe_market_data()
	{
		char buf[sizeof(contract_codes)];
		memcpy(buf, contract_codes, sizeof(buf));

		vector<char*> id_tab;
		id_tab.push_back(&buf[0]);

		for (uint16_t i = 0; i < sizeof(buf); i++) {
			if (buf[i] == 0)
				break;

			if (isspace(buf[i])) {
				buf[i] = 0;
				if (isalpha(buf[i+1]))
					id_tab.push_back(buf + i + 1);
			}
		}

		char *ids[id_tab.size()];
		for (uint16_t i = 0; i < id_tab.size(); i++)
			ids[i] = id_tab.at(i);

		return api->SubscribeMarketData(ids, id_tab.size());
	}

public:
	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected()
	{
		login();
	}

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///		0x1001 网络读失败
	///		0x1002 网络写失败
	///		0x2001 接收心跳超时
	///		0x2002 发送心跳失败
	///		0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason)
	{
		switch (nReason) {
			case 0x1001: LOG(ERROR) << "网络读失败"; break;
			case 0x1002: LOG(ERROR) << "网络写失败"; break;
			case 0x2001: LOG(ERROR) << "接收心跳超时"; break;
			case 0x2002: LOG(ERROR) << "发送心跳失败"; break;
			case 0x2003: LOG(ERROR) << "收到错误报文"; break;
		}
	}

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse)
	{
		LOG(INFO) << "OnHeartBeatWarning ...";
	}

	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo->ErrorID) {
			LOG(FATAL) << "ErrorID: " << pRspInfo->ErrorID
				<< " ,ErrorMsg: " << pRspInfo->ErrorMsg;
			exit(EXIT_FAILURE);
		}
		LOG(INFO) << "success to login";

		subscribe_market_data();
	}

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG(INFO) << "OnRspUserLogout ...";
	}

	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG(INFO) << "OnRspError ...";
	}

	///订阅行情应答
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo->ErrorID) {
			LOG(FATAL) << "ErrorID: " << pRspInfo->ErrorID
				<< " ,ErrorMsg: " << pRspInfo->ErrorMsg;
			exit(EXIT_FAILURE);
		}
		LOG(INFO) << "success to subscribe " << pSpecificInstrument->InstrumentID;
	}

	///取消订阅行情应答
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo->ErrorID) {
			LOG(FATAL) << "ErrorID: " << pRspInfo->ErrorID
				<< " ,ErrorMsg: " << pRspInfo->ErrorMsg;
			exit(EXIT_FAILURE);
		}
		LOG(INFO) << "success to unsubscribe " << pSpecificInstrument->InstrumentID;
	}

	///订阅询价应答
	virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG(INFO) << "OnRspSubForQuoteRsp ...";
	}

	///取消订阅询价应答
	virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG(INFO) << "OnRspUnSubForQuoteRsp ...";
	}

	///深度行情通知
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
	{
		contract_tick tick;
		memcpy(tick.contract_code, pDepthMarketData->InstrumentID, sizeof(tick.contract_code));
		strncpy(tick.trading_day, pDepthMarketData->TradingDay, sizeof(tick.trading_day));
		boost::posix_time::ptime time = boost::posix_time::ptime(
				boost::gregorian::from_undelimited_string(pDepthMarketData->ActionDay),
				boost::posix_time::duration_from_string(pDepthMarketData->UpdateTime));
		strncpy(tick.last_time, to_iso_extended_string(time).c_str(), sizeof(tick.last_time));
		tick.last_time[10] = ' ';
		tick.last_price = pDepthMarketData->LastPrice;
		tick.last_day_volume = pDepthMarketData->Volume;
		tick.sell_price = pDepthMarketData->AskPrice1;
		tick.sell_volume = pDepthMarketData->AskVolume1;
		tick.buy_price = pDepthMarketData->BidPrice1;
		tick.buy_volume = pDepthMarketData->BidVolume1;
		tick.open_volume = pDepthMarketData->OpenInterest;

		if (tick_event)
			tick_event(tick, udata);
	}

	///询价通知
	virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
	{
		LOG(INFO) << "OnRtnForQuoteRsp ...";
	}
};

}

#endif
