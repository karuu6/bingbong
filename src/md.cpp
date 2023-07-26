#include <cmath>
#include <cstring>
#include <unistd.h>
#include <iostream>

#include "config.h"
#include "RApiPlus.h"

int lastMs;
int lastTime;
int n_trades;
int MdLoggedIn = 0;
int IhLoggedIn = 0;
bool ReplayDone = false;

bool cmp(tsNCharcb a, tsNCharcb b)
{
    return (a.iDataLen == b.iDataLen && memcmp(a.pData, b.pData, a.iDataLen) == 0);
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
    virtual int Bar(RApi::BarInfo *pInfo, void *pContext, int *aiCode);
    virtual int BarReplay(RApi::BarReplayInfo *pInfo, void *pContext, int *aiCode);
};

int AdmCallbacks::Alert(RApi::AlertInfo *pInfo, void *pContext, int *aiCode)
{
    //std::cout << "AdmCallback " << pInfo->iAlertType << ": " << pInfo->sMessage.pData << std::endl;
    *aiCode = API_OK;
    return OK;
}

int Callbacks::Alert(RApi::AlertInfo *pInfo, void *pContext, int *aiCode)
{
    //std::cout << "RCallback " << pInfo->iAlertType << ": " << pInfo->sMessage.pData << std::endl;
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
    else if (pInfo->iConnectionId == RApi::INTRADAY_HISTORY_CONNECTION_ID)
    {
        if (pInfo->iAlertType == RApi::ALERT_LOGIN_FAILED)
        {
            IhLoggedIn = -1;
        }
        else if (pInfo->iAlertType == RApi::ALERT_LOGIN_COMPLETE)
        {
            IhLoggedIn = 1;
        }
    }

    *aiCode = API_OK;
    return OK;
}

int Callbacks::Bar(RApi::BarInfo *pInfo, void *pContext, int *aiCode)
{
    n_trades++;
    lastMs = pInfo->iEndUsecs;
    lastTime = pInfo->iEndSsboe;

    std::cout 
        << pInfo->iStartSsboe
        << "," << pInfo->dOpenPrice
        << "," << pInfo->dHighPrice
        << "," << pInfo->dLowPrice
        << "," << pInfo->dClosePrice
        << "," << pInfo->llVolume
    << std::endl;
    *aiCode = API_OK;
    return OK;
}

int Callbacks::BarReplay(RApi::BarReplayInfo *pInfo, void *pContext, int *aiCode)
{
    ReplayDone = true;
    *aiCode = API_OK;
    return OK;
}

int main(int argc, char **argv)
{
    if (argc < 7)
    {
        std::cerr << "usage: " << argv[0] << " username password exchange ticker start end" << std::endl;
        return BAD;
    }

    RApi::REngine *pEngine;
    Callbacks *pCallbacks;
    AdmCallbacks *pAdmCallbacks;

    int aiCode;
    // char **envp;
    // char *MdConnectPt;
    // char *IhConnectPt;

    char *username = argv[1];
    char *password = argv[2];
    char *exchange = argv[3];
    char *ticker = argv[4];
    int start = std::atoi(argv[5]);
    int end = std::atoi(argv[6]);

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
    oParams.sAppName = {"md", (int)strlen("md")};
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
    oLoginParams.sIhUser = {username, (int)strlen(username)};
    oLoginParams.sIhPassword = {password, (int)strlen(password)};
    oLoginParams.sIhCnnctPt = {IhConnectPt, (int)strlen(IhConnectPt)};

    if (!pEngine->login(&oLoginParams, &aiCode))
    {
        std::cerr << "error logging into md/ih: " << aiCode << std::endl;
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
        std::cerr << "error logging into md" << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    while (IhLoggedIn == 0)
    {
        sleep(1);
    }

    if (IhLoggedIn == -1)
    {
        std::cerr << "error logging into hs" << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    std::cout
        << "time,"
        << "open,"
        << "high,"
        << "low,"
        << "close,"
        << "volume"
    << std::endl;

    RApi::ReplayBarParams params;
    params.sTicker = {ticker, (int)strlen(ticker)};
    params.sExchange = {exchange, (int)strlen(exchange)};
    params.iType = RApi::BAR_TYPE_MINUTE;
    params.iSpecifiedMinutes = 1;
    params.iEndSsboe = end;
    lastTime = start;
    lastMs = 0;

    runitback:
    n_trades = 0;
    ReplayDone = false;
    params.iStartSsboe = lastTime;
    params.iStartUsecs = lastMs;

    if (!pEngine->replayBars(&params, &aiCode))
    {
        std::cerr << "error requesting historical trades: " << aiCode << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    while (!ReplayDone)
    {
        sleep(1);
    }

    if (n_trades == 10000)
    {
        goto runitback;
    }
    else if (n_trades == 0)
    {
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    delete pEngine;
    delete pCallbacks;
    delete pAdmCallbacks;
    return GOOD;
}