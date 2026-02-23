#include "FileManager.h"

FileManager::FileManager() 
{
}

void FileManager::init()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("[FM] [Error] could not mount LittleFS. Trying to format...");
        return;
    }
    Serial.println("[FM] LittleFS initalized.");
    Serial.printf("FS: %u / %u bytes\n",
                  LittleFS.usedBytes(),
                  LittleFS.totalBytes());
    Serial.println("Listing files:");
    FileManager::printFilesInDirectory("/", 1);
}

bool FileManager::exists(String filePath)
{
    return LittleFS.exists(filePath);
}

File FileManager::openFile(String filePath, bool write)
{
    if (!filePath.startsWith("/")) filePath = "/" + filePath;

    Serial.println("[FM] open " + filePath);

    if (!write && !LittleFS.exists(filePath))
    {
        Serial.println("[FM] " + filePath + " does not exist !");
        return File();
    }
    return LittleFS.open(filePath.c_str(), write ? "w" : "r");
}

void FileManager::printFilesInDirectory(String dirname, uint8_t levels)
{
    if (!dirname.startsWith("/"))
        dirname = "/" + dirname;

    std::vector<String> fileNameList;

    File root = LittleFS.open(dirname, "r");

    if (!root) Serial.println("[FM] Failed to open directory");
    else if (!root.isDirectory()) Serial.println("[FM] Not a directory");
    else 
    {
        File file = root.openNextFile();
        while (file)
        {
            if (file.isDirectory())
            {
                Serial.println("\t [DIR] " + String(file.name()));
                if (levels)
                {
                    FileManager::printFilesInDirectory(file.path(), levels - 1);
                }
            }
            else
            {
                String fileName = String(file.name());
                if (strcmp(dirname.c_str(), "/")) Serial.print("\t"); // indent if file is in dir
                Serial.print("\t");
                Serial.println(fileName + " (" + String(file.size()) + " bytes)");
            }
            file = root.openNextFile();
        }
    }
}

String FileManager::getCurrentConfigName()
{
    Preferences prefs;
    prefs.begin("YOLO");
    String configFileName;
    if (prefs.isKey("config")) configFileName = prefs.getString("config");
    else configFileName = "no_config";
    prefs.end();
    return configFileName;
}

String FileManager::getCurrentConfigNiceName()
{
    String name = getCurrentConfigName();
    name.replace("_", " ");

    String niceName = "";
    for (int i = 0; i < name.length(); i++)
    {
        if (i == 0 || name.charAt(i - 1) == ' ') niceName += (char)toUpperCase(name.charAt(i));
        else niceName += name.charAt(i);
    }
    return niceName;
}

void FileManager::setNewConfig(String configName)
{
    Preferences prefs;
    prefs.begin("YOLO");
    if (isValidConfigName(configName)) prefs.putString("config", configName);
    else
        prefs.putString("config", "default");
    prefs.end();
}

bool FileManager::deleteConfigFile(String configName)
{
    String path = "/" + String(ARDUINO_BOARD) + "/" + configName + ".json";
    // String path = "/config/"+configName+".json";
    if (LittleFS.exists(path) && LittleFS.remove(path))
        return true;
    return false;
}

File FileManager::openConfigFile(String name)
{
    if (!LittleFS.exists("/" +String(ARDUINO_BOARD)))
    {
        Serial.println("[FM] ERROR " + String(ARDUINO_BOARD) + " directory does not exist !");
        return File();
    }

    if (name == "") name = FileManager::getCurrentConfigName();
    else if (!FileManager::isValidConfigName(name))
    {
        Serial.println("[FM] ERROR " + name + " is not a valid config name !");
        setNewConfig("no_config");
        return File();
    }
    return FileManager::openFile("/" + String(ARDUINO_BOARD) + "/" + name + ".json");
    // return FileManager::openFile("/config/"+name+".json");
}

bool FileManager::isValidConfigName(String name)
{
    std::vector<String> configs = FileManager::getAvailableConfigNames();
    return std::find(configs.begin(), configs.end(), name) != configs.end();
}

std::vector<String> FileManager::getAvailableConfigNames()
{
    std::vector<String> fileNameList;

    File root = LittleFS.open("/" + String(ARDUINO_BOARD), "r");
    // File root = LittleFS.open("/config", "r");

    if (!root) Serial.println("[FM] Failed to open directory");
    else if (!root.isDirectory()) Serial.println("[FM] Not a directory");
    else
    {
        File file = root.openNextFile();
        while (file)
        {
            String fileName = String(file.name());
            
            if (fileName.endsWith(".json")) fileNameList.emplace_back(fileName.substring(0, fileName.length() - 5));
            file = root.openNextFile();
        }
    }
    return fileNameList;
}

int FileManager::indexOfCred(String ssid)
{
    Preferences prefs;
    prefs.begin("wifi_creds");
    int index = -1;
    for (int i = 0; i < NUM_CREDENTIALS; i++)
    {
        if (prefs.isKey(("ssid-" + String(i)).c_str()) && prefs.getString(("ssid-" + String(i)).c_str()).equals(ssid))
        {
            index = i;
            break;
        }
    }
    prefs.end();
    return index;
}

bool FileManager::setWifiCredentials(String ssid)
{
    int index = indexOfCred(ssid);
    if (index < 0)
        return false;

    Serial.println("[FM] set ssid " + String(index));
    Preferences prefs;
    prefs.begin("wifi_creds");
    prefs.putInt("currentIndex", index);
    prefs.end();
    return true;
}

bool FileManager::deleteWifiCredentials(String ssid)
{
    int index = indexOfCred(ssid);
    if (index < 0)
        return false;

    Serial.println("[FM] delete ssid: " + String(index));
    Preferences prefs;
    prefs.begin("wifi_creds");

    for (int i = index; i < NUM_CREDENTIALS; i++)
    {
        // override with next one if it exists, otherwise erase it and break
        if (prefs.isKey(("ssid-" + String(i + 1)).c_str()) && prefs.isKey(("pwd-" + String(i + 1)).c_str()))
        {
            // Serial.println("override "+String(i)+" with "+prefs.getString(("ssid-"+String(i+1)).c_str()));
            prefs.putString(("ssid-" + String(i)).c_str(), prefs.getString(("ssid-" + String(i + 1)).c_str()));
            prefs.putString(("pwd-" + String(i)).c_str(), prefs.getString(("pwd-" + String(i + 1)).c_str()));
        }
        else
        {
            // Serial.println("erase "+String(i));
            prefs.remove(("ssid-" + String(i)).c_str());
            prefs.remove(("pwd-" + String(i)).c_str());
            break;
        }
    }
    prefs.end();
    FileManager::printWifiCredentials();
    return true;
}

String FileManager::currentSSID()
{
    Preferences prefs;
    prefs.begin("wifi_creds");
    int i = prefs.getInt("currentIndex");
    String ssid = prefs.getString(("ssid-" + String(i)).c_str(), "");
    prefs.end();
    return ssid;
}

String FileManager::currentPwd()
{
    Preferences prefs;
    prefs.begin("wifi_creds");
    int i = prefs.getInt("currentIndex");
    String pwd = prefs.getString(("pwd-" + String(i)).c_str(), "");
    prefs.end();
    return pwd;
}

String FileManager::getSSID(int index)
{
    if (index < 0 || index > NUM_CREDENTIALS)
        return "";
    Preferences prefs;
    prefs.begin("wifi_creds");
    String ssid = prefs.isKey(("ssid-" + String(index)).c_str()) ? prefs.getString(("ssid-" + String(index)).c_str()) : "";
    prefs.end();
    return ssid;
}

void FileManager::printWifiCredentials()
{
    Serial.println("");
    Serial.println("[FM] registered wifi credentials (current: " + currentSSID() + ")");
    Preferences prefs;
    prefs.begin("wifi_creds");
    for (int i = 0; i < NUM_CREDENTIALS; i++)
        if (prefs.isKey(("ssid-" + String(i)).c_str()) && prefs.isKey(("pwd-" + String(i)).c_str()))
            Serial.println("\t" + String(i) + ": " + prefs.getString(("ssid-" + String(i)).c_str()) + " / " + prefs.getString(("pwd-" + String(i)).c_str()));
    prefs.end();
}

void FileManager::clearWifiCredentials()
{
    Preferences prefs;
    prefs.begin("wifi_creds");
    prefs.clear();
    prefs.end();
}

void FileManager::registerWifiCredentials(String ssid, String pwd)
{
    Serial.println("[FM] new wifi credentials: " + ssid + " / " + pwd);
    Preferences prefs;
    prefs.begin("wifi_creds");

    int index = indexOfCred(ssid); // check if this ssid was already registered
    if (index < 0)                 // if not, increment the last index
    {
        index = prefs.getInt("lastIndex", -1);
        index++;
        if (index == NUM_CREDENTIALS)
            index = 0;
        prefs.putInt("lastIndex", index);
    }
    prefs.putString(("ssid-" + String(index)).c_str(), ssid.c_str());
    prefs.putString(("pwd-" + String(index)).c_str(), pwd.c_str());
    prefs.putInt("currentIndex", index);

    prefs.end();
}