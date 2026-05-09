#pragma once
#include "common/Dependencies.h"
#include "common/Component.h"

// TODO en faire un module ! => les config et credentials sont des StringListParameters
#define NUM_CREDENTIALS 5
class FileManager
{
    public:
        FileManager();
        static void init();

        static void printFilesInDirectory(String dirname, uint8_t levels);
        static File openFile(String fileName, bool write = false);
        static bool exists(String fileName);

        static std::vector<String> getAvailableConfigNames();
        static String getCurrentConfigName();
        static String getCurrentConfigNiceName();
        static bool isValidConfigName(String name);

        static File openConfigFile(String name = "");
        static void setNewConfig(String configName);
        static bool deleteConfigFile(String configName);
        
        static void printWifiCredentials();
        static void registerWifiCredentials(String ssid, String pwd);
        static void clearWifiCredentials();
        static bool setWifiCredentials(String ssid);
        static bool deleteWifiCredentials(String ssid);
        static String currentSSID();
        static String currentPwd();
        static String getSSID(int index);

        protected:
        static int indexOfCred(String ssid);
};