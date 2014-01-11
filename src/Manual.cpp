#include "Manual.h"

#include <fstream>
#include <iostream>

#include "Input.h"
#include "Engine.h"
#include "TextFormatting.h"
#include "Renderer.h"

void Manual::readFile() {
  string curLine;
  ifstream file("manual.txt");

  vector<string> formatted;

  if(file.is_open()) {
    while(getline(file, curLine)) {
      if(curLine.empty()) {
        lines.push_back(curLine);
      } else {
        //Do not format lines that start with two spaces
        bool shouldFormatLine = true;
        if(curLine.size() > 1) {
          if(curLine.at(0) == ' ' && curLine.at(1) == ' ') {
            shouldFormatLine = false;
          }
        }
        if(shouldFormatLine) {
          TextFormatting::lineToLines(curLine, MAP_W - 2, formatted);
          for(unsigned int i = 0; i < formatted.size(); i++) {
            lines.push_back(formatted.at(i));
          }
        } else {
          curLine.erase(curLine.begin());
          lines.push_back(curLine);
        }
      }
    }
  }

  file.close();
}

void Manual::drawManualInterface() {
  const string decorationLine(MAP_W - 2, '-');

  eng.renderer->coverArea(panel_screen, Pos(0, 1), Pos(MAP_W, 2));
  eng.renderer->drawText(decorationLine, panel_screen,
                          Pos(1, 1), clrWhite);

  eng.renderer->drawText(" Displaying manual ", panel_screen,
                          Pos(3, 1), clrWhite);

  eng.renderer->drawText(decorationLine, panel_screen,
                          Pos(1, SCREEN_H - 1), clrWhite);

  eng.renderer->drawText(" 2/8, down/up to navigate | space/esc to exit ",
                          panel_screen, Pos(3, SCREEN_H - 1), clrWhite);
}

void Manual::run() {
  eng.renderer->clearScreen();

  string str;

  int topElement = 0;
  int btmElement = min(topElement + MAP_H - 1, int(lines.size()) - 1);

  drawManualInterface();

  Pos pos(1, 2);

  for(int i = topElement; i <= btmElement; i++) {
    eng.renderer->drawText(lines.at(i), panel_screen, pos, clrWhite);
    pos.y++;
  }

  eng.renderer->updateScreen();

  //Read keys
  bool done = false;
  while(done == false) {
    const KeyboardReadReturnData& d = eng.input->readKeysUntilFound();

    if(d.key_ == '2' || d.sdlKey_ == SDLK_DOWN) {
      topElement = min(topElement + 3, int(lines.size()) - int(MAP_H));
      topElement = max(0, topElement);
      btmElement = min(topElement + MAP_H - 1, int(lines.size()) - 1);
      eng.renderer->coverArea(panel_screen, Pos(0, 2),
                               Pos(MAP_W, MAP_H));
      drawManualInterface();
      pos.y = 2;
      for(int i = topElement; i <= btmElement; i++) {
        eng.renderer->drawText(lines.at(i), panel_screen, pos, clrWhite);
        pos.y++;
      }
      eng.renderer->updateScreen();
    } else if(d.key_ == '8' || d.sdlKey_ == SDLK_UP) {
      topElement = min(topElement - 3, int(lines.size()) - int(MAP_H));
      topElement = max(0, topElement);
      btmElement = min(topElement + MAP_H - 1, int(lines.size()) - 1);
      eng.renderer->coverArea(panel_screen, Pos(0, 2),
                               Pos(MAP_W, MAP_H));
      drawManualInterface();
      pos.y = 2;
      for(int i = topElement; i <= btmElement; i++) {
        eng.renderer->drawText(lines.at(i), panel_screen, pos, clrWhite);
        pos.y++;
      }
      eng.renderer->updateScreen();
    } else if(d.sdlKey_ == SDLK_SPACE || d.sdlKey_ == SDLK_ESCAPE) {
      done = true;
    }
  }

  eng.renderer->coverPanel(panel_screen);
}
