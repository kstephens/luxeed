#!/usr/bin/env python3.11
from keyboard import Keyboard, key_iter
from key import LUXEED_NUM_OF_KEYS
import time
from random import randint
from icecream import ic

def main():
  keyboard = Keyboard()
  keyboard.open()
  keyboard.init()

  def update():
    keyboard.update()
    time.sleep(0.01)

  while True:
    keyboard.clear()
    update()

    for i in range(100):
      color = (randint(0, 255), randint(0, 255), randint(0, 255))
      keyboard.clear(color)
      update()

    for key in key_iter():
      keyboard.clear()
      color = (randint(0, 255), randint(0, 255), randint(0, 255))
      keyboard.set_key_color(key, color)
      update()

    keyboard.clear()
    update()

    for i in range(100):
      key = randint(0, LUXEED_NUM_OF_KEYS - 1)
      color = (randint(0, 255), randint(0, 255), randint(0, 255))
      keyboard.set_key_color(key, color)
      update()

main()
