#include <cstring>
#include <unistd.h>
#include <iostream>

#include "config.h"
#include "RApiPlus.h"

int MdLoggedIn = 0;
int TsLoggedIn = 0;

RApi::REngine *pEngine;
RApi::LimitOrderParams order;

int state = 0;
long long tickTs = 0;

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
    if (state == 0)
    {
        tickTs = pInfo->iSsboe;
        tickTs = (tickTs * 1000 * 1000) + pInfo->iUsecs;

        order.dPrice = pInfo->dPrice - 10;

        int iCode;
        if (!pEngine->sendOrder(&order, &iCode))
        {
            std::cerr << "error placing order: " << iCode << std::endl;
        }
        else
        {
            state = 1;
        }
    }
    *aiCode = API_OK;
    return OK;
}

int Callbacks::LineUpdate(RApi::LineInfo *pInfo, void *pContext, int *aiCode)
{
    if (cmp(pInfo->sStatus, RApi::sLINE_STATUS_ORDER_SENT_TO_EXCH))
    {
        state = 0;
        long long tradeTs = pInfo->iSsboe;
        tradeTs = (tradeTs * 1000 * 1000) + pInfo->iUsecs;
        std::cout << tradeTs - tickTs << " micros" << std::endl;
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
    oParams.sAppName = {"latency", (int)strlen("latency")};
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
    }

    tsNCharcb sTicker = {ticker, (int)strlen(ticker)};
    tsNCharcb sExchange = {"CME", (int)strlen("CME")};
    tsNCharcb sTradeRoute = {"globex", (int)strlen("globex")};
    
    order.iQty = 1;
    order.sTicker = sTicker;
    order.sExchange = sExchange;
    order.pAccount = &pAccount;
    order.sTradeRoute = sTradeRoute;
    order.sDuration = RApi::sORDER_DURATION_FOK;
    order.sBuySellType = RApi::sBUY_SELL_TYPE_BUY;
    order.sEntryType = RApi::sORDER_ENTRY_TYPE_AUTO;

    int iFlags = (RApi::MD_PRINTS);

    if (!pEngine->subscribe(&sExchange, &sTicker, iFlags, &iCode))
    {
        std::cerr << "error subscribing to market data: " << iCode << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
    }

    fgetc(stdin);

    delete pEngine;
    delete pCallbacks;
    delete pAdmCallbacks;
    return GOOD;
}