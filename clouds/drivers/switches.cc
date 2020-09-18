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

#include "clouds/drivers/switches.h"

#include <algorithm>

namespace clouds {

using namespace std;

void Switches::Init() {
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

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

  fill(&switch_state_[0], &switch_state_[kNumSwitches], 0xff);
}

void Switches::Debounce() {
  const uint16_t pins[] = { GPIO_Pin_11, GPIO_Pin_10 };
  for (uint8_t i = 0; i < 2; ++i) {
    switch_state_[i] = (switch_state_[i] << 1) | GPIO_ReadInputDataBit(GPIOC, pins[i]);
  }
  switch_state_[2] = (switch_state_[2] << 1) | GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8);
  switch_state_[3] = (switch_state_[3] << 1) | GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6);
}

}  // namespace clouds
