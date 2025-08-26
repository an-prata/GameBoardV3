# GameBoardV3
A half sized, 3D printed, and hand-wired keyboard for playing games. The keyboard uses an Adafruit SAMD21 QT Py board.

## Required Parts
All model files can be found in the `Models` directory, the SolidWorks files (`.SLDPRT` and `.SLDASM`) are not necessary unless you wish to make modifications to the models. All models are designed as to not require any support (tested on a Bambu A1 printer). When slicing the models you will need to reorrient them in your slicer, the Bambu Studio slicer's automatic orientation function finds the correct orientation for all these models if you intend to print them on a Bambu.

These parts will need to be 3D printed:
* `Base Plate.STL`
* `Switch Plate.STL`
* `Fastener.STL` (18x)

And you will also need these:
* Choice of mechanical keyboard switch (28x)
* Choice of 1U keycaps (28x)
  * The [OLKB Preonic Acute Keycaps](https://drop.com/buy/drop-olkb-preonic-acute-keycaps) are super cheap and fairly good quality, and are what I've used for my build.
* Wire
  * I'd recommend solid core wire for connecting switches to eachother in columns and rows, and stranded wire for connecting those columns and rows to the controller.
* Adafruit SAMD21 QT Py
  * I'm sure you could use any number of other controllers, this is just the one I've used and tested. You will however need at least 11 digital pins.
* Diode (28x)
  * Recourses I've found tend to recommend a [1N4148 diode](https://www.digikey.com/en/products/detail/onsemi/1N4148/458603). I'm fairly sure a number of diodes would work, and you can also find the 1N4148 diode on Amazon if DigiKey isn't your deal. Its advisable to get more than just 28 in case of mistakes.

Optionally you may print one or both of these to give your keyboard a ten degree tilt:
* `Angled Holder.STL`
  * Single large part. I like to do this in a different color to the plate files.
* `Angled Plate.STL` + `Angled Plate Peg.STL` (14x)

> NOTE: For the `Fastener.STL` file and `Angled Plate Peg.STL` I recommend printing a few spare, prints for these guys fail pretty easily and they can be damaged when trying to install. I've done 20 and 16 respectively and thats been plenty.

> Assembly segment coming soon.
