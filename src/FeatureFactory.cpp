#include "Init.h"

#include "FeatureFactory.h"

#include <assert.h>

#include "FeatureDoor.h"
#include "FeatureLever.h"
#include "FeatureLitDynamite.h"
#include "FeatureTrap.h"
#include "FeatureSmoke.h"
#include "FeatureProxEvent.h"
#include "Map.h"
#include "FeatureExaminable.h"
#include "FeatureLiquid.h"
#include "FeatureWall.h"

#ifdef DEMO_MODE
#include "Renderer.h"
#include "SdlWrapper.h"
#endif // DEMO_MODE

using namespace std;

namespace FeatureFactory {

namespace {

void replaceStaticFeatureAt(FeatureStatic* const newFeature, const Pos& pos) {
  Cell& cell = Map::cells[pos.x][pos.y];

  FeatureStatic* const oldFeature = cell.featureStatic;

  if(oldFeature != nullptr) {delete oldFeature;}

  cell.featureStatic = newFeature;

#ifdef DEMO_MODE
  if(newFeature->getId() == FeatureId::floor) {
    for(int y = 0; y < MAP_H; y++) {
      for(int x = 0; x < MAP_W; x++) {
        Map::cells[x][y].isSeenByPlayer = true;
        Map::cells[x][y].isExplored     = true;
      }
    }
    Renderer::drawMap();
    Renderer::updateScreen();
    SdlWrapper::sleep(30);
  }
#endif // DEMO_MODE
}

} //namespace

Feature* mk(const FeatureId id, const Pos pos, FeatureSpawnData* spawnData) {
  const FeatureDataT* const data = FeatureData::getData(id);

  //General (simple) features
  if(data->spawnType == featureSpawnType_static) {
    assert(spawnData == nullptr);
    FeatureStatic* feature = new FeatureStatic(id, pos);
    replaceStaticFeatureAt(feature, pos);
    return feature;
  }
  if(data->spawnType == featureSpawnType_mob) {
    assert(spawnData == nullptr);
    FeatureMob* feature = new FeatureMob(id, pos);
    GameTime::addFeatureMob(feature);
    return feature;
  }

  //Features with specific class
  switch(id) {
    case FeatureId::door: {
      assert(spawnData->getFeatureSpawnDataType() == featureSpawnData_door);
      Door* door =
        new Door(id, pos, dynamic_cast<DoorSpawnData*>(spawnData));
      replaceStaticFeatureAt(door, pos);
      delete spawnData;
      return door;
    }
    case FeatureId::lever: {
      assert(spawnData->getFeatureSpawnDataType() == featureSpawnData_lever);
      FeatureLever* lever =
        new FeatureLever(
        id, pos, dynamic_cast<LeverSpawnData*>(spawnData));
      replaceStaticFeatureAt(lever, pos);
      delete spawnData;
      return lever;
    }
    case FeatureId::trap: {
      assert(spawnData->getFeatureSpawnDataType() == featureSpawnData_trap);
      Trap* trap =
        new Trap(id, pos, dynamic_cast<TrapSpawnData*>(spawnData));
      replaceStaticFeatureAt(trap, pos);
      delete spawnData;
      return trap;
    }
    case FeatureId::litDynamite: {
      assert(spawnData->getFeatureSpawnDataType() == featureSpawnData_dynamite);
      LitDynamite* dynamite =
        new LitDynamite(
        id, pos, dynamic_cast<DynamiteSpawnData*>(spawnData));
      GameTime::addFeatureMob(dynamite);
      delete spawnData;
      return dynamite;
    }
    case FeatureId::litFlare: {
      assert(spawnData->getFeatureSpawnDataType() == featureSpawnData_dynamite);
      LitFlare* flare =
        new LitFlare(
        id, pos, dynamic_cast<DynamiteSpawnData*>(spawnData));
      GameTime::addFeatureMob(flare);
      delete spawnData;
      return flare;
    }
    case FeatureId::smoke: {
      assert(spawnData->getFeatureSpawnDataType() == featureSpawnData_smoke);
      Smoke* smoke =
        new Smoke(id, pos, dynamic_cast<SmokeSpawnData*>(spawnData));
      GameTime::addFeatureMob(smoke);
      delete spawnData;
      return smoke;
    }
    case FeatureId::proxEventWallCrumble: {
      assert(spawnData->getFeatureSpawnDataType() ==
             featureSpawnData_proxEventWallCrumble);
      ProxEventWallCrumble* proxEvent =
        new ProxEventWallCrumble(
        id, pos, dynamic_cast<ProxEventWallCrumbleSpawnData*>(spawnData));
      GameTime::addFeatureMob(proxEvent);
      delete spawnData;
      return proxEvent;
    }
    case FeatureId::tomb: {
      assert(spawnData == nullptr);
      Tomb* tomb = new Tomb(id, pos);
      replaceStaticFeatureAt(tomb, pos);
      return tomb;
    }
//    case FeatureId::pillarCarved: {
//        assert(spawnData == nullptr);
//        CarvedPillar* pillar = new CarvedPillar(id, pos);
//        replaceStaticFeatureAt(pillar, pos);
//        return pillar;
//      }
//      break;
//    case FeatureId::barrel: {
//        assert(spawnData == nullptr);
//        Barrel* barrel = new Barrel(id, pos);
//        replaceStaticFeatureAt(barrel, pos);
//        return barrel;
//      }
//      break;
    case FeatureId::cabinet: {
      assert(spawnData == nullptr);
      Cabinet* cabinet = new Cabinet(id, pos);
      replaceStaticFeatureAt(cabinet, pos);
      return cabinet;
    }
    case FeatureId::chest: {
      assert(spawnData == nullptr);
      Chest* chest = new Chest(id, pos);
      replaceStaticFeatureAt(chest, pos);
      return chest;
    }
    case FeatureId::fountain: {
      assert(spawnData == nullptr);
      Fountain* fountain = new Fountain(id, pos);
      replaceStaticFeatureAt(fountain, pos);
      return fountain;
    }
    case FeatureId::cocoon: {
      assert(spawnData == nullptr);
      Cocoon* cocoon = new Cocoon(id, pos);
      replaceStaticFeatureAt(cocoon, pos);
      return cocoon;
    }
//    case FeatureId::altar: {
//        assert(spawnData == nullptr);
//        Altar* altar = new Altar(id, pos);
//        replaceStaticFeatureAt(altar, pos);
//
//        return altar;
//      }
//      break;
    case FeatureId::shallowMud:
    case FeatureId::shallowWater:
    case FeatureId::poolBlood: {
      assert(spawnData == nullptr);
      FeatureLiquidShallow* liquid = new FeatureLiquidShallow(id, pos);
      replaceStaticFeatureAt(liquid, pos);
      return liquid;
    }
    case FeatureId::deepWater: {
      assert(spawnData == nullptr);
      FeatureLiquidDeep* liquid = new FeatureLiquidDeep(id, pos);
      replaceStaticFeatureAt(liquid, pos);
      return liquid;
    }
    case FeatureId::gravestone: {
      assert(spawnData == nullptr);
      Grave* grave = new Grave(id, pos);
      replaceStaticFeatureAt(grave, pos);
      return grave;
    }
    case FeatureId::stairs: {
      assert(spawnData == nullptr);
      Stairs* stairs = new Stairs(id, pos);
      replaceStaticFeatureAt(stairs, pos);
      return stairs;
    }
    case FeatureId::wall: {
      assert(spawnData == nullptr);
      Wall* wall = new Wall(id, pos);
      replaceStaticFeatureAt(wall, pos);
      return wall;
    }
    default: {} break;
  }

  return nullptr;
}

} //FeatureFactory
