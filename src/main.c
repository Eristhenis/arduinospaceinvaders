/*
  main.c - this file is responsible for the  main
  procedure as well as all of the Space  Invaders
  replica logic.

  In this file there should be no reference to any
  pins, ports or hardware-specific registers making
  it easy to port to other devices. Note that the
  following functions should be implemented (as
  found in other files) if somebody wishes to port
  this to another device:

   * all functions in lcd.h,
   * all functions in input.h,
   * and init function.

  Author: Group 10 (Michael Nolan)
*/
#include <WProgram.h>
#include <avr/pgmspace.h>
#include "lcd.h"
#include "input.h"
#include "gamedefs.h"

/* GAME DATA */
static uint8_t                   lives;
static uint8_t                   shipAlive;
static uint8_t                   shipX,  shipY;
static float                     enemyX, enemyY;
static float                     enemyDX;
static uint8_t                   enemyAlive[ENEMY_COUNT];
static uint8_t                   enemyRemaining;
static uint8_t                   currBulletId;
static uint8_t                   bulletWait;
static uint8_t                   bulletX[MAX_PLAYER_BULLETS];
static uint8_t                   bulletY[MAX_PLAYER_BULLETS];
static uint8_t                   currEnemyBulletId;
static uint8_t                   enemyBulletWait;
static uint8_t                   enemyBulletX[MAX_ENEMY_BULLETS];
static uint8_t                   enemyBulletY[MAX_ENEMY_BULLETS];

/* alien8 and ship8 are 8x8 bitmaps used to represent
 * the aliens and the player's ship respectively. They
 * are encoded horizontal-first, i.e. each byte of the
 * 8 bytes given is a horizontal row where 1 is pixel
 * and 0 is no-pixel (i.e. space).
 */
static char alien8[] = { 0x00, 0x18, 0x3C, 0x7E, 0x5A, 0xFF, 0x54, 0xAA };
static char ship8 [] = { 0x00, 0x18, 0x3C, 0x18, 0x99, 0xBD, 0xff, 0xE7 };

/* miscellaneous helpers */
static char     stringHolder[17];

/* 17-byte strings (16 characters plus 1 byte null terminator in C) */
/* (keeping strings out of the stack since they really eat it up )  */
volatile const char __attribute((__progmem__)) GAME_OVER_STRING[] = { "Game Over       " };
volatile const char __attribute((__progmem__)) PRESS_ANY_STRING[] = { "Press any key to" };
volatile const char __attribute((__progmem__)) PLAY_AGAN_STRING[] = { "play again      " };

/* static prototypes */
static void gameReset(void);
static void gameOver (void);
static void gameLoop (void);
static void draw8by8 (char* bitmap, uint8_t x, uint8_t y);

/* macros */
#define drawAlien(x, y)  { draw8by8(alien8, x, y); }
#define drawShip(x, y)   { draw8by8(ship8,  x, y); }
#define drawBullet(x, y) { lcdDrawPixel(x, y); lcdDrawPixel(x, y+1); }

static void gameReset(void)
{
	uint8_t i;

	lives             = 3;
	shipAlive         = ALIVE;
	shipX             = 60;
	shipY             = 50;
	enemyX            = 10.0f;
	enemyY            = 0.0f;
	enemyDX           = 0.5f;
	enemyRemaining    = ENEMY_COUNT;
	currBulletId      = 0;
	bulletWait        = 0;
	currEnemyBulletId = 0;
	enemyBulletWait   = ENEMY_WAIT_BETWEEN_FIRE;

	for(i=0; i<ENEMY_COUNT; i++){
		enemyAlive[i] = ALIVE;
	}

	for(i=0; i<MAX_PLAYER_BULLETS; i++){
		bulletX[i]      = bulletY[i]      = 0;
	}

	for(i=0; i<MAX_ENEMY_BULLETS; i++){
		enemyBulletX[i] = enemyBulletY[i] = 0;
	}
}

static void gameOver(void)
{
	gameReset();

	memcpy_P(stringHolder, GAME_OVER_STRING, sizeof(GAME_OVER_STRING) );
	lcdPrintText(stringHolder, 0);
	memcpy_P(stringHolder, PRESS_ANY_STRING, sizeof(PRESS_ANY_STRING) );
	lcdPrintText(stringHolder, 1);
	memcpy_P(stringHolder, PLAY_AGAN_STRING, sizeof(PLAY_AGAN_STRING) );
	lcdPrintText(stringHolder, 2);

	lcdRepaint();
	while( isAnyKeyDown()); /* wait firstly for user to release any keys */
	while(!isAnyKeyDown()); /* wait for another press */
}

static void draw8by8(char* bitmap, uint8_t x, uint8_t y)
{
	/* Given that the lcd display uses an 8 bit vertically
	 * aligned pixel bus, it would have been more optimal
	 * if we had arranged the bitmaps for the ships and
	 * aliens vertically-aligned instead of horizontally
	 * as was done and then blitted only to the two 8-bit
	 * segments that we needed to on the LCD.
	 *
	 * However, this method is pretty general and could be
	 * ported to a much wider number of devices - not to
	 * mention it is easier to read, understand and maintain.
	 */

	uint8_t i, j;

	for(j=0; j<8; j++){
		for(i=0; i<8; i++){
			if((bitmap[j] & (1<<i)) != 0x00){
				lcdDrawPixel(x+i, y+j);
			}
		}
	}
}

static void gameLoop(void)
{
	uint8_t i, j, k;
	int x, y;
	int shipToFire = -1;

	/* Check user input.   If any performed logic
	 * needs   to   be   then   we   do   so here.
	 */
	if(isButtonDown(BUTTON_USER_LEFT)){
		if(shipX > 0)
			shipX -= SHIP_X_MOVE;
	}

	if(isButtonDown(BUTTON_USER_RIGHT)){
		if(shipX < (SCREEN_WIDTH-SHIP_WIDTH))
			shipX += SHIP_X_MOVE;
	}

	/* Check to see if the user is allowed to fire.
	 */
	if(bulletWait > 0){
		bulletWait -- ;
	}

	/* If the user  is  allowed to fire (i.e. the
	 * fire-waiting  count down  has reached zero)
	 * and the user  is pressing the  fire  button
	 * then we fire a bullet.
	 */
	if(isButtonDown(BUTTON_USER_FIRE) && bulletWait == 0){
		/* The bullet should be offset so it gives
		 * the appearance that is is  coming from
		 * the front-middle of our ship.
		 */
		bulletX[currBulletId] = shipX + SHIP_WIDTH/2;
		/* By design, the ship's Y coordinate  is
		 * at  the  top-most  point  of the  ship
		 * already.
		 */
		bulletY[currBulletId] = shipY;

		/* There will only be a finite number  of
		 * bullets that can be displayed  on  the
		 * display at one time  (since there is a
		 * waiting-fire count  down). As such, we
		 * need only keep track of visible bullets
		 * in a small circular buffer.
		 */
		currBulletId++;
		currBulletId %= MAX_PLAYER_BULLETS;

		/* After the user has fired  a bullet set
		 * the waiting-fire count down to the value
		 * specified above (PLAYER_WAIT_BETWEEN_FIRE)
		 * to allow a delay between firing.
		 */
		bulletWait    = PLAYER_WAIT_BETWEEN_FIRE;
	}

	/* At this point we draw the ship, but we do
	 * so only if 1. the ship is alive and 2. if
	 * shipAlive is an odd number. The reason for
	 * this is to create a simple special effect
	 * whereby when the user is shot by an alien
	 * ship and hence dying, we 'flash' the ship
	 * to draw attention from the user and let
	 * them realise if they haden't already that
	 * they've lost a life.
	 *
	 * The 'special effect' is created simply by
	 * drawing the ship when shipAlive is odd
	 * and not drawing it when it is even - there-
	 * -fore we go through values of shipAlive
	 * before reaching zero (and hence the eventual
	 * demise of the player) to generate the off-on
	 * flashing of the ship.
	 */
	if(shipAlive % 2 == 1){
		/* since this reduces shipAlive (mod 2)
		 * then this if statement does not require
		 * the '== 1' but it is left simply for
		 * clarity */
		drawShip(shipX, shipY);
	}
	if(shipAlive > 0 && shipAlive < ALIVE){
		/* As was fore-mentioned,  we   decrement
		 * shipAlive when it is not either 0  or
		 * 'ALIVE' and hence is being responsible
		 * for the effect.
		 */
		shipAlive --;
	}
	/* When the eventual demise of the ship does
	 * occur  we  decide  how to proceed in this
	 * small if-branch.
	 */
	if(shipAlive == 0){
		/* Either  the  user has lives  remaining
		 * and is allowed to continue the game as
		 * it stands.
		 */
		if(lives > 1){
			/* To continue on, but   with one less
			 * life, we simply decrement the lives
			 * and change  the state of  shipAlive
			 * to ALIVE.
			 */
			lives --;
			shipAlive = ALIVE;

			/* To give the user  a  bit more of a
			 * chance.  We  allow  them  to  fire
			 * right-away after dying,  but  also
			 * offset the length of time that the
			 * aliens must wait  prior  to  their
			 * next fire.
			 */
			bulletWait      = 0;
			enemyBulletWait = ENEMY_WAIT_BETWEEN_FIRE;
		}else{
			/* Or the user has no  lives and must
			 * go back to the beginning.
			 */
			gameOver();
			return;
		}
	}

	/* There is also the other condition that will
	 * cause the game to end. This is when the user
	 * wins -  although  at  current  there are no
	 * further levels, as such when the user wins
	 * it is the same as loosing and the game will
	 * reset to allow the player to have another go.
	 */
	if(enemyRemaining <= 0){
		/* Player won, but still GAME OVER :( */
		gameOver();
		return;
	}

	/* When the enemy aliens are allowed to shoot
	 * then the firing mechanism  is quite simple
	 * but addresses  a  problem  with the layout
	 * of the aliens.  The aliens  in  this  game
	 * are drawn in 2  rows,  each row  is  drawn
	 * separately and the aliens in that particular
	 * row are offset  differently (in the second
	 * row there is a separator of about 9 pixels
	 * from the left  side of the  screen). If we
	 * were to ID each alien  ship  and then pick
	 * a ship at  random from  which should  fire
	 * we would need to:
	 *
	 *   1. check that ship is alive,
	 *
	 *   2. find  the (x, y)-coordinates of  that
	 *      ship.
	 *
	 * 1. is trivial, however although 2. can  be
	 * easily  worked  out   it   is   far   more
	 * simple to keep track of the number of ships
	 * remaining and pick a random number less than
	 * this we then, as we come  to draw each row,
	 * deduct 1 from this number each time we draw
	 * a new alien -  once we reach zero then the
	 * intended  alien has been found and we will
	 * have handy at that point the (x, y)-coordinates
	 * of that alien (as would be needed to draw
	 * it anyway). This is far more convenient.
	 *
	 * Also  note  here  the  the  aliens  firing
	 * sequence must also obey a similar delay to
	 * which the  player must obey  (usually this
	 * will disadvantage the aliens more).
	 */
	if(enemyBulletWait > 0){
		enemyBulletWait -- ;
		if(enemyBulletWait == 0 && enemyRemaining > 0){
			shipToFire      = rand() % enemyRemaining;
			enemyBulletWait = ENEMY_WAIT_BETWEEN_FIRE;
		}
	}

	/* Here is where we start to draw the aliens.
	 *
	 * As fore-mentioned, the aliens are drawn in
	 * two rows, as  such two different for-loops
	 * have  been  tiled  out to   produce   each
	 * row separately.
	 *
	 * While this produces better speed, it is less
	 * convenient for adding more rows and hence more
	 * aliens - although other ways were considered
	 * for this purpose, the LCD display being used
	 * to  demonstrate  this has a somewhat small
	 * resolution and as such there is not much room
	 * for more aliens  - maybe one more row, but
	 * certainly not much more! As such, the tiling
	 * for-loops were chosen.
	 */
	for(i=0; i<ROW1_ENEMY_COUNT; i++){
		/* We use the same method here  as we had
		 * done with the  player-ship to generate
		 * the  simple  special  effect   of  the
		 * ship-dying  but   now with the  aliens.
		 */
		if(enemyAlive[i] % 2 == 1){
			x = (int)(enemyX + ALIEN_BETWEEN_OFFSET*i);
			y = (int)(enemyY + 0);

			drawAlien(x, y);

			/* This is commented  as above.  Here
			 * we are looking to  see wither   or
			 * not   the   ship   currently being
			 * drawn   is   the  one selected for
			 * firing.
			 *
			 * If not  ship  was  selected,  then
			 * shipToFire will be -1 when we start
			 * to draw ships, equivalently (since
			 * we  are  using  8-bits to hold it,
			 * 0xff = 255, so  as long  as we are
			 * not drawing 255 alien ships - which
			 * we wont as they  wouldn't even fit
			 * into the  screen,  this  will  not
			 * reach zero  when  no ship has been
			 * selected to fire.)
			 */
			if(shipToFire-- == 0){
				/* The aliens will fire 'downward'
				 * as opposed to  upward like the
				 * player's ship  will be  doing.
				 * As  such,  although  we   will
				 * prefer the x-coordinate of the
				 * bullet to begin in the  middle
				 * of  the  alien's  ship,    the
				 * y-coordinate  will  be at  the
				 * bottom (which  is   the  front
				 * relative to the aliens) so  we
				 * add ALIEN_HEIGHT,  the  height
				 * in pixels of the alien ship to
				 * the enemy bullet's y-coordinate.
				 */
				enemyBulletY[currEnemyBulletId] = y + ALIEN_HEIGHT  ;
				enemyBulletX[currEnemyBulletId] = x + ALIEN_WIDTH /2;

				/* As with  the player's  bullets
				 * only a small number of bullets
				 * will  be  visible at any  time
				 * so we only keep track of those.
				 */
				currEnemyBulletId++;
				currEnemyBulletId %= MAX_ENEMY_BULLETS;
				enemyBulletWait    = ENEMY_WAIT_BETWEEN_FIRE;
			}

			/* In keeping with the original Space
			 * Invaders idea, when the  left-most
			 * or right-most  alien  touches  the
			 * edge of the screen, the aliens will
			 * descend a little  bit further  and
			 * increase speed.
			 */
			if( x <= 0 || (x + ALIEN_WIDTH) >= SCREEN_WIDTH ){
				/* Here we  switch the   direction
				 * the aliens are travelling along
				 * the x-axis  and   increase  the
				 * speed by the ALIEN_INCREASE_SPEED_BY
				 * factor. Also,  we  increase the
				 * y-coordinate of all aliens too.
				 * (Keep in mind   that the higher
				 * the y-coordinate, the lower down
				 * the screen we are  going - this
				 * is just due to the way we happened
				 * to insert  the screen  into the
				 * breadboard and could be changed
				 * easily later.)
				 */
				enemyDX *= -ALIEN_INCREASE_SPEED_BY;
				enemyY  +=  ALIEN_INCREASE_Y_BY;
			}

			/* We want to check that the aliens haven't
			 * touched down on  the planet either
			 * because then the  player has  lost
			 * and it's game over.
			 */
			if(y + ALIEN_HEIGHT >= SCREEN_HEIGHT){
				gameOver();
				return;
			}

			/* Naturally since we have the (x, y)
			 * coordinates of the alien we're drawing
			 * right now at our disposal we can also
			 * check wither any of the bullets that
			 * the player has shot will intersect with
			 * (and thus kill) this alien.
			 *
			 * Thus could be better done by solving
			 * some linear equations at the time when
			 * the player fired the bullet (since
			 * the aliens are obviously going in a
			 * predictable  path).  The  approach
			 * implemented below however is more obvious
			 * and easier to read and is more than
			 * sufficient for this little project.
			 *
			 * (It is a simple brute force algorithm
			 * that checks if any active player bullet
			 * has come within the rectangular
			 * bounds of the alien currently being
			 * drawn.
			 */
			for(j=0; j<MAX_PLAYER_BULLETS; j++){
				/* Since no alien ship will ever
				 * be at coordinate (0, 0) then
				 * there is no additional requirement
				 * to check wither a bullet is
				 * 'active' or not - although anybody
				 * intending to do work on this
				 * should be aware that any bullets
				 * at coordinates (0, 0) are considered
				 * inactive and should not be tested.
				 */
				if(    x <= bulletX[j] && bulletX[j] <= x + ALIEN_WIDTH
					&& y <= bulletY[j] && bulletY[j] <= y + ALIEN_HEIGHT){

					/* To begin the dying effect
					 * we simply set the enemyAlive
					 * status for this particular
					 * alien to be 'START_DYING' */
					enemyAlive[i] = START_DYING;

					/* Also blow up bullet that hit enemy.
					 * There are two reasons for doing this
					 * firstly, the physical significance
					 * is that if a bullet has hit something
					 * it is probably going to stop with that.
					 * Secondly, we don't want to let the
					 * player be able to kill  two aliens
					 * with one bullet - it would be unfair
					 * to the aliens, even they deserve a
					 * chance! */
					bulletX[j] = bulletY[j] = 0;
				}
			}
		}
		/* Now, when the enemy is dying, we want
		 * to make sure we create the same flashing
		 * effect as we did before for the ship.
		 * Here we are  doing that same  thing
		 * (i.e. but decrementing the value of
		 * enemyAlive specific  to that  dying
		 * alien ship now).
		 */
		if(enemyAlive[i] > 0 && enemyAlive[i] < ALIVE){
			enemyAlive[i] --;
			if(enemyAlive[i] == 0){
				enemyRemaining--;
			}
		}
	}

	/* The following for loop is much like the
	 * previous one, it simply deals with  the
	 * logic and drawing the aliens of the second
	 * row, the only notable difference  being
	 * that the aliens of  the  second row are
	 * slightly offset coordinate wise.
	 */
	for(i=0; i<ROW2_ENEMY_COUNT; i++){
		/* The offset variable k is used merely
		 * for convenience here.
		 */
		k = ROW1_ENEMY_COUNT + i;
		if(enemyAlive[k] % 2 == 1){
			x = (int)(enemyX + 9 + ALIEN_BETWEEN_OFFSET*i);
			y = (int)(enemyY + ALIEN_HEIGHT);

			drawAlien(x, y);

			if(shipToFire-- == 0){
				enemyBulletY[currEnemyBulletId] = y  + ALIEN_HEIGHT  ;
				enemyBulletX[currEnemyBulletId] = x  + ALIEN_WIDTH /2;

				currEnemyBulletId++;
				currEnemyBulletId %= MAX_ENEMY_BULLETS;
				enemyBulletWait    = ENEMY_WAIT_BETWEEN_FIRE;
			}

			if( x <= 0 || SCREEN_WIDTH <= (x + ALIEN_WIDTH) ){
				enemyDX *= -ALIEN_INCREASE_SPEED_BY;
				enemyY  += ALIEN_INCREASE_Y_BY;
			}

			if(y + ALIEN_HEIGHT >= SCREEN_HEIGHT){
				gameOver();
				return;
			}

			for(j=0; j<MAX_PLAYER_BULLETS; j++){
				if(    x <= bulletX[j] && bulletX[j] <= x + ALIEN_WIDTH
					&& y <= bulletY[j] && bulletY[j] <= y + ALIEN_HEIGHT){
					enemyAlive[k] = START_DYING;
					bulletX[j]    = bulletY[j] = 0;
				}
			}
		}

		if(enemyAlive[k] > 0 && enemyAlive[k] < ALIVE){
			enemyAlive[k] --;
			if(enemyAlive[k] == 0){
				enemyRemaining--;
			}
		}
	}

	/* We alter the x-offset of every alien by
	 * altering enemyX. We use this to move aliens
	 * left or right depending on enemyDX. */
	enemyX += enemyDX;

	/* Here we loop through and draw all bullets.
	 * Originally we considered using one array to
	 * hold all (enemy and player) bullets which
	 * would have made drawing them all in one loop
	 * easily enough to do, but in testing which
	 * bullets should affect the player ship
	 * and which bullets should affect alien ships
	 * we found that two separate arrays are easier
	 * to maintain - some other ideas included
	 * using two parts (the top and bottom) of the
	 * same array like the way computers generally
	 * store stack and the heap side-by-side but this
	 * is a fairly simple game, such alterations were
	 * found to be unnecessary as speed here was not
	 * as much a concern (see main function). */
	for(i=0; i<MAX_PLAYER_BULLETS; i++){
		/* Recall that a bullet is inactive (and hence
		 * is not drawn) if it is a coordinate (0, 0).
		 */
		if(bulletX[i] != 0 && bulletY[i] != 0){
			drawBullet(bulletX[i], bulletY[i]);

			/* Update bullet position while we're drawing */
			bulletY[i] -= SHIP_BULL_SPEED;

			/* From a players perspective the bullet
			 * moves up and hence down y-coordinates
			 * (recall the flip LCD breadboard comment above) as such
			 * when the bullet is 0 (or goes below - but
			 * by using unsigned integers it can only be above
			 * or equal to 0) then we must de-activate it by
			 * setting both x and y coordinate to zero.
			 *
			 * In the instance that SHIP_BULL_SPEED is not 1
			 * then it will be possible for the bullet's
			 * y coordinate to slip below zero, as we use
			 * unsigned integers to store bulletY this will
			 * cause bulletY to underflow up to (depending on
			 * SHIP_BULL_SPEED) about 0xff, if SHIP_BULL_SPEED
			 * isn't too big then we can spot this by noticing
			 * that the bulletY coordinate is much larger than
			 * the screen height and hence de-activate this
			 * bullet under the assumption underflow happened
			 * and we went below zero.
			 */
			if(bulletY[i] == 0 || bulletY[i] > SCREEN_HEIGHT){
				/* Remove the bullet if it is no longer visible */
				bulletX[i] = 0;
				bulletY[i] = 0;
			}
		}
	}

	/* The following is the loop used to render any
	 * bullets that enemy aliens have shot toward the
	 * player. We also check here if any of the bullets
	 * intersect the ship's rectangular bounds and hence
	 * hit. This is a pretty simple collision detection
	 * scheme - i.e. it's not pixel to pixel, so it is
	 * possible that the player could avoid a bullet by
	 * a pixel on the screen but because of this collision
	 * detection the game will still register that as
	 * a hit - although in this project we saw no reason
	 * to implement any more advanced schemes for collision
	 * detection, we are very much aware of them.
	 */
	for(i=0; i<MAX_ENEMY_BULLETS; i++){
		if(enemyBulletX[i] != 0 && enemyBulletY[i] != 0){
			drawBullet(enemyBulletX[i], enemyBulletY[i]);

			enemyBulletY[i] += ENEMY_BULLET_SPEED;

			if( shipX <= enemyBulletX[i] && enemyBulletX[i] <= shipX+SHIP_WIDTH &&
				shipY <= enemyBulletY[i] && enemyBulletY[i] <= shipY+SHIP_HEIGHT){
				/* Start player's dying animation */
				if(shipAlive == ALIVE){
					shipAlive = START_DYING;
				}

				/* Destroy the bullet */
				enemyBulletX[i] = enemyBulletY[i] = 0;
			}

			if(enemyBulletY[i] >= SCREEN_HEIGHT){
				/* Remove bullet as it is no longer visible */
				enemyBulletX[i] = 0;
				enemyBulletY[i] = 0;
			}
		}
	}
}

int main(void)
{
	/* Must call init for arduino to work properly */
	init();
	initLcdScreen();
	initButtons();

	gameReset();

	while(1){
		/* Before we render a frame we clear the frame buffer */
		lcdClear();

		/* Now we render the scene - as this is a microcontroller
		 * and not a full-blown GPCPU we've chosen to also do the
		 * game logic in gameLoop, this would probably otherwise
		 * be decoupled into two functions
		 *   * update, and
		 *   * render.
		 */
		gameLoop();

		/* We flush the frame buffer out to the lcd screen here */
		lcdRepaint();

		/* We keep it at about 15 fps for nice retro effect :) */
		delay(50);
	}
}
