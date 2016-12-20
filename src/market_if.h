#ifndef _MARKET_IF_H
#define _MARKET_IF_H

#include "yx/types.h"
#include <ThostFtdcMdApi.h>

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
	int api_reqid;

private:
	// rc
	char market_addr[64];
	char broker_id[64];
	char username[64];
	char password[64];

public:
	// signals
	void *udata;
	void (*login_event)(market_if *sender, void *udata);
	void (*tick_event)(market_if *sender, void *udata, futures_tick &tick);

public:
	market_if(char const *market_addr, char const *broker_id,
			char const *username, char const *password);
	virtual ~market_if();

public:
	void run();
	int subscribe_market_data(const char *codes, size_t count = 1);
private:
	int login();

public:
	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected();

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///		0x1001 网络读失败
	///		0x1002 网络写失败
	///		0x2001 接收心跳超时
	///		0x2002 发送心跳失败
	///		0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason);

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse);

	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///订阅行情应答
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///取消订阅行情应答
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///订阅询价应答
	virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///取消订阅询价应答
	virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///深度行情通知
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

	///询价通知
	virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp);
};

}

#endif
