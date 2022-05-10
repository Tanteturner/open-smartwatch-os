
#include "./apps/main/switcher.h"
#include "easing.h"

#define SLEEP_TIMOUT 1000

void OswAppSwitcher::setup() {
  appOnScreenSince = millis();
  if (*_rtcAppIndex >= _appCount) {
    *_rtcAppIndex = 0;
  }
  _apps[*_rtcAppIndex]->setup();
}

void OswAppSwitcher::loop() {
  OswHal* hal = OswHal::getInstance();

  if (appOnScreenSince == 0) {
    // if appOnScreenSince was 0, it was set to 0 before light sleep. this is a nasty hack.
    appOnScreenSince = millis();
  }

  // if we enable sending the watch to sleep by clicking (really really) long enough
  if (_enableSleep && hal->btnIsDownSince(_btn) > DEFAULTLAUNCHER_LONG_PRESS + SLEEP_TIMOUT) {
    // remember we need to sleep once the button goes up
    _doSleep = true;
  }

  // detect switch action depending on mode
  switch (_type) {
    case LONG_PRESS:
      if (hal->btnIsDownSince(_btn) > DEFAULTLAUNCHER_LONG_PRESS) {
        _doSwitch = true;
      }
      break;
    case SHORT_PRESS:
    default:
      if (hal->btnHasGoneDown(_btn)) {
        _doSwitch = true;
      }
  }

  // do action only once the button goes up
  if (hal->btnHasGoneUp(_btn)) {
    if (_doSleep) {
      _doSleep = false;
      sleep();
    }
    if (_doSwitch) {
      _doSwitch = false;
      cycleApp();
      // we need to clear the button state, otherwise nested switchers
      // using the same button will switch too
      hal->clearButtonState(_btn);
    }
  }

  if (_enableAutoSleep && *_rtcAppIndex == 0 && !hal->btnIsDown(_btn)) {
    short timeout = OswConfigAllKeys::settingDisplayTimeout.get();
    if (*_rtcAppIndex == 0 && (millis() - appOnScreenSince) > timeout * 1000) {
      if (hal->btnIsDown(BUTTON_1) || hal->btnIsDown(BUTTON_2) || hal->btnIsDown(BUTTON_3)) {
        appOnScreenSince = millis();
      } else if(timeout > 0) {
        Serial.print("Going to sleep after ");
        Serial.print(timeout);
        Serial.println(" seconds");
        this->sleep();
      }
    }
  }

  hal->gfx()->resetText();
  OswUI::getInstance()->resetTextColors();  // yes this resets the colors in hal->gfx()
  _apps[*_rtcAppIndex]->loop();

  // draw Pagination Indicator
  if(_paginationIndicator){
    uint16_t rDot = 4; // Size (radius) of the dot
    uint16_t spacing = 10; // Distance (deg) between dots
    uint16_t r = (DISP_W / 2) - 8; // Distance from the center of the watch (radius)
    uint16_t alpha0 = 146 + (spacing / 2 * (_appCount-1)); // Angle of the first Element (146deg = Button 1)
    for(uint8_t i = 0; i < _appCount; i++){
      uint16_t alpha = alpha0 - (i * spacing);
      uint16_t x = (DISP_W / 2) + (cos(alpha * PI / 180) * r);
      uint16_t y = (DISP_H / 2) + (sin(alpha * PI / 180) * r);
      if(i == *_rtcAppIndex){
        hal->getCanvas()->getGraphics2D()->fillCircle(x, y, rDot, OswUI::getInstance()->getInfoColor());
      } else {
        hal->getCanvas()->getGraphics2D()->fillCircle(x, y, rDot, OswUI::getInstance()->getForegroundColor());
      }
    }
  }

  // draw app switcher
  if (hal->btnIsDown(_btn)) {
    uint8_t progX = 121;
    uint8_t progY = 200;

    boolean displaySleep = _enableSleep && hal->btnIsDownSince(_btn) > DEFAULTLAUNCHER_LONG_PRESS + SLEEP_TIMOUT;

    if(!displaySleep){
      if(_type == LONG_PRESS){
        // long press has the hollow square that fills (draws around short press)
        if (hal->btnIsDownSince(_btn) > DEFAULTLAUNCHER_LONG_PRESS) {
          // draw a large frame
          hal->gfx()->fillCircle(progX, progY, 14, OswUI::getInstance()->getInfoColor());
        } else {
          uint8_t progress = 0;
          if (hal->btnIsDownSince(_btn) > DEFAULTLAUNCHER_LONG_PRESS / 2) {
            progress = (hal->btnIsDownSince(_btn) - (DEFAULTLAUNCHER_LONG_PRESS / 2)) /
                      ((DEFAULTLAUNCHER_LONG_PRESS / 2) / 255.0);
          }
          if(progress != 0){
            double progressPercent = progress / 255.0;
            double progressPercentEase = getEasingFunction(EaseOutQuart)(progressPercent);
            double beginningPercent = progressPercent / 0.5;
            if(beginningPercent > 1) {beginningPercent = 1;}
            beginningPercent = getEasingFunction(EaseOutBack)(beginningPercent);
            hal->gfx()->drawArc(progX, progY, 180, progressPercentEase * 360 + 180, progress * 4,beginningPercent * 12, 2, OswUI::getInstance()->getInfoColor());
          }
        }
      }
    }
    else {
      hal->gfx()->drawArc(progX, progY, 45, 315, 12, 15, 3, OswUI::getInstance()->getDangerColor());
      hal->gfx()->drawThickLine(progX, progY - 6, progX, progY - 18, 3, OswUI::getInstance()->getDangerColor());
    }
  }
}

void OswAppSwitcher::cycleApp() {
  appOnScreenSince = millis();
  if(_pagination){
    _apps[*_rtcAppIndex]->stop();
    *_rtcAppIndex = *_rtcAppIndex + 1;
    if (*_rtcAppIndex == _appCount) {
      *_rtcAppIndex = 0;
    }
    _apps[*_rtcAppIndex]->setup();
  }
  OswHal::getInstance()->suppressButtonUntilUp(_btn);
}

void OswAppSwitcher::paginationDisable() {
  _pagination = false;
  _paginationIndicator = false;
}

void OswAppSwitcher::paginationEnable() {
  _pagination = true;
  _paginationIndicator = true;
}

void OswAppSwitcher::sleep() {
  OswHal* hal = OswHal::getInstance();
  hal->gfx()->fill(rgb565(0, 0, 0));
  hal->flushCanvas();

  appOnScreenSince = 0; // nasty hack to prevent re-sleep after wakeup from light sleep
  hal->lightSleep();
}

void OswAppSwitcher::stop() { _apps[*_rtcAppIndex]->stop(); }

void OswAppSwitcher::registerApp(OswApp* app) {
  _appCount++;
  _apps.push_back(app);
}
