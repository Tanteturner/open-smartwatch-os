
#include "./apps/main/switcher.h"

#define SLEEP_TIMOUT 1000

void OswAppSwitcher::setup(OswHal* hal) {
  if (*_rtcAppIndex >= _appCount) {
    *_rtcAppIndex = 0;
  }
  _apps[*_rtcAppIndex]->setup(hal);
}

void OswAppSwitcher::loop(OswHal* hal) {
  static unsigned long appOnScreenSince = millis();

  // if we enable sending the watch to sleep by clicking (really really) long enough
  if (_enableDeepSleep && hal->btnIsDownSince(_btn) > DEFAULTLAUNCHER_LONG_PRESS + SLEEP_TIMOUT) {
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
      if (hal->btnIsDownSince(_btn)) {
        _doSwitch = true;
      }
  }

  // do action only once the button goes up
  if (hal->btnHasGoneUp(_btn)) {
    if (_doSleep) {
      _doSleep = false;
      sleep(hal);
    }
    if (_doSwitch) {
      _doSwitch = false;
      cycleApp(hal);
    }
  }

  if (_enableAutoSleep && *_rtcAppIndex == 0) {
    if (*_rtcAppIndex == 0 && (millis() - appOnScreenSince) > 15000) {
      hal->gfx()->fill(rgb565(0, 0, 0));
      hal->flushCanvas();
      hal->deepSleep();
    }
  }

  hal->gfx()->resetText();
  _apps[*_rtcAppIndex]->loop(hal);

  // draw app switcher
  if (hal->btnIsDown(_btn)) {
    uint8_t btnX = 0;
    uint8_t btnY = 0;
    int16_t progressOffset = 0;
    // draw button switcher close to button
    switch (_btn) {
      case BUTTON_2:
        btnX = 214;
        btnY = 190;
        progressOffset = 135;
        break;
      case BUTTON_3:
        btnX = 214;
        btnY = 50;
        progressOffset = 135;
        break;
      case BUTTON_1:
      default:
        btnX = 26;
        btnY = 190;
        progressOffset = -45;
        break;
    }

    switch (_type) {
      case LONG_PRESS:
        // long press has the hollow square that fills (draws around short press)
        if (hal->btnIsDownSince(_btn) > DEFAULTLAUNCHER_LONG_PRESS) {
          // draw a large frame
          hal->gfx()->fillCircle(btnX, btnY, 20, rgb565(255, 255, 255));
        } else {
          uint8_t progress = 0;
          if (hal->btnIsDownSince(_btn) > DEFAULTLAUNCHER_LONG_PRESS / 2) {
            progress = (hal->btnIsDownSince(_btn) - (DEFAULTLAUNCHER_LONG_PRESS / 2)) /
                       ((DEFAULTLAUNCHER_LONG_PRESS / 2) / 255.0);
          }
          hal->gfx()->drawArc(btnX, btnY, progressOffset, progressOffset + (progress / 255.0) * 180, progress / 4, 20,
                              3, rgb565(255, 255, 255));
        }
        break;
      case SHORT_PRESS:
      default:
        hal->gfx()->fillCircle(btnX, btnY, 10, rgb565(255, 255, 255));
    }

    if (_enableDeepSleep && hal->btnIsDownSince(_btn) > DEFAULTLAUNCHER_LONG_PRESS + SLEEP_TIMOUT) {
      // draw half moon
      hal->gfx()->fillCircle(btnX, btnY, 9, rgb565(128, 128, 128));
      hal->gfx()->fillCircle(btnX, btnY, 8, rgb565(255, 255, 255));
      hal->gfx()->fillCircle(btnX + 3, btnY, 6, rgb565(0, 0, 0));
    }
  }
}

void OswAppSwitcher::cycleApp(OswHal* hal) {
  _apps[*_rtcAppIndex]->stop(hal);
  *_rtcAppIndex = *_rtcAppIndex + 1;
  if (*_rtcAppIndex == _appCount) {
    *_rtcAppIndex = 0;
  }
  _apps[*_rtcAppIndex]->setup(hal);
  hal->suppressButtonUntilUp(_btn);
}

void OswAppSwitcher::sleep(OswHal* hal) {
  hal->gfx()->fill(rgb565(0, 0, 0));
  hal->flushCanvas();
  hal->deepSleep();
}

void OswAppSwitcher::stop(OswHal* hal) { _apps[*_rtcAppIndex]->stop(hal); }

void OswAppSwitcher::registerApp(OswApp* app) {
  _appCount++;
  _apps.push_back(app);
}