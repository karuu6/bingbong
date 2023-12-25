#include <cstring>
#include <unistd.h>
#include "config.h"
#include "RApiPlus.h"

#include <iomanip>
#include <iostream>

#define TH 2

int TsLoggedIn = 0;
int MdLoggedIn = 0;

RApi::REngine *pEngine;
RApi::AccountInfo pAccount;

tsNCharcb sTicker;
tsNCharcb sExchange;
tsNCharcb sTradeRoute;

char acOrderNum[32];
char bcOrderNum[32];
bool aOrderGood = true;
bool bOrderGood = true;
tsNCharcb aOrderNum = {acOrderNum, 0};
tsNCharcb bOrderNum = {bcOrderNum, 0};

double last_bid = 0;
double last_ask = 0;

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
    // virtual int TradePrint(RApi::TradeInfo *pInfo, void *pContext, int *aiCode);
    virtual int BestBidQuote(RApi::BidInfo *pInfo, void *pContext, int *aiCode);
    virtual int BestAskQuote(RApi::AskInfo *pInfo, void *pContext, int *aiCode);

    virtual int StatusReport(RApi::OrderStatusReport *pReport, void *pContext, int *aiCode);
    virtual int FillReport(RApi::OrderFillReport *pReport, void *pContext, int *aiCode);
    virtual int ModifyReport(RApi::OrderModifyReport *pReport, void *pContext, int *aiCode);
    virtual int FailureReport(RApi::OrderFailureReport *pReport, void *pContext, int *aiCode);

    virtual int Alert(RApi::AlertInfo *pInfo, void *pContext, int *aiCode);
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

int Callbacks::BestBidQuote(RApi::BidInfo *pInfo, void *pContext, int *aiCode)
{
    double bid = pInfo->dPrice;
    if (bid != last_bid && bOrderGood)
    {
        int iCode;
        if (bOrderNum.iDataLen == 0)
        {
            RApi::OrderParams pEntry;
            pEntry.dPrice = bid - TH;
            pEntry.iQty = 1;
            pEntry.pAccount = &pAccount;
            pEntry.sBuySellType = RApi::sBUY_SELL_TYPE_BUY;
            pEntry.sOrderType = RApi::sORDER_TYPE_LIMIT;
            pEntry.sDuration = RApi::sORDER_DURATION_GTC;
            pEntry.sEntryType = RApi::sORDER_ENTRY_TYPE_AUTO;
            pEntry.sExchange = sExchange;
            pEntry.sTicker = sTicker;
            pEntry.sTradeRoute = sTradeRoute;
            pEntry.sTag = {"B", 1};

            RApi::BracketTier target, stop;
            target.iTicks = TH * 4;
            target.llQty = 1;
            stop.iTicks = TH * 4;
            stop.llQty = 1;

            RApi::BracketParams pBracketParams;
            pBracketParams.asStopArray = &stop;
            pBracketParams.asTargetArray = &target;
            pBracketParams.iStopArrayLen = 1;
            pBracketParams.iTargetArrayLen = 1;
            pBracketParams.sBracketType = RApi::sBRACKET_TYPE_TARGET_AND_STOP;
            pBracketParams.sOrderOperationType = RApi::sORDER_OPERATION_TYPE_FOCCA;

            pEngine->sendBracketOrder(&pEntry, &pBracketParams, &iCode);
        }
        else
        {
            RApi::ModifyLimitOrderParams pParams;
            pParams.dPrice = bid - TH;
            pParams.iQty = 1;
            pParams.pAccount = &pAccount;
            pParams.sEntryType = RApi::sORDER_ENTRY_TYPE_AUTO;
            pParams.sOrderNum = bOrderNum;
            pParams.sTicker = sTicker;
            pParams.sExchange = sExchange;

            pEngine->modifyOrder(&pParams, &iCode);
        }
        bOrderGood = false;
    }
    last_bid = bid;

    *aiCode = API_OK;
    return OK;
}

int Callbacks::BestAskQuote(RApi::AskInfo *pInfo, void *pContext, int *aiCode)
{
    double ask = pInfo->dPrice;
    if (ask != last_ask && aOrderGood)
    {
        int iCode;
        if (aOrderNum.iDataLen == 0)
        {
            RApi::OrderParams pEntry;
            pEntry.dPrice = ask + TH;
            pEntry.iQty = 1;
            pEntry.pAccount = &pAccount;
            pEntry.sBuySellType = RApi::sBUY_SELL_TYPE_SELL;
            pEntry.sOrderType = RApi::sORDER_TYPE_LIMIT;
            pEntry.sDuration = RApi::sORDER_DURATION_GTC;
            pEntry.sEntryType = RApi::sORDER_ENTRY_TYPE_AUTO;
            pEntry.sExchange = sExchange;
            pEntry.sTicker = sTicker;
            pEntry.sTradeRoute = sTradeRoute;
            pEntry.sTag = {"A", 1};

            RApi::BracketTier target, stop;
            target.iTicks = TH * 4;
            target.llQty = 1;
            stop.iTicks = TH * 4;
            stop.llQty = 1;

            RApi::BracketParams pBracketParams;
            pBracketParams.asStopArray = &stop;
            pBracketParams.asTargetArray = &target;
            pBracketParams.iStopArrayLen = 1;
            pBracketParams.iTargetArrayLen = 1;
            pBracketParams.sBracketType = RApi::sBRACKET_TYPE_TARGET_AND_STOP;
            pBracketParams.sOrderOperationType = RApi::sORDER_OPERATION_TYPE_FOCCA;

            pEngine->sendBracketOrder(&pEntry, &pBracketParams, &iCode);
        }
        else
        {
            RApi::ModifyLimitOrderParams pParams;
            pParams.dPrice = ask + TH;
            pParams.iQty = 1;
            pParams.pAccount = &pAccount;
            pParams.sEntryType = RApi::sORDER_ENTRY_TYPE_AUTO;
            pParams.sOrderNum = aOrderNum;
            pParams.sTicker = sTicker;
            pParams.sExchange = sExchange;

            pEngine->modifyOrder(&pParams, &iCode);
        }
        aOrderGood = false;
    }
    last_ask = ask;

    *aiCode = API_OK;
    return OK;
}

int Callbacks::StatusReport(RApi::OrderStatusReport *pReport, void *pContext, int *aiCode)
{
    if (pReport->sTag.iDataLen > 0 && *pReport->sTag.pData == 'A')
    {
        aOrderGood = true;
        aOrderNum.iDataLen = pReport->sOrderNum.iDataLen;
        strncpy(acOrderNum, pReport->sOrderNum.pData, pReport->sOrderNum.iDataLen);
        acOrderNum[pReport->sOrderNum.iDataLen] = 0;

        std::cout << pReport->iSsboe << "." << pReport->iUsecs << "\t: Ask placed | " << acOrderNum << " | " << pReport->dPriceToFill << std::endl;
    }
    else if (pReport->sTag.iDataLen > 0 && *pReport->sTag.pData == 'B')
    {
        bOrderGood = true;
        bOrderNum.iDataLen = pReport->sOrderNum.iDataLen;
        strncpy(bcOrderNum, pReport->sOrderNum.pData, pReport->sOrderNum.iDataLen);
        bcOrderNum[pReport->sOrderNum.iDataLen] = 0;

        std::cout << pReport->iSsboe << "." << pReport->iUsecs << "\t: Bid placed | " << bcOrderNum << " | " << pReport->dPriceToFill << std::endl;
    }
    *aiCode = API_OK;
    return OK;
}

int Callbacks::ModifyReport(RApi::OrderModifyReport *pReport, void *pContext, int *aiCode)
{
    if (pReport->sTag.iDataLen > 0 && *pReport->sTag.pData == 'A')
    {
        aOrderGood = true;
        std::cout << pReport->iSsboe << "." << pReport->iUsecs << "\t: Ask modified | " << acOrderNum << " | " << pReport->dPriceToFill << std::endl; 
    }
    else if (pReport->sTag.iDataLen > 0 && *pReport->sTag.pData == 'B')
    {
        bOrderGood = true;
        std::cout << pReport->iSsboe << "." << pReport->iUsecs << "\t: Bid modified | " << bcOrderNum << " | " << pReport->dPriceToFill << std::endl; 
    }
    *aiCode = API_OK;
    return OK;
}

int Callbacks::FillReport(RApi::OrderFillReport *pReport, void *pContext, int *aiCode)
{
    if (pReport->sTag.iDataLen > 0 && *pReport->sTag.pData == 'A')
    {
        aOrderGood = true;
        aOrderNum.iDataLen = 0;
        std::cout << pReport->iSsboe << "." << pReport->iUsecs << "\t: Ask filled | " << acOrderNum << " | " << pReport->dFillPrice << std::endl;
    }
    else if (pReport->sTag.iDataLen > 0 && *pReport->sTag.pData == 'B')
    {
        bOrderGood = true;
        bOrderNum.iDataLen = 0;
        std::cout << pReport->iSsboe << "." << pReport->iUsecs << "\t: Bid filled | " << bcOrderNum << " | " << pReport->dFillPrice << std::endl;
    }
    *aiCode = API_OK;
    return OK;
}

int Callbacks::FailureReport(RApi::OrderFailureReport *pReport, void *pContext, int *aiCode)
{
    if (pReport->sTag.iDataLen > 0 && *pReport->sTag.pData == 'A')
    {
        aOrderGood = true;
    }
    else if (pReport->sTag.iDataLen > 0 && *pReport->sTag.pData == 'B')
    {
        bOrderGood = true;
    }

    char msg[pReport->sStatus.iDataLen + 1];
    strncpy(msg, pReport->sStatus.pData, pReport->sStatus.iDataLen);
    msg[pReport->sStatus.iDataLen] = 0;

    std::cout << pReport->iSsboe << "." << pReport->iUsecs << "\t: Order failed | " << msg << std::endl;

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
    oParams.sAppName = {appname, (int)strlen(appname)};
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
        std::cerr << "login to ih failed, check user/pass or permissions" << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

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

    sTicker = {ticker, (int)strlen(ticker)};
    sExchange = {"CME", (int)strlen("CME")};
    sTradeRoute = {"globex", (int)strlen("globex")};

    int md_flags = (RApi::MD_PRINTS | RApi::MD_BEST);
    if (!pEngine->subscribe(&sExchange, &sTicker, md_flags, &iCode))
    {
        std::cerr << "error subscribing to market data: " << iCode << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    sleep(23460);

    delete pEngine;
    delete pCallbacks;
    delete pAdmCallbacks;
    return GOOD;
}