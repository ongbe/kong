#include "ctp_market_if.h"
#include <cctype>
#include <cstring>
#include <ctime>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <glog/logging.h>

namespace ctp {

market_if::market_if(const std::string &market_addr,
		     const std::string &broker_id,
		     const std::string &username,
		     const std::string &password)
	: api(NULL)
	, api_reqid(0)
	, market_addr(market_addr)
	, broker_id(broker_id)
	, username(username)
	, password(password)
	, private_data(NULL)
	, login_event(NULL)
	, tick_event(NULL)
{
	api = CThostFtdcMdApi::CreateFtdcMdApi("/tmp/");
}

market_if::~market_if()
{
	if (api) {
		api->Release();
		api = NULL;
	}
}

void market_if::run()
{
	api->RegisterSpi(this);
	api->RegisterFront(const_cast<char*>(market_addr.c_str()));
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
	strncpy(req.BrokerID, broker_id.c_str(), sizeof(req.BrokerID));
	strncpy(req.UserID, username.c_str(), sizeof(req.UserID));
	strncpy(req.Password, password.c_str(), sizeof(req.Password));
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
void market_if::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
			       CThostFtdcRspInfoField *pRspInfo,
			       int nRequestID, bool bIsLast)
{
	if (pRspInfo->ErrorID) {
		LOG(FATAL) << "ErrorID: " << pRspInfo->ErrorID
			<< " ,ErrorMsg: " << pRspInfo->ErrorMsg;
		exit(EXIT_FAILURE);
	}
	LOG(INFO) << "success to login";

	if (login_event)
		login_event(this);
}

///登出请求响应
void market_if::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
				CThostFtdcRspInfoField *pRspInfo,
				int nRequestID, bool bIsLast)
{
	LOG(INFO) << "OnRspUserLogout ...";
}

///错误应答
void market_if::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	LOG(INFO) << "OnRspError ...";
}

///订阅行情应答
void market_if::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
				   CThostFtdcRspInfoField *pRspInfo,
				   int nRequestID, bool bIsLast)
{
	if (pRspInfo->ErrorID) {
		LOG(FATAL) << "ErrorID: " << pRspInfo->ErrorID
			<< " ,ErrorMsg: " << pRspInfo->ErrorMsg;
		exit(EXIT_FAILURE);
	}
	LOG(INFO) << "success to subscribe " << pSpecificInstrument->InstrumentID;
}

///取消订阅行情应答
void market_if::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
				     CThostFtdcRspInfoField *pRspInfo,
				     int nRequestID, bool bIsLast)
{
	if (pRspInfo->ErrorID) {
		LOG(FATAL) << "ErrorID: " << pRspInfo->ErrorID
			<< " ,ErrorMsg: " << pRspInfo->ErrorMsg;
		exit(EXIT_FAILURE);
	}
	LOG(INFO) << "success to unsubscribe " << pSpecificInstrument->InstrumentID;
}

///订阅询价应答
void market_if::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
				    CThostFtdcRspInfoField *pRspInfo,
				    int nRequestID, bool bIsLast)
{
	LOG(INFO) << "OnRspSubForQuoteRsp ...";
}

///取消订阅询价应答
void market_if::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
				      CThostFtdcRspInfoField *pRspInfo,
				      int nRequestID, bool bIsLast)
{
	LOG(INFO) << "OnRspUnSubForQuoteRsp ...";
}

///深度行情通知
void market_if::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	/*
	 * Call auction: 8:59:00, 20:59:00
	 * Open time:    9:00:00, 10:30:00, 13:30:00, 21:00:00
	 * Close time:   10:15:00, 11:30:00, 15:00:00, 23:00:00, 23:30:00, 01:00:00, 02:30:00
	 */

	using namespace boost::posix_time;
	using namespace boost::gregorian;
	ptime remote_ptime = ptime(from_undelimited_string(pDepthMarketData->TradingDay),
				   duration_from_string(pDepthMarketData->UpdateTime));
	struct tm remote_tm = to_tm(remote_ptime);
	time_t remote_time = mktime(&remote_tm);
	time_t utc_time = time(NULL);

	// fix remote date
	if ((remote_ptime.time_of_day() >= time_duration(8,59,0) &&
	     remote_ptime.time_of_day() <= time_duration(15,0,0)) ||
	    remote_ptime.time_of_day() >= time_duration(20,59,0) ||
	    remote_ptime.time_of_day() <= time_duration(2,30,0)) {
		while (remote_time + 1800 < utc_time) // 1800 other than 3600
			remote_time += 3600;
		while (remote_time - 1800 > utc_time)
			remote_time -= 3600;
	}

	// fix remote time
	if (remote_tm.tm_sec == 0) {
#ifdef CTP_FIX_CALL_AUCTION
		if ((remote_tm.tm_hour == 8 && remote_tm.tm_min == 59) ||
		    (remote_tm.tm_hour == 20 && remote_tm.tm_min == 59))
			remote_time += 60;
#endif
#ifdef CTP_FIX_OPEN_TIME
		if ((remote_tm.tm_hour == 9 && remote_tm.tm_min == 0) ||
		    (remote_tm.tm_hour == 10 && remote_tm.tm_min == 30) ||
		    (remote_tm.tm_hour == 13 && remote_tm.tm_min == 30) ||
		    (remote_tm.tm_hour == 21 && remote_tm.tm_min == 0))
			remote_time++;
#endif
#ifdef CTP_FIX_CLOSE_TIME
		if ((remote_tm.tm_hour == 10 && remote_tm.tm_min == 15) ||
		    (remote_tm.tm_hour == 11 && remote_tm.tm_min == 30) ||
		    (remote_tm.tm_hour == 15 && remote_tm.tm_min == 0) ||
		    (remote_tm.tm_hour == 23 && remote_tm.tm_min == 0) ||
		    (remote_tm.tm_hour == 23 && remote_tm.tm_min == 30) ||
		    (remote_tm.tm_hour == 1 && remote_tm.tm_min == 0) ||
		    (remote_tm.tm_hour == 2 && remote_tm.tm_min == 30))
			remote_time--;
#endif
	}

	// tick base
	tick_t tick;
	strncpy(tick.symbol, pDepthMarketData->InstrumentID, sizeof(tick.symbol));
	tick.last_time = remote_time;
	tick.last_volume = 0;
	tick.last_price = pDepthMarketData->LastPrice;
	tick.sell_volume = pDepthMarketData->AskVolume1;
	tick.sell_price = pDepthMarketData->AskPrice1;
	tick.buy_volume = pDepthMarketData->BidVolume1;
	tick.buy_price = pDepthMarketData->BidPrice1;
	tick.open_interest = pDepthMarketData->OpenInterest;
	tick.day_volume = pDepthMarketData->Volume;

	if (tick_event)
		tick_event(this, tick);
}

///询价通知
void market_if::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{
	LOG(INFO) << "OnRtnForQuoteRsp ...";
}

}
