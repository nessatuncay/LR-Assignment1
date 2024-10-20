#include <raylib.h>
#include "Math.h"
#include "raudio.c"

#include <cassert>
#include <array>
#include <vector>
#include <algorithm>

const float SCREEN_SIZE = 800;

const int TILE_COUNT = 20;
const float TILE_SIZE = SCREEN_SIZE / TILE_COUNT;

//Texture2D bullettex = LoadTexture("Bullet.png");

//int frameWidth = bullettex.width;
//int frameHeight = bullettex.height;

//Rectangle sourceRec = { 0.0f, 0.0f, frameWidth, frameHeight };

//Vector2 origin = { frameWidth, frameHeight };

int rotation = 0;

enum TileType : int
{
    GRASS,      // Marks unoccupied space, can be overwritten 
    DIRT,       // Marks the path, cannot be overwritten
    WAYPOINT,   // Marks where the path turns, cannot be overwritten
    TURRET,
    COUNT
};

struct Cell
{
    int row;
    int col;
};

constexpr std::array<Cell, 4> DIRECTIONS{ Cell{ -1, 0 }, Cell{ 1, 0 }, Cell{ 0, -1 }, Cell{ 0, 1 } };

inline bool InBounds(Cell cell, int rows = TILE_COUNT, int cols = TILE_COUNT)
{
    return cell.col >= 0 && cell.col < cols && cell.row >= 0 && cell.row < rows;
}

void DrawTile(int row, int col, Color color)
{
    DrawRectangle(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, color);
}

void DrawTile(int row, int col, int type)
{
    Color color = type > 0 ? BEIGE : GREEN;
    DrawTile(row, col, color);
}

Vector2 TileCenter(int row, int col)
{
    float x = col * TILE_SIZE + TILE_SIZE * 0.5f;
    float y = row * TILE_SIZE + TILE_SIZE * 0.5f;
    return { x, y };
}

Vector2 TileCorner(int row, int col)
{
    float x = col * TILE_SIZE;
    float y = row * TILE_SIZE;
    return { x, y };
}

// Returns a collection of adjacent cells that match the search value.
std::vector<Cell> FloodFill(Cell start, int tiles[TILE_COUNT][TILE_COUNT], TileType searchValue)
{
    // "open" = "places we want to search", "closed" = "places we've already searched".
    std::vector<Cell> result;
    std::vector<Cell> open;
    bool closed[TILE_COUNT][TILE_COUNT];
    for (int row = 0; row < TILE_COUNT; row++)
    {
        for (int col = 0; col < TILE_COUNT; col++)
        {
            // We don't want to search zero-tiles, so add them to closed!
            closed[row][col] = tiles[row][col] == 0;
        }
    }

    // Add the starting cell to the exploration queue & search till there's nothing left!
    open.push_back(start);
    while (!open.empty())
    {
        // Remove from queue and prevent revisiting
        Cell cell = open.back();
        open.pop_back();
        closed[cell.row][cell.col] = true;

        // Add to result if explored cell has the desired value
        if (tiles[cell.row][cell.col] == searchValue)
            result.push_back(cell);

        // Search neighbours
        for (Cell dir : DIRECTIONS)
        {
            Cell adj = { cell.row + dir.row, cell.col + dir.col };
            if (InBounds(adj) && !closed[adj.row][adj.col] && tiles[adj.row][adj.col] > 0)
                open.push_back(adj);
        }
    }

    return result;
}

struct Bullet
{
    Vector2 position{};
    Vector2 direction{};
    float time = 0.0f;
    bool enabled = true;
};

struct Grenade
{
    Vector2 position{};
    Vector2 direction{};
    float time = 0.0f;
    bool enabled = true;
};

struct Missile
{
    Vector2 position{};
    Vector2 direction{};
    float time = 0.0f;
    bool enabled = true;
};

// I needed to make 3 unique position variables to get this to make enemies oh my god
struct Enemy
{
    Vector2 enemyInitPos{};
    Vector2 enemyPos{};
    Vector2 enemyDirection{};
    bool enemyEnabled = true;

};

struct Zombie
{
    Vector2 zombieInitPos{};
    Vector2 zombiePos{};
    Vector2 zombieDirection{};
    bool zombieEnabled = true;
};

struct Vampire
{
    Vector2 vampireInitPos{};
    Vector2 vampirePos{};
    Vector2 vampireDirection{};
    bool vampireEnabled = true;
};


struct Turret
{
    Vector2 turretPos{};
    bool turretEnabled = true;
    TileType type;
};


int main()
{
    int tiles[TILE_COUNT][TILE_COUNT]
    {
        //col:0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19    row:
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 }, // 0
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 1
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 2
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 3
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 4
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 5
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 6
            { 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0 }, // 7
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 9
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 10
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 11
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 12
            { 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0 }, // 13
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 14
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 15
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 16
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0 }, // 17
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 18
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }  // 19
    };

    std::vector<Cell> waypoints = FloodFill({ 0, 12 }, tiles, WAYPOINT);
    size_t curr = 0;
    size_t next = curr + 1;
    size_t spawn = 0;

    //turret info
    Turret turret;
    turret.type = TURRET;
    std::vector<Turret> turrets;
    const float turretRadius = 20.0f;
    float turretCount = 0.0f;
    Vector2 turretPosition{}; 


    //enemy info
    std::vector<Enemy> enemies;
    const float enemySpeed = 250.0f;
    const float enemyRadius = 20.0f;
    float enemyCount = 0.0f;
    Vector2 enemyPosition{};
    float enemyTime = 0.0f;
    float enemySpawn = 1.0f;
    bool atEnd = false;
    float enemyHP = 0.0f;
    ;

    //zombie info
    std::vector<Zombie> zombies;
    const float zombieSpeed = 30.0f;
    const float zombieRadius = 40.0f;
    float zombieCount = 0.0f;
    Vector2 zombiePosition{};
    float zombieTime = 0.0f;
    float zombieSpawn = 1.0f;
    bool atEndZombie = false; 
    float zombieHP = 20.0f;
    ;



    //vampire info
    std::vector<Vampire> vampires;
    const float vampireSpeed = 300.0f;
    const float vampireRadius = 5.0f;
    float vampireCount = 0.0f;
    Vector2 vampirePosition{}; 
    float vampireTime = 0.0f; 
    float vampireSpawn = 5.0f; 
    bool atEndVampire = false; 
    float vampireHP = 30.0f; 
    ;


    //bullet info
    const float bulletTime = 1.0f;
    const float bulletSpeed = 500.0f;
    const float bulletRadius = 15.0f;

    std::vector<Bullet> bullets;
    float shootCurrent = 0.0f;
    float shootTotal = 0.25f;

    //grenade info
    const float grenadeTime = 1.0f; 
    const float grenadeSpeed = 300.0f; 
    const float grenadeRadius = 40.0f;

    std::vector<Grenade> grenades; 
    float throwCurrent = 0.0f; 
    float throwTotal = 0.25f;


    //missile info
    const float missileTime = 1.0f; 
    const float missileSpeed = 800.0f; 
    const float missileRadius = 35.0f; 

    std::vector<Missile> missiles;
    float launchCurrent = 0.0f; 
    float launchTotal = 0.25f; 


    //audio info
    InitAudioDevice(); 
    Sound sound1 = LoadSound("bullet.sound.mp3"); 
    Sound sound2 = LoadSound("turret.create.mp3"); 
    Sound sound3 = LoadSound("turret.delete.mp3"); 
    Sound sound4 = LoadSound("enemy.hit.mp3"); 
    Sound sound5 = LoadSound("enemy.death.mp3"); 



    InitWindow(SCREEN_SIZE, SCREEN_SIZE, "Tower Defense");
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Path following

        enemyTime += dt;
        if( enemyCount <= 9.0f && enemyTime >= enemySpawn)
        {
            enemyTime = 0;

            Enemy enemy; 
            enemyHP == 2.0f;
            enemy.enemyInitPos = TileCenter(waypoints[spawn].row, waypoints[spawn].col);
            enemyCount += 1.0f;
            enemy.enemyPos = enemy.enemyInitPos;
            enemies.push_back(enemy);

        }

        for (Enemy& enemy : enemies)
        {
            if (!atEnd)
            {
                Vector2 from = TileCenter(waypoints[curr].row, waypoints[curr].col);
                Vector2 to = TileCenter(waypoints[next].row, waypoints[next].col);
                enemy.enemyDirection = Normalize(to - from);
                enemy.enemyPos = enemy.enemyPos + enemy.enemyDirection * enemySpeed * dt;
                if (CheckCollisionPointCircle(enemy.enemyPos, to, enemyRadius))
                {
                    curr++;
                    next++;
                    atEnd = next == waypoints.size();
                    enemy.enemyPos = TileCenter(waypoints[curr].row, waypoints[curr].col);
                }
                enemyPosition = enemy.enemyPos;
            }
            if (enemyHP <= 0.0f)
            {
                !enemy.enemyEnabled;
            }

        }

        // Shooting
        shootCurrent += dt;
        if (shootCurrent >= shootTotal)
        {
            shootCurrent = 0.0f;

            Bullet bullet;
            Turret turret;
            bullet.position = turretPosition; 
            bullet.direction = Normalize(enemyPosition - bullet.position);
            bullets.push_back(bullet);
            PlaySound(sound1); 
        }

        //launching
        launchCurrent += dt;
        if (launchCurrent >= launchTotal)
        {
            launchCurrent = 0.0f;

            Missile missile; 
            Turret turret;
            missile.position = turretPosition;
            missile.direction = Normalize(zombiePosition - missile.position);
            missiles.push_back(missile); 
            PlaySound(sound1);
        }


        //throwing
        throwCurrent += dt;
        if (throwCurrent >= throwTotal)
        {
            throwCurrent = 0.0f;

            Grenade grenade;
            Turret turret;
            grenade.position = turretPosition;
            grenade.direction = Normalize(vampirePosition - grenade.position);
            grenades.push_back(grenade);
            PlaySound(sound1);
        }


        // Bullet update
        for (Bullet& bullet : bullets)
        {
            bullet.position = bullet.position + bullet.direction * bulletSpeed * dt;
            bullet.time += dt;

            bool expired = bullet.time >= bulletTime;
            bool collision = CheckCollisionCircles(enemyPosition, enemyRadius, bullet.position, bulletRadius);


            bullet.enabled = !expired && !collision;
            if ( collision )
            {
                enemyHP = enemyHP - 1.0f;
                PlaySound(sound4);
                if (enemyHP <= 0.0f)
                {
                    PlaySound(sound5);
                    enemies.end(); 
                }
            }
        }

        //Missile Update
        for (Missile& missile : missiles)
        {
            missile.position = missile.position + missile.direction * missileSpeed * dt;
            missile.time += dt;

            bool expired = missile.time >= missileTime;
            bool collision = CheckCollisionCircles(zombiePosition, zombieRadius, missile.position, missileRadius);


            missile.enabled = !expired && !collision;
            if (collision)
            {
                zombieHP = zombieHP - 1.0f; 
                PlaySound(sound4);
                if (zombieHP <= 0.0f)
                {
                    PlaySound(sound5);
                    zombies.end();
                }
            }
        }


        //Grenade Update
        for (Grenade& grenade : grenades)
        {
            grenade.position = grenade.position + grenade.direction * grenadeSpeed * dt;
            grenade.time += dt;

            bool expired = grenade.time >= grenadeTime;
            bool collision = CheckCollisionCircles(vampirePosition, vampireRadius, grenade.position, grenadeRadius);


            grenade.enabled = !expired && !collision;
            if (collision)
            {
                vampireHP = vampireHP - 1.0f;
                PlaySound(sound4);
                if (vampireHP <= 0.0f)
                {
                    PlaySound(sound5);
                    vampires.end(); 
                }
            }
        }

 
        //enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        //    [&enemies](Enemy enemy)
        //    {
        //        return !enemy.enemyEnabled;
        //    }), enemies.end());

        // Bullet removal
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [&bullets](Bullet bullet) {
                return !bullet.enabled;
            }), bullets.end());

        //missile removal
        missiles.erase(std::remove_if(missiles.begin(), missiles.end(),
            [&missiles](Missile missile) {
                return !missile.enabled;
            }), missiles.end());


        //grenade removal
        grenades.erase(std::remove_if(grenades.begin(), grenades.end(),
            [&grenades](Grenade grenade) { 
                return !grenade.enabled;
            }), grenades.end());


        // Turret creation
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (turretCount <= 5.0f)
            {
                Turret turret;
                turretCount = turretCount + 1.0f;
                Vector2 mousePosition = GetMousePosition();
                turretPosition = mousePosition;
                if (turretCount + 1.0f)
                {
                    PlaySound(sound2);
                }

            }
            else
            {
                DrawText(TextFormat("You cannot make any more turrets"), 10, 10, 20, PINK);
            }
        }

        //turret deletion
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
            Turret turret;
            turretCount = turretCount - 1.0f;
            if (turretCount - 1.0f)
            {
                PlaySound(sound3);
            }
        }




        BeginDrawing();
        ClearBackground(BLACK);
        for (int row = 0; row < TILE_COUNT; row++)
        {
            for (int col = 0; col < TILE_COUNT; col++)
            {
                DrawTile(row, col, tiles[row][col]);
            }
        }
        //enemy draw
        for (const Enemy& enemy : enemies)
            DrawCircleV(enemy.enemyPos, enemyRadius, RED);

        //zombie draw
        for (const Zombie& zombie : zombies)
            DrawCircleV(zombie.zombiePos, zombieRadius, PURPLE); 

        //vampire draw
        for (const Vampire& vampire : vampires)
            DrawCircleV(vampire.vampirePos, vampireRadius, ORANGE); 

        //turret draw
        for (const Turret& turret : turrets)
            DrawCircleV(turretPosition, turretRadius, PINK);  

        // Render bullets
        for (const Bullet& bullet : bullets)
            DrawCircleV(bullet.position, bulletRadius, BLUE); 
        DrawText(TextFormat("Total bullets: %i", bullets.size()), 10, 10, 20, BLUE);

        //render missiles
        for (const Missile& missile : missiles)
            DrawCircleV(missile.position, missileRadius, YELLOW); 
        DrawText(TextFormat("Total missiles: %i", missiles.size()), 10, 25, 20, BLUE);


        //render grenades
        for (const Grenade& grenade : grenades)
            DrawCircleV(grenade.position, grenadeRadius, MAROON); 
        DrawText(TextFormat("Total grenades: %i", grenades.size()), 10, 35, 20, BLUE);

        EndDrawing();
    }
    CloseWindow();
    CloseAudioDevice();
    return 0;
}
