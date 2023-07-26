#include <cstring>
#include <unistd.h>
#include <iostream>

#include "config.h"
#include "RApiPlus.h"

int MdLoggedIn = 0;
int TsLoggedIn = 0;
int ReceivedAccount = 0;

RApi::REngine *pEngine;
const int   g_iMAX_LEN         = 256;
char        g_cAccountId[g_iMAX_LEN];
char        g_cFcmId[g_iMAX_LEN];
char        g_cIbId[g_iMAX_LEN];
char        g_cExchange[g_iMAX_LEN];
char        g_cTradeRoute[g_iMAX_LEN];

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
    virtual int AccountList(RApi::AccountListInfo *pInfo, void *pContext, int *aiCode);
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

int Callbacks::AccountList(RApi::AccountListInfo *pInfo, void *pContext, int *aiCode)
{
    if (pInfo->iArrayLen > 0)
    {
        RApi::AccountInfo *pAccount = &pInfo->asAccountInfoArray[0];
        memccpy(g_cAccountId, pAccount->sAccountId.pData, 0, g_iMAX_LEN);
        memccpy(g_cIbId, pAccount->sIbId.pData, 0, g_iMAX_LEN);
        memccpy(g_cFcmId, pAccount->sFcmId.pData, 0, g_iMAX_LEN);
        ReceivedAccount = 1;
    }
    else
    {
        ReceivedAccount = -1;
    }
    *aiCode = API_OK;
    return OK;
}

int Callbacks::TradePrint(RApi::TradeInfo *pInfo, void *pContext, int *aiCode)
{
    std::cout << pInfo->dPrice << " " << pInfo->llSize << std::endl;
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
    oParams.sAppName = {"luper", (int)strlen("luper")};
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

    while (ReceivedAccount == 0)
    {
        sleep(1);
    }
    if (ReceivedAccount == -1)
    {
        std::cerr << "didn't receive any accounts..." << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }


    std::cout << g_cAccountId << std::endl;
    std::cout << g_cIbId << std::endl;
    std::cout << g_cFcmId << std::endl;

    tsNCharcb sExchange = {"CME", 3};
    tsNCharcb sTradeRoute = {"globex", 6};
    tsNCharcb sTicker = {ticker, (int)strlen(ticker)};

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