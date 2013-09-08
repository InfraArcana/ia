#include "ItemMedicalBag.h"

#include "Engine.h"
#include "Properties.h"
#include "ActorPlayer.h"
#include "Log.h"
#include "Render.h"
#include "Inventory.h"
#include "PlayerBonuses.h"
#include "Popup.h"

bool MedicalBag::activateDefault(Actor* const actor) {
  (void)actor;

  curAction_ = playerChooseAction();

  if(curAction_ != endOfMedicalBagActions) {
    //Check if chosen action can be done
    const PropHandler* const prop =
      eng->player->getPropHandler();
    switch(curAction_) {
      case medicalBagAction_sanitizeInfection: {
        if(prop->hasProp(propInfected) == false) {
          eng->log->addMsg("I have no infections to sanitize.");
          curAction_ = endOfMedicalBagActions;
        }
      } break;

      case medicalBagAction_treatWound: {
        if(prop->hasProp(propWound) == false) {
          eng->log->addMsg("I have no wounds to treat.");
          curAction_ = endOfMedicalBagActions;
        }
      } break;

      case endOfMedicalBagActions: {} break;
    }

    if(curAction_ != endOfMedicalBagActions) {
      if(getNrSuppliesNeededForAction(curAction_) > nrSupplies_) {
        eng->log->addMsg("I do not have enough supplies for that.");
        curAction_ = endOfMedicalBagActions;
      }
    }

    if(curAction_ != endOfMedicalBagActions) {
      //Action can be done
      nrTurnsLeft_ = getTotTurnsForAction(curAction_);
      eng->player->activeMedicalBag = this;
      eng->gameTime->endTurnOfCurrentActor();
    }
  }

  return false;
}

MedicalBagAction_t MedicalBag::playerChooseAction() const {
  vector<string> choiceLabels;
  for(int actionNr = 0; actionNr < endOfMedicalBagActions; actionNr++) {
    string label = "";
    switch(actionNr) {
      case medicalBagAction_sanitizeInfection: {
        label = "Sanitize infection";
      } break;

      case medicalBagAction_treatWound: {
        label = "Treat wound";
      } break;
    }

    const int NR_TURNS_NEEDED =
      getTotTurnsForAction(MedicalBagAction_t(actionNr));
    const int NR_SUPPL_NEEDED =
      getNrSuppliesNeededForAction(MedicalBagAction_t(actionNr));
    label += " (" + intToString(NR_SUPPL_NEEDED) + " suppl";
    label += "/"  + intToString(NR_TURNS_NEEDED) + " turns)";
    choiceLabels.push_back(label);
  }
  choiceLabels.push_back("Cancel");

  const string nrSuppliesMsg =
    intToString(nrSupplies_) + " medical supplies available.";

  return MedicalBagAction_t(eng->popup->showMultiChoiceMessage(
                              nrSuppliesMsg, true, choiceLabels,
                              "Use medical bag"));
}

void MedicalBag::continueAction() {
  nrTurnsLeft_--;
  if(nrTurnsLeft_ <= 0) {
    finishCurAction();
  } else {
    eng->gameTime->endTurnOfCurrentActor();
  }
}

void MedicalBag::finishCurAction() {
  eng->player->activeMedicalBag = NULL;

  switch(curAction_) {
    case medicalBagAction_sanitizeInfection: {
    } break;

    case medicalBagAction_treatWound: {
      Prop* prop =
        eng->player->getPropHandler()->getAppliedProp(propWound);
      if(prop == NULL) {
        trace << "[WARNING] No wound prop found, ";
        trace << "in MedicalBag::finishCurAction()" << endl;
      } else {
        dynamic_cast<PropWound*>(prop)->healOneWound();
      }
//        eng->log->clearLog();
//        eng->log->addMsg("I finish applying first aid.");
//        eng->renderer->drawMapAndInterface();
    } break;

    case endOfMedicalBagActions: {} break;
  }

  nrSupplies_ -= getNrSuppliesNeededForAction(curAction_);

  curAction_ = endOfMedicalBagActions;

  if(nrSupplies_ <= 0) {
    Inventory* const inv = eng->player->getInventory();
    inv->removetemInGeneralWithPointer(this, true);
  }
}

void MedicalBag::interrupted() {
//  switch(curAction_) {
//    case medicalBagAction_sanitizeInfection: {
//    } break;
//
//    case medicalBagAction_treatWound: {
//    } break;
//  }
//
//  eng->log->addMsg("My applying of first aid is disrupted.", clrWhite,
//                          messageInterrupt_never);
//  nrTurnsLeft_ = -1;
//
//  eng->player->activeMedicalBag = NULL;
}

int MedicalBag::getTotTurnsForAction(const MedicalBagAction_t action) const {
  const bool IS_WOUND_TREATER =
    eng->playerBonHandler->isBonPicked(playerBon_skillfulWoundTreater);

  switch(action) {
    case medicalBagAction_sanitizeInfection: {
      const int TURNS_BEFORE_BON = 20;
      return IS_WOUND_TREATER ? TURNS_BEFORE_BON / 2 : TURNS_BEFORE_BON;
    } break;

    case medicalBagAction_treatWound: {
      const int TURNS_BEFORE_BON = 70;
      return IS_WOUND_TREATER ? TURNS_BEFORE_BON / 2 : TURNS_BEFORE_BON;
    } break;

    case endOfMedicalBagActions: {
      //Should not happen
    } break;
  }
  return -1;
}

int MedicalBag::getNrSuppliesNeededForAction(
  const MedicalBagAction_t action) const {
  switch(action) {
    case medicalBagAction_sanitizeInfection: {
      return 1;
    } break;

    case medicalBagAction_treatWound: {
      return 5;
    } break;

    case endOfMedicalBagActions: {
      //Should not happen
    } break;
  }
  return -1;
}

// (Interrupted)
//const bool IS_FAINTED = statusEffectsHandler_->hasProp(statusFainted);
//const bool IS_PARALYSED = statusEffectsHandler_->hasProp(statusParalyzed);
//const bool IS_DEAD = deadState != actorDeadState_alive;
//getSpotedEnemies();
//const int TOTAL_TURNS = getHealingTimeTotal();
//const bool IS_ENOUGH_TIME_PASSED = firstAidTurnsLeft < TOTAL_TURNS - 10;
//const int MISSING_HP = getHpMax(true) - getHp();
//const int HP_HEALED_IF_ABORTED =
//  IS_ENOUGH_TIME_PASSED ? (MISSING_HP * (TOTAL_TURNS - firstAidTurnsLeft)) / TOTAL_TURNS  : 0;
//
//bool isAborted = false;
//if(spotedEnemies.size() > 0 || IS_FAINTED || IS_PARALYSED || IS_DEAD || PROMPT_FOR_ABORT == false) {
//  firstAidTurnsLeft = -1;
//  isAborted = true;
//  eng->log->addMsg("I stop tending to my wounds.", clrWhite);
//  eng->renderer->drawMapAndInterface();
//} else {
//  const string TURNS_STR = intToString(firstAidTurnsLeft);
//  const string ABORTED_HP_STR = intToString(HP_HEALED_IF_ABORTED);
//  string abortStr = "Continue healing (" + TURNS_STR + " turns)? (y/n), ";
//  abortStr += ABORTED_HP_STR + " HP restored if canceled.";
//  eng->log->addMsg(abortStr , clrWhiteHigh);
//  eng->renderer->drawMapAndInterface();
//
//  if(eng->query->yesOrNo() == false) {
//    firstAidTurnsLeft = -1;
//    isAborted = true;
//  }
//
//  eng->log->clearLog();
//  eng->renderer->drawMapAndInterface();
//}
//if(isAborted && IS_ENOUGH_TIME_PASSED) {
//  restoreHP(HP_HEALED_IF_ABORTED);
//}


// (Request healing)
//    clearLogMessages();
//    if(eng->player->deadState == actorDeadState_alive) {
//      if(eng->player->getPropHandler()->hasProp(statusPoisoned)) {
//        eng->log->addMsg("Not while poisoned.");
//        eng->renderer->drawMapAndInterface();
//      } else {
//        bool allowHeal = false;
//        const bool IS_DISEASED = eng->player->getPropHandler()->hasProp(statusDiseased);
//
//        if(eng->player->getHp() < eng->player->getHpMax(true)) {
//          allowHeal = true;
//        } else if(IS_DISEASED && eng->playerBonHandler->isBonPicked(playerBon_curer)) {
//          allowHeal = true;
//        }
//
//        if(allowHeal) {
//          eng->player->getSpotedEnemies();
//          if(eng->player->spotedEnemies.size() == 0) {
//            const int TURNS_TO_HEAL = eng->player->getHealingTimeTotal();
//            const string TURNS_STR = intToString(TURNS_TO_HEAL);
//            eng->log->addMsg("I rest here and attend my wounds (" + TURNS_STR + " turns)...");
//            eng->player->firstAidTurnsLeft = TURNS_TO_HEAL - 1;
//            eng->gameTime->endTurnOfCurrentActor();
//          } else {
//            eng->log->addMsg("Not while an enemy is near.");
//            eng->renderer->drawMapAndInterface();
//          }
//        } else {
//          if(IS_DISEASED) {
//            eng->log->addMsg("I cannot heal this disease.");
//          } else {
//            eng->log->addMsg("I am already at good health.");
//          }
//          eng->renderer->drawMapAndInterface();
//        }
//      }
//    }
//    clearEvents();
//    return;