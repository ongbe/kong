#include "market_if.h"
#include <cctype>
#include <cstring>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <glog/logging.h>

namespace ctp {

market_if::market_if(char const *market_addr, char const *broker_id,
		char const *username, char const *password)
	: api(NULL)
	, api_reqid(0)
	, udata(NULL)
	, login_event(NULL)
	, tick_event(NULL)
{
	strncpy(this->market_addr, market_addr, sizeof(this->market_addr));
	strncpy(this->broker_id, broker_id, sizeof(this->broker_id));
	strncpy(this->username, username, sizeof(this->username));
	strncpy(this->password, password, sizeof(this->password));
	api = CThostFtdcMdApi::CreateFtdcMdApi("/tmp/");
}

market_if::~market_if()
{
	if (api)
		api->Release();
}

void market_if::run()
{
	api->RegisterSpi(this);
	api->RegisterFront(this->market_addr);
	api->Init();
}

int market_if::subscribe_market_data(const char *codes, size_t count)
{
	// easy way when count == 1
	if (count == 1)
		return api->SubscribeMarketData(const_cast<char**>(&codes), 1);

	// hard way when count > 1
	char buf[strlen(codes)+1];
	strncpy(buf, codes, sizeof(buf));

	char *ids[count];
	size_t index = 0;
	ids[index++] = buf;

	for (uint16_t i = 0; i < sizeof(buf) && index < count; i++) {
		if (buf[i] == 0)
			break;

		if (isspace(buf[i])) {
			buf[i] = 0;
			if (isalpha(buf[i+1]))
				ids[index++] = buf + i + 1;
		}
	}

	return api->SubscribeMarketData(ids, count);
}

int market_if::login()
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strncpy(req.BrokerID, broker_id, sizeof(req.BrokerID));
	strncpy(req.UserID, username, sizeof(req.UserID));
	strncpy(req.Password, password, sizeof(req.Password));
	return api->ReqUserLogin(&req, api_reqid++);
}

///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
void market_if::OnFrontConnected()
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
void market_if::OnFrontDisconnected(int nReason)
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
void market_if::OnHeartBeatWarning(int nTimeLapse)
{
	LOG(INFO) << "OnHeartBeatWarning ...";
}

///登录请求响应
void market_if::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo->ErrorID) {
		LOG(FATAL) << "ErrorID: " << pRspInfo->ErrorID
			<< " ,ErrorMsg: " << pRspInfo->ErrorMsg;
		exit(EXIT_FAILURE);
	}
	LOG(INFO) << "success to login";

	if (login_event)
		login_event(this, udata);
}

///登出请求响应
void market_if::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	LOG(INFO) << "OnRspUserLogout ...";
}

///错误应答
void market_if::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	LOG(INFO) << "OnRspError ...";
}

///订阅行情应答
void market_if::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo->ErrorID) {
		LOG(FATAL) << "ErrorID: " << pRspInfo->ErrorID
			<< " ,ErrorMsg: " << pRspInfo->ErrorMsg;
		exit(EXIT_FAILURE);
	}
	LOG(INFO) << "success to subscribe " << pSpecificInstrument->InstrumentID;
}

///取消订阅行情应答
void market_if::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo->ErrorID) {
		LOG(FATAL) << "ErrorID: " << pRspInfo->ErrorID
			<< " ,ErrorMsg: " << pRspInfo->ErrorMsg;
		exit(EXIT_FAILURE);
	}
	LOG(INFO) << "success to unsubscribe " << pSpecificInstrument->InstrumentID;
}

///订阅询价应答
void market_if::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	LOG(INFO) << "OnRspSubForQuoteRsp ...";
}

///取消订阅询价应答
void market_if::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	LOG(INFO) << "OnRspUnSubForQuoteRsp ...";
}

///深度行情通知
void market_if::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	// FIXME: fix last time
	//        even can't resolve recieve last friday night tick's time
	//        in monday moning when restart ctp
	boost::posix_time::ptime remote_time = boost::posix_time::ptime(
		boost::gregorian::from_undelimited_string(pDepthMarketData->TradingDay),
		boost::posix_time::duration_from_string(pDepthMarketData->UpdateTime));
	boost::posix_time::ptime china_time = boost::posix_time::second_clock::universal_time()
		+ boost::posix_time::time_duration(8, 0, 0);
	if (remote_time - china_time > boost::posix_time::time_duration(12, 0, 0))
		remote_time -= boost::posix_time::time_duration(24, 0, 0);
	struct tm tm_remote_time = boost::posix_time::to_tm(remote_time);

	// tick base
	tick_t tick;
	strncpy(tick.symbol, pDepthMarketData->InstrumentID, sizeof(tick.symbol));
	tick.last_time = mktime(&tm_remote_time);
	tick.last_volume = 0;
	tick.last_price = pDepthMarketData->LastPrice;
	tick.sell_volume = pDepthMarketData->AskVolume1;
	tick.sell_price = pDepthMarketData->AskPrice1;
	tick.buy_volume = pDepthMarketData->BidVolume1;
	tick.buy_price = pDepthMarketData->BidPrice1;
	tick.open_interest = pDepthMarketData->OpenInterest;
	tick.day_volume = pDepthMarketData->Volume;

	if (tick_event)
		tick_event(this, udata, tick);
}

///询价通知
void market_if::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{
	LOG(INFO) << "OnRtnForQuoteRsp ...";
}

}
