#include <queue>
#include <cstring>
#include <unistd.h>
#include <iostream>

#include "config.h"
#include "RApiPlus.h"

#define M0 -0.16907
#define M1 0.06205
#define M2 0.69479
#define M3 0.85671
#define WIN 6947
#define QTY 1

int MdLoggedIn = 0;
int TsLoggedIn = 0;
bool shutdown = false;

RApi::REngine *pEngine;
RApi::MarketOrderParams buyOrder;
RApi::MarketOrderParams sellOrder;

int state = 0;
double sumpx = 0; 
std::queue<double> vals;

bool cmp(tsNCharcb a, tsNCharcb b)
{
    return (a.iDataLen == b.iDataLen) && (strncmp(a.pData, b.pData, a.iDataLen) == 0);
}

class AdmCallbacks : public RApi::AdmCallbacks
{
public:
    AdmCallbacks() {}
    ~AdmCallbacks() {}
    virtual int Alert(RApi::AlertInfo *pInfo, void *pContext, int *aiCode);
};

class Callbacks : public RApi::RCallbacks
{
public:
    Callbacks() {}
    ~Callbacks() {}
    virtual int Alert(RApi::AlertInfo *pInfo, void *pContext, int *aiCode);
    virtual int TradePrint(RApi::TradeInfo *pInfo, void *pContext, int *aiCode);
    virtual int LineUpdate(RApi::LineInfo *pInfo, void *pContext, int *aiCode);
};

int AdmCallbacks::Alert(RApi::AlertInfo *pInfo, void *pContext, int *aiCode)
{
    std::cout << "AdmCallback " << pInfo->iAlertType << ": " << pInfo->sMessage.pData << std::endl;
    
    *aiCode = API_OK;
    return OK;
}

int Callbacks::Alert(RApi::AlertInfo *pInfo, void *pContext, int *aiCode)
{
    std::cout << "RCallback " << pInfo->iAlertType << ": " << pInfo->sMessage.pData << std::endl;
    if (pInfo->iConnectionId == RApi::MARKET_DATA_CONNECTION_ID)
    {
        if (pInfo->iAlertType == RApi::ALERT_LOGIN_FAILED)
        {
            MdLoggedIn = -1;
        }
        else if (pInfo->iAlertType == RApi::ALERT_LOGIN_COMPLETE)
        {
            MdLoggedIn = 1;
        }
    }
    else if (pInfo->iConnectionId == RApi::TRADING_SYSTEM_CONNECTION_ID)
    {
        if (pInfo->iAlertType == RApi::ALERT_LOGIN_FAILED)
        {
            TsLoggedIn = -1;
        }
        else if (pInfo->iAlertType == RApi::ALERT_LOGIN_COMPLETE)
        {
            TsLoggedIn = 1;
        }
    }

    *aiCode = API_OK;
    return OK;
}

int Callbacks::TradePrint(RApi::TradeInfo *pInfo, void *pContext, int *aiCode)
{
    double price = pInfo->dPrice;
    sumpx += price;
    vals.push(price);

    if (vals.size() == WIN)
    {
        double mn = sumpx / WIN;
        
        double val = vals.front();
        sumpx -= val;
        vals.pop();

        double score = M0 * (price - mn) + M1 * (price - val);
        
        if (state == 0 && score > M3)
        {
            int iCode;
            if (!pEngine->sendOrder(&buyOrder, &iCode))
            {
                std::cerr << "error placing order: " << iCode << std::endl;
            }
            else 
            {
                state = 1;
            }
        }
        else if (state == 1 && score < -M3)
        {
            int iCode;
            if (!pEngine->sendOrder(&sellOrder, &iCode))
            {
                std::cerr << "error placing order: " << iCode << std::endl;
            }
            else 
            {
                state = 0;
            }
        }
    }

    *aiCode = API_OK;
    return OK;
}

int Callbacks::LineUpdate(RApi::LineInfo *pInfo, void *pContext, int *aiCode)
{
    if (pInfo->bAvgFillPriceFlag) {
        std::cout << *pInfo->sBuySellType.pData << " | " << pInfo->dAvgFillPrice << std::endl;
    }

    *aiCode = API_OK;
    return OK;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cerr << "usage: " << argv[0] << " username password ticker" << std::endl;
        return BAD;
    }

    int iCode;
    char *username = argv[1];
    char *password = argv[2];
    char *ticker = argv[3];

    Callbacks *pCallbacks;
    AdmCallbacks *pAdmCallbacks;

    try
    {
        pAdmCallbacks = new AdmCallbacks();
    }
    catch (OmneException &oEx)
    {
        std::cerr << "error instantiating AdmCallbacks: " << oEx.getErrorCode() << " | " << oEx.getErrorString() << std::endl;
        delete pAdmCallbacks;
        return BAD;
    }

    RApi::REngineParams oParams;
    oParams.sAppName = {"gg", (int)strlen("gg")};
    oParams.sAppVersion = {version, (int)strlen(version)};
    oParams.envp = Envp;
    oParams.pAdmCallbacks = pAdmCallbacks;
    oParams.sLogFilePath = {logfile, (int)strlen(logfile)};

    try
    {
        pEngine = new RApi::REngine(&oParams);
    }
    catch (OmneException &oEx)
    {
        std::cerr << "error instantiating REngine: " << oEx.getErrorCode() << " | " << oEx.getErrorString() << std::endl;
        delete pEngine;
        delete pAdmCallbacks;
        return BAD;
    }

    try
    {
        pCallbacks = new Callbacks();
    }
    catch (OmneException &oEx)
    {
        std::cerr << "error instantiating RCallbacks: " << oEx.getErrorCode() << " | " << oEx.getErrorString() << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    RApi::LoginParams oLoginParams;
    oLoginParams.pCallbacks = pCallbacks;
    oLoginParams.sMdUser = {username, (int)strlen(username)};
    oLoginParams.sMdPassword = {password, (int)strlen(password)};
    oLoginParams.sMdCnnctPt = {MdConnectPt, (int)strlen(MdConnectPt)};
    oLoginParams.sTsUser = {username, (int)strlen(username)};
    oLoginParams.sTsPassword = {password, (int)strlen(password)};
    oLoginParams.sTsCnnctPt = {TsConnectPt, (int)strlen(TsConnectPt)};

    if (!pEngine->login(&oLoginParams, &iCode))
    {
        std::cerr << "error logging into md/ts: " << iCode << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    while (MdLoggedIn == 0)
    {
        sleep(1);
    }

    if (MdLoggedIn == -1)
    {
        std::cerr << "login to md failed, check user/pass or permissions" << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    while (TsLoggedIn == 0)
    {
        sleep(1);

    }

    if (TsLoggedIn == -1)
    {
        std::cerr << "login to ts failed, check user/pass or permissions" << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    RApi::AccountInfo pAccount;
    pAccount.sIbId = {"Ironbeam", (int)strlen("Ironbeam")};
    pAccount.sFcmId = {"Ironbeam", (int)strlen("Ironbeam")};
    pAccount.sAccountId = {username, (int)strlen(username)};

    if (!pEngine->subscribeOrder(&pAccount, &iCode))
    {
        std::cerr << "error subscribing to orders: " << iCode << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    tsNCharcb sTicker = {ticker, (int)strlen(ticker)};
    tsNCharcb sExchange = {"CME", (int)strlen("CME")};
    tsNCharcb sTradeRoute = {"globex", (int)strlen("globex")};
    
    buyOrder.iQty = QTY;
    buyOrder.sTicker = sTicker;
    buyOrder.sExchange = sExchange;
    buyOrder.pAccount = &pAccount;
    buyOrder.sTradeRoute = sTradeRoute;
    buyOrder.sDuration = RApi::sORDER_DURATION_DAY;
    buyOrder.sBuySellType = RApi::sBUY_SELL_TYPE_BUY;
    buyOrder.sEntryType = RApi::sORDER_ENTRY_TYPE_AUTO;

    sellOrder.iQty = QTY;
    sellOrder.sTicker = sTicker;
    sellOrder.sExchange = sExchange;
    sellOrder.pAccount = &pAccount;
    sellOrder.sTradeRoute = sTradeRoute;
    sellOrder.sDuration = RApi::sORDER_DURATION_DAY;
    sellOrder.sBuySellType = RApi::sBUY_SELL_TYPE_SELL;
    sellOrder.sEntryType = RApi::sORDER_ENTRY_TYPE_AUTO;


    int iFlags = (RApi::MD_PRINTS);

    if (!pEngine->subscribe(&sExchange, &sTicker, iFlags, &iCode))
    {
        std::cerr << "error subscribing to market data: " << iCode << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    while (!shutdown) {
        sleep(60);
    }

    delete pEngine;
    delete pCallbacks;
    delete pAdmCallbacks;
    return GOOD;
}