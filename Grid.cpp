#include "precomp.h"
#include "Grid.h"

Grid::Grid(vec2 _TopLeft, vec2 _BottomRight, int _indentifier)
{
    this->TopLeft = _TopLeft;
    this->BottomRight = _BottomRight;
    this->indentifier = _indentifier;
}

bool Grid::IsInBounds(Tank& tank)
{
    vec2 TankPos = tank.position;
    // check if tank position is in bounds
    if (TankPos.x > this->TopLeft.x && TankPos.x < this->BottomRight.x && TankPos.y > this->TopLeft.y && TankPos.y < this->BottomRight.y)
    {
        return true;
    }
    return false;
}

vector<Tank> Grid::GetTanksOutBounds()
{
    hasRedTanks = false;
    hasBlueTanks = false;
    hasActiveTanks = false;
    hasTanks = false;
    // check if tank position is in bounds
    vector<Tank> OutOfBoundsTanks;
    vector<Tank> InBoundsTanks;
    for (Tank& tank : this->tanks)
    {
        if (!IsInBounds(tank))
        {
            OutOfBoundsTanks.push_back(tank);
        }
        else
        {
            InBoundsTanks.push_back(tank);
            if(hasActiveTanks == false && tank.active)
            {
                hasActiveTanks = true;
            }
            if(tank.allignment == allignments::RED)
            {
                hasRedTanks = true;
            } else
            {
                hasBlueTanks = true;
            }
        }
    }
    
    if(InBoundsTanks.size() > 0)
    {
        this->tanks = InBoundsTanks;
        hasTanks = true;
    } else
    {
        this->tanks.clear();
        hasTanks = false;
    }
        
    return OutOfBoundsTanks;
}

void Grid::AddTank(Tank& tank)
{
    this->tanks.push_back(tank);
    hasTanks = true;
    if(hasActiveTanks == false && tank.active)
    {
        hasActiveTanks = true;
    }
    if(tank.allignment == allignments::RED)
    {
        hasRedTanks = true;
    } else
    {
        hasBlueTanks = true;
    }
}

int Grid::GetGridIndex(vec2 position, int gridSize, size_t gridWidth)
{
    return (int)(position.y / gridSize) * gridWidth + (int)(position.x / gridSize);
}
