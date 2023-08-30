#include <queue>
#include <cstring>
#include <unistd.h>
#include <iostream>

#include "config.h"
#include "RApiPlus.h"

#define A 1e-3
#define QTY 1
#define MXCP 3
#define SZ 5

#define M0 5.951e-03
#define M1 -5.344e-04
#define M2 9.351e-03

int MdLoggedIn = 0;
int TsLoggedIn = 0;

RApi::REngine *pEngine;
RApi::MarketOrderParams buyOrder;
RApi::MarketOrderParams sellOrder;

int cur_pos = 0;
double margin = 0;

long vl = 0;
double opx = 0;
double hpx = 0;
double lpx = 0;
double cpx = 0;
long double ts = 0;

double Q[4] = {0, 0, 0, 0};
double l_f[4] = {0, 0, 0, 0};

void OnRangeBar();

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
    if (opx == 0)
    {
        opx = pInfo->dPrice;
        hpx = pInfo->dPrice;
        lpx = pInfo->dPrice;
        cpx = pInfo->dPrice;
        ts = pInfo->iSsboe;
        ts = (ts * 1000 * 1000) + pInfo->iUsecs;
    }
    vl += pInfo->llSize;
    cpx = pInfo->dPrice;
    hpx = std::max(hpx, pInfo->dPrice);
    lpx = std::min(lpx, pInfo->dPrice);

    if (hpx - lpx >= SZ)
    {
        long double cts = pInfo->iSsboe;
        cts = (cts * 1000 * 1000) + pInfo->iUsecs;
        ts = (cts - ts) / 1e6;
        
        OnRangeBar();
        
        vl = 0;
        ts = 0;
        opx = 0;
        hpx = 0;
        lpx = 0;
        cpx = 0;
    }

    *aiCode = API_OK;
    return OK;
}

void OnRangeBar()
{
    double f0 = (cpx - opx) / SZ;
    double f1 = (hpx - cpx) / SZ;
    double f2 = (cpx - lpx) / SZ;
    double f3 = vl / (ts * 100);
    double dotp = (l_f[0] * Q[0]) + (l_f[1] * Q[1]) + (l_f[2] * Q[2]) + (l_f[3] * Q[3]);

    Q[0] += A * (dotp - f0) * l_f[0];
    Q[1] += A * (dotp - f0) * l_f[1];
    Q[2] += A * (dotp - f0) * l_f[2];
    Q[3] += A * (dotp - f0) * l_f[3];

    l_f[0] = f0;
    l_f[1] = f1;
    l_f[2] = f2;
    l_f[3] = f3;

    dotp = (l_f[0] * Q[0]) + (l_f[1] * Q[1]) + (l_f[2] * Q[2]) + (l_f[3] * Q[3]);
    double z = std::abs(Q[0]) + std::abs(Q[1]) + std::abs(Q[2]) + std::abs(Q[3]);

    int iCode;
    if (dotp > M0 && z > M2 && cur_pos < MXCP)
    {
        pEngine->sendOrder(&buyOrder, &iCode);
        // cur_pos += 1;
        // margin -= cpx;
    }
    else if (dotp < M1 && z > M2 && cur_pos > -MXCP)
    {
        pEngine->sendOrder(&sellOrder, &iCode);
        // cur_pos -= 1;
        // margin += cpx;
    }
    else if (dotp < M1 && z < M2 && cur_pos < MXCP)
    {
        pEngine->sendOrder(&buyOrder, &iCode);
        // cur_pos += 1;
        // margin -= cpx;
    }
    else if (dotp > M0 && z < M2 && cur_pos > -MXCP)
    {
        pEngine->sendOrder(&sellOrder, &iCode);
        // cur_pos -= 1;
        // margin += cpx;
    }

    std::cout << "new bar: o|" << opx << " h|" << hpx << " l|" << lpx << " c|" << cpx << " v|" << vl << " t|" << ts << std::endl;
    std::cout << "pnl: " << margin + cur_pos * cpx << " pos: " << cur_pos << std::endl << std::endl;
}

int Callbacks::LineUpdate(RApi::LineInfo *pInfo, void *pContext, int *aiCode)
{
    if (pInfo->bAvgFillPriceFlag) {
        char side = *pInfo->sBuySellType.pData;
        if (side == 'B')
        {
            cur_pos += pInfo->llFilled;
            margin -= pInfo->llFilled * pInfo->dAvgFillPrice;
        }
        else
        {
            cur_pos -= pInfo->llFilled;
            margin += pInfo->llFilled * pInfo->dAvgFillPrice;
        }
        std::cout << side << " | " << pInfo->dAvgFillPrice << std::endl << std::endl;
    }

    *aiCode = API_OK;
    return OK;
}

int main(int argc, char **argv)
{
    if (argc < 5)
    {
        std::cerr << "usage: " << argv[0] << " username password tickerMd tickerOrder" << std::endl;
        return BAD;
    }

    int iCode;
    char *username = argv[1];
    char *password = argv[2];
    char *tickerMd = argv[3];
    char *tickerOrder = argv[4];

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
    oParams.sAppName = {"q", (int)strlen("q")};
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

    tsNCharcb sTickerMd = {tickerMd, (int)strlen(tickerMd)};
    tsNCharcb sTickerOrder = {tickerOrder, (int)strlen(tickerOrder)};

    tsNCharcb sExchange = {"CME", (int)strlen("CME")};
    tsNCharcb sTradeRoute = {"globex", (int)strlen("globex")};
    
    buyOrder.iQty = QTY;
    buyOrder.sTicker = sTickerOrder;
    buyOrder.sExchange = sExchange;
    buyOrder.pAccount = &pAccount;
    buyOrder.sTradeRoute = sTradeRoute;
    buyOrder.sDuration = RApi::sORDER_DURATION_DAY;
    buyOrder.sBuySellType = RApi::sBUY_SELL_TYPE_BUY;
    buyOrder.sEntryType = RApi::sORDER_ENTRY_TYPE_AUTO;

    sellOrder.iQty = QTY;
    sellOrder.sTicker = sTickerOrder;
    sellOrder.sExchange = sExchange;
    sellOrder.pAccount = &pAccount;
    sellOrder.sTradeRoute = sTradeRoute;
    sellOrder.sDuration = RApi::sORDER_DURATION_DAY;
    sellOrder.sBuySellType = RApi::sBUY_SELL_TYPE_SELL;
    sellOrder.sEntryType = RApi::sORDER_ENTRY_TYPE_AUTO;

    int iFlags = (RApi::MD_PRINTS);

    if (!pEngine->subscribe(&sExchange, &sTickerMd, iFlags, &iCode))
    {
        std::cerr << "error subscribing to market data: " << iCode << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    sleep(23400);

    tsNCharcb exitType = RApi::sORDER_ENTRY_TYPE_AUTO;
    if (!pEngine->exitPosition(&pAccount, NULL, NULL, &exitType, NULL, NULL, &iCode))
    {
        std::cerr << "error exiting all positions: " << iCode << std::endl;
    }

    delete pEngine;
    delete pCallbacks;
    delete pAdmCallbacks;
    return GOOD;
}