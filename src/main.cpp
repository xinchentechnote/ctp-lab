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

    std::cout << "Login successful!" << std::endl;
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

int main() {
  CThostFtdcTraderApi *trader_api = CThostFtdcTraderApi::CreateFtdcTraderApi();

  CTPTraderSpi spi(trader_api);
  trader_api->RegisterSpi(&spi);

  trader_api->SubscribePrivateTopic(THOST_TERT_QUICK);
  trader_api->SubscribePublicTopic(THOST_TERT_QUICK);

  char *front_addr = std::getenv("FRONT_ADDR");
  trader_api->RegisterFront(front_addr);

  trader_api->Init();

  std::cout << "Press Enter to exit..." << std::endl;
  std::cin.get();

  trader_api->Release();
  return 0;
}