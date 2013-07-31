#include "PlayerCreateCharacter.h"

#include "Engine.h"
#include "Render.h"
#include "ActorPlayer.h"
#include "Input.h"
#include "MenuInputHandler.h"
#include "TextFormatting.h"

const int NR_TRAITS = 2;
const int NR_SKILLS = 2;

void PlayerCreateCharacter::run() {
  //------------------------------------------------------ TRAITS AND SKILLS
  vector<PlayerBon_t> bonsTraits;
  vector<PlayerBon_t> bonsSkills;

  for(int i = 0; i < endOfPlayerBons; i++) {
    PlayerBon_t bon = PlayerBon_t(i);
    if(eng->playerBonHandler->getBonType(bon) == playerBonType_trait) {
      bonsTraits.push_back(bon);
    } else {
      bonsSkills.push_back(bon);
    }
  }

  int nrTraitsPicked = 0;
  int nrSkillsPicked = 0;

  bool isDonePickingBons = false;

  MenuBrowser browser(bonsTraits.size(), bonsSkills.size());
  draw(bonsTraits, bonsSkills, browser, isDonePickingBons);
  while(isDonePickingBons == false) {
    const MenuAction_t action = eng->menuInputHandler->getAction(browser);
    switch(action) {
      case menuAction_browsed: {
        draw(bonsTraits, bonsSkills, browser, isDonePickingBons);
      } break;

      case menuAction_canceled: {} break;

      case menuAction_selected: {
        const Pos browserPos = browser.getPos();
        if(browserPos.x == 0) {
          const PlayerBon_t bon = bonsTraits.at(browser.getPos().y);
          if(nrTraitsPicked < NR_TRAITS &&
              eng->playerBonHandler->isBonPicked(bon) == false) {
            eng->playerBonHandler->pickBon(bon);
            nrTraitsPicked++;
          }
        } else {
          const PlayerBon_t bon = bonsSkills.at(browser.getPos().y);
          if(nrSkillsPicked < NR_SKILLS &&
              eng->playerBonHandler->isBonPicked(bon) == false) {
            eng->playerBonHandler->pickBon(bon);
            nrSkillsPicked++;
          }
        }
        isDonePickingBons =
          nrTraitsPicked == NR_TRAITS && nrSkillsPicked == NR_SKILLS;
        draw(bonsTraits, bonsSkills, browser, isDonePickingBons);
      }
      break;

      case menuAction_selectedWithShift: {} break;
    }
  }

  //------------------------------------------------------ NAME
  PlayerEnterName playerEnterName(eng);
  playerEnterName.run(Pos(50, Y0_CREATE_CHARACTER + 2));
}

void PlayerCreateCharacter::draw(const vector<PlayerBon_t>& bonsTraits,
                                 const vector<PlayerBon_t>& bonsSkills,
                                 const MenuBrowser& browser,
                                 const bool IS_DONE_PICKING_BONS) const {
  eng->renderer->coverPanel(panel_screen);

  const unsigned int NR_BONS_TRAITS = bonsTraits.size();
  const unsigned int NR_BONS_SKILLS = bonsSkills.size();

  const int LEN_OF_LONGEST_LIST = max(NR_BONS_SKILLS, NR_BONS_SKILLS);

  const int Y0_TITLE = Y0_CREATE_CHARACTER;

  eng->renderer->drawTextCentered(
    "Describe yourself", panel_screen,
    Pos(MAP_X_CELLS_HALF, Y0_TITLE), clrWhite, clrBlack, true);

  const Pos browserPos = browser.getPos();

  const int X_TRAITS = 5; //14;

  const int Y0_BONUSES = Y0_TITLE + 4;
  int yPos = Y0_BONUSES;
  int lenOfWidestName = 0;
  eng->renderer->drawText(
    "Pick two traits", panel_screen, Pos(X_TRAITS - 2, yPos - 2), clrWhite);
  for(unsigned int i = 0; i < NR_BONS_TRAITS; i++) {
    const PlayerBon_t bon = bonsTraits.at(i);
    const string name     = eng->playerBonHandler->getBonTitle(bon);
    if(int(name.size()) > lenOfWidestName) {lenOfWidestName = name.size();}
    const bool IS_MARKED  = browserPos.x == 0 && browserPos.y == int(i) &&
                            IS_DONE_PICKING_BONS == false;
    const bool IS_PICKED  = eng->playerBonHandler->isBonPicked(bon);
    const SDL_Color clr   = IS_MARKED ? clrNosferatuTealLgt :
                            clrNosferatuTealDrk;
    eng->renderer->drawText(name, panel_screen, Pos(X_TRAITS, yPos), clr);
    if(IS_PICKED) {
      if(eng->config->isTilesMode) {
        eng->renderer->drawTile(
          tile_elderSign, panel_screen, Pos(X_TRAITS - 2, yPos), clrGray);
      } else {

      }
    }
    yPos++;
  }
  const Rect traitsBorders(X_TRAITS - 3, Y0_BONUSES - 1,
                           X_TRAITS + lenOfWidestName + 1,
                           Y0_BONUSES + LEN_OF_LONGEST_LIST + 1);
  eng->renderer->drawPopupBox(traitsBorders, panel_screen);

  const int X_SKILLS = X_TRAITS + lenOfWidestName + 5;

  yPos = Y0_BONUSES;
  lenOfWidestName = 0;
  eng->renderer->drawText(
    "Pick two skills", panel_screen, Pos(X_SKILLS - 2, yPos - 2), clrWhite);
  for(unsigned int i = 0; i < NR_BONS_SKILLS; i++) {
    const PlayerBon_t bon = bonsSkills.at(i);
    const string name     = eng->playerBonHandler->getBonTitle(bon);
    if(int(name.size()) > lenOfWidestName) {lenOfWidestName = name.size();}
    const bool IS_MARKED  = browserPos.x == 1 && browserPos.y == int(i) &&
                            IS_DONE_PICKING_BONS == false;
    const bool IS_PICKED  = eng->playerBonHandler->isBonPicked(bon);
    const SDL_Color clr   = IS_MARKED ? clrNosferatuTealLgt :
                            clrNosferatuTealDrk;
    eng->renderer->drawText(name, panel_screen, Pos(X_SKILLS, yPos), clr);
    if(IS_PICKED) {
      if(eng->config->isTilesMode) {
        eng->renderer->drawTile(
          tile_elderSign, panel_screen, Pos(X_SKILLS - 2, yPos), clrGray);
      } else {

      }
    }
    yPos++;
  }
  const Rect skillsBorders(X_SKILLS - 3, Y0_BONUSES - 1,
                           X_SKILLS + lenOfWidestName + 1,
                           Y0_BONUSES + LEN_OF_LONGEST_LIST + 1);
  eng->renderer->drawPopupBox(skillsBorders, panel_screen);

  //Draw effects description
  if(IS_DONE_PICKING_BONS == false) {
    const int Y0_DESCR = Y0_BONUSES + NR_BONS_SKILLS + 2;
    yPos = Y0_DESCR;
    const PlayerBon_t markedBon =
      browserPos.x == 0 ? bonsTraits.at(browserPos.y) :
      bonsSkills.at(browserPos.y);
    string effectDescr =
      "Effect(s): " + eng->playerBonHandler->getBonEffectDescr(markedBon);
    const int MAX_WIDTH_DESCR = 60;
    vector<string> descrLines =
      eng->textFormatting->lineToLines(effectDescr, MAX_WIDTH_DESCR);
    for(unsigned int i = 0; i < descrLines.size(); i++) {
      eng->renderer->drawText(
        descrLines.at(i), panel_screen, Pos(X_TRAITS - 2, yPos), clrGray);
      yPos++;
    }
  }

  eng->renderer->updateScreen();
}

void PlayerEnterName::run(const Pos& pos) {
  string name = "";
  draw(name, pos);
  bool done = false;
  while(done == false) {
    if(eng->config->isBotPlaying == false) {
      readKeys(name, done, pos);
    } else {
      name = "AZATHOTH";
      done = true;
    }
  }

  ActorDefinition& iDef = *(eng->player->getDef());
  iDef.name_a = iDef.name_the = name;
}

void PlayerEnterName::draw(const string& currentString, const Pos& pos) {
  eng->renderer->coverArea(panel_screen, pos, Pos(16, 2));
  const string LABEL = "What is your name?";
  eng->renderer->drawText(LABEL, panel_screen, pos, clrWhite);
  const string NAME_STR =
    currentString.size() < PLAYER_NAME_MAX_LENGTH ? currentString + "_" :
    currentString;
  eng->renderer->drawText(
    NAME_STR, panel_screen, pos + Pos(0, 1), clrNosferatuTealLgt);
  eng->renderer->updateScreen();
}

void PlayerEnterName::readKeys(string& currentString, bool& done,
                               const Pos& pos) {
  const KeyboardReadReturnData& d = eng->input->readKeysUntilFound();

  if(d.sdlKey_ == SDLK_RETURN) {
    done = true;
    currentString = currentString == "" ? "Rogue" : currentString;
    return;
  }

  if(currentString.size() < PLAYER_NAME_MAX_LENGTH) {
    if(
      d.sdlKey_ == SDLK_SPACE ||
      (d.key_ >= int('a') && d.key_ <= int('z')) ||
      (d.key_ >= int('A') && d.key_ <= int('Z')) ||
      (d.key_ >= int('0') && d.key_ <= int('9'))) {
      if(d.sdlKey_ == SDLK_SPACE) {
        currentString.push_back(' ');
      } else {
        currentString.push_back(char(d.key_));
      }
      draw(currentString, pos);
      return;
    }
  }

  if(currentString.size() > 0) {
#if (defined MACOSX)
    if(d.sdlKey_ == SDLK_BACKSPACE || d.sdlKey_ == SDLK_DELETE) {
#else
    if(d.sdlKey_ == SDLK_BACKSPACE) {
#endif
      currentString.erase(currentString.end() - 1);
      draw(currentString, pos);
    }
  }
}
