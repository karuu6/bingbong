#ifndef CONFIG_H
#define CONFIG_H

#define BAD 1
#define GOOD 0

char *appname = "kasu:bingbong";
char *version = "0.0.1";
char *logfile = "log/bingbong.log";

char *Envp[9] = {
    "MML_DMN_SRVR_ADDR=ritpz01001.01.rithmic.com:65000~ritpz01000.01.rithmic.com:65000~ritpz01001.01.rithmic.net:65000~ritpz01000.01.rithmic.net:65000~ritpz01001.01.theomne.net:65000~ritpz01000.01.theomne.net:65000~ritpz01001.01.theomne.com:65000~ritpz01000.01.theomne.com:65000",
    "MML_DOMAIN_NAME=rithmic_prod_01_dmz_domain",
    "MML_LIC_SRVR_ADDR=ritpz01000.01.rithmic.com:56000~ritpz01001.01.rithmic.com:56000~ritpz01000.01.rithmic.net:56000~ritpz01001.01.rithmic.net:56000~ritpz01000.01.theomne.net:56000~ritpz01001.01.theomne.net:56000~ritpz01000.01.theomne.com:56000~ritpz01001.01.theomne.com:56000",
    "MML_LOC_BROK_ADDR=ritpz01000.01.rithmic.com:64100",
    "MML_LOGGER_ADDR=ritpz01000.01.rithmic.com:45454~ritpz01000.01.rithmic.net:45454~ritpz01000.01.theomne.net:45454~ritpz01000.01.theomne.com:45454",
    "MML_LOG_TYPE=log_net",
    "MML_SSL_CLNT_AUTH_FILE=rithmic_ssl_cert_auth_params",
    "USER=kasu",
    0
};
char *MdConnectPt = "login_agent_tp01_sumc";
char *TsConnectPt = "login_agent_prodc";
char *IhConnectPt = "login_agent_historyc";

#endif
