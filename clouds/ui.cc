// Copyright 2014 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// User interface.

#include "ui.h"

#include "stmlib/system/system_clock.h"

#include "clouds/cv_scaler.h"
#include "clouds/drivers/gate_input.h"
#include "clouds/dsp/granular_processor.h"
#include "clouds/meter.h"

namespace clouds {

const int32_t kLongPressDuration     = 1000;
const int32_t kVeryLongPressDuration = 1500;
const size_t  kNumPresetLeds         = 4;

using namespace stmlib;

void Ui::Splash(uint32_t clock) {
  uint8_t index = ((clock >> 8) + 1) & 3;
  uint8_t fade  = (clock >> 2);
  fade          = fade <= 127 ? (fade << 1) : 255 - (fade << 1);
  leds_.set_intensity(3 - index, fade);
}

void Ui::Init(Settings* settings, CvScaler* cv_scaler, GranularProcessor* processor, Meter* meter) {
  settings_  = settings;
  cv_scaler_ = cv_scaler;
  leds_.Init();
  switches_.Init();

  processor_       = processor;
  meter_           = meter;
  mode_            = UI_MODE_SPLASH;
  ignore_releases_ = 0;

  const State& state = settings_->state();

  // Sanitize saved settings.
  cv_scaler_->set_blend_parameter(static_cast<BlendParameter>(state.blend_parameter & 3));
  processor_->set_quality(state.quality & 3);
  processor_->set_playback_mode(
    static_cast<PlaybackMode>(state.playback_mode % PLAYBACK_MODE_LAST));
  for (int32_t i = 0; i < BLEND_PARAMETER_LAST; ++i) {
    cv_scaler_->set_blend_value(static_cast<BlendParameter>(i),
                                static_cast<float>(state.blend_value[i]) / 255.0f);
  }
  cv_scaler_->UnlockBlendKnob();

  if (switches_[SWITCH_WRITE]->pressed_immediate()) {
    mode_            = UI_MODE_CALIBRATION_1;
    ignore_releases_ = 1;
  }
}

void Ui::SaveState() {
  State* state           = settings_->mutable_state();
  state->blend_parameter = cv_scaler_->blend_parameter();
  state->quality         = processor_->quality();
  state->playback_mode   = processor_->playback_mode();
  for (int32_t i = 0; i < BLEND_PARAMETER_LAST; ++i) {
    state->blend_value[i] =
      static_cast<uint8_t>(cv_scaler_->blend_value(static_cast<BlendParameter>(i)) * 255.0f);
  }
  settings_->Save();
}

void Ui::Poll() {
  system_clock.Tick();
  switches_.Scan();

  for (uint8_t i = 0; i < kNumSwitches; ++i) {
    Switch* s = switches_[i];

    if (s->just_pressed()) {
      queue_.AddEvent(CONTROL_SWITCH, i, 0);
      s->capture_press();
      continue;
    }
    if (!s->pressed() || s->press_time() == 0) {
      continue;
    }

    int32_t pressed_time = system_clock.milliseconds() - s->press_time();
    if (pressed_time > kLongPressDuration && s->state() == SwitchPressed) {
      queue_.AddEvent(CONTROL_SWITCH, i, pressed_time);
      s->set_state(SwitchLongPressed);
    }
    if (pressed_time > kVeryLongPressDuration && s->state() == SwitchLongPressed) {
      queue_.AddEvent(CONTROL_SWITCH, i, pressed_time);
      s->set_state(SwitchVLongPressed);
    }
    if (s->released()) {
      queue_.AddEvent(CONTROL_SWITCH, i, pressed_time + 1);
      s->reset();
    }
  }
  PaintLeds();
}

void Ui::PaintLeds() {
  leds_.Clear();
  uint32_t clock = system_clock.milliseconds();
  bool     blink = (clock & 127) > 64;
  bool     flash = (clock & 511) < 16;
  uint8_t  fade  = clock >> 1;
  fade           = fade <= 127 ? (fade << 1) : 255 - (fade << 1);
  fade           = static_cast<uint16_t>(fade) * fade >> 8;

  leds_.set_enabled(!processor_->bypass());

  switch (mode_) {
    case UI_MODE_SPLASH:
      Splash(clock);
      break;

    case UI_MODE_VU_METER:
      if (processor_->bypass()) {
        leds_.PaintBar(0);
      } else {
        leds_.PaintBar(lut_db[meter_->peak() >> 7]);
      }
      break;

    case UI_MODE_QUALITY:
      leds_.set_status(processor_->quality(), 255, 0);
      break;

    case UI_MODE_BLENDING:
      leds_.set_status(cv_scaler_->blend_parameter(), 0, 255);
      break;

    case UI_MODE_PLAYBACK_MODE:
      if (blink) {
        for (int i = 0; i < 4; i++)
          leds_.set_status(i, 0, 0);
      } else if (processor_->playback_mode() < 4) {
        leds_.set_status(processor_->playback_mode(), 128 + (fade >> 1), 255 - (fade >> 1));
      } else {
        for (int i = 0; i < 4; i++)
          leds_.set_status(i, 128 + (fade >> 1), 255 - (fade >> 1));
        leds_.set_status(processor_->playback_mode() & 3, 0, 0);
      }

      break;

    case UI_MODE_LOAD:
    case UI_MODE_SAVE:
      VisualizePresetLocation(fade, flash);
      break;

    case UI_MODE_SAVING:
      leds_.set_status(load_save_location_, 255, 0);
      break;

    case UI_MODE_CALIBRATION_1:
      leds_.set_status(0, blink ? 255 : 0, blink ? 255 : 0);
      leds_.set_status(1, blink ? 255 : 0, blink ? 255 : 0);
      leds_.set_status(2, 0, 0);
      leds_.set_status(3, 0, 0);
      break;

    case UI_MODE_CALIBRATION_2:
      leds_.set_status(0, blink ? 255 : 0, blink ? 255 : 0);
      leds_.set_status(1, blink ? 255 : 0, blink ? 255 : 0);
      leds_.set_status(2, blink ? 255 : 0, blink ? 255 : 0);
      leds_.set_status(3, blink ? 255 : 0, blink ? 255 : 0);
      break;

    case UI_MODE_PANIC:
      leds_.set_status(0, 255, 0);
      leds_.set_status(1, 255, 0);
      leds_.set_status(2, 255, 0);
      leds_.set_status(3, 255, 0);
      break;

    case UI_MODE_BLEND_METER:
    default:
      break;
  }

  bool freeze = processor_->frozen();
  if (processor_->reversed()) {
    freeze ^= flash;
  }
  leds_.set_freeze(freeze);

  leds_.Write();
}

void Ui::FlushEvents() {
  queue_.Flush();
}

void Ui::OnSwitchPressed(const Event& e) {}

void Ui::CalibrateC1() {
  cv_scaler_->CalibrateC1();
  cv_scaler_->CalibrateOffsets();
  mode_ = UI_MODE_CALIBRATION_2;
}

void Ui::CalibrateC3() {
  bool success = cv_scaler_->CalibrateC3();
  if (success) {
    settings_->Save();
    mode_ = UI_MODE_VU_METER;
  } else {
    mode_ = UI_MODE_PANIC;
  }
}

void Ui::OnSecretHandshake() {
  mode_ = UI_MODE_PLAYBACK_MODE;
}

void Ui::OnSwitchReleased(const Event& e) {
  // hack for double presses
  if (ignore_releases_ > 0) {
    ignore_releases_--;
    return;
  }

  switch (e.control_id) {
    case SWITCH_BYPASS:
      if (e.data >= kLongPressDuration) {
        processor_->set_inf_reverb(true);
      } else {
        processor_->ToggleBypass();
      }
      break;

    case SWITCH_FREEZE:
      if (e.data >= kLongPressDuration) {
        processor_->ToggleReverse();
      } else {
        processor_->ToggleFreeze();
      }
      break;

    case SWITCH_MODE:
      if (e.data >= kLongPressDuration) {
        if (mode_ == UI_MODE_QUALITY) {
          mode_ = UI_MODE_VU_METER;
        } else {
          mode_ = UI_MODE_LOAD;
        }
        break;
      }

      switch (mode_) {
        case UI_MODE_VU_METER:
        case UI_MODE_BLEND_METER:
          mode_ = UI_MODE_QUALITY;
          break;

        case UI_MODE_BLENDING:
          cv_scaler_->set_blend_parameter(
            static_cast<BlendParameter>((cv_scaler_->blend_parameter() + 1) & 3));
          SaveState();
          break;

        case UI_MODE_QUALITY:
          processor_->set_quality((processor_->quality() + 1) & 3);
          SaveState();
          break;

        case UI_MODE_PLAYBACK_MODE:
          DecrementPlaybackMode();
          break;

        case UI_MODE_SAVE:
          IncrementLoadSaveLocation();
          break;

        case UI_MODE_LOAD:
          // processor_->LoadPersistentData(settings_->sample_flash_data(load_save_location_));
          processor_->LoadPreset(settings_->ConstPreset(load_save_bank_, load_save_location_));
          load_save_location_ = (load_save_location_ + 1) & 3;
          mode_               = UI_MODE_VU_METER;
          break;

        default:
          mode_ = UI_MODE_VU_METER;
      }
      break;

    case SWITCH_WRITE:
      if (e.data >= kLongPressDuration) {
        mode_ = UI_MODE_SAVE;
        break;
      }

      switch (mode_) {
        case UI_MODE_CALIBRATION_1:
          CalibrateC1();
          break;

        case UI_MODE_CALIBRATION_2:
          CalibrateC3();
          break;

        case UI_MODE_SAVE:
          mode_ = UI_MODE_SAVING;
          SavePreset();
          mode_ = UI_MODE_VU_METER;
          break;

        case UI_MODE_LOAD:
          IncrementLoadSaveLocation();
          break;

        case UI_MODE_PLAYBACK_MODE:
          IncrementPlaybackMode();
          break;

        default:
          mode_ = UI_MODE_PLAYBACK_MODE;
          break;
      }
    default:
      break;
  }
}

void Ui::DoEvents() {
  while (queue_.available()) {
    Event e = queue_.PullEvent();
    if (e.control_type != CONTROL_SWITCH) {
      continue;
    }

    if (e.data == 0) {
      OnSwitchPressed(e);
    } else if (e.data >= kLongPressDuration && e.control_id == SWITCH_MODE &&
               switches_[SWITCH_WRITE]->pressed()) {
      switches_[SWITCH_WRITE]->reset();
      OnSecretHandshake();
    } else {
      OnSwitchReleased(e);
    }
  }

  if (queue_.idle_time() > 1000 && mode_ == UI_MODE_PANIC) {
    queue_.Touch();
    mode_ = UI_MODE_VU_METER;
  }

  if ((mode_ == UI_MODE_VU_METER || mode_ == UI_MODE_BLEND_METER || mode_ == UI_MODE_BLENDING) &&
      cv_scaler_->blend_knob_touched()) {
    queue_.Touch();
    // mode_ = UI_MODE_BLEND_METER;
  }

  if (queue_.idle_time() > 3000) {
    queue_.Touch();
    if (mode_ == UI_MODE_BLENDING || mode_ == UI_MODE_QUALITY || mode_ == UI_MODE_PLAYBACK_MODE ||
        mode_ == UI_MODE_SAVE || mode_ == UI_MODE_LOAD || mode_ == UI_MODE_BLEND_METER ||
        mode_ == UI_MODE_SPLASH) {
      mode_ = UI_MODE_VU_METER;
    }
  }

  if (processor_->inf_reverb() && !switches_[SWITCH_BYPASS]->pressed()) {
    processor_->set_inf_reverb(false);
  }
}

uint8_t Ui::HandleFactoryTestingRequest(uint8_t command) {
  uint8_t argument = command & 0x1f;
  command          = command >> 5;
  uint8_t reply    = 0;
  switch (command) {
    case FACTORY_TESTING_READ_POT:
    case FACTORY_TESTING_READ_CV:
      reply = cv_scaler_->adc_value(argument);
      break;

    case FACTORY_TESTING_READ_GATE:
      if (argument <= 2) {
        return switches_[argument]->pressed();
      } else {
        return cv_scaler_->gate(argument - 3);
      }
      break;

    case FACTORY_TESTING_SET_BYPASS:
      processor_->set_bypass(argument);
      break;

    case FACTORY_TESTING_CALIBRATE:
      if (argument == 0) {
        mode_ = UI_MODE_CALIBRATION_1;
      } else if (argument == 1) {
        CalibrateC1();
      } else {
        CalibrateC3();
        cv_scaler_->set_blend_parameter(static_cast<BlendParameter>(0));
        SaveState();
      }
      break;
  }
  return reply;
}

void Ui::SavePreset(void) {
  // Silence the processor during the long erase/write.
  processor_->set_silence(true);
  system_clock.Delay(5);
  processor_->ExportPreset(settings_->Preset(load_save_bank_, load_save_location_));
  settings_->SavePresets();
  processor_->set_silence(false);
  IncrementLoadSaveLocation();
}

void Ui::IncrementLoadSaveLocation(void) {
  settings_->IncrementPresetLocation(load_save_bank_, load_save_location_);
}

void Ui::DecrementPlaybackMode(void) {
  uint8_t mode =
    processor_->playback_mode() == 0 ? PLAYBACK_MODE_LAST - 1 : processor_->playback_mode() - 1;
  processor_->set_playback_mode(static_cast<PlaybackMode>(mode));
  SaveState();
}

void Ui::IncrementPlaybackMode(void) {
  uint8_t mode = (processor_->playback_mode() + 1) % PLAYBACK_MODE_LAST;
  processor_->set_playback_mode(static_cast<PlaybackMode>(mode));
  SaveState();
}

void Ui::VisualizePresetLocation(uint8_t fade, bool flash) {
  uint8_t red   = (load_save_bank_ & 1) ? 0 : 255;
  uint8_t white = (load_save_bank_ & 3) ? 255 : 0;
  for (size_t i = 0; i < kNumPresetLeds; i++) {
    leds_.set_status(i, fade & red, fade & white);
  }
  leds_.set_status(load_save_location_, flash ? red : 0, flash ? white : 0);
}

}  // namespace clouds
