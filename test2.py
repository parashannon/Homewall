import board
import neopixel
from time import sleep

pixels = neopixel.NeoPixel(board.D18,10,brightness = 1)
pixels.fill((0,0,0))
sleep (2)
pixels.fill((255,0,0))
pixels.show()
