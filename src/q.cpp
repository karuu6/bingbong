#include <cmath>
#include <limits>
#include <cstring>
#include <unistd.h>
#include <iostream>

#include "config.h"
#include "RApiPlus.h"

#define ROWS 10000
#define SIG_WIDTH 5
#define FEAT_WIDTH 7

#define WIN 1 // MINUTES
#define FWD 5

int IhLoggedIn = 0;
int MdLoggedIn = 0;
RApi::REngine *pEngine;

unsigned int ix = 0;
double signals[ROWS][SIG_WIDTH];
double features[ROWS][FEAT_WIDTH];

double max_feat(unsigned int start, unsigned int end, unsigned int col);
double min_feat(unsigned int start, unsigned int end, unsigned int col);

double max_feat(unsigned int start, unsigned int end, unsigned int col)
{
    if (col >= FEAT_WIDTH || start < 0 || end >= ROWS || end < start)
    {
        return nan("");
    }
    double mx = std::numeric_limits<double>::min();
    for (unsigned int j = start; j <= end; j++)
    {
        mx = std::max(mx, features[j][col]);
    }
    return mx;
}

double min_feat(unsigned int start, unsigned int end, unsigned int col)
{
    if (col >= FEAT_WIDTH || start < 0 || end >= ROWS || end < start)
    {
        return nan("");
    }
    double mn = std::numeric_limits<double>::max();
    for (unsigned int j = start; j <= end; j++)
    {
        mn = std::min(mn, features[j][col]);
    }
    return mn;
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
    virtual int Bar(RApi::BarInfo *pInfo, void *pContext, int *aiCode);
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
    features[ix][0] = pInfo->dHighPrice;
    features[ix][1] = pInfo->dLowPrice;
    features[ix][2] = pInfo->dClosePrice;
    if (ix >= 8)
    {
        features[ix][3] = (max_feat(ix - 8, ix, 0) + min_feat(ix - 8, ix, 1)) / 2;
    }
    if (ix >= 25)
    {
        features[ix][4] = (max_feat(ix - 25, ix, 0) + min_feat(ix - 25, ix, 1)) / 2;
    }
    if (ix >= 50)
    {
        features[ix][5] = (features[ix - 25][3] + features[ix - 25][4])/ 2;
    }
    if (ix >= 76)
    {
        features[ix][6] = (max_feat(ix - 76, ix - 25, 0) + min_feat(ix - 76, ix - 25, 1)) / 2;

        signals[ix][0] = (features[ix][3] - features[ix][4]) / features[ix][4];
        signals[ix][1] = (pInfo->dClosePrice - features[ix][4]) / features[ix][4];
        signals[ix][2] = (pInfo->dClosePrice - std::max(features[ix][5], features[ix][6])) / std::max(features[ix][5], features[ix][6]);
        signals[ix][3] = (pInfo->dClosePrice - features[ix][3]) / features[ix][3];
        signals[ix][4] = (features[ix][5] - features[ix][6]) / features[ix][6];
    }

    std::cout << ix << std::endl;
    for (unsigned int j = 0; j < FEAT_WIDTH; j++)
    {
        std::cout << features[ix][j] << "\t";
    }
    std::cout << std::endl;
    for (unsigned int j = 0; j < SIG_WIDTH; j++)
    {
        std::cout << signals[ix][j] << "\t";
    }
    std::cout << std::endl;

    ix++;
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
    oLoginParams.sIhUser = {username, (int)strlen(username)};
    oLoginParams.sIhPassword = {password, (int)strlen(password)};
    oLoginParams.sIhCnnctPt = {IhConnectPt, (int)strlen(IhConnectPt)};

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

    tsNCharcb sTicker = {ticker, (int)strlen(ticker)};
    tsNCharcb sExchange = {"CME", (int)strlen("CME")};

    RApi::BarParams bar_params;
    bar_params.sTicker = sTicker;
    bar_params.sExchange = sExchange;
    bar_params.iSpecifiedMinutes = WIN;
    bar_params.iType = RApi::BAR_TYPE_MINUTE;

    if (!pEngine->subscribeBar(&bar_params, &iCode))
    {
        std::cerr << "error subscribing to market data: " << iCode << std::endl;
        delete pEngine;
        delete pCallbacks;
        delete pAdmCallbacks;
        return BAD;
    }

    std::cout << "armed..." << std::endl;
    sleep(23400);

    delete pEngine;
    delete pCallbacks;
    delete pAdmCallbacks;
    return GOOD;
}