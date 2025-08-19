#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"
#include "ThostFtdcUserApiDataType.h"
#include "ThostFtdcUserApiStruct.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

class CTPTraderSpi : public CThostFtdcTraderSpi {
public:
  explicit CTPTraderSpi(CThostFtdcTraderApi *api) : api_(api), request_id_(0) {}

  void OnFrontConnected() override {
    std::cout << "Connected to CTP server!" << std::endl;
    const char *brokerId = std::getenv("BROKER_ID");
    const char *userId = std::getenv("USER_ID");
    const char *passwd = std::getenv("PASSWD");
    CThostFtdcReqUserLoginField login_req{};
    strncpy(login_req.BrokerID, brokerId, sizeof(login_req.BrokerID));
    strncpy(login_req.UserID, userId, sizeof(login_req.UserID));
    strncpy(login_req.Password, passwd, sizeof(login_req.Password));

    if (!brokerId || !userId || !passwd) {
      std::cerr << "Missing environment variables: "
                << "BROKER_ID / USER_ID / PASSWD" << std::endl;
      return;
    }

    int ret = api_->ReqUserLogin(&login_req, ++request_id_);
    if (ret != 0) {
      std::cerr << "Failed to send login request. Error code: " << ret
                << std::endl;
    }
  }

  void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                      CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                      bool bIsLast) override {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
      std::cerr << "Login failed! ErrorID: " << pRspInfo->ErrorID
                << ", ErrorMsg: " << pRspInfo->ErrorMsg << std::endl;
      return;
    }

    std::cout << "Login trader server successful!" << std::endl;
    std::cout << "Trading day: " << pRspUserLogin->TradingDay << std::endl;
    std::cout << "Login time: " << pRspUserLogin->LoginTime << std::endl;
  }

  void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                  bool bIsLast) override {
    std::cerr << "Error received! ErrorID: " << pRspInfo->ErrorID
              << ", ErrorMsg: " << pRspInfo->ErrorMsg << std::endl;
  }

private:
  CThostFtdcTraderApi *api_;
  int request_id_;
};

class MarketDataCollector : public CThostFtdcMdSpi {

public:
  explicit MarketDataCollector(CThostFtdcMdApi *api)
      : api_(api), request_id_(0) {}
  void OnFrontConnected() override {
    std::cout << "Front connected!" << std::endl;
    const char *brokerId = std::getenv("BROKER_ID");
    const char *userId = std::getenv("USER_ID");
    const char *passwd = std::getenv("PASSWD");

    if (!brokerId || !userId || !passwd) {
      std::cerr << "Missing environment variables: "
                << "BROKER_ID / USER_ID / PASSWD" << std::endl;
      return;
    }

    CThostFtdcReqUserLoginField login_req{};
    strncpy(login_req.BrokerID, brokerId, sizeof(login_req.BrokerID));
    strncpy(login_req.UserID, userId, sizeof(login_req.UserID));
    strncpy(login_req.Password, passwd, sizeof(login_req.Password));

    int ret = api_->ReqUserLogin(&login_req, ++request_id_);
    if (ret != 0) {
      std::cerr << "Failed to send login request. Error code: " << ret
                << std::endl;
    }
  }

  void OnFrontDisconnected(int nReason) override {
    std::cout << "Front disconnected! Reason: " << nReason << std::endl;
  }

  void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                      CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                      bool bIsLast) override {
    if (pRspInfo->ErrorID != 0) {
      std::cerr << "Login failed! ErrorID: " << pRspInfo->ErrorID
                << ", ErrorMsg: " << pRspInfo->ErrorMsg << std::endl;
      return;
    }
    std::cout << "Login md server successful!" << std::endl;
    std::cout << "Trading day: " << pRspUserLogin->TradingDay << std::endl;
    std::cout << "Login time: " << pRspUserLogin->LoginTime << std::endl;
  }

  void OnRtnDepthMarketData(
      CThostFtdcDepthMarketDataField *pDepthMarketData) override {
    std::cout << "InstrumentID: " << pDepthMarketData->InstrumentID
              << ", LastPrice: " << pDepthMarketData->LastPrice << std::endl;
  }

private:
  CThostFtdcMdApi *api_;
  int request_id_;
};

int main() {
  CThostFtdcTraderApi *trader_api = CThostFtdcTraderApi::CreateFtdcTraderApi();

  CTPTraderSpi spi(trader_api);
  trader_api->RegisterSpi(&spi);

  trader_api->SubscribePrivateTopic(THOST_TERT_QUICK);
  trader_api->SubscribePublicTopic(THOST_TERT_QUICK);

  char *trader_addr = std::getenv("TRADER_ADDR");
  if(!trader_addr){
    throw std::runtime_error("TRADER_ADDR not set");
  }
  trader_api->RegisterFront(trader_addr);

  trader_api->Init();

  CThostFtdcMdApi *md_api = CThostFtdcMdApi::CreateFtdcMdApi();
  MarketDataCollector md_collector(md_api);
  md_api->RegisterSpi(&md_collector);
  char *md_addr = std::getenv("MD_ADDR");
  if(!md_addr){
    throw std::runtime_error("MD_ADDR not set");
  }
  md_api->RegisterFront(md_addr);
  md_api->Init();

  std::cout << "Press Enter to exit..." << std::endl;
  std::cin.get();

  trader_api->Release();
  return 0;
}