#include "gui_networkMenu.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "../gui.h"
#include "../gui_confirm.h"
#include "../gui_keyboard.h"
#include "../../lang.h"
#include <cassert> // assert
#include <iostream>
#include <unistd.h>
#ifndef __APPLE__
#include <wait.h>
#endif

using namespace std;

enum { Text=0, SSID, PW, Blank1, WriteFile, Blank2, InitNetwork };

//*******************************
// GuiNetworkMenu::init
//*******************************
void GuiNetworkMenu::init() {
    GuiMenuBase::init();    // call the base init

    // read from ssid.cfg if it exists
    readCfgFile();

    fill();
}

//*******************************
// GuiNetworkMenu::readCfgFile
//*******************************
void GuiNetworkMenu::readCfgFile() {
    // read from ssid.cfg if it exists
    string cfgPath = getCfgPath();
    if (DirEntry::exists(cfgPath)) {
        ifstream ssidFile;
        ssidFile.open(cfgPath.c_str(), ios::binary);

        getline(ssidFile, ssid);
        Util::removeCRLFFromString(ssid);

        getline(ssidFile, password);
        Util::removeCRLFFromString(password);

        ssidFile.close();
    }
    else {
        ssid = "";
        password = "";
    }
}

//*******************************
// GuiNetworkMenu::writeCfgFile
//*******************************
void GuiNetworkMenu::writeCfgFile() {
    string cfgPath = getCfgPath();
    ofstream ssidFile(cfgPath, ios_base::trunc);
    ssidFile << ssid << endl;
    ssidFile << password << endl;
    ssidFile.flush();
    ssidFile.close();
}

//*******************************
// GuiNetworkMenu::initializeWifi
//*******************************
void GuiNetworkMenu::initializeWifi() {
    string runPath = getRunPath();
    if (DirEntry::exists(runPath)) {
        cout << "Attempting to initialize wifi networking" << endl;
        std::vector<const char *> argvNew;
        argvNew.push_back(runPath.c_str());
        argvNew.push_back(nullptr);

        for (const char *s:argvNew) {
            if (s != nullptr) {
                cout << s << " ";
            }
        }
        cout << endl;


        int pid = fork();
        if (!pid) {
            execvp(runPath.c_str(), (char **) argvNew.data());
        }
        waitpid(pid, NULL, 0);
        usleep(3 * 1000);
    }
}

//*******************************
// GuiNetworkMenu::getSSID
//*******************************
string GuiNetworkMenu::getSSID() {
    string ret = getSSIDfrom_ssidcfg();
    if (ret == "")
        ret = getSSIDfrom_wpa_supplicant();

    return ret;
}

//*******************************
// GuiNetworkMenu::getSSIDfrom_ssidcfg
//*******************************
string GuiNetworkMenu::getSSIDfrom_ssidcfg() {
    string ret;
    // read from ssid.cfg if it exists
    string cfgPath = getCfgPath();
    if (DirEntry::exists(cfgPath)) {
        ifstream ssidFile;
        ssidFile.open(cfgPath.c_str(), ios::binary);

        getline(ssidFile, ret);
        Util::removeCRLFFromString(ret);

        ssidFile.close();
    }

        return ret;
}

//*******************************
// GuiNetworkMenu::getSSIDfrom_wpa_supplicant
//*******************************
string GuiNetworkMenu::getSSIDfrom_wpa_supplicant() {
    string ret;
    string wpaPath = getWpaSupplicantPath();
    if (DirEntry::exists(wpaPath)) {
        ifstream wpaFile;
        wpaFile.open(wpaPath.c_str(), ios::binary);
        string line;
        string searchFor = "ssid=";
        while (getline(wpaFile, line)) {
            trim(line);
            if (!line.empty()) {
                auto pos = line.find(searchFor);
                if (pos != string::npos) {  // if found on this line
                    // get the rest of the line
                    ret = line.substr(pos + string(searchFor).size(), string::npos);
                    // remove the "" around the ssid
                    Util::removeCharsFromString(ret, "\"");
                    if (ret == "1")
                        ret = "";   // bleemsync will create an ssid of 1 if there is no ssid.cfg
                    break;
                }
            }
        }
        wpaFile.close();
    }

    return ret;
}

//*******************************
// GuiNetworkMenu::fill
//*******************************
void GuiNetworkMenu::fill() {
    lines.clear();
    lines.emplace_back(_("New Connection:"));
    lines.emplace_back(_("SSID:") + " " + ssid);
    lines.emplace_back(_("Password:") + " " + password);
    lines.emplace_back("");
    lines.emplace_back(_("Write new connect info to ssid.cfg file"));
    lines.emplace_back("");
    lines.emplace_back(_("Initialize Network using prior connect info"));
}

//*******************************
// GuiNetworkMenu::getStatusLine
//*******************************
// returns the status line at the bottom
string GuiNetworkMenu::getStatusLine() {
    switch (selected) {
        case Text:
            return "   |@O| " + _("Cancel") + " |";

        case SSID:
            return "   |@X| " + _("Edit SSID") +
                    "   |@O| " + _("Cancel") + " |";

        case PW:
            return "   |@X| " + _("Edit Password") +
                   "   |@O| " + _("Cancel") + " |";

        case Blank1:
            return "   |@O| " + _("Cancel") + " |";

        case WriteFile:
            return "   |@X| " + _("Write ssid.cfg file") +
                   "   |@O| " + _("Cancel") + " |";

        case Blank2:
            return "   |@O| " + _("Cancel") + " |";

        case InitNetwork:
            return "   |@X| " + _("Initialize Network") +
                   "   |@O| " + _("Cancel") + " |";

        default:
            assert(false);
            return "   |@O| " + _("Cancel") + " |";
    }
}

//*******************************
// GuiNetworkMenu::doCirclePressed
//*******************************
void GuiNetworkMenu::doCircle_Pressed() {
    Mix_PlayChannel(-1, gui->cancel, 0);
    menuVisible = false;
}

//*******************************
// GuiNetworkMenu::doCrossPressed
//*******************************
void GuiNetworkMenu::doCross_Pressed() {
    Mix_PlayChannel(-1, gui->cursor, 0);

    switch (selected) {
        case Text:
            break;

        case SSID: {
            GuiKeyboard *keyboard = new GuiKeyboard(renderer);
            keyboard->label = _("Enter SSID");
            keyboard->result = ssid;
            keyboard->show();
            string result = keyboard->result;
            bool cancelled = keyboard->cancelled;
            delete (keyboard);
            if (!cancelled) {
                ssid = result;
                fill();
            }
        }
            break;

        case PW: {
            GuiKeyboard *keyboard = new GuiKeyboard(renderer);
            keyboard->label = _("Enter Password");
            keyboard->result = password;
            keyboard->show();
            string result = keyboard->result;
            bool cancelled = keyboard->cancelled;
            delete (keyboard);
            if (!cancelled) {
                password = result;
                fill();
            }
        }
            break;

        case Blank1:
            break;

        case WriteFile: {
            writeCfgFile();
            string runPath = getRunPath();
            if (DirEntry::exists(runPath)) {
                auto confirm = new GuiConfirm(renderer);
                confirm->label = _("Initialize Wi-Fi Now?");
                confirm->show();
                bool result = confirm->result;
                delete confirm;
                if (result) {
#if defined(__x86_64__) || defined(_M_X64)
                    // nope
#else
                    initializeWifi();
#endif
                }
            }
            menuVisible = false;
            }
            break;

        case Blank2:
            break;

        case InitNetwork: {
            string runPath = getRunPath();
            if (DirEntry::exists(runPath)) {
#if defined(__x86_64__) || defined(_M_X64)
                // nope
#else
                initializeWifi();
#endif
            }
            }
            break;

        default:
            assert(false);
    }

    render();
}