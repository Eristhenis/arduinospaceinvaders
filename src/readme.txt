Throughout the code there will  be  instances where  code  could have been
simplified,  for  instance where  we  have chosen to use '1<<1' as opposed
to simply  0x02,  or   2.  We have left the shift operators in for clarity
where  appropriate, for  example we will often access pins on a particular
port  by  comparing  the  data on that port with '1<<3' to get pin 3 data,
from this it is obvious we are refering to pin 3, whereas 0x04 is somewhat
less obvious. This does also include the trivial '1<<0' to refer to pin  0
where no shift operation actually takes place.

The source code is split into 2 parts: software files and hardware files.

  main.c 
    is responsible for calling hardware
    set up code and also holds all of the game
    logic but there should not be any explicit
    reference to any hardware-specific registers
    or interrupts or anything that may prevent
    this file from being ported to another
    hardware easily - this file should be easy
    to port to multiple platforms.
    
  lcd.c, input.c
    These files are hardware specific. They will
    change depending on the hardware and circuit
    diagram. The code provided is quite easy to
    read and hence we hope can be changed easily 
    for other platforms or circuits if needed.
    
  gamedefs.h
    Due too the need to keep some constants for
    the game, we have chosen to lump them all to-
    -gether into an easy-to-change configuration
    header. (This is used by the game logic in
    main.c )
    
  data.c
    Somethings were needed to be kept in program
    memory space, and these took up a lot of code
    in other files, as such, we have kept these
    look up tables in this file for convienence.
    
  wiring.c
    This is the only Arduino-provided file that
    we decided to keep, because of it's useful
    delay functions which we make use of both in
    main.c and in lcd.c as well as the Arduino
    setup function 'init' 