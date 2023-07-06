#include "precomp.h" // include (only) this in every .cpp file

constexpr auto num_tanks_blue = 2048;
constexpr auto num_tanks_red = 2048;

constexpr auto tank_max_health = 1000;
constexpr auto rocket_hit_value = 60;
constexpr auto particle_beam_hit_value = 50;

constexpr auto tank_max_speed = 1.0;

constexpr auto health_bar_width = 70;

constexpr auto max_frames = 2000;

//Global performance timer
constexpr auto REF_PERFORMANCE = 39245; //2300000 for pc // 166965 for laptop on battery //114757 for og //UPDATE THIS WITH YOUR REFERENCE PERFORMANCE (see console after 2k frames)
static timer perf_timer;
static float duration;

//Load sprite files and initialize sprites
static Surface* tank_red_img = new Surface("assets/Tank_Proj2.png");
static Surface* tank_blue_img = new Surface("assets/Tank_Blue_Proj2.png");
static Surface* rocket_red_img = new Surface("assets/Rocket_Proj2.png");
static Surface* rocket_blue_img = new Surface("assets/Rocket_Blue_Proj2.png");
static Surface* particle_beam_img = new Surface("assets/Particle_Beam.png");
static Surface* smoke_img = new Surface("assets/Smoke.png");
static Surface* explosion_img = new Surface("assets/Explosion.png");

static Sprite tank_red(tank_red_img, 12);
static Sprite tank_blue(tank_blue_img, 12);
static Sprite rocket_red(rocket_red_img, 12);
static Sprite rocket_blue(rocket_blue_img, 12);
static Sprite smoke(smoke_img, 4);
static Sprite explosion(explosion_img, 9);
static Sprite particle_beam_sprite(particle_beam_img, 3);

const static vec2 tank_size(7, 9);
const static vec2 rocket_size(6, 6);

const static float tank_radius = 3.f;
const static float rocket_radius = 5.f;

//optimized
// -----------------------------------------------------------
// Initialize the simulation state
// This function does not count for the performance multiplier
// (Feel free to optimize anyway though ;) )
// -----------------------------------------------------------
void Game::init()
{
    frame_count_font = new Font("assets/digital_small.png", "ABCDEFGHIJKLMNOPQRSTUVWXYZ:?!=-0123456789.");

    tanks.reserve(num_tanks_blue + num_tanks_red);

    uint max_rows = 24;

    float start_blue_x = tank_size.x + 40.0f;
    float start_blue_y = tank_size.y + 30.0f;

    float start_red_x = 1088.0f;
    float start_red_y = tank_size.y + 30.0f;

    float spacing = 7.5f;

    //////////////
    //Create Grid of tanks
    //////////////
    const size_t gridWidth = background_terrain.GetWidth();
    const size_t gridHeight = background_terrain.GetHeight();
    
    const size_t gridsCount = gridWidth * gridHeight;
    grids = vector<Grid>(gridsCount);
    
    for (size_t i = 0; i < gridsCount - 1; i++)
    {
        vec2 position{(float)(i % gridWidth) * gridSize, (float)(i / gridWidth) * gridSize };
        grids[i] = Grid(position, position + vec2(gridSize, gridSize), i);
    }

    //Spawn blue tanks
    for (int i = 0; i < num_tanks_blue; i++)
    {
        vec2 position{ start_blue_x + ((i % max_rows) * spacing), start_blue_y + ((i / max_rows) * spacing) };
        tanks.push_back(Tank(position.x, position.y, BLUE, &tank_blue, &smoke, 1100.f, position.y + 16, tank_radius, tank_max_health, tank_max_speed));
        //Get gridcell for tank
        int gridIndex = Grid::GetGridIndex(position, gridSize, gridWidth);
        if (gridIndex < 0 || gridIndex >= grids.size())
        {
            std::cout << "!ERROR! Tank out of bounds. This shouldn't happen Add Blue Tank!" << std::endl;
            continue;
        }
        grids[gridIndex].AddTank(tanks.back());
    }
    //Spawn red tanks
    for (int i = 0; i < num_tanks_red; i++)
    {
        vec2 position{ start_red_x + ((i % max_rows) * spacing), start_red_y + ((i / max_rows) * spacing) };
        tanks.push_back(Tank(position.x, position.y, RED, &tank_red, &smoke, 100.f, position.y + 16, tank_radius, tank_max_health, tank_max_speed));
        //Get gridcell for tank
        int gridIndex = Grid::GetGridIndex(position, gridSize, gridWidth);
        if (gridIndex < 0 || gridIndex >= grids.size())
        {
            std::cout << "!ERROR! Tank out of bounds. This shouldn't happen Add RED Tank!" << std::endl;
            continue;
        }
        grids[gridIndex].AddTank(tanks.back());
    }

    particle_beams.push_back(Particle_beam(vec2(590, 327), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));
    particle_beams.push_back(Particle_beam(vec2(64, 64), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));
    particle_beams.push_back(Particle_beam(vec2(1200, 600), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));

    ActiveTanks.reserve(num_tanks_blue + num_tanks_red);
    ActiveTanks = tanks;

    cout << "Initialization done. Got: " << ActiveTanks.size() << " Tanks. Spread over " << gridsCount << " grids." << endl;
}

// -----------------------------------------------------------
// Close down application
// -----------------------------------------------------------
void Game::shutdown()
{
}

//unoptimized - use grid or box search
// -----------------------------------------------------------
// Iterates through all tanks and returns the closest enemy tank for the given tank
// -----------------------------------------------------------
Tank& Game::find_closest_enemy(Tank& current_tank)
{
    int gridIndex = Grid::GetGridIndex(current_tank.position, gridSize, background_terrain.GetWidth());
    if (gridIndex < 0 || gridIndex >= grids.size())
    {
        std::cout << "!ERROR! Tank out of bounds. This shouldn't happen. CLOSEST ENEMY" << std::endl;
        return current_tank;
    }
    
    Grid& currentGrid = grids[gridIndex];

    //find closest grid that has tanks of enemy color
    size_t closestGridIndex = -1;
    float closestGridDistance = numeric_limits<float>::infinity();
    for (size_t i = 0; i < grids.size(); i++)
    {
        if(grids.at(i).hasTanks == false || grids.at(i).indentifier == currentGrid.indentifier)
            continue;

        //Check if enemy tanks exist in grid
        if(current_tank.allignment == allignments::BLUE && grids.at(i).hasRedTanks == false)
            continue;
        if(current_tank.allignment == allignments::RED && grids.at(i).hasBlueTanks == false)
            continue;

        float sqr_dist = fabsf((grids.at(i).GetCenter() - current_tank.get_position()).sqr_length());
        if (sqr_dist < closestGridDistance)
        {
            closestGridIndex = i;
            closestGridDistance = sqr_dist;
        }
    }

    if(closestGridIndex == -1)
    {
        std::cout << "!ERROR! closest grid index faulty" << std::endl;
    }

    Grid& closestGrid = grids[closestGridIndex];
    vector<Tank>& closestGridTanks = closestGrid.GetTanks();
    float closestTankDistance = numeric_limits<float>::infinity();
    int closestTankIndex = 0;
    for(size_t i = 0; i < closestGridTanks.size(); i++)
    {
        Tank& tank = closestGridTanks[i];
        if(tank.allignment == current_tank.allignment)
            continue;
        
        float sqr_dist = fabsf((tank.position - current_tank.get_position()).sqr_length());
        if (sqr_dist < closestTankDistance)
        {
            closestTankDistance = sqr_dist;
            closestTankIndex = i;
        }
    }
    return closestGridTanks[closestTankIndex];
}

//optimized
//Checks if a point lies on the left of an arbitrary angled line
bool Tmpl8::Game::left_of_line(vec2 line_start, vec2 line_end, vec2 point)
{
    return ((line_end.x - line_start.x) * (point.y - line_start.y) - (line_end.y - line_start.y) * (point.x - line_start.x)) < 0;
}

//unoptimized
// -----------------------------------------------------------
// Update the game state:
// Move all objects
// Update sprite frames
// Collision detection
// Targeting etc..
// -----------------------------------------------------------
void Game::update(float deltaTime)
{
    //Check Active Tanks
    size_t gridWidth = background_terrain.GetWidth();
    for(Grid& g : grids)
    {
        g.ClearTanks();
    }
    
    vector<Tank> Temp = vector<Tank>();
    for (Tank& t : ActiveTanks)
    {
        if(t.active)
        {
            Temp.push_back(t);
            int gridIndex = Grid::GetGridIndex(t.position, gridSize, gridWidth);
            if (gridIndex < 0 || gridIndex >= grids.size())
            {
                std::cout << "!ERROR! Tank out of bounds. This shouldn't happen UPDATE ACTIVE TANKS" << std::endl;
                continue;
            }
            grids[gridIndex].AddTank(t);
        }
        else
        {
            DeadTanks.push_back(t);
        }
    }
    ActiveTanks = Temp;
    Temp = vector<Tank>();

    std::cout << "Loop with: " << ActiveTanks.size() << " Tanks" << std::endl;
    
    //optimized
    //Calculate the route to the destination for each tank using BFS
    //Initializing routes here so it gets counted for performance..
    if (frame_count == 0)
    {
        for (Tank& t : ActiveTanks)
        {
            t.set_route(background_terrain.get_route(t, t.target));
        }
    }
    
    /// new check using grids offset tanks on collision
    for(Grid& g : grids)
    {
        for(Tank& t : g.GetTanks())
        {
            for(Tank& ot : g.GetTanks())
            {
                if (&t == &ot) continue;

                vec2 dir = t.get_position() - ot.get_position();
                float dir_squared_len = dir.sqr_length();

                float col_squared_len = (t.get_collision_radius() + ot.get_collision_radius());
                col_squared_len *= col_squared_len;

                if (dir_squared_len < col_squared_len)
                {
                    t.push(dir.normalized(), 1.f);
                }
            }
        }
    }

    //optimized
    //Update tanks
    for (Tank& tank : ActiveTanks)
    {
        //Move tanks according to speed and nudges (see above) also reload
        tank.tick(background_terrain);

        //Shoot at closest target if reloaded
        if (tank.rocket_reloaded())
        {
            Tank& target = find_closest_enemy(tank);
            rockets.push_back(Rocket(tank.position, (target.get_position() - tank.position).normalized() * 3, rocket_radius, tank.allignment, ((tank.allignment == RED) ? &rocket_red : &rocket_blue)));

            tank.reload_rocket();
        }
    }
    
    //Update smoke plumes
    for (Smoke& smoke : smokes)
    {
        smoke.tick();
    }

    //Calculate "forcefield" around active tanks
    forcefield_hull.clear();

    /// next calculations are also altered
    
    // same as Astar sort. keep all active tanks in new list. skip others.
    // combine the other active loop for hulls with this one
    // Should lower calls when tanks are no longer active because they're not needed.
    vec2 point_found_on_hull = vec2();
    Tank* FirstActive = nullptr;

    for (Tank& tank : ActiveTanks)
    {
        if(FirstActive == nullptr)
        {
            FirstActive = &tank;
        }
        
        if (tank.position.x <= point_found_on_hull.x || point_found_on_hull == vec2())
        {
            point_found_on_hull = tank.position;
        }
    }

    // Seems still extremely dirty Unoptimized? Use out most grids that contains tanks to find the points
    for (Tank& tank : ActiveTanks)
    {
        if(FirstActive == nullptr)
        {
            std::cout << "Problem Found no Active Tanks!" << std::endl;
        }
        
        forcefield_hull.push_back(point_found_on_hull);
        vec2 endpoint = FirstActive->position;

        for (Tank& tank : ActiveTanks)
        {
            if (tank.active)
            {
                if ((endpoint == point_found_on_hull) || left_of_line(point_found_on_hull, endpoint, tank.position))
                {
                    endpoint = tank.position;
                }
            }
        }
        point_found_on_hull = endpoint;

        if (endpoint == forcefield_hull.at(0))
        {
            break;
        }
    }
    
    //Update rockets /// ALTERED
    for (Rocket& rocket : rockets)
    {
        rocket.tick();

        int gridIndex = Grid::GetGridIndex(rocket.position, gridSize, gridWidth);
        if (gridIndex < 0 || gridIndex >= grids.size())
        {
            std::cout << "!ERROR! Tank out of bounds. This shouldn't happen CHECK ROCKETS" << std::endl;
            continue;
        }

        Grid& g = grids[gridIndex];
        for (Tank& tank : g.GetTanks())
        {
            if ((tank.allignment != rocket.allignment) && rocket.intersects(tank.position, tank.collision_radius))
            {
                explosions.push_back(Explosion(&explosion, tank.position));

                if (tank.hit(rocket_hit_value))
                {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(7, 24)));
                }

                rocket.active = false;
                break;
            }
        }
    }

    //Disable rockets if they collide with the "forcefield" around active tanks
    //Hint: A point to convex hull intersection test might be better here? :) (Disable if outside)
    for (Rocket& rocket : rockets)
    {
        if (rocket.active)
        {
            for (size_t i = 0; i < forcefield_hull.size(); i++)
            {
                if (circle_segment_intersect(forcefield_hull.at(i), forcefield_hull.at((i + 1) % forcefield_hull.size()), rocket.position, rocket.collision_radius))
                {
                    explosions.push_back(Explosion(&explosion, rocket.position));
                    rocket.active = false;
                }
            }
        }
    }

    //optimized
    //Remove exploded rockets with remove erase idiom
    rockets.erase(std::remove_if(rockets.begin(), rockets.end(), [](const Rocket& rocket) { return !rocket.active; }), rockets.end());
    
    //Update particle beams //// Altered
    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.tick(ActiveTanks);

        //Damage all tanks within the damage window of the beam (the window is an axis-aligned bounding box)
        for (Tank& tank : ActiveTanks)
        {
            if (particle_beam.rectangle.intersects_circle(tank.get_position(), tank.get_collision_radius()))
            {
                if (tank.hit(particle_beam.damage))
                {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                }
            }
        }
    }

    //optimized
    //Update explosion sprites and remove when done with remove erase idiom
    for (Explosion& explosion : explosions)
    {
        explosion.tick();
    }
    
    //optimized
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(), [](const Explosion& explosion) { return explosion.done(); }), explosions.end());
    std::cout << "update complete" << std::endl;
}

//optimized
// -----------------------------------------------------------
// Draw all sprites to the screen
// (It is not recommended to multi-thread this function)
// -----------------------------------------------------------
void Game::draw()
{
    // clear the graphics window
    screen->clear(0);

    //Draw background
    background_terrain.draw(screen);

    //Draw sprites /// Altered
    for (auto t : DeadTanks)
    {
        t.draw(screen);
    }
    for (auto t : ActiveTanks)
    {
        t.draw(screen);
    }
    
    for (Rocket& rocket : rockets)
    {
        rocket.draw(screen);
    }

    for (Smoke& smoke : smokes)
    {
        smoke.draw(screen);
    }

    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.draw(screen);
    }

    for (Explosion& explosion : explosions)
    {
        explosion.draw(screen);
    }

    //Draw forcefield (mostly for debugging, its kinda ugly..)
    for (size_t i = 0; i < forcefield_hull.size(); i++)
    {
        vec2 line_start = forcefield_hull.at(i);
        vec2 line_end = forcefield_hull.at((i + 1) % forcefield_hull.size());
        line_start.x += HEALTHBAR_OFFSET;
        line_end.x += HEALTHBAR_OFFSET;
        screen->line(line_start, line_end, 0x0000ff);
    }

    tanks = ActiveTanks;
    tanks.insert(tanks.end(), DeadTanks.begin(), DeadTanks.end());

    //Draw sorted health bars
    for (int t = 0; t < 2; t++)
    {
        const int NUM_TANKS = ((t < 1) ? num_tanks_blue : num_tanks_red);

        const int begin = ((t < 1) ? 0 : num_tanks_blue);
        std::vector<const Tank*> sorted_tanks;
        insertion_sort_tanks_health(tanks, sorted_tanks, begin, begin + NUM_TANKS);
        sorted_tanks.erase(std::remove_if(sorted_tanks.begin(), sorted_tanks.end(), [](const Tank* tank) { return !tank->active; }), sorted_tanks.end());

        draw_health_bars(sorted_tanks, t);
    }
}

//unoptimized?
// -----------------------------------------------------------
// Sort tanks by health value using insertion sort
// -----------------------------------------------------------
void Tmpl8::Game::insertion_sort_tanks_health(const std::vector<Tank>& original, std::vector<const Tank*>& sorted_tanks, int begin, int end)
{
    const int NUM_TANKS = end - begin;
    sorted_tanks.reserve(NUM_TANKS);
    sorted_tanks.emplace_back(&original.at(begin));

    for (int i = begin + 1; i < (begin + NUM_TANKS); i++)
    {
        const Tank& current_tank = original.at(i);

        for (int s = (int)sorted_tanks.size() - 1; s >= 0; s--)
        {
            const Tank* current_checking_tank = sorted_tanks.at(s);

            if ((current_checking_tank->compare_health(current_tank) <= 0))
            {
                sorted_tanks.insert(1 + sorted_tanks.begin() + s, &current_tank);
                break;
            }

            if (s == 0)
            {
                sorted_tanks.insert(sorted_tanks.begin(), &current_tank);
                break;
            }
        }
    }
}

//optimized
// -----------------------------------------------------------
// Draw the health bars based on the given tanks health values
// -----------------------------------------------------------
void Tmpl8::Game::draw_health_bars(const std::vector<const Tank*>& sorted_tanks, const int team)
{
    int health_bar_start_x = (team < 1) ? 0 : (SCRWIDTH - HEALTHBAR_OFFSET) - 1;
    int health_bar_end_x = (team < 1) ? health_bar_width : health_bar_start_x + health_bar_width - 1;

    for (int i = 0; i < SCRHEIGHT - 1; i++)
    {
        //Health bars are 1 pixel each
        int health_bar_start_y = i * 1;
        int health_bar_end_y = health_bar_start_y + 1;

        screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
    }

    //Draw the <SCRHEIGHT> least healthy tank health bars
    int draw_count = std::min(SCRHEIGHT, (int)sorted_tanks.size());
    for (int i = 0; i < draw_count - 1; i++)
    {
        //Health bars are 1 pixel each
        int health_bar_start_y = i * 1;
        int health_bar_end_y = health_bar_start_y + 1;

        float health_fraction = (1 - ((double)sorted_tanks.at(i)->health / (double)tank_max_health));

        if (team == 0) { screen->bar(health_bar_start_x + (int)((double)health_bar_width * health_fraction), health_bar_start_y, health_bar_end_x, health_bar_end_y, GREENMASK); }
        else { screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x - (int)((double)health_bar_width * health_fraction), health_bar_end_y, GREENMASK); }
    }
}

//optimized
// -----------------------------------------------------------
// When we reach max_frames print the duration and speedup multiplier
// Updating REF_PERFORMANCE at the top of this file with the value
// on your machine gives you an idea of the speedup your optimizations give
// -----------------------------------------------------------
void Tmpl8::Game::measure_performance()
{
    char buffer[128];
    if (frame_count >= max_frames)
    {
        if (!lock_update)
        {
            duration = perf_timer.elapsed();
            cout << "Duration was: " << duration << " (Replace REF_PERFORMANCE with this value)" << endl;
            lock_update = true;
        }

        frame_count--;
    }

    if (lock_update)
    {
        screen->bar(420 + HEALTHBAR_OFFSET, 170, 870 + HEALTHBAR_OFFSET, 430, 0x030000);
        int ms = (int)duration % 1000, sec = ((int)duration / 1000) % 60, min = ((int)duration / 60000);
        sprintf(buffer, "%02i:%02i:%03i", min, sec, ms);
        frame_count_font->centre(screen, buffer, 200);
        sprintf(buffer, "SPEEDUP: %4.1f", REF_PERFORMANCE / duration);
        frame_count_font->centre(screen, buffer, 340);
    }
}

//optimized
// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::tick(float deltaTime)
{
    if (!lock_update)
    {
        update(deltaTime);
    }
    draw();

    measure_performance();

    // print something in the graphics window
    //screen->Print("hello world", 2, 2, 0xffffff);

    // print something to the text window
    //cout << "This goes to the console window." << std::endl;

    //Print frame count
    frame_count++;
    string frame_count_string = "FRAME: " + std::to_string(frame_count);
    frame_count_font->print(screen, frame_count_string.c_str(), 350, 580);
}
