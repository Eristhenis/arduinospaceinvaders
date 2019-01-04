# Project Title

Space Invaders -- Arduino simulation

## Overview

Simple Space Invaders arcade game implemented in C for an Arduino Mega platform with KS0108B text graphics display and push buttons as controllers.

## Description

For an honours project class in my undergraduate degree, we were tasked to put together a small embedded device using an Arduino Mega controller. It could be anything really but I really wanted to try and do something a bit different and more impressive so I originally attempted to port a working emulator of a gameboy from windows into the small arduino platform. 

I realized with time (to render the screen) and memory constraints, not to mention the time constraints on the project itself, this was becoming a bad idea. I had thought that maybe I could take away some of the memory constraints by using external SRAM, however this would have introduced massive amounts of lag in the time taken to fetch the new program instructions or data, which would have made the whole thing completely infeasible to play.

With my time consumed on trying to bring my idea into reality, my other classes were starting to feel the brunt. I eventually decided it would be best to abandon this approach after about two weeks into it. It was just the weekend before submission was due and instead I quickly coded up a -much simpler- space invaders game that I could port to the Arduino instead. 

I opted to recreate this using a few push buttons and a KS0108B graphics display. Before I did port the code to the device, I tested the game logic by writing the playable Java simulation which is also available separately at https://github.com/ngb07170/spaceinvaders. 

<table>
<tr>
<td><img src="https://github.com/ngb07170/arduinospaceinvaders/blob/master/image3.png?raw=true"  width="200px" /></td>
<td><img src="https://github.com/ngb07170/arduinospaceinvaders/blob/master/image4.png?raw=true" width="200px"/></td>
<td><img src="https://github.com/ngb07170/arduinospaceinvaders/blob/master/image2.png?raw=true" width="200px" /></td>
<td><img src="https://github.com/ngb07170/arduinospaceinvaders/blob/master/image.jpg?raw=true" width="200px" /></td>
</tr>
</table>

## Deployment

To demo this application, you will need to recreate the arduino circuit shown in the images. Unfortunately I did not document which pins and ports I used at the time but made it fairly easy to change these to suit individual device set ups. To build the source files I recommend the AVR-eclipse plugin and the Eclipse C/C++ IDE. The Arduino IDE will probably also work but I haven't tested it with that particular IDE.

## Built With

* [Eclipse](https://www.eclipse.org/) - The IDE used

## Versioning

Version 1.0 of the Java source only.

## Authors

* **Michael Nolan** - *Initial work* - [ngb07170](https://github.com/ngb07170)

