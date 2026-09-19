#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 360
#define SCALE 2.5

/* #define WINDOW_WIDTH 1900 */
/* #define WINDOW_HEIGHT 1100 */
#define WINDOW_WIDTH (SCREEN_WIDTH * SCALE)
#define WINDOW_HEIGHT (SCREEN_HEIGHT * SCALE)

#define TILESIZE 16
/* #define TILES_X (SCREEN_WIDTH / TILESIZE) */
/* #define TILES_Y ((SCREEN_HEIGHT / TILESIZE) - 2) */
#define TILES_X 40
#define TILES_Y 40

#define TILESHEET_WIDTH 49
#define TILESHEET_HEIGHT 22
#define TILESHEET_GAP 1

#define LEVEL_MIN_ROOMS 20
#define LEVEL_MAX_ROOMS 500
#define LEVEL_TILES_TOTAL (TILES_X * TILES_Y)
#define MAX_LEVELS 10
#define LEVEL_MIN_ENEMIES 6
#define LEVEL_MAX_ENEMIES 12

#define ROOM_MIN_WIDTH 2
#define ROOM_MAX_WIDTH 14
#define ROOM_MIN_HEIGHT 2
#define ROOM_MAX_HEIGHT 14
#define ROOM_MIN_ENEMIES 0
#define ROOM_MAX_ENEMIES 2

#define TILE_WALL 843
#define TILE_FLOOR 0
#define TILE_DIRT_VARIANT_1 1
#define TILE_DIRT_VARIANT_2 2
#define TILE_DIRT_VARIANT_3 3
#define TILE_DIRT_VARIANT_4 4
#define TILE_GRASS 98
#define TILE_STAIRS_UP 296
#define TILE_STAIRS_DOWN 297
#define TILE_BAT 418
#define TILE_RAT 423
#define TILE_SPIDER 275
#define TILE_TREE_VARIANT_1 49
#define TILE_TREE_VARIANT_2 50
#define TILE_TREE_VARIANT_3 51
#define TILE_TREE_VARIANT_4 52
#define TILE_TREE_VARIANT_5 53
#define TILE_TREE_VARIANT_6 54

#define TILE_PLAYER_BLUE 361
#define TILE_PLAYER_YELLOW 410
#define TILE_PLAYER_GREEN 459

#define MOVE_SPEED 10.0f
#define FOV_RADIUS 10

#define VIEW_WIDTH (SCREEN_WIDTH * SCALE)
#define VIEW_HEIGHT (SCREEN_HEIGHT * SCALE)

typedef struct Camera
{
    float x;
    float y;
} Camera;

typedef struct Tile
{
    int x;
    int y;
    int tileset_index;
    bool blocked;
    bool transparent;
    bool visible;
    bool seen;
    bool has_enemy;
    SDL_Rect src;
} Tile;

typedef struct Room
{
    int x;
    int y;
    int w;
    int h;
    int num_enemies;
    int max_enemies;
} Room;

typedef struct Player
{
    int x;
    int y;
    SDL_Rect src;
    int damage;
    // animation
    bool is_moving;
    float move_progress;
    float start_x;
    float start_y;
    float target_x;
    float target_y;
    // attacking
    int attacking_phase;
} Player;

typedef enum
{
    SPIDER = 1,
    RAT = 2,
    BAT = 3,
} EnemyType;

typedef enum
{
    IDLE,
    CHASING,
    ATTACKING,
} EnemyState;

typedef struct Enemy
{
    EnemyType type;
    int x;
    int y;
    SDL_Rect src;
    int health;
    // animation
    bool is_moving;
    float move_progress;
    float start_x;
    float start_y;
    float target_x;
    float target_y;
    // hit flash animation
    bool is_hit;
    float hit_timer;
    float hit_duration;

    int attacking_phase;
    
    EnemyState state;
} Enemy;

typedef struct Level
{
    Tile tiles[LEVEL_TILES_TOTAL];
    Room rooms[LEVEL_MAX_ROOMS];
    int num_rooms;
    Enemy enemies[LEVEL_MAX_ENEMIES];
    int num_enemies;
} Level;

typedef struct Game
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    Level levels[MAX_LEVELS];
    int current_level;
    int turn;
    bool turn_taken;
} Game;
// structs

int rand_in_range(int min, int max);
int rand_one_of(int a, int b);
bool rand_true_or_false();
void insertion_sort_int(int arr[], int size);
float lerp(float a, float b, float t);

bool sdl_init(Game* game);
void sdl_quit(Game* game);

int tile_get_index(int x, int y);
int tile_get_player(Level* level);
bool tile_is_in_bounds(int x, int y);
void tileset_create(SDL_Rect* tileset);

void camera_update(Level* level, float dt);

void level_create(void);
void level_set_tile(Level* level, int x, int y, int tileset_index);
void level_update(Level* level, float dt);
void level_draw(Level* level);
void level_set_rooms(Level* level);
void level_set_stairs_down(Level* level);
int level_get_tile_index(Level* level, int tile);
void level_enemies_create(Level* level);

void room_create(Room* room);
bool room_collides(Room* r1, Room* r2);
bool room_contains(Room* r, int x, int y);
void insertion_sort_room(Room arr[], int size, bool x, bool greater);

void corridors_create(Level* level);
void corridors_create_horizontal(Level* level, int x1, int x2, int y);
void corridors_create_vertical(Level* level, int y1, int y2, int x);

void player_init(Player* player);
void player_draw(Level* level);
void player_move(Level* level, int x, int y);
void player_set_pos_coord(int x, int y);
void player_set_pos_tile(int tile);

int get_sign(int a);
bool line_of_sight(Level* level, int x1, int y1, int x2, int y2);
void fov_make(Level* level);
void fov_clear(Level* level);

bool enemy_create(Level* level);
void enemy_draw(Level* level, Enemy* enemy);
void enemy_take_damage(Level* level, int enemy_index);
void enemy_update(Level* level, Enemy* enemy);
// funcs

Game game;
Player player;
Camera camera;
SDL_Texture* spritesheet;
SDL_Texture* spritesheet_transparent;
SDL_Rect tileset[TILESHEET_WIDTH * TILESHEET_HEIGHT];
SDL_Rect tileset_transparent[TILESHEET_WIDTH * TILESHEET_HEIGHT];

//startmain
int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    srand(time(0));

    memset(&game, 0, sizeof(Game));
    memset(&player, 0, sizeof(Player));

    game.running = true;

    if (!sdl_init(&game))
    {
        return 1;
    }
    SDL_ShowCursor(SDL_DISABLE);

    spritesheet = IMG_LoadTexture(game.renderer, "./colored.png");
    SDL_SetTextureBlendMode(spritesheet, SDL_BLENDMODE_BLEND);
    spritesheet_transparent = IMG_LoadTexture(game.renderer, "./colored-transparent.png");
    SDL_SetTextureBlendMode(spritesheet, SDL_BLENDMODE_BLEND);
    tileset_create(tileset);
    tileset_create(tileset_transparent);
    level_create();
    game.current_level = 0;
    game.turn = 0;
    game.turn_taken = false;
    Level* level = &game.levels[game.current_level];
    player_init(&player);
    player_set_pos_tile(level_get_tile_index(level, TILE_STAIRS_UP));
    camera.x = 0.0f;
    camera.y = 0.0f;
    fov_make(level);

    Uint32 frame_start;
    float dt = 0.0f;

    SDL_Event event;
    while (game.running)
    {
        frame_start = SDL_GetTicks();

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT: game.running = false; break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        game.running = false;
                        break;
                    }
                    else if (!player.is_moving)
                    {
                        if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_k)
                        {
                            player_move(level, 0, -1);
                            break;
                        }
                        if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_j)
                        {
                            player_move(level, 0, 1);
                            break;
                        }
                        if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_h)
                        {
                            player_move(level, -1, 0);
                            break;
                        }
                        if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_l)
                        {
                            player_move(level, 1, 0);
                            break;
                        }
                        if (event.key.keysym.sym == SDLK_y)
                        {
                            player_move(level, -1, -1);
                            break;
                        }
                        if (event.key.keysym.sym == SDLK_u)
                        {
                            player_move(level, 1, -1);
                            break;
                        }
                        if (event.key.keysym.sym == SDLK_b)
                        {
                            player_move(level, -1, 1);
                            break;
                        }
                        if (event.key.keysym.sym == SDLK_n)
                        {
                            player_move(level, 1, 1);
                            break;
                        }
                        if ((event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_RETURN) && (tile_get_player(level) == TILE_STAIRS_UP || tile_get_player(level) == TILE_STAIRS_DOWN))
                        {
                            if (tile_get_player(level) == TILE_STAIRS_UP)
                            {
                                if (game.current_level - 1 >= 0)
                                {
                                    game.current_level--;
                                }
                                else
                                {
                                    game.running = false;
                                }

                                level = &game.levels[game.current_level];
                                player_set_pos_tile(level_get_tile_index(level, TILE_STAIRS_DOWN));
                                fov_make(level);
                            }
                            else if (tile_get_player(level) == TILE_STAIRS_DOWN)
                            {
                                if (game.current_level + 1 < MAX_LEVELS)
                                {
                                    game.current_level++;
                                }
                                else
                                {
                                    game.running = false;
                                }

                                level = &game.levels[game.current_level];
                                player_set_pos_tile(level_get_tile_index(level, TILE_STAIRS_UP));
                                fov_make(level);
                            }
                        }
                        /* if (event.key.keysym.sym == SDLK_PERIOD && event.key.keysym.mod & KMOD_SHIFT && tile_get_player(level) == TILE_STAIRS_DOWN) */
                        /* { */
                        /*     if (game.current_level + 1 < MAX_LEVELS) */
                        /*     { */
                        /*         game.current_level++; */
                        /*     } */
                        /*     else */
                        /*     { */
                        /*         game.running = false; */
                        /*     } */
                        /*     level = &game.levels[game.current_level]; */
                        /*     player_set_pos_tile(level_get_tile_index(level, TILE_STAIRS_UP)); */
                        /*     fov_make(level); */
                        /* } */
                        /* if (event.key.keysym.sym == SDLK_COMMA && event.key.keysym.mod & KMOD_SHIFT && tile_get_player(level) == TILE_STAIRS_UP) */
                        /* { */
                        /*     if (game.current_level - 1 >= 0) */
                        /*     { */
                        /*         game.current_level--; */
                        /*     } */
                        /*     else */
                        /*     { */
                        /*         game.running = false; */
                        /*     } */
                        /*     level = &game.levels[game.current_level]; */
                        /*     player_set_pos_tile(level_get_tile_index(level, TILE_STAIRS_DOWN)); */
                        /* } */

                        game.turn++;
                    }
                default: break;
            }
        }

        SDL_RenderClear(game.renderer);

        level_update(level, dt);
        camera_update(level, dt);

        level_draw(level);
        player_draw(level);

        SDL_RenderPresent(game.renderer);

        game.turn_taken = false;

        dt = (SDL_GetTicks() - frame_start) / 1000.0f;
    }

    
    SDL_DestroyTexture(spritesheet);
    SDL_DestroyTexture(spritesheet_transparent);
    sdl_quit(&game);
    return 0;
}
// endmain

int rand_in_range(int min, int max)
{
    int n = rand() % (max - min + 1) + min;
    return n;
}

int rand_one_of(int a, int b)
{
    int n = rand_in_range(0, 1);
    
    if (n == 0)
    {
        return a;
    }
    else
    {
        return b;
    }
}

bool rand_true_or_false()
{
    int n = rand_in_range(0, 1);
    
    if (n == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void insertion_sort_int(int arr[], int size)
{
    for (int i = 1; i < size; ++i)
    {
        int key = arr[i];
        int j = i - 1;

        while (j >= 0 && arr[j] > key)
        {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

void insertion_sort_room(Room arr[], int size, bool x, bool greater)
{
    for (int i = 1; i < size; ++i)
    {
        Room key = arr[i];
        int j = i - 1;

        if (x && greater)
        {
            while (j >= 0 && arr[j].x > key.x)
            {
                arr[j + 1] = arr[j];
                j--;
            }
            arr[j + 1] = key;
        }
        else if (x && !greater)
        {
            while (j >= 0 && arr[j].x < key.x)
            {
                arr[j + 1] = arr[j];
                j--;
            }
            arr[j + 1] = key;
        }
        else if (!x && greater)
        {
            while (j >= 0 && arr[j].y > key.y)
            {
                arr[j + 1] = arr[j];
                j--;
            }
            arr[j + 1] = key;
        }
        else if (!x && !greater)
        {
            while (j >= 0 && arr[j].y < key.y)
            {
                arr[j + 1] = arr[j];
                j--;
            }
            arr[j + 1] = key;
        }
    }
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

bool sdl_init(Game* game)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "Couldn't initialize SDL! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    game->window = SDL_CreateWindow("asdf", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,  WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!game->window)
    {
        fprintf(stderr, "Couldn't initialize SDL_Window! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    game->renderer = SDL_CreateRenderer(game->window, -1, 0);
    if (!game->renderer)
    {
        fprintf(stderr, "Couldn't initialize SDL_Renderer! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(game->window);
        SDL_Quit();
        return false;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0)
    {
        fprintf(stderr, "Couldn't initialize IMG! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyRenderer(game->renderer);
        SDL_DestroyWindow(game->window);
        SDL_Quit();
        return false;
    }

    return true;
}

void sdl_quit(Game* game)
{
    IMG_Quit();
    SDL_DestroyRenderer(game->renderer);
    SDL_DestroyWindow(game->window);
    SDL_Quit();
}

int tile_get_index(int x, int y)
{
    return (y * TILES_X) + x;
}

int tile_get_player(Level* level)
{
    int tile_index = tile_get_index(player.x, player.y);
    return level->tiles[tile_index].tileset_index;
}

bool tile_is_in_bounds(int x, int y)
{
    return 0 <= x && x < TILES_X && 0 <= y && y < TILES_Y;
}

void tileset_create(SDL_Rect* tileset)
{
    for (int i = 0; i < TILESHEET_WIDTH * TILESHEET_HEIGHT; ++i)
    {
        int col = i % TILESHEET_WIDTH;
        int row = i / TILESHEET_WIDTH;

        tileset[i].x = col * (TILESIZE + TILESHEET_GAP);
        tileset[i].y = row * (TILESIZE + TILESHEET_GAP);
        tileset[i].w = TILESIZE;
        tileset[i].h = TILESIZE;
    }
}

void camera_update(Level* level, float dt)
{
    float tilesize = TILESIZE * SCALE;

    float target_x = player.x * tilesize + tilesize / 2.0f - VIEW_WIDTH / 2.0f;
    float target_y = player.y * tilesize + tilesize / 2.0f - VIEW_HEIGHT / 2.0f;

    float map_width = TILES_X * tilesize;
    float map_height = TILES_Y * tilesize;

    float max_x = map_width - SCREEN_WIDTH;
    float max_y = map_height - SCREEN_HEIGHT;

    if (target_x < 0)
    {
        target_x = 0;
    }
    if (target_x > max_x)
    {
        target_x = max_x;
    }
    if (target_y < 0)
    {
        target_y = 0;
    }
    if (target_y > max_y)
    {
        target_y = max_y;
    }

    camera.x = lerp(camera.x, target_x, MOVE_SPEED * dt);
    camera.y = lerp(camera.y, target_y, MOVE_SPEED * dt);

    if (camera.x < 0)
    {
        camera.x = 0;
    }
    if (camera.x > max_x)
    {
        camera.x = max_x;
    }
    if (camera.y < 0)
    {
        camera.y = 0;
    }
    if (camera.y > max_y)
    {
        camera.y = max_y;
    }
}

void level_create(void)
{
    for (int i = 0; i < MAX_LEVELS; ++i)
    {
        Level* level = &game.levels[i];

        for (int x = 0; x < TILES_X; ++x)
        {
            for (int y = 0; y < TILES_Y; ++y)
            {
                int tile_index = tile_get_index(x, y);

                level->tiles[tile_index].x = x;
                level->tiles[tile_index].y = y;
                level->tiles[tile_index].tileset_index = TILE_WALL;
                level->tiles[tile_index].blocked = true;
                level->tiles[tile_index].transparent = false;
                //showalltiles displayalltiles
                level->tiles[tile_index].visible = false;
                level->tiles[tile_index].seen = false;
                level->tiles[tile_index].has_enemy = false;

                level->tiles[tile_index].src = tileset[TILE_WALL];
            }
        }

        level_set_rooms(level);
        corridors_create(level);

        int stairs_up_room = rand_in_range(0, level->num_rooms - 1);
        int rand_x = rand_in_range(level->rooms[stairs_up_room].x, level->rooms[stairs_up_room].x + level->rooms[stairs_up_room].w - 1);
        int rand_y = rand_in_range(level->rooms[stairs_up_room].y, level->rooms[stairs_up_room].y + level->rooms[stairs_up_room].h - 1);
        level_set_tile(level, rand_x, rand_y, TILE_STAIRS_UP);

        level_set_stairs_down(level);

        level_enemies_create(level);
    }
}

void level_set_tile(Level* level, int x, int y, int tileset_index)
{
    if (x < 0 || x >= TILES_X || y < 0 || y >= TILES_Y) return;

    Tile* t = &level->tiles[tile_get_index(x, y)];
    t->tileset_index = tileset_index;
    t->src = tileset[tileset_index];
}

void level_update(Level* level, float dt)
{

    if (player.is_moving)
    {
        player.move_progress += MOVE_SPEED * dt;

        if (player.attacking_phase == 1)
        {
            if (player.move_progress >= 1.0f)
            {
                player.attacking_phase = 2;
                float temp_x = player.start_x;
                float temp_y = player.start_y;
                player.start_x = player.target_x;
                player.start_y = player.target_y;
                player.target_x = temp_x;
                player.target_y = temp_y;
                player.move_progress = 0.0f;
            }
        }
        else if (player.attacking_phase == 2)
        {
            if (player.move_progress >= 1.0f)
            {
                player.x = (int)player.target_x;
                player.y = (int)player.target_y;
                player.is_moving = false;
                player.move_progress = 0.0f;
                player.attacking_phase = 0;

                game.turn_taken = true;
            }
        }
        else if (player.move_progress >= 1.0f)
        {
            player.move_progress = 0.0f;
            player.is_moving = false;

            game.turn_taken = true;
        }
    }

    for (int i = 0; i < level->num_enemies; ++i)
    {
        Enemy* enemy = &level->enemies[i];

        if (game.turn_taken)
        {
            enemy_update(level, enemy);
        }

        if (enemy->is_moving)
        {
            enemy->move_progress += MOVE_SPEED * dt;

            if (enemy->move_progress >= 1.0f)
            {
                enemy->move_progress = 0.0f;
                enemy->is_moving = false;
            }
        }

        enemy_draw(level, enemy);

        if (enemy->is_hit)
        {
            enemy->hit_timer += dt;
            if (enemy->hit_timer >= enemy->hit_duration)
            {
                enemy->is_hit = false;
                enemy->hit_timer = 0.0f;
            }
        }
    }
}

void level_draw(Level* level)
{
    float tilesize = TILESIZE * SCALE;

    int start_x = (int)(camera.x / tilesize);
    int start_y = (int)(camera.y / tilesize);

    int end_x = (int)((camera.x + VIEW_WIDTH) / tilesize) + 1;
    int end_y = (int)((camera.y + VIEW_HEIGHT) / tilesize) + 1;

    if (start_x < 0)
    {
        start_x = 0;
    }
    if (end_x >= TILES_X)
    {
        end_x = TILES_X - 1;
    }
    if (start_y < 0)
    {
        start_y = 0;
    }
    if (end_y >= TILES_Y)
    {
        end_y = TILES_Y - 1;
    }

    for (int x = start_x; x <= end_x; ++x)
    {
        for (int y = start_y; y <= end_y; ++y)
        {
            Tile t = level->tiles[tile_get_index(x, y)];
            /* bool player_here = (t.x == player.x && t.y == player.y); */

            if (t.visible)
            {
                SDL_SetTextureColorMod(spritesheet, 255, 255, 255);
                /* SDL_SetTextureAlphaMod(spritesheet, (player_here && !(t.tileset_index == TILE_FLOOR)) ? 250 : 255); */
                SDL_SetTextureAlphaMod(spritesheet, 255);

                SDL_Rect dstRect = (SDL_Rect){t.x * TILESIZE * SCALE - camera.x, t.y * TILESIZE * SCALE - camera.y, TILESIZE * SCALE, TILESIZE * SCALE};
                SDL_RenderCopy(game.renderer, spritesheet, &t.src, &dstRect);
            }
            else if (t.seen)
            {
                SDL_SetTextureColorMod(spritesheet, 100, 100, 100);
                SDL_SetTextureAlphaMod(spritesheet, 160);

                SDL_Rect dstRect = (SDL_Rect){t.x * TILESIZE * SCALE - camera.x, t.y * TILESIZE * SCALE - camera.y, TILESIZE * SCALE, TILESIZE * SCALE};
                SDL_RenderCopy(game.renderer, spritesheet, &t.src, &dstRect);
            }
        }
    }

    for (int i = 0; i < level->num_enemies; ++i)
    {
        enemy_draw(level, &level->enemies[i]);
    }
}

void level_set_rooms(Level* level)
{
    level->num_rooms = rand_in_range(LEVEL_MIN_ROOMS, LEVEL_MAX_ROOMS);
    int rooms_placed = 0;
    int max_attempts = 10000;

    while (rooms_placed < level->num_rooms && max_attempts > 0)
    {
        room_create(&level->rooms[rooms_placed]);
        bool collides = false;

        for (int j = 0; j < rooms_placed; ++j)
        {
            if (room_collides(&level->rooms[rooms_placed], &level->rooms[j]))
            {
                collides = true;
                break;
            }
        }

        if (!collides)
        {
            rooms_placed++;
        }
        else
        {
            max_attempts--;
        }
    }

    level->num_rooms = rooms_placed;

    for (int i = 0; i < level->num_rooms; ++i)
    {
        Room room = level->rooms[i];
        for (int x = room.x; x < room.x + room.w; ++x)
        {
            for (int y = room.y; y < room.y + room.h; ++y)
            {
                level_set_tile(level, x, y, TILE_FLOOR);
                level->tiles[tile_get_index(x, y)].blocked = false;
                level->tiles[tile_get_index(x, y)].transparent = true;
            }
        }

        // dirt
        int num_variant_tiles_dirt = rand_in_range(0, 3);
        int dirt_variant_tiles[] = {TILE_DIRT_VARIANT_1, TILE_DIRT_VARIANT_2, TILE_DIRT_VARIANT_3, TILE_DIRT_VARIANT_4};
        int num_variants_dirt = sizeof(dirt_variant_tiles) / sizeof(dirt_variant_tiles[0]);
        for (int n = 0; n < num_variant_tiles_dirt; ++n)
        {
            int dirt_variant_index = rand_in_range(0, num_variants_dirt - 1);
            int dirt_variant_tile = dirt_variant_tiles[dirt_variant_index];
            int rand_x = rand_in_range(room.x, room.x + room.w - 1);
            int rand_y = rand_in_range(room.y, room.y + room.h - 1);
            if (rand_x >= 0 && rand_x < TILES_X && rand_y >= 0 && rand_y < TILES_Y)
                level_set_tile(level, rand_x, rand_y, dirt_variant_tile);
        }

        // grass
        int num_variant_tiles_grass = rand_in_range(0, 2);
        for (int n = 0; n < num_variant_tiles_grass; ++n)
        {
            int rand_x = rand_in_range(room.x, room.x + room.w - 1);
            int rand_y = rand_in_range(room.y, room.y + room.h - 1);
            if (rand_x >= 0 && rand_x < TILES_X && rand_y >= 0 && rand_y < TILES_Y)
            {
                level_set_tile(level, rand_x, rand_y, TILE_GRASS);
                level->tiles[tile_get_index(rand_x, rand_y)].blocked = false;
                level->tiles[tile_get_index(rand_x, rand_y)].transparent = true;
            }
        }
    }
}

void level_set_stairs_down(Level* level)
{
    int stairs_up_x;
    int stairs_up_y;
    for (int x = 0; x < TILES_X; ++x)
    {
        for (int y = 0; y < TILES_Y; ++y)
        {
            int tile = tile_get_index(x, y);
            if (level->tiles[tile].tileset_index == TILE_STAIRS_UP)
            {
                stairs_up_x = x;
                stairs_up_y = y;
            }
        }
    }
    int stairs_down_room;
    bool same_room = true;
    while (same_room)
    {
        stairs_down_room = rand_in_range(0, level->num_rooms - 1);
        same_room = room_contains(&level->rooms[stairs_down_room], stairs_up_x, stairs_up_y);
    }

    int rand_x = rand_in_range(level->rooms[stairs_down_room].x, level->rooms[stairs_down_room].x + level->rooms[stairs_down_room].w - 1);
    int rand_y = rand_in_range(level->rooms[stairs_down_room].y, level->rooms[stairs_down_room].y + level->rooms[stairs_down_room].h - 1);

    level_set_tile(level, rand_x, rand_y, TILE_STAIRS_DOWN);
}

int level_get_tile_index(Level* level, int tile)
{
    for (int x = 0; x < TILES_X; ++x)
    {
        for (int y = 0; y < TILES_Y; ++y)
        {
            int tile_index = tile_get_index(x, y);
            if (level->tiles[tile_index].tileset_index == tile)
            {
                return tile_index;
            }
        }
    }

    return -1;
}

void level_enemies_create(Level* level)
{
    int target = rand_in_range(LEVEL_MIN_ENEMIES, LEVEL_MAX_ENEMIES);
    int enemies_created = 0;
    int max_attempts = 500;

    while (level->num_enemies < target && max_attempts > 0)
    {
        bool new_enemy_created = enemy_create(level);
        
        if (new_enemy_created)
        {
            enemies_created++;
        }
        else
        {
            max_attempts--;
        }
    }
}

void room_create(Room* room)
{
    int margin = 1;

    room->w = rand_in_range(ROOM_MIN_WIDTH, ROOM_MAX_WIDTH);
    room->h = rand_in_range(ROOM_MIN_HEIGHT, ROOM_MAX_HEIGHT);
    room->x = rand_in_range(margin, TILES_X - room->w - margin);
    room->y = rand_in_range(margin, TILES_Y - room->h - margin);
    room->max_enemies = rand_in_range(1, 2);
    room->num_enemies = 0;
}

bool room_collides(Room* r1, Room* r2)
{
    int margin = 1;

    return (r1->x - margin < r2->x + r2->w + margin
            && r1->x + r1->w + margin > r2->x - margin
            && r1->y - margin < r2->y + r2->h + margin
            && r1->y + r1->h + margin > r2->y - margin
            );
}

bool room_contains(Room* r, int x, int y)
{
    return (x >= r->x && x <= r->x + r->w - 1 && y >= r->y && y <= r->y + r->h - 1);
}

void corridors_create(Level* level)
{
    Room rooms[level->num_rooms];
    for (int i = 0; i < level->num_rooms; ++i)
    {
        rooms[i] = level->rooms[i];
    }

    int sorting_dir = rand_in_range(0, 3);
    switch (sorting_dir)
    {
        case 0:
            insertion_sort_room(rooms, level->num_rooms, true, true);
            break;
        case 1:
            insertion_sort_room(rooms, level->num_rooms, true, false);
            break;
        case 2:
            insertion_sort_room(rooms, level->num_rooms, false, true);
            break;
        case 3:
            insertion_sort_room(rooms, level->num_rooms, false, false);
            break;
    }

    int start_horizontally = rand_in_range(0, 1);
    if (start_horizontally)
    {
        for (int i = 0; i < level->num_rooms - 1; ++i)
        {
            int x1 = rooms[i].x + rooms[i].w / 2;
            int y1 = rooms[i].y + rooms[i].h / 2;
            int x2 = rooms[i + 1].x + rooms[i + 1].w / 2;
            corridors_create_horizontal(level, x1, x2, y1);
        }
        for (int i = 0; i < level->num_rooms - 1; ++i)
        {
            int y1 = rooms[i].y + rooms[i].h / 2;
            int x2 = rooms[i + 1].x + rooms[i + 1].w / 2;
            int y2 = rooms[i + 1].y + rooms[i + 1].h / 2;
            corridors_create_vertical(level, y1, y2, x2);
        }
    }
    else
    {
        for (int i = 0; i < level->num_rooms - 1; ++i)
        {
            int x1 = rooms[i].x + rooms[i].w / 2;
            int y1 = rooms[i].y + rooms[i].h / 2;
            int y2 = rooms[i + 1].y + rooms[i + 1].h / 2;
            corridors_create_vertical(level, y1, y2, x1);
        }
        for (int i = 0; i < level->num_rooms - 1; ++i)
        {
            int x1 = rooms[i].x + rooms[i].w / 2;
            int x2 = rooms[i + 1].x + rooms[i + 1].w / 2;
            int y2 = rooms[i + 1].y + rooms[i + 1].h / 2;
            corridors_create_horizontal(level, x1, x2, y2);
        }
    }
}

void corridors_create_horizontal(Level* level, int x1, int x2, int y)
{
    if (x1 < x2)
    {
        for (int x = x1; x < x2; ++x)
        {
            level_set_tile(level, x, y, TILE_FLOOR);
            level->tiles[tile_get_index(x, y)].blocked = false;
            level->tiles[tile_get_index(x, y)].transparent = true;
        }
    }
    else
    {
        for (int x = x1; x > x2; --x)
        {
            level_set_tile(level, x, y, TILE_FLOOR);
            level->tiles[tile_get_index(x, y)].blocked = false;
            level->tiles[tile_get_index(x, y)].transparent = true;
        }
    }
}

void corridors_create_vertical(Level* level, int y1, int y2, int x)
{
    if (y1 < y2)
    {
        for (int y = y1; y < y2; ++y)
        {
            level_set_tile(level, x, y, TILE_FLOOR);
            level->tiles[tile_get_index(x, y)].blocked = false;
            level->tiles[tile_get_index(x, y)].transparent = true;
        }
    }
    else
    {
        for (int y = y1; y > y2; --y)
        {
            level_set_tile(level, x, y, TILE_FLOOR);
            level->tiles[tile_get_index(x, y)].blocked = false;
            level->tiles[tile_get_index(x, y)].transparent = true;
        }
    }
}

void player_init(Player* player)
{
    player->src = tileset[TILE_PLAYER_YELLOW];
    player->damage = 10;
    player->is_moving = false;
    player->move_progress = 0.0f;
}

void player_draw(Level* level)
{
    float draw_x;
    float draw_y;

    if (player.is_moving)
    {
        float t = player.move_progress;
        draw_x = lerp(player.start_x, player.target_x, t);
        draw_y = lerp(player.start_y, player.target_y, t);
    }
    else
    {
        draw_x = player.x;
        draw_y = player.y;
    }

    SDL_SetTextureColorMod(spritesheet_transparent, 255, 255, 255);
    SDL_SetTextureAlphaMod(spritesheet_transparent, 255);

    SDL_Rect dst = (SDL_Rect){draw_x * TILESIZE * SCALE - camera.x, draw_y * TILESIZE * SCALE - camera.y, TILESIZE * SCALE, TILESIZE * SCALE};
    SDL_RenderCopy(game.renderer, spritesheet_transparent, &player.src, &dst);
}

void player_move(Level* level, int dx, int dy)
{
    int new_x = player.x + dx;
    int new_y = player.y + dy;

    if (tile_is_in_bounds(new_x, new_y))
    {
        int new_pos = tile_get_index(new_x, new_y);

        if (level->tiles[new_pos].has_enemy && !player.is_moving)
        {
            int enemy_index = -1;
            for (int i = 0; i < level->num_enemies; ++i)
            {
                if (level->enemies[i].x == new_x && level->enemies[i].y == new_y)
                {
                    enemy_index = i;
                    break;
                }
            }

            if (enemy_index != -1)
            {
                enemy_take_damage(level, enemy_index);
            }

            float attack_distance = 0.3f;

            float dir_x = new_x - player.x;
            float dir_y = new_y - player.y;

            float attack_x = player.x + (dir_x * attack_distance);
            float attack_y = player.y + (dir_y * attack_distance);

            player.start_x = player.x;
            player.start_y = player.y;
            player.target_x = attack_x;
            player.target_y = attack_y;
            player.is_moving = true;
            player.move_progress = 0.0f;
            player.attacking_phase = 1;
        }
        else if (!level->tiles[new_pos].blocked && !player.is_moving)
        {
            fov_clear(level);

            player.start_x = player.x;
            player.start_y = player.y;
            player.target_x = new_x;
            player.target_y = new_y;

            player.x = new_x;
            player.y = new_y;

            player.is_moving = true;
            player.move_progress = 0.0f;
            player.attacking_phase = 0;

            fov_make(level);
        }
    }
}

void player_set_pos_coord(int x, int y)
{
    player.x = x;
    player.y = y;
}

void player_set_pos_tile(int tile)
{
    int x = tile % TILES_X;
    int y = tile / TILES_X;
    player.x = x;
    player.y = y;
}

int get_sign(int a)
{
    if (a < 0)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

bool line_of_sight(Level* level, int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;

    int dx_abs = abs(dx);
    int dy_abs = abs(dy);

    int x_sign = get_sign(dx);
    int y_sign = get_sign(dy);

    int x = x1;
    int y = y1;

    int steps_needed = dx_abs + dy_abs;

    if (dx_abs > dy_abs)
    {
        int t = dy_abs * 2 - dx_abs;

        for (int i = 0; i < steps_needed; ++i)
        {
            if (level->tiles[tile_get_index(x, y)].transparent && tile_is_in_bounds(x, y))
            {
                if (t >= 0)
                {
                    y += y_sign;
                    t -= dx_abs * 2;
                }

                x += x_sign;
                t += dy_abs * 2;

                if (x == x2 && y == y2)
                {
                    return true;
                }
            }
        }

        return false;
    }
    else
    {
        int t = dx_abs * 2 - dy_abs;

        for (int i = 0; i < steps_needed; ++i)
        {
            if (level->tiles[tile_get_index(x, y)].transparent && tile_is_in_bounds(x, y))
            {
                if (t >= 0)
                {
                    x += x_sign;
                    t -= dy_abs * 2;
                }

                y += y_sign;
                t += dx_abs * 2;

                if (x == x2 && y == y2)
                {
                    return true;
                }
            }
        }

        return false;
    }
}

void fov_make(Level* level)
{
    level->tiles[tile_get_index(player.x, player.y)].visible = true;
    level->tiles[tile_get_index(player.x, player.y)].seen = true;

    for (int y = player.y - FOV_RADIUS; y <= player.y + FOV_RADIUS; ++y)
    {
        for (int x = player.x - FOV_RADIUS; x <= player.x + FOV_RADIUS; ++x)
        {
            int tile_index = tile_get_index(x, y);
            int distance = abs(x - player.x) + abs(y - player.y);

            if (tile_is_in_bounds(x, y) && distance <= FOV_RADIUS && line_of_sight(level, player.x, player.y, x, y))
            {
                level->tiles[tile_index].visible = true;
                level->tiles[tile_index].seen = true;
            }
        }
    }
}

void fov_clear(Level* level)
{
    for (int y = player.y - FOV_RADIUS; y <= player.y + FOV_RADIUS; ++y)
    {
        for (int x = player.x - FOV_RADIUS; x <= player.x + FOV_RADIUS; ++x)
        {
            int tile_index = tile_get_index(x, y);
            int distance = abs(x - player.x) + abs(y - player.y);

            if (tile_is_in_bounds(x, y) && distance <= FOV_RADIUS)
            {
                level->tiles[tile_index].visible = false;
            }
        }
    }
}

bool enemy_create(Level* level)
{
    int rand_room = -1;
    int attempts = 0;
    int max_attempts = level->num_rooms * 4;
    while (attempts < max_attempts)
    {
        int candidate = rand_in_range(0, level->num_rooms - 1);
        if (level->rooms[candidate].num_enemies < level->rooms[candidate].max_enemies)
        {
            rand_room = candidate;
            break;
        }
        attempts++;
    }

    if (rand_room == -1) return false;
    
    Room* r = &level->rooms[rand_room];
    Enemy e;
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        e.type = rand_one_of(SPIDER, RAT);

        if ((r->num_enemies + e.type) > r->max_enemies)
        {
            continue;
        }

        int rand_x = rand_in_range(r->x, r->x + r->w - 1);
        int rand_y = rand_in_range(r->y, r->y + r->h - 1);
        int tile = tile_get_index(rand_x, rand_y);

        if (level->tiles[tile].has_enemy)
        {
            continue;
        }

        e.x = rand_x;
        e.y = rand_y;
        e.is_moving = false;
        e.state = IDLE;

        switch (e.type)
        {
            case SPIDER:
                e.src = tileset_transparent[TILE_SPIDER];
                e.health = rand_in_range(8, 16);
                break;
            case RAT:
                e.src = tileset_transparent[TILE_RAT];
                e.health = rand_in_range(11, 24);
                break;
            default: break;
        }

        r->num_enemies += e.type;

        level->enemies[level->num_enemies] = e;
        level->num_enemies++;
        level->tiles[tile_get_index(e.x, e.y)].has_enemy = true;

        return true;
    }

    return false;
}

void enemy_draw(Level* level, Enemy* enemy)
{
    int enemy_tile = tile_get_index(enemy->x, enemy->y);
    if (level->tiles[enemy_tile].visible)
    {
        float draw_x;
        float draw_y;

        if (enemy->is_moving)
        {
            float t = enemy->move_progress;
            draw_x = lerp(enemy->start_x, enemy->target_x, t);
            draw_y = lerp(enemy->start_y, enemy->target_y, t);
        }
        else
        {
            draw_x = enemy->x;
            draw_y = enemy->y;
        }

        if (enemy->is_hit)
        {
            int flash_speed = 8;
            bool is_not_flashing = (int)(enemy->hit_timer * flash_speed) % 2 == 0;

            if (is_not_flashing)
            {
                SDL_SetTextureColorMod(spritesheet_transparent, 255, 255, 255);
            }
            else
            {
                SDL_SetTextureColorMod(spritesheet_transparent, 255, 0, 0);
            }
            SDL_SetTextureAlphaMod(spritesheet_transparent, 255);
        }
        else
        {
            SDL_SetTextureColorMod(spritesheet_transparent, 255, 255, 255);
            SDL_SetTextureAlphaMod(spritesheet_transparent, 255);
        }

        SDL_Rect dst = (SDL_Rect){draw_x * TILESIZE * SCALE - camera.x, draw_y * TILESIZE * SCALE - camera.y, TILESIZE * SCALE, TILESIZE * SCALE};
        SDL_RenderCopy(game.renderer, spritesheet_transparent, &enemy->src, &dst);
    }
}

void enemy_take_damage(Level* level, int enemy_index)
{
    Enemy* enemy = &level->enemies[enemy_index];

    enemy->is_hit = true;
    enemy->hit_timer = 0.0f;
    enemy->hit_duration = 0.3f;

    enemy->health -= player.damage;
    if (enemy->health <= 0)
    {
        level->tiles[tile_get_index(enemy->x, enemy->y)].has_enemy = false;
        for (int i = enemy_index; i < level->num_enemies - 1; ++i)
        {
            level->enemies[i] = level->enemies[i + 1];
        }
        level->num_enemies--;
    }
}

void enemy_update(Level* level, Enemy* enemy)
{
    switch (enemy->state)
    {
        case IDLE:
            bool move_or_not = rand_true_or_false();

            if (move_or_not)
            {
                int max_retries = 8;

                for (int attempt = 0; attempt < max_retries; attempt++)
                {
                    int direction = rand_in_range(0, 3);
                    int dx = 0;
                    int dy = 0;

                    switch (direction)
                    {
                        case 0: dx = 1; break;
                        case 1: dx = -1; break;
                        case 2: dy = 1; break;
                        case 3: dy = -1; break;
                    }

                    /* int dx = rand_one_of(-1, 1); */
                    /* int dy = rand_one_of(-1, 1); */

                    /* if (dx == 0 && dy == 0) */
                    /* { */
                    /*     continue; */
                    /* } */

                    int new_x = enemy->x + dx;
                    int new_y = enemy->y + dy;

                    if (!tile_is_in_bounds(new_x, new_y))
                    {
                        continue;
                    }

                    int new_pos = tile_get_index(new_x, new_y);

                    if (level->tiles[new_pos].has_enemy ||
                            tile_get_index(player.x, player.y) == new_pos ||
                            level->tiles[new_pos].blocked)
                    {
                        continue;
                    }

                    int curr_pos = tile_get_index(enemy->x, enemy->y);

                    level->tiles[curr_pos].has_enemy = false;
                    level->tiles[new_pos].has_enemy = true;

                    enemy->start_x = enemy->x;
                    enemy->start_y = enemy->y;
                    enemy->target_x = new_x;
                    enemy->target_y = new_y;

                    enemy->x = new_x;
                    enemy->y = new_y;

                    enemy->is_moving = true;
                    enemy->move_progress = 0.0f;
                    enemy->attacking_phase = 0;

                    break;
                }
            }
            break;
        case CHASING:
            break;
        case ATTACKING:
            break;
        default:
            break;
    }
}
