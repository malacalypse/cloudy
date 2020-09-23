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
// Driver for the front panel switches.

#include "switches.h"

#include <algorithm>

namespace clouds {

using namespace std;

void Switch::Init(GPIO_TypeDef* gpio, uint16_t pin) {
  gpio_            = gpio;
  pin_             = pin;
  debounce_buffer_ = 0xff;
  press_time_      = 0;
  state_           = SwitchReleased;
}

void Switches::Init() {
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  // TODO: Put these switch definitions in a configuration file.
  GPIO_InitTypeDef gpio_init;
  gpio_init.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_8;
  gpio_init.GPIO_Mode  = GPIO_Mode_IN;
  gpio_init.GPIO_OType = GPIO_OType_PP;
  gpio_init.GPIO_Speed = GPIO_Speed_25MHz;
  gpio_init.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &gpio_init);

  gpio_init.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11;
  gpio_init.GPIO_Mode  = GPIO_Mode_IN;
  gpio_init.GPIO_OType = GPIO_OType_PP;
  gpio_init.GPIO_Speed = GPIO_Speed_25MHz;
  gpio_init.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(GPIOC, &gpio_init);

  switches_[0].Init(GPIOC, GPIO_Pin_10);  // MODE
  switches_[1].Init(GPIOC, GPIO_Pin_11);  // WRITE
  switches_[2].Init(GPIOB, GPIO_Pin_6);   // FREEZE
  switches_[3].Init(GPIOB, GPIO_Pin_8);   // BYPASS
}

void Switches::Scan() {
  for (size_t i = 0; i < kNumSwitches; i++) {
    Switch* s = &switches_[i];
    s->scan();
  }
}

}  // namespace clouds
