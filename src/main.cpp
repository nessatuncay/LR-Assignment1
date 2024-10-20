#include <raylib.h>
#include "Math.h"
#include "raudio.c"

#include <cassert>
#include <array>
#include <vector>
#include <algorithm>

#define MAX_TURRETS 5

const float SCREEN_SIZE = 800;

const int TILE_COUNT = 20;
const float TILE_SIZE = SCREEN_SIZE / TILE_COUNT;

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

// I needed to make 3 unique position variables to get this to make enemies oh my god
struct Enemy
{
    Vector2 enemyInitPos{};
    Vector2 enemyPos{};
    Vector2 enemyDirection{};
    bool enemyEnabled = true;
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

    Turret turret;
    turret.type = TURRET;  
    std::vector<Turret> turrets;
    const float turretRadius = 20.0f;
    float turretCount = 0.0f;
    Vector2 turretPosition{};
    

    std::vector<Enemy> enemies;
    const float enemySpeed = 250.0f;
    const float enemyRadius = 20.0f;
    float enemyCount = 0.0f;
    Vector2 enemyPosition{};
    float enemyTime = 0.0f;
    float enemySpawn = 1.0f;
    bool atEnd = false;
; 

    const float bulletTime = 1.0f;
    const float bulletSpeed = 500.0f;
    const float bulletRadius = 15.0f;

    std::vector<Bullet> bullets;
    float shootCurrent = 0.0f;
    float shootTotal = 0.25f;

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
        }

        // Shooting
        shootCurrent += dt;
        if (shootCurrent >= shootTotal)
        {
            shootCurrent = 0.0f;

            Turret turret;
            Bullet bullet;
            bullet.position = turretPosition;
            bullet.direction = Normalize(enemyPosition - bullet.position);
            bullets.push_back(bullet);
        }

        // Bullet update
        for (Bullet& bullet : bullets)
        {
            bullet.position = bullet.position + bullet.direction * bulletSpeed * dt; 
            bullet.time += dt;

            bool expired = bullet.time >= bulletTime;
            bool collision = CheckCollisionCircles(enemyPosition, enemyRadius, bullet.position, bulletRadius);
            bullet.enabled = !expired && !collision;
        }

        // Bullet removal
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), 
            [&bullets](Bullet bullet) { 
                return !bullet.enabled; 
            }), bullets.end()); 


        // Turret creation
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (turretCount <= 5.0f)
            {
                Turret turret;
                turretCount = turretCount + 1.0f;
                Vector2 mousePosition = GetMousePosition();
                turretPosition = mousePosition;
                
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
        for (const Enemy& enemy : enemies)
            DrawCircleV(enemy.enemyPos, enemyRadius, RED);

        for (const Turret& turret : turrets) 
            DrawCircleV(turret.turretPos, turretRadius, PINK);  

        // Render bullets
        for (const Bullet& bullet : bullets)
            DrawCircleV(bullet.position, bulletRadius, BLUE);
        DrawText(TextFormat("Total bullets: %i", bullets.size()), 10, 10, 20, BLUE);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
