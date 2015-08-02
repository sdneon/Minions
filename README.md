Super But Old Mini on's
=======================
Minions are small, yellow pill-like creatures who have existed since the beginning of time. Now the chosen ones invade Pebble Times.
These super heroes' bum shows the current hour, and a hand points to the minutes.

Inspired by a mashup of a minion and batman & superman picture somewhere on the web, and of course the uber cute Minions (butt) movie poster.

Heroes:
* Green Arrow.
* Aquaman.
* Avatar Aang.
* Batman.
* Captain America.
* The Flash.
* Hulk.
* Spiderman.
* Superman.
* Thor.

## Dev Notes
This is tough trying to load just 2 heroes (full screen PNGs)! Had to squeeze out every available byte by compressing the pictures, cutting out lots of code, etc!
The pictures alone had to be reduced to the Pebble palette with dithering, so that they will look fine on Pebble Time. Though they do look awful in en emulator as the individual pixels could be seen. Thankfully Pebble Time has a high dpi.
The hands were supposed to be unique to the heroes (e.g. red laser beams for Superminion), but that will have to wait till the memory situation improves.

The [Developer Retreat 2014](http://developer.getpebble.com/community/events/developer-retreat-2014/) tech videos were very helpful in providing tips to optimize for Pebble.

## Changelog
* v1.2 work-in-progress
  * Added backgrounds for day & night.
* v1.1
  * Added more hands.
  * Optimizing for size: aggressively remove unneeded codes/displays. E.g. circle around date.
* v1.0
  * Initial release.
 
##TODO
* Add alternate hands if memory permits. Outstanding: Captain America and Spiderman.
* Alternate backgrounds/colours for night, am/pm?
* Add how-to guide/tutorial on processing the PNGs.
