/* 
 * File:   main.cpp
 * Author: screemer
 *
 * Created on 11 Dec 2018, 20:37
 */

#include <iostream>
#include "engine/database.h"
#include "engine/scanner.h"
#include "gui/gui.h"
#include "main.h"
#include "ver_migration.h"
#include "engine/coverdb.h"
#include "util.h"
#include <unistd.h>
#include "engine/GetGameDirHierarchy.h"

using namespace std;

#include "engine/memcard.h"
#include "lang.h"
#include "launcher/emu_interceptor.h"
#include "launcher/pcsx_interceptor.h"
#include "launcher/retboot_interceptor.h"
#include "engine/GetGameDirHierarchy.h"

Database * db;

//*******************************
// scanGames
//*******************************
int scanGames(string path, USBGames &allGames, string dbpath) {
    shared_ptr<Gui> gui(Gui::getInstance());
    shared_ptr<Scanner> scanner(Scanner::getInstance());

    if (!db->createInitialDatabase()) {
        cout << "Error creating db structure" << endl;

        return EXIT_FAILURE;
    };

    if (!db->truncate())
    {
        gui->drawText("ERROR IN DB");
        sleep(1);
        return EXIT_FAILURE;
    }

    scanner->scanUSBGamesDirectory(path);
    scanner->updateDB(gui->db);

    gui->drawText(_("Total:") + " " + to_string(scanner->games.size()) + " " + _("games scanned") + ".");
    sleep(1);
    scanner->games.clear();
    return (EXIT_SUCCESS);
}

//*******************************
// main
//*******************************
int main(int argc, char *argv[]) {
    shared_ptr<Lang> lang(Lang::getInstance());
    if (argc < 3) {
        cout << "USAGE: autobleem-gui /path/dbfilename.db /path/to/games" << endl;
        return EXIT_FAILURE;
    }
    shared_ptr<Gui> gui(Gui::getInstance());
    shared_ptr<Scanner> scanner(Scanner::getInstance());

    lang->load(gui->cfg.inifile.values["language"]);

    Coverdb *coverdb = new Coverdb();
    gui->coverdb = coverdb;

    db = new Database();
    if (!db->connect(argv[1])) {
        delete db;
        return EXIT_FAILURE;
    }
    gui->db=db;
    db->createInitialDatabase();
    db->createFavColumn();

    string dbpath = argv[1];
    string path = argv[2];

    Memcard *memcardOperation = new Memcard(path);
    memcardOperation->restoreAll(path + DirEntry::separator() + "!SaveStates");
    delete memcardOperation;

    bool prevFileExists = DirEntry::exists(DirEntry::getWorkingPath() + DirEntry::separator() + "autobleem.prev");

    bool thereAreGameFilesInGamesDir = scanner->areThereGameFilesInDir(path);
    if (thereAreGameFilesInGamesDir)
        scanner->copyGameFilesInGamesDirToSubDirs(path);

    GameSubDirRows gameRows = GameSubDir::scanGamesHierarchy(path);
    USBGames allGames;
    if (gameRows.size() > 0)
        allGames = gameRows[0]->allGames;
    Scanner::sortByFullPath(allGames);

    bool autobleemPrevOutOfDate = scanner->gamesDoNotMatchAutobleemprev(allGames, DirEntry::getWorkingPath() + DirEntry::separator() + "autobleem.prev");
    if (!prevFileExists || thereAreGameFilesInGamesDir || autobleemPrevOutOfDate) {
        scanner->forceScan = true;
    }

    gui->display(scanner->forceScan, path, db, false);

    while (gui->menuOption == MENU_OPTION_SCAN || gui->menuOption == MENU_OPTION_START) {

        gui->menuSelection();
        gui->saveSelection();
        if (gui->menuOption == MENU_OPTION_SCAN) {
            scanGames(path, allGames, dbpath);
            if (gui->forceScan) {
                gui->forceScan = false;
            } else {
                //break;
            }
        }

        if (gui->menuOption == MENU_OPTION_START) {
#if defined(__x86_64__) || defined(_M_X64)
            cout << "I'm sorry Dave I'm afraid I can't do that on this system." << endl;

            // just a temp to test exec
            EmuInterceptor *interceptor;
            if (gui->runningGame->foreign)
            {
                interceptor = new RetroArchInterceptor();
            } else {
                if (gui->emuMode == EMU_PCSX) {
                    interceptor = new PcsxInterceptor();
                } else {
                    interceptor = new RetroArchInterceptor();
                }
            }

            interceptor->memcardIn(gui->runningGame);
            interceptor->prepareResumePoint(gui->runningGame, gui->resumepoint);
            interceptor->execute(gui->runningGame, gui->resumepoint );
            interceptor->memcardOut(gui->runningGame);
            delete (interceptor);

            SDL_InitSubSystem(SDL_INIT_JOYSTICK);

            usleep(300*1000);
            gui->runningGame.reset();    // replace with shared_ptr pointing to nullptr
            gui->startingGame = false;

            gui->display(false, path, db, true);

#else
            cout << "Starting game" << endl;
            gui->finish();

            int numtimesopened, frequency, channels;
            Uint16 format;
            numtimesopened=Mix_QuerySpec(&frequency, &format, &channels);
            for (int i=0;i<numtimesopened;i++)
            {
                Mix_CloseAudio();
            }
            numtimesopened=Mix_QuerySpec(&frequency, &format, &channels);

            for (SDL_Joystick* joy:gui->joysticks) {
                if (SDL_JoystickGetAttached(joy)) {
                    SDL_JoystickClose(joy);
                }
            }
            SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

            gui->saveSelection();
            EmuInterceptor *interceptor;
            if (gui->runningGame->foreign)
            {
                interceptor = new RetroArchInterceptor();
            } else {
                if (gui->emuMode == EMU_PCSX) {
                    interceptor = new PcsxInterceptor();
                } else {
                    interceptor = new RetroArchInterceptor();
                }
            }

            interceptor->memcardIn(gui->runningGame);
            interceptor->prepareResumePoint(gui->runningGame, gui->resumepoint);
            interceptor->execute(gui->runningGame, gui->resumepoint );
            interceptor->memcardOut(gui->runningGame);
            delete (interceptor);

            SDL_InitSubSystem(SDL_INIT_JOYSTICK);

            usleep(300*1000);
            gui->runningGame.reset();    // replace with shared_ptr pointing to nullptr
            gui->startingGame = false;

            gui->display(false, path, db, true);
#endif
        }
    }
    db->disconnect();
    delete db;

    gui->logText(_("Loading ... Please Wait ..."));
    gui->finish();
    SDL_Quit();
    delete coverdb;

    exit(0);
}
