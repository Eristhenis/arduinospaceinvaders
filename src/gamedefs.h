#ifndef gamedefsh
#define gamedefsh

#define SCREEN_WIDTH             128
#define SCREEN_HEIGHT            64
#define ALIEN_WIDTH              8
#define ALIEN_HEIGHT             8
#define SHIP_WIDTH               8
#define SHIP_HEIGHT              8
#define SHIP_X_MOVE              2
#define ENEMY_WAIT_BETWEEN_FIRE  5
#define PLAYER_WAIT_BETWEEN_FIRE 10
#define ROW1_ENEMY_COUNT         5
#define ROW2_ENEMY_COUNT         4
#define ENEMY_BULLET_SPEED       1
#define SHIP_BULL_SPEED          1
#define ALIEN_BETWEEN_OFFSET     18
#define ALIEN_INCREASE_SPEED_BY  1.2f
#define ALIEN_INCREASE_Y_BY      4
#define PLAYER_POINTS_PER_ALIEN  2
#define MAX_PLAYER_BULLETS       20
#define MAX_ENEMY_BULLETS        20
#define ENEMY_COUNT              (ROW1_ENEMY_COUNT + ROW2_ENEMY_COUNT) /* this should be a constant, but the compiler seems to be fine with this - presumably the compiler has inlined the addition anyway */

#define ALIVE                    7 /* may be any non negative non multiple of 2 */
#define START_DYING              (ALIVE - 1)
#define DEAD                     0 /* should always be 0 */

#define BUTTON_USER_LEFT         0
#define BUTTON_USER_RIGHT        1
#define BUTTON_USER_FIRE         2

#endif
