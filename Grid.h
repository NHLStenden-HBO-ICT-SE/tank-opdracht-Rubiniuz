#pragma once

class Grid
{
public:
    Grid(){}
    Grid(vec2 _TopLeft, vec2 _BottomRight, int _indentifier);

    int indentifier = 0;
    bool hasTanks = false;
    bool hasActiveTanks = false;
    bool hasRedTanks = false;
    bool hasBlueTanks = false;

    bool IsInBounds(Tank& tank);
    vector<Tank> GetTanksOutBounds();

    vec2 GetTopLeft() { return this->TopLeft; }
    vec2 GetBottomRight() { return this->BottomRight; }
    vec2 GetCenter() { return vec2((this->TopLeft.x + this->BottomRight.x) / 2, (this->TopLeft.y + this->BottomRight.y) / 2); }

    void AddTank(Tank& tank);
    vector<Tank>& GetTanks() { return this->tanks; }
    void ClearTanks() { this->tanks.clear(); }

    static int GetGridIndex(vec2 position, int gridSize, size_t gridWidth);

private:
    vector<Tank> tanks = vector<Tank>();
    
    vec2 TopLeft;
    vec2 BottomRight;
};
