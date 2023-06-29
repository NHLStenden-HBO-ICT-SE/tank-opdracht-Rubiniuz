#include "precomp.h"
#include "terrain.h"

#include <map>

namespace fs = std::filesystem;
namespace Tmpl8
{
    Terrain::Terrain()
    {
        //Load in terrain sprites
        grass_img = std::make_unique<Surface>("assets/tile_grass.png");
        forest_img = std::make_unique<Surface>("assets/tile_forest.png");
        rocks_img = std::make_unique<Surface>("assets/tile_rocks.png");
        mountains_img = std::make_unique<Surface>("assets/tile_mountains.png");
        water_img = std::make_unique<Surface>("assets/tile_water.png");


        tile_grass = std::make_unique<Sprite>(grass_img.get(), 1);
        tile_forest = std::make_unique<Sprite>(forest_img.get(), 1);
        tile_rocks = std::make_unique<Sprite>(rocks_img.get(), 1);
        tile_water = std::make_unique<Sprite>(water_img.get(), 1);
        tile_mountains = std::make_unique<Sprite>(mountains_img.get(), 1);


        //Load terrain layout file and fill grid based on tiletypes
        fs::path terrain_file_path{ "assets/terrain.txt" };
        std::ifstream terrain_file(terrain_file_path);

        if (terrain_file.is_open())
        {
            std::string terrain_line;

            std::getline(terrain_file, terrain_line);
            std::istringstream lineStream(terrain_line);

            int rows;

            lineStream >> rows;

            for (size_t row = 0; row < rows; row++)
            {
                std::getline(terrain_file, terrain_line);

                for (size_t collumn = 0; collumn < terrain_line.size(); collumn++)
                {
                    switch (std::toupper(terrain_line.at(collumn)))
                    {
                    case 'G':
                        tiles.at(row).at(collumn).tile_type = TileType::GRASS;
                        break;
                    case 'F':
                        tiles.at(row).at(collumn).tile_type = TileType::FORREST;
                        break;
                    case 'R':
                        tiles.at(row).at(collumn).tile_type = TileType::ROCKS;
                        break;
                    case 'M':
                        tiles.at(row).at(collumn).tile_type = TileType::MOUNTAINS;
                        break;
                    case 'W':
                        tiles.at(row).at(collumn).tile_type = TileType::WATER;
                        break;
                    default:
                        tiles.at(row).at(collumn).tile_type = TileType::GRASS;
                        break;
                    }
                }
            }
        }
        else
        {
            std::cout << "Could not open terrain file! Is the path correct? Defaulting to grass.." << std::endl;
            std::cout << "Path was: " << terrain_file_path << std::endl;
        }

        //Instantiate tiles for path planning
        for (size_t y = 0; y < tiles.size(); y++)
        {
            for (size_t x = 0; x < tiles.at(y).size(); x++)
            {
                tiles.at(y).at(x).position_x = x;
                tiles.at(y).at(x).position_y = y;

                if (is_accessible(y, x + 1)) { tiles.at(y).at(x).exits.push_back(&tiles.at(y).at(x + 1)); }
                if (is_accessible(y, x - 1)) { tiles.at(y).at(x).exits.push_back(&tiles.at(y).at(x - 1)); }
                if (is_accessible(y + 1, x)) { tiles.at(y).at(x).exits.push_back(&tiles.at(y + 1).at(x)); }
                if (is_accessible(y - 1, x)) { tiles.at(y).at(x).exits.push_back(&tiles.at(y - 1).at(x)); }
            }
        }
    }

    void Terrain::update()
    {
        //Pretend there is animation code here.. next year :)
    }

    void Terrain::draw(Surface* target) const
    {

        for (size_t y = 0; y < tiles.size(); y++)
        {
            for (size_t x = 0; x < tiles.at(y).size(); x++)
            {
                int posX = (x * sprite_size) + HEALTHBAR_OFFSET;
                int posY = y * sprite_size;

                switch (tiles.at(y).at(x).tile_type)
                {
                case TileType::GRASS:
                    tile_grass->draw(target, posX, posY);
                    break;
                case TileType::FORREST:
                    tile_forest->draw(target, posX, posY);
                    break;
                case TileType::ROCKS:
                    tile_rocks->draw(target, posX, posY);
                    break;
                case TileType::MOUNTAINS:
                    tile_mountains->draw(target, posX, posY);
                    break;
                case TileType::WATER:
                    tile_water->draw(target, posX, posY);
                    break;
                default:
                    tile_grass->draw(target, posX, posY);
                    break;
                }
            }
        }
    }

    int GuessDistanceToEnd(TerrainTile* start, TerrainTile* end)
    {
        //simple estimation with pythagoras
        const unsigned long long& sx = start->position_x;
        const unsigned long long& ex = end->position_x;
        const unsigned long long& sy = start->position_y;
        const unsigned long long& ey = end->position_y;
        
        const unsigned long long x = sx - ex;
        const unsigned long long y = sy - ey;
        
        return sqrt((x*x)+(y*y));
    }

    struct Route
    {
        vector<TerrainTile*> route;
        int score;
        Route(vector<TerrainTile*> _route, double _score)
        {
            route = _route;
            score = _score;
        }

        bool operator < (const Route& r) const
        {
            return (score < r.score);
        }

        bool operator == (const Route& r) const
        {
            return (score == r.score);
        }
    };

    struct less_score_than
    {
        inline bool operator() (const Route& route1, const Route& route2)
        {
            return (route1.score < route2.score);
        }
    };

    //New Algorithm 
    int FindScoreIndex(const vector<Route*> sortedList, int newScore)
    {
        int size = sortedList.size();
        if(size < 1) return 0;

        // Lower and upper bounds
        int start = 0;
        int end = size - 1;
        
        // Traverse the search space
        while (start <= end) {
            int mid = (start + end) / 2;
            // If element is found
            if (sortedList[mid]->score == newScore)
                return mid;
            else if (sortedList[mid]->score < newScore)
                start = mid + 1;
            else
                end = mid - 1;
        }
        // Return insert position
        return end + 1;
    }

    //Use Breadth-first search to find shortest route to the destination
    vector<vec2> Terrain::get_route(const Tank& tank, const vec2& target)
    {
        //Find start and target tile
        const size_t pos_x = tank.position.x / sprite_size;
        const size_t pos_y = tank.position.y / sprite_size;

        const size_t target_x = target.x / sprite_size;
        const size_t target_y = target.y / sprite_size;

        //Init queue with start tile
        TerrainTile* start = &tiles.at(pos_y).at(pos_x);
        TerrainTile* end = &tiles.at(target_y).at(target_x);

        // Dick Astar Test
        vector<Route*> list = vector<Route*>();
        Route* startR = new Route(vector<TerrainTile*>(), GuessDistanceToEnd(start, end) + 1);
        startR->route.push_back(start);
        list.push_back(startR);

        bool route_found = false;
        vector<TerrainTile*> current_route;
        vector<TerrainTile*> visited;
        
        //Dick aStar
        while (!route_found && list.size() > 0)
        {
            Route* route = list.front();
            current_route = route->route;
            TerrainTile* current_tile = current_route.back();
            
            //Check all exits, if target then done, else if unvisited push a new partial route
            for (TerrainTile * exit : current_tile->exits)
            {
                if (exit->position_x == target_x && exit->position_y == target_y)
                {
                    current_route.push_back(exit);
                    route_found = true;
                    break;
                }
                
                if (!exit->visited)
                {
                    exit->visited = true;
                    visited.push_back(exit);
                    int new_score = GuessDistanceToEnd(exit,end) + current_route.size();
                    Route* r = new Route(current_route, new_score);
                    r->route.push_back(exit);

                    int foundIndex = FindScoreIndex(list, new_score);
                    list.insert(list.begin() + foundIndex, r);
                }
            }
            
            if(!route_found)
            {
                auto it = std::find(list.begin(), list.end(), route);
                list.erase(it);
            }
        }
        
        //Reset tiles
        for (TerrainTile * tile : visited)
        {
            tile->visited = false;
        }

        if (route_found)
        {
            //Convert route to vec2 to prevent dangling pointers
            std::vector<vec2> route;
            for (TerrainTile* tile : current_route)
            {
                route.push_back(vec2((float)tile->position_x * sprite_size, (float)tile->position_y * sprite_size));
            }

            return route;
        }
        else
        {
            return  std::vector<vec2>();
        }

    }

    //TODO: Function not used, convert BFS to dijkstra and take speed into account next year :)
    float Terrain::get_speed_modifier(const vec2& position) const
    {
        const size_t pos_x = position.x / sprite_size;
        const size_t pos_y = position.y / sprite_size;

        switch (tiles.at(pos_y).at(pos_x).tile_type)
        {
        case TileType::GRASS:
            return 1.0f;
            break;
        case TileType::FORREST:
            return 0.5f;
            break;
        case TileType::ROCKS:
            return 0.75f;
            break;
        case TileType::MOUNTAINS:
            return 0.0f;
            break;
        case TileType::WATER:
            return 0.0f;
            break;
        default:
            return 1.0f;
            break;
        }
    }

    bool Terrain::is_accessible(int y, int x)
    {
        //Bounds check
        if ((x >= 0 && x < terrain_width) && (y >= 0 && y < terrain_height))
        {
            //Inaccessible terrain check
            if (tiles.at(y).at(x).tile_type != TileType::MOUNTAINS && tiles.at(y).at(x).tile_type != TileType::WATER)
            {
                return true;
            }
        }

        return false;
    }
}